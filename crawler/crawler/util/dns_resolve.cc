#include "crawler/util/dns_resolve.h"

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#include <fstream>
#include <sstream>
#include <algorithm>
#include <deque>

#include "base/common/basic_types.h"
#include "base/common/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/string_tokenize.h"
#include "base/strings/string_split.h"
#include "base/time/timestamp.h"
#include "base/common/sleep.h"
#include "base/random/pseudo_random.h"
#include "base/hash_function/fast_hash.h"
#include "base/thread/blocking_queue.h"
#include "base/thread/thread_pool.h"
#include "base/thread/thread_util.h"
#include "base/common/closure.h"

namespace crawler {

DEFINE_int32(dns_inner_timeout_in_ms, 5000, "DNS 解析超时时间, Bind 默认为 5000毫秒");
DEFINE_int32(dns_inner_try_times, 4, "Server 返回前, DNS 解析最大尝试次数, Bind 默认为 4 次");
DEFINE_int32(dns_outer_try_times, 0, "Server 返回结果是超时时, 再次发起查询的最大重试次数, 为 0 则不重试");

// When a query is complicated, this callback will be called.
// void AresHostCallbackOnVector(void *arg, int status, int timeouts, hostent *hostent);
void AresHostCallbackOnQueue(void *arg, int status, int timeouts, hostent *hostent);

struct RequestOnQueue;
class DnsResolver {
 public:
  // dns_servers 是用逗号分割的多个 dns server ip
  // window_size 并发请求数
  explicit DnsResolver(const std::string &dns_servers, int32 window_size);
  ~DnsResolver();

  bool Init();

  ares_channel* channels() { return channels_; }

  // In order to be easy to test those 3 functions, Have to make them public
  void GetResolvConfNameServer(std::string *server_str, const std::string &resolvefile = "/etc/resolv.conf");

  // 查询并原地修改 host 数组
  void ResolveHostQueue(RequestOnQueue* request_on_queue);
 public:
  std::string reason;
  int dns_status;
 private:
  ares_channel *channels_;
  ares_options options_;
  int32 channel_num_;
  std::string dns_servers_;
  int32 window_size_;
};
DnsResolver::DnsResolver(const std::string &dns_servers, int32 window_size) {
  std::vector<std::string> flds;
  base::SplitStringWithOptions(dns_servers, ",", true, true, &flds);
  CHECK_LE(flds.size(), 1u) << dns_servers;
  dns_servers_ = dns_servers;
  window_size_ = window_size;
}
DnsResolver::~DnsResolver() {
  for (int i = 0; i < channel_num_; ++i) {
    ares_destroy(channels_[i]);
  }
  delete [] channels_;
  // XXX(suhua): ares_library_cleanup() 会全局销毁 cares, 多线程下有问题,
  // 所以这里不调用 ares_library_cleanup()
  // ares_library_cleanup();
}
bool DnsResolver::Init() {
  // XXX(suhua):
  // ares_library_init() 不是线程安全的, 进程内只能调用一次, 用静态变量初始化,
  // 实现 ares_library_init() 全局只调用一次
  static int status = ares_library_init(ARES_LIB_INIT_ALL);
  LOG_IF(FATAL, status != 0) << "ares_library_init() fail, err msg: " << ares_strerror(status);

  // 设置 DNS 解析的 超时 和 尝试次数
  options_.timeout = FLAGS_dns_inner_timeout_in_ms;
  options_.tries = FLAGS_dns_inner_try_times;
  int optmask = ARES_OPT_TIMEOUTMS | ARES_OPT_TRIES;

  std::vector<std::string> tokens;
  base::TrimWhitespaces(&dns_servers_);
  if (dns_servers_.empty()) {  // use default name server in /etc/resolv.conf or local named
    channel_num_ = 1;
    LOG(INFO) << "user not appointing a name server, use default name server in /etc/resolv.conf";
  } else {
    channel_num_ = base::Tokenize(dns_servers_, ",", &tokens);
    LOG(INFO) << "user appointing a name server: " << dns_servers_;
  }
  CHECK_EQ(channel_num_, 1);
  channels_ = new ares_channel();
  status = ares_init_options(channels_, &options_, optmask);
  LOG_IF(FATAL, status != ARES_SUCCESS) << "ares_init() fail, err msg: " << ares_strerror(status);
  if (!dns_servers_.empty()) {
    status = ares_set_servers_csv(*channels_, tokens[0].c_str());
    LOG_IF(FATAL, status != ARES_SUCCESS) << "ares_set_servers() fail, err msg: " << ares_strerror(status);
  }
  return true;
}
void DnsResolver::
GetResolvConfNameServer(std::string *server_str, const std::string &resolvefile) {
  CHECK_NOTNULL(server_str);
  server_str->clear();
  std::ifstream myfin(resolvefile);
  LOG_IF(FATAL, !myfin.good()) << "Failed to open default resolve.conf file: " << resolvefile;
  std::string line;
  bool first_time_flag = true;
  while (getline(myfin, line)) {
    if (!base::StartsWith(line, "nameserver", false)) continue;
    std::string::size_type pos = std::string("nameserver").size();
    while (pos != std::string::npos && (line[pos] == ' ' || line[pos] == '\t' || line[pos] == '\r')) ++pos;
    LOG_IF(FATAL, pos == std::string::npos) << "File[" << resolvefile << "] format error, "
                                            << "not find nameserver in this line: " << line;
    std::string nameserver = line.substr(pos);
    base::TrimTrailingWhitespaces(&nameserver);
    if (nameserver.empty()) continue;
    if (first_time_flag) {
      *server_str = nameserver;
      first_time_flag = false;
    } else {
      *server_str = *server_str + "," + nameserver;
    }
  }
}

struct QueueData {
  HostIpInfo* info;
  void* root;
};
struct RequestOnQueue {
  std::vector<QueueData> requests;  // request 队列, 其长度就是最大同时请求数
  int64 issued_request_num;  // 提起的 issue request 总数
  int64 active_request_num;  // 当前正在请求的 request 数量
  thread::BlockingQueue<HostIpInfo *>* all_hosts;  // 该 name server 下的所有待请求的数据 

  thread::BlockingQueue<int64> availables;  // |requests| 数组中可以再发起请求的位置
  pthread_mutex_t mutex;
  int64 success_num;
  int64 fail_num;
  int32 max_retry;

  RequestOnQueue(int64 request_window_size,
                 thread::BlockingQueue<HostIpInfo *>* hosts,
                 int64 mretry)
      : issued_request_num(0), active_request_num(0),
      all_hosts(hosts),
      success_num(0), fail_num(0), max_retry(mretry) {
        CHECK_NOTNULL(all_hosts);

        pthread_mutex_init(&mutex, NULL);
        requests.resize(request_window_size);
        for (int64 i = 0; i < (int64)requests.size(); ++i) {
          requests[i].info = NULL;
          requests[i].root = this;
          availables.Put(i);
        }
      }
  ~RequestOnQueue() {
    pthread_mutex_destroy(&mutex);
  }
  bool NoMoreToRequest(void) {
    return all_hosts->Closed() && all_hosts->Empty();
  }
  int64 Request(ares_channel* channel) {
    // 数据已经处理完, 无需再提交新请求
    if (NoMoreToRequest()) {
      return 0;
    }
    // 提交新请求
    std::deque<int64> indics;
    int ret = availables.TimedMultiTake(10, &indics);
    DVLOG(11) << "indics size: " << indics.size() << ", ret: " << ret;
    if (ret != -1) {
      int64 issue = 0;
      while (!indics.empty() && !all_hosts->Empty()) {
        int64 index = indics.back();
        HostIpInfo*& info = requests[index].info;
        info = NULL;
        int ret2 = all_hosts->TimedTake(10, &info);
        if (ret2 <= 0) {
          continue;
        }
        ares_gethostbyname(*channel, info->host.c_str(), AF_INET,
                           AresHostCallbackOnQueue, &requests[index]);
        indics.pop_back();
        DVLOG(10) << "issue host: " << info->host
                  << ", queue size: " << all_hosts->Size()
                  << ", active num: " << active_request_num;

        ++issued_request_num;
        pthread_mutex_lock(&mutex);
        active_request_num++;
        pthread_mutex_unlock(&mutex);
        ++issue;
      }
      while (!indics.empty()) {
        availables.Put(indics.back());
        indics.pop_back();
      }
      return issue;
    } else {
      return -1;
    }
  }
};

void DnsResolver::
ResolveHostQueue(RequestOnQueue* request_on_queue) {
  LOG(INFO) << "thead: " << thread::GetThreadID() << ", queue size: " << request_on_queue->all_hosts->Size();

  ares_channel* channelptr = channels();
  CHECK(channelptr && request_on_queue->all_hosts);

  const int kMaxEvent = 1000;
  int epollfd = epoll_create(kMaxEvent);
  PLOG_IF(FATAL, epollfd < 0) << "epoll create fail";

  bool status = true;
  std::string reason("success");
  int64 nfds = 0;
  int32 bitmask, fd_num;
  timeval tv, *tvp;
  ares_socket_t socks[ARES_GETSOCK_MAXNUM];
  while (true) {
    request_on_queue->Request(channelptr);
    bitmask = ares_getsock(*channelptr, socks, ARES_GETSOCK_MAXNUM);

    epoll_event ev;
    fd_num = 0;
    for (int i = 0; i < ARES_GETSOCK_MAXNUM; ++i) {
      ev.events = 0;
      if (ARES_GETSOCK_READABLE(bitmask, i)) {
        ev.events |= EPOLLRDNORM|EPOLLIN;
        ev.data.fd = socks[i];
      }
      if (ARES_GETSOCK_WRITABLE(bitmask, i)) {
        ev.events |= EPOLLWRNORM|EPOLLOUT;
        ev.data.fd = socks[i];
      }
      if (ev.events == 0) break;
      ++fd_num;
      if (0 != epoll_ctl(epollfd, EPOLL_CTL_ADD, ev.data.fd, &ev)) {
        if (0 != epoll_ctl(epollfd, EPOLL_CTL_MOD, ev.data.fd, &ev)) {
          std::stringstream msg;
          msg << "epoll_ctl() fail";
          PLOG(ERROR) << msg.str();
          status = false;
          reason = msg.str();
          goto END_RESOLVER;
        }
      } else {
        DVLOG(5) << "Adding " << ev.data.fd << " to epoll success";
      }
    }
    if (fd_num == 0) {
      pthread_mutex_lock(&request_on_queue->mutex);
      if (request_on_queue->active_request_num == 0 &&
          request_on_queue->NoMoreToRequest()) {
        pthread_mutex_unlock(&request_on_queue->mutex);
        // 处理完所有请求
        break;
      }
      pthread_mutex_unlock(&request_on_queue->mutex);
      base::SleepForMilliseconds(30);
      continue;
    }

    tvp = ares_timeout(*channelptr, NULL, &tv);
    CHECK_NOTNULL(tvp);
    int max_wait_time = tvp->tv_sec * 1000 + tvp->tv_usec / 1000;
    if (max_wait_time == 0) max_wait_time = 20;
    CHECK_GE(max_wait_time, 0);

    epoll_event events[kMaxEvent];
    nfds = epoll_wait(epollfd, events, kMaxEvent, max_wait_time);
    if (nfds == -1 && errno != EINTR) {
      std::stringstream msg;
      msg  << "epoll_wait() fail, errno: " << errno;
      PLOG(ERROR)  << msg.str();
      reason = msg.str();
      status = false;
      goto END_RESOLVER;
    }
    if (nfds == 0) {
      ares_process_fd(*channelptr, ARES_SOCKET_BAD, ARES_SOCKET_BAD);
      continue;
    }

    // XXX(huangboxiang): 只是打日志而已, 不加锁
    LOG_EVERY_N(INFO, 500) << "thread: " << thread::GetThreadID()
                           << ", fdnm=" << fd_num << ", rqst_que_sz=" << request_on_queue->all_hosts->Size()
                           << ", rqst_issu=" << request_on_queue->issued_request_num
                           << ", actv_rqst=" << request_on_queue->active_request_num
                           << ", succ_nm=" << request_on_queue->success_num
                           << ", fal_nm=" << request_on_queue->fail_num
                           << ", max wtm=" << max_wait_time << ", nfds=" << nfds;

    for (int32 i = 0; i < nfds; ++i) {
      epoll_event *ptr_ev = &events[i];
      ares_process_fd(*channelptr,
                      ptr_ev->events & (EPOLLRDNORM|EPOLLIN) ?
                      ptr_ev->data.fd:ARES_SOCKET_BAD,
                      ptr_ev->events & (EPOLLWRNORM|EPOLLOUT) ?
                      ptr_ev->data.fd:ARES_SOCKET_BAD);
    }
  }

 END_RESOLVER:
  LOG(INFO) << "thread: " << thread::GetThreadID()
            << ", sum: rqst_que_sz=" << request_on_queue->all_hosts->Size()
            << ", rqst_issu=" << request_on_queue->issued_request_num
            << ", actv_rqst=" << request_on_queue->active_request_num
            << ", succ_nm=" << request_on_queue->success_num
            << ", fal_nm=" << request_on_queue->fail_num;
  close(epollfd);
  this->dns_status = (status ? 0 : -1);
  this->reason = reason;
  LOG(INFO) << "thread: " << thread::GetThreadID()
            << ", status = " << (status ? "SUCCESS" : "FAILED")
            << ", reason = " << reason;
  // end thread, and cancel all request has not finished yet
  if (status) {
    CHECK_EQ(request_on_queue->all_hosts->Size(), 0);
    CHECK(request_on_queue->all_hosts->Closed());
    CHECK_EQ(request_on_queue->active_request_num, 0);
    CHECK_EQ(request_on_queue->availables.Size(), (int64)request_on_queue->requests.size());
  }

  pthread_mutex_lock(&request_on_queue->mutex);
  if (!status && request_on_queue->active_request_num != 0) {
    LOG(WARNING) << "Cancel DNS channel, remaining active request num: "
                 << request_on_queue->active_request_num;
    ares_cancel(*channelptr);
  }
  pthread_mutex_unlock(&request_on_queue->mutex);
}


void AresHostCallbackOnQueue(void *arg, int status, int timeouts, hostent *hostent) {
  DVLOG(5) << "ARES Call back " << ", status = " << status
           << ", timeouts = " << timeouts
           << ", host: " << static_cast<QueueData *>(arg)->info->host;
  QueueData* queue_data = static_cast<QueueData *>(arg);
  HostIpInfo* host_info = queue_data->info;
  CHECK_NOTNULL(host_info);

  host_info->ips.clear();
  if (status != ARES_SUCCESS) {
    DVLOG(2) << "Get Ip fail, host: " << host_info->host
             << ", err code: " << status << " err msg: " << ares_strerror(status);
    if (status == ARES_ETIMEOUT || status == ARES_ECONNREFUSED) {
      // ARES_ECONNREFUSED : Could not contact DNS servers, Maybe DNS server too busy to accept this query
      host_info->ip_state = kGetIpFailureTimeOut;
    } else if (status == ARES_ECANCELLED) {
      host_info->ip_state = kCancel;
    } else {
      host_info->ip_state = kGetIpFailureNoIp;
    }
  } else {
    CHECK_NOTNULL(hostent);
    char ip[INET_ADDRSTRLEN];
    for (int i = 0; hostent->h_addr_list[i]; ++i) {
      ip[0] = '\0';
      CHECK_NOTNULL(inet_ntop(AF_INET, hostent->h_addr_list[i], ip, INET_ADDRSTRLEN));
      host_info->ips.push_back(std::string(ip));
    }
    host_info->ip_state = kGetIpOK;
  }

  RequestOnQueue* rqueue = reinterpret_cast<RequestOnQueue *>(queue_data->root);
  CHECK_NOTNULL(rqueue);
  {
    pthread_mutex_lock(&rqueue->mutex);
    if (host_info->ip_state == kGetIpOK) {
      ++rqueue->success_num;
    } else if (host_info->ip_state == kGetIpFailureTimeOut) {
      if (queue_data->info->retry >= rqueue->max_retry) {
        ++rqueue->fail_num;
      } else {
        queue_data->info->retry++;
        rqueue->all_hosts->Put(queue_data->info);
        DVLOG(5) << "Retry: " << queue_data->info->host << ", " << queue_data->info->retry;
      }
    } else {
      ++rqueue->fail_num;
    }
    --rqueue->active_request_num;
    rqueue->availables.Put(queue_data - &rqueue->requests[0]);
    DVLOG(5) << queue_data - &rqueue->requests[0] << "th pos return back to availables";
    DVLOG(5) << "active request num: " << rqueue->active_request_num;
    pthread_mutex_unlock(&rqueue->mutex);
  }
  return;
}

static int32 StatusAndReasons(const std::vector<DnsResolver *>& resolvers,
                       std::vector<std::string>* reasons) {
  if (reasons) {
    for (int32 i = 0; i < (int32)resolvers.size(); ++i) {
      reasons->push_back(resolvers[i]->reason);
    }
  }
  int status = 0;
  for (int32 i = 0; i < (int32)resolvers.size(); ++i) {
    if (resolvers[i]->dns_status != 0) {
      status = resolvers[i]->dns_status;
      break;
    }
  }
  return status;
}

int32 DNSQueries(const std::string& dns_servers,
                 int32 window_size_each_server,
                 std::vector<HostIpInfo> *hosts,
                 std::vector<std::string> *reasons) {
  std::vector<std::string> servers;
  base::SplitStringWithOptions(dns_servers, ",", true, true, &servers);

  std::vector<DnsResolver *> resolvers;
  std::vector<RequestOnQueue *> requesters;
  int32 threads_num;
  if (servers.empty()) {
    // use default name server in /etc/resolv.conf or local named
    threads_num = 1;
    servers.push_back("");
  } else {
    threads_num = (int32)servers.size();
  }

  // Split hosts to each name server by CithHash(host)
  std::vector<thread::BlockingQueue<HostIpInfo *> *> requests_queues;
  for (int32 i = 0; i < threads_num; ++i) {
    thread::BlockingQueue<HostIpInfo *> *block_queue = new thread::BlockingQueue<HostIpInfo *>;
    requests_queues.push_back(block_queue);
  }
  for (int64 i = 0; i < (int64)hosts->size(); ++i) {
    HostIpInfo& info = (*hosts)[i];
    info.retry = 0;
    info.ips.clear();
    int server_id = base::CityHash64(info.host.data(), info.host.size()) % threads_num;
    requests_queues[server_id]->Put(&info);
  }

  thread::ThreadPool requests_pool(threads_num);
  // 启动查询
  for (int32 i = 0; i < threads_num; ++i) {
    DnsResolver* res = new DnsResolver(servers[i], window_size_each_server);
    RequestOnQueue* request_on_queue = new RequestOnQueue(window_size_each_server,
                                                          requests_queues[i],
                                                          FLAGS_dns_outer_try_times);
    CHECK(res->Init());
    resolvers.push_back(res);
    requesters.push_back(request_on_queue);
    requests_pool.AddTask(
        NewCallback(res, &DnsResolver::ResolveHostQueue, request_on_queue));
  }

  // 检查所有查询结束
  int successive = 0;
  while (true) {
    base::SleepForMilliseconds(200);
    if (successive > 2) {
      break;
    }
    int32 all_zero = 0;
    for (int32 i = 0; i < threads_num; ++i) {
      if (requests_queues[i]->Empty()) {
        pthread_mutex_lock(&requesters[i]->mutex);
        if (requesters[i]->active_request_num == 0) {
          all_zero++;
        }
        pthread_mutex_unlock(&requesters[i]->mutex);
      }
    }
    if (all_zero == threads_num) {
      successive++;
      continue;
    } else {
      successive = 0;
    }
  }

  //  Close all block queue
  for (int32 i = 0; i < threads_num; ++i) {
    requests_queues[i]->Close();
  }
  DVLOG(5) << "Close all requests queue";
  requests_pool.JoinAll();

  int32 ret = StatusAndReasons(resolvers, reasons);
  for (int32 i = 0; i < threads_num; ++i) {
    delete resolvers[i];
    delete requesters[i];
    delete requests_queues[i];
  }

  return ret;
}
}  // end namespace
