#pragma once

#include <curl/curl.h>

#include <string>
#include <utility>

#include "base/common/basic_types.h"
#include "base/time/timestamp.h"
#include "crawler/proto/crawled_resource.pb.h"

namespace crawler {

// ResourceType 在文件 crawled_resource.proto 定义
struct UrlHostIP {
  std::string url;
  std::string refer_or_modified_time;
  std::string site;
  std::string ip;
  ResourceType resource_type;
  double importance;
  // 表示该 Url 的数据来源
  // 'U' : 表示来自 User Log
  // 'S' : 表示来自 Search Log
  // 'P' : 表示来自 PV Log
  // 'N' : 表示来自 网页提取出来的新的 link
  // 'E' : 表示来自 用户通过 Seeds 配置文件
  // 'L' : 表示来自 Linkbase
  // 'M' : 表示来之 更新模块的任务
  // 'K' : 表示不知道 该 url 的来源
  char from;
  // 表示该 URL 的 refer 的来源
  char refer_from;
  // 该 链接的状态 (是否已经调度)
  // '0': 表示还没有调度
  char status;
  std::string payload;
  UrlHostIP() {}
  explicit UrlHostIP(const std::string &url): url(url), resource_type(kHTML), importance(0.5), from('K'),
    refer_from('K'), status('0') {}
  explicit UrlHostIP(const std::string &url, const std::string &r_m, const std::string &site,
                     const std::string &ip, const ResourceType resource_type, double importance, char from,
                     char refer_from, const std::string &payload)
      : url(url), refer_or_modified_time(r_m), site(site), ip(ip), resource_type(resource_type),
      importance(importance),
      from(from), refer_from(refer_from), status('0'), payload(payload) {}
  bool operator<(const UrlHostIP &t2) const {
    return url < t2.url;
  }
  bool operator==(const UrlHostIP &t2) const {
    return url == t2.url;
  }
};

struct PrivateData {
  int buffer_id;
  UrlHostIP url_info;
  ::curl_slist *slist;
  // 抓取的起始时间点
  int64 begin_timepoint;
  PrivateData(): buffer_id(-1), slist(NULL), begin_timepoint(base::GetTimestamp()) {}
  PrivateData(int id, const UrlHostIP &url_info): buffer_id(id), url_info(url_info), slist(NULL),
    begin_timepoint(base::GetTimestamp()) {}
};

// 记录站点访问统计信息
struct HostLoadInfo {
  // 当对一个站点进行过载保护时, 保护截止时间点
  int64 expire_timepoint;
  // 一个保护周期的开始时间点, 用于实时计算 QPS 时需要用到
  int64 begin_timepoint;
  // 连续成功访问次数
  int success_cnt;
  // 连续失败次数
  int fail_cnt;
  // 一个保护周期内的当前总访问次数
  int total_cnt;
  // 并发访问数
  int concurrent_access_cnt;

  HostLoadInfo(): expire_timepoint(0), begin_timepoint(base::GetTimestamp()), success_cnt(0), fail_cnt(0),
    total_cnt(0), concurrent_access_cnt(0) {}
  HostLoadInfo(int64 expire, int64 begin, int success_cnt, int fail_cnt, int total_cnt, int current_cnt):
      expire_timepoint(expire), begin_timepoint(begin), success_cnt(success_cnt), fail_cnt(fail_cnt),
      total_cnt(total_cnt), concurrent_access_cnt(current_cnt) {}
};

// 站点访问负载控制参数
struct HostLoadControl {
  // 控制策略生效的时间区间, 前闭后闭
  int start_hour;
  int start_min;
  int end_hour;
  int end_min;
  // 并发访问链接数
  int max_concurrent_access;
  int max_qps;
  HostLoadControl(int s_h, int s_m, int e_h, int e_m, int cur_acc, int qps):
      start_hour(s_h), start_min(s_m), end_hour(e_h), end_min(e_m), max_concurrent_access(cur_acc),
      max_qps(qps) {}
};


// 判断 |host| 是否是 VIP host.
// TODO(pengdan): 不要使用这个函数
// NOTE:
// 爬虫只会从 VIP host 的站点首页或者目录首页提起新的链接
bool IsVIPHost(const std::string &host);
}  // end crawler
