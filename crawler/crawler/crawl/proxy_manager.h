#pragma once

#include <string>
#include <vector>
#include <map>

#include "base/common/basic_types.h"
#include "base/thread/sync.h"

namespace crawler2 {
namespace crawl {
class ProxyManager {
 public:
  struct Options {
    // 失败多少次后需要休息
    int max_successive_failures;
    // 失败次数次数过多之后休息的时间
    // 如果为-1， 那么不再使用代理
    int64 holdon_duration_after_failures;  // 单位 second
    std::vector<std::string> proxy;
    Options()
        : max_successive_failures(20)
        , holdon_duration_after_failures(30 * 1000 * 1000l) {
    }
  };

  explicit ProxyManager(const Options& options)
      : options_(options) {
  }

  void Init();
  std::string SelectBestProxy(int64 timestamp);
  void ReportStatus(const std::string& proxy, bool success, int64 timestamp);
 private:
  struct ProxyData {
    std::string proxy;
    int failed_times;   // 失败的次数
    int success_times;  // 成功的次数
    int successive_failed_times;  // 连续失败的次数
    int last_failed_timestamp;  // 最后一次失败的时间
    int tasks;  // 当前在线的任务

    explicit ProxyData(const std::string& p)
        : proxy(p)
        , failed_times(0)
        , success_times(0)
        , successive_failed_times(0)
        , last_failed_timestamp(-1)
        , tasks(0) {
    }
  };
  Options options_;
  std::vector<ProxyData> proxy_;
  std::map<std::string, int> proxy_index_;
  ::thread::Mutex mutex_;
  DISALLOW_COPY_AND_ASSIGN(ProxyManager);
};

}  // namespace crawl
}  // namespace crawler2
