//
// Created by yumeng on 2017/3/19.
//

#include <string>
#include <list>
#include <set>
#include <map>
#include <vector>
#include <sstream>

#include "base/random/pseudo_random.h"
#include "base/strings/string_split.h"
#include "crawler/proto2/resource.pb.h"
#include "crawler/realtime_crawler/crawl_queue.pb.h"

namespace spider {

static base::PseudoRandom proxy_rander;

// 从一组代理列表里，随机选一个代理出来，排除掉except_proxy
std::string SelectRandProxy(crawler2::crawl_queue::JobInput &job) {
  std::vector<std::string> proxy_list;
  std::vector<std::string> valid_proxy_list;
  // 按逗号进行分隔，取多个代理列表
  base::SplitString(job.detail().proxy(), ",", &proxy_list);

  for (auto it = proxy_list.begin(); it != proxy_list.end(); ++it) {
    std::string proxy = *it;
    base::TrimWhitespaces(&proxy);
    if (!proxy.empty()) {
      valid_proxy_list.push_back(proxy);
    }
  }
  if (valid_proxy_list.empty()) {
    return "";
  }

  int r = proxy_rander.GetInt(0, (int) valid_proxy_list.size() - 1);
  std::string selected_proxy = valid_proxy_list[r];

  // 如果可用代理大于1个，就删掉本次随机选取的代理，下一次就会使用其它代理了
  if (valid_proxy_list.size() > 1) {
    // 从未使用的数组里删除它
    valid_proxy_list.erase(valid_proxy_list.begin() + r);
    // 更新未使用过的代理使用情况
    // 先关闭，可能有
    // job.mutable_detail()->set_proxy(base::JoinStrings(valid_proxy_list, ","));
  }
  return selected_proxy;
}

} // namespace spider
