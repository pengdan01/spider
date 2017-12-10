#include "crawler/crawl/page_crawler.h"

#include <signal.h>
#include <sys/epoll.h>

#include <algorithm>

#include "base/common/logging.h"
#include "base/thread/sync.h"
#include "base/hash_function/city.h"
#include "web/url_norm/url_norm.h"

namespace crawler2 {
namespace crawl {
const int kMaxEpollEvent = 1024;

PageCrawler::PageCrawler(const Options& options)
    : thread_started_(false)
    , current_tasks_num_(0)
    , stopped_(false)
    , options_(options)
    , epoll_fd_(-1)
    , epoll_timeout_ms_(100) {
  url_norm_ = new web::UrlNorm();
  curl_multi_handlers_.resize(10);
  CHECK_GT(options_.connect_timeout_in_seconds, 0);
  CHECK_GT(options_.transfer_timeout_in_seconds, 0);
  CHECK_GT(options_.low_speed_limit_in_bytes_per_second, 0);
  CHECK_GT(options_.low_speed_duration_in_seconds, 0);
  CHECK_GE(options_.max_redir_times, 0);
  CHECK_GT(options_.max_fetch_times_to_reset_cookie, 0);
}

PageCrawler::~PageCrawler() {
  tasks_.Close();

  if (url_norm_) {
    delete url_norm_;
  }

  if (epoll_fd_ != -1) {
    close(epoll_fd_);
  }

  for (auto it = curl_multi_handlers_.begin();
       it != curl_multi_handlers_.end(); ++it) {
    curl_multi_cleanup(*it);
  }
}

// 全局清理无法执行， 由于多线程问题
// curl_global_cleanup();

// done = NewCallback(this, &GeneralCrawler::Process,task, status, res);
void PageCrawler::AddTask(CrawlTask* task, CrawlRoutineStatus* status,
                          Resource* res, Closure* done) {
  current_tasks_num_++;
  tasks_.Put(new CrawlRoutineData(task, status, res, done));
}

void PageCrawler::AddMultiTask(std::vector<CrawlTask*>* tasks,
                               std::vector<CrawlRoutineStatus*>* status,
                               std::vector<Resource*>* resources, Closure* done) {
  CHECK_NOTNULL(tasks);
  CHECK_NOTNULL(status);
  CHECK_NOTNULL(resources);
  CHECK_NOTNULL(done);
  CHECK_EQ(tasks->size(), status->size());
  CHECK_EQ(tasks->size(), resources->size());

  WholeTask* whole_task = new WholeTask;
  whole_task->tasks = tasks;
  whole_task->resources = resources;
  whole_task->whole_task_done = done;
  for (size_t i = 0; i < tasks->size(); ++i) {
    Closure* single_task_done = NewCallback(this, &PageCrawler::SingleTaskDone,
                                            whole_task);
    AddTask((*tasks)[i], (*status)[i], (*resources)[i], single_task_done);
  }
}

void PageCrawler::SingleTaskDone(WholeTask* task) {
  CHECK_NOTNULL(task);
  if (++task->completed_task == task->tasks->size()) {
    task->whole_task_done->Run();
  }
}


static void ThreadSafeGlobalInit() {
  // 利用编译器的局部静态变量只初始化一次, 来保证全局初始化之进行一次.
  static int ret = curl_global_init(CURL_GLOBAL_ALL);
  CHECK_EQ(ret, 0);
}

void PageCrawler::Init() {
  CHECK(url_norm_);
  ThreadSafeGlobalInit();

  CHECK_GT(curl_multi_handlers_.size(), 0u);

  PLOG_IF(FATAL, (epoll_fd_ = epoll_create(kMaxEpollEvent)) == -1) << "failed to create epoll fd";
  VLOG(3) << "create epoll fd: " << epoll_fd_;
  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = tasks_.GetEventFD();
  if (0 != epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, ev.data.fd, &ev)) {
    PLOG_IF(FATAL, epoll_ctl(epoll_fd_, EPOLL_CTL_MOD,
                             ev.data.fd, &ev) != 0);
  }

  for (auto it = curl_multi_handlers_.begin();
       it != curl_multi_handlers_.end(); ++it) {
    CHECK_NOTNULL(*it = curl_multi_init());
    // 由 epoll fd 监听 multi handler 的事件
    CHECK_EQ(CURLM_OK, curl_multi_setopt(*it, CURLMOPT_SOCKETFUNCTION,
                                         CallbackSocket));
    CHECK_EQ(CURLM_OK, curl_multi_setopt(*it, CURLMOPT_SOCKETDATA,
                                         &epoll_fd_));

    // 通过 callback 获取 epoll 事件的超时时间
    CHECK_EQ(CURLM_OK, curl_multi_setopt(*it, CURLMOPT_TIMERFUNCTION,
                                         &CallbackTimer));
    CHECK_EQ(CURLM_OK, curl_multi_setopt(*it, CURLMOPT_TIMERDATA,
                                         &epoll_timeout_ms_));
  }

  // 显示忽略掉 SIGPIP 信号，因为在 DNS 异步解析时可能会产生这个信号(即使设置了 CURLOPT_NOSIGNAL)
  CHECK_NE(signal(SIGPIPE, SIG_IGN), SIG_ERR);

  // 加载需要 hack 处理的 url 列表
  if (!options_.client_redirect_urls_filepath.empty()) {
    LoadClientRedirectFile(options_.client_redirect_urls_filepath, &client_redirect_urls_);
  }
}

CURLM *PageCrawler::GetMultiHandle(CrawlRoutineData *routine) {
  // 如果有 IP 使用 IP, 否则使用 domain
  std::string key;
  const CrawlTask& task = *(routine->task);
  if (!task.ip.empty()) {
    key = task.ip;
  } else {
    std::string host = GURL(task.url).host();
    std::string tld;
    std::string domain;
    std::string subdomain;
    if (web::ParseHost(host, &tld, &domain, &subdomain)) {
      key = domain;
    } else {
      key = host;
    }
  }

  int id = ::base::CityHash64(key.c_str(), key.length());
  int idx = id % curl_multi_handlers_.size();
  return curl_multi_handlers_[idx];
}

void PageCrawler::StartFetchTask(CrawlRoutineData *routine) {
  CURL *eh = curl_easy_init();
  CHECK_NOTNULL(eh);
  // 设置数据
  SetEasyHandler(eh, routine);
  // 加入到 multi handler
  curl_multi_add_handle(GetMultiHandle(routine), eh);
}

void PageCrawler::WaitForCompleted() {
  while (!IsAllTasksCompleted()) {
    CheckTaskStatus();
  }
}

void PageCrawler::CheckTaskStatus() {
  epoll_event events[kMaxEpollEvent];
  int running;
  for (auto it = curl_multi_handlers_.begin();
       it != curl_multi_handlers_.end(); ++it) {
    curl_multi_socket_all(*it, &running);
  }

  VLOG(4) << "epoll wait, epoll fd: " << epoll_fd_
          << ", max_events: " << kMaxEpollEvent
          << ", epoll timeout: " << epoll_timeout_ms_ << "ms ";

  int status = epoll_wait(epoll_fd_, events, kMaxEpollEvent,
                          epoll_timeout_ms_);
  PLOG_IF(FATAL, status == -1 && errno != EINTR) << "epoll_wait() fail";

  for (int i = 0; i < status; ++i) {
    // 如果有新增的 url 将它添加进去
    if (events[i].data.fd == tasks_.GetEventFD()) {
      std::deque<CrawlRoutineData*> new_tasks;
      tasks_.TryMultiTake(&new_tasks);
      while (!new_tasks.empty()) {
        StartFetchTask(new_tasks.front());
        new_tasks.pop_front();
      }
    }
  }

  for (auto it = curl_multi_handlers_.begin();
       it != curl_multi_handlers_.end(); ++it) {
    CURLMsg *msg;
    int32 msg_in_queue;  // 无用，丢弃
    while ((msg = curl_multi_info_read(*it, &msg_in_queue))) {
      LOG_IF(FATAL, msg->msg != CURLMSG_DONE)
          << "Transfer session fail, err msg: " << msg->msg;
      OnFinishTask(msg);
    }
  }
}

void PageCrawler::OnFinishTask(CURLMsg* msg) {
  CrawlRoutineData *routine = NULL;
  LOG_IF(FATAL, CURLE_OK != curl_easy_getinfo(msg->easy_handle,
                                              CURLINFO_PRIVATE, &routine))
      << "Get private data curl_easy_getinfo() fail.";

  ExtractInfoFromCurl(msg, routine);

  // 3. 删除 easy handler
  CURL *eh = msg->easy_handle;
  // XXX(suhua): once curl_multi_remove_handle() is called, |msg| is invalid
  // and can't be used.
  curl_multi_remove_handle(GetMultiHandle(routine), eh);
  // XXX(suhua): use |eh| init before, should not use msg->easy_handle, for
  // |msg| is invalid after the curl call above.
  curl_easy_cleanup(eh);

  // call: GeneralCrawler::Process
  if (routine->done) {
      routine->done->Run();
  }

  delete routine;

  // 计数减少
  current_tasks_num_--;
}

bool PageCrawler::IsAllTasksCompleted() const {
  return current_tasks_num_ == 0;
}
}  // namespace crawl
}  // namespace crawler
