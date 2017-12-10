#include "crawler/crawl/page_crawler.h"

#include <string>
#include "base/common/logging.h"
#include "crawler/crawl/crawl_task.h"


namespace crawler2 {
namespace crawl {
int PageCrawler::CallbackBodyData(char* data, size_t num, size_t unit_size, void* user_data) {
  CrawlRoutineData *routine = static_cast<CrawlRoutineData*>(user_data);
  CHECK_NOTNULL(routine);

  std::string &str = routine->private_data.body;

  int32 old_bytes = str.size();
  int64 delta_bytes = num * unit_size;

  VLOG(4) << "receive body: " << delta_bytes << " bytes";

  if (old_bytes + delta_bytes > kMaxHTMLSize) {
    LOG(WARNING) << "Page size: " << old_bytes + delta_bytes << ", exceed max size: "
                 << kMaxHTMLSize << ", truncated.";
    str.append(data, kMaxHTMLSize - old_bytes);

    // 为了避免截断一个汉字或其他编码字符，回退到最后一个 tag  结尾符 ‘>’
    // while (!str.empty() && *str.rbegin() != '>') {
    //   str.resize(str.size() - 1);
    // }
    // XXX(suhua): 注释掉 "回退到 结尾符 '>'" 这种操作. 作为通用的 fetcher,
    // 不能做这种 tricky 的处理. 截断问题, 由后续应用处理.

    return kMaxHTMLSize - old_bytes;
  } else {
    str.append(data, delta_bytes);
    return delta_bytes;
  }
}

int PageCrawler::CallbackHeadData(char* data, size_t num, size_t unit_size, void* user_data) {
  CrawlRoutineData *routine = static_cast<CrawlRoutineData*>(user_data);
  CHECK_NOTNULL(routine);

  std::string &str = routine->private_data.head;

  int32 old_bytes = str.size();
  int64 delta_bytes = num * unit_size;

  VLOG(4) << "receive head: " << delta_bytes << " bytes";

  const int kMaxHeadSize = 16*1024;
  if (old_bytes + delta_bytes > kMaxHeadSize) {
    LOG(WARNING) << "Head size: " << old_bytes + delta_bytes
                 << ", exceed max size: " << kMaxHeadSize << ", truncated.";
    str.append(data, kMaxHeadSize - old_bytes);
    return kMaxHeadSize - old_bytes;
  } else {
    str.append(data, delta_bytes);
    return delta_bytes;
  }
}

int PageCrawler::CallbackSocket(CURL *easy, curl_socket_t s, int action, void *userp, void *socketp) {
  int epollfd  = *(static_cast<int *>(userp));
  switch (action) {
    case CURL_POLL_IN: {
      VLOG(4) << "socket callback called on epoll fd: " << epollfd
              << ", read event";
      struct epoll_event ev;
      ev.events = 0;
      ev.events |= EPOLLRDNORM|EPOLLIN;
      ev.data.fd = s;
      if (0 != epoll_ctl(epollfd, EPOLL_CTL_ADD, ev.data.fd, &ev)) {
        PLOG_IF(FATAL, epoll_ctl(epollfd, EPOLL_CTL_MOD, ev.data.fd, &ev) != 0);
      }
      break;
    }

  case CURL_POLL_OUT: {
    VLOG(4) << "socket callback called on epoll fd: " << epollfd
            << ", write event";
    struct epoll_event ev;
    ev.events = 0;
    ev.events |= EPOLLRDNORM|EPOLLOUT;
    ev.data.fd = s;
    if (0 != epoll_ctl(epollfd, EPOLL_CTL_ADD, ev.data.fd, &ev)) {
      PLOG_IF(FATAL, epoll_ctl(epollfd, EPOLL_CTL_MOD, ev.data.fd, &ev) != 0);
    }
    break;
  }
  case CURL_POLL_REMOVE: {
    VLOG(4) << "socket callback called on epoll fd: " << epollfd
            << ", remove event";
    struct epoll_event ev;
    ev.events = EPOLLRDNORM|EPOLLIN|EPOLLOUT;
    ev.data.fd = s;
    // XXX(suhua): EPOLL_CTL_DEL failures should be ignored, google "high
    // performace libcurl tips" and see item 2
    // 原因是, fd 可能已经是被关闭, 又被重新打开了, epoll del 可能会失败,
    // 直接忽略失败即可
    epoll_ctl(epollfd, EPOLL_CTL_DEL, ev.data.fd, &ev);
    break;
  }
  default: {
    LOG(ERROR) << "action is not CURL_POLL_IN, CURL_POLL_OUT or CURL_POLL_REMOVE.";
  }
}
return 0;
}


int PageCrawler::CallbackTimer(CURLM *cm, int64 timeout_ms, void *userp) {
  VLOG(4) << "timer callback called: " << timeout_ms << "ms";

  int64 *timeout_ptr = static_cast<int64_t*>(userp);
  if (timeout_ms < 0) {
    *timeout_ptr = 100;
  } else if (timeout_ms > 1000) {
    *timeout_ptr = 1000;
  } else {
    *timeout_ptr = timeout_ms;
  }
  return 0;
}

}  // namespace crawl
}  // namespace crawler2
