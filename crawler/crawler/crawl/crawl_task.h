#pragma once

#include <string>
#include <deque>
#include <vector>

#include "base/common/basic_types.h"
#include "base/common/closure.h"
#include "crawler/proto2/resource.pb.h"

namespace crawler2 {
namespace crawl {

// 此数据仅仅用来保存一条任务
// extra 保存用户数据，需用户自己释放
struct CrawlTask {
  std::string url;
  std::string referer;
  std::string ip;
  int resource_type;
  double importance;
  bool use_proxy_dns:1;
  bool https_supported:1;  // 是否支持 https
  bool always_create_new_link:1;  // 是否每次都创建新连接

  // 如果使用了代理， 此处保存代理的相关数据
  std::string proxy;  // 代理服务器的详细信息, 如果为空表示不是用代理
  std::string dns;   // DNS 服务器的详细信息
  std::string user_agent;  // 抓取该 url 使用该 UA
  std::string cookie;  // 抓取该 url 使用该  cookie
  void* extra;   // 用户数据
  std::string post_body;  // post body data in POST request

  CrawlTask()
      : resource_type(kHTML)
      , importance(0.5f)
      , use_proxy_dns(false)
      , https_supported(false)
      , always_create_new_link(false)
      , extra(NULL) {
  }
};

// 抓取当中的状态
struct CrawlRoutineStatus {
  int ret_code;  // 抓取返回值
  bool success:1;
  int64 begin_time;
  int64 end_time;
  int retry_times;
  int redir_times;  // 跳转次数
  int file_bytes;
  int download_bytes;
  std::string effective_url;
  bool truncated:1;
  std::string error_desc;

  CrawlRoutineStatus()
      : ret_code(0)
      , success(false)
      , begin_time(-1)
      , end_time(-1)
      , retry_times(0)
      , redir_times(0)
      , file_bytes(0)
      , download_bytes(0)
      , truncated(false) {
  }
};

// 同时抓取多个网页时使用
struct WholeTask {
  size_t completed_task;
  std::vector<CrawlTask*>* tasks;
  std::vector<CrawlRoutineStatus*>* status;
  std::vector<Resource*>* resources;
  Closure* whole_task_done;

  WholeTask()
      : completed_task(0)
      , tasks(NULL)
      , status(NULL)
      , resources(NULL)
      , whole_task_done(NULL) {
  }
};

}  // namespace crawl
}  // namespace crawler
