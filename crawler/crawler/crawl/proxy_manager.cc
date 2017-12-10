#include "crawler/crawl/proxy_manager.h"

#include <algorithm>
#include <limits>

#include "base/file/file_util.h"
#include "base/strings/string_split.h"
#include "base/hash_function/city.h"



namespace crawler2 {
namespace crawl {

void ProxyManager::Init() {
  int idx = 0;
  for (auto iter = options_.proxy.begin();
       iter != options_.proxy.end(); ++iter, ++idx) {
    proxy_.push_back(ProxyData(*iter));
    proxy_index_[*iter] = idx;
  }
}

std::string ProxyManager::SelectBestProxy(int64 timestamp) {
  int best_proxy_idx = -1;
  int min_task_num = std::numeric_limits<int>::max();
  ::thread::AutoLock auto_lock(&mutex_);
  for (size_t i = 0; i < proxy_.size(); ++i) {
    if (proxy_[i].successive_failed_times >= options_.max_successive_failures) {
      // 如果在某个代理上连续失败次数过多，则等待一段时间后重试
      if (timestamp - proxy_[i].last_failed_timestamp <
          options_.holdon_duration_after_failures) {
        continue;
      } else {
        // 如果已经等够了时间，则将连续失败次数减少 2
        // 并将最后一次失败时间设成当前时间
        // 如果连续失败 2 次，则继续等待一段时间
        proxy_[i].successive_failed_times
            = std::max(0, proxy_[i].successive_failed_times - 2);
        proxy_[i].last_failed_timestamp = timestamp;
      }
    }

    if (proxy_[i].tasks < min_task_num) {
      best_proxy_idx = i;
      min_task_num = proxy_[i].tasks;
    }
  }

  if (-1 != best_proxy_idx) {
    proxy_[best_proxy_idx].tasks++;
    return proxy_[best_proxy_idx].proxy;
  } else {
    return "";
  }
}

void ProxyManager::ReportStatus(const std::string& proxy, bool success,
                                int64 timestamp) {
  ::thread::AutoLock auto_lock(&mutex_);
  auto iter = proxy_index_.find(proxy);
  CHECK(iter != proxy_index_.end()) << "cannot find proxy: " << proxy;
  int idx = iter->second;
  if (success) {
    proxy_[idx].success_times++;
    proxy_[idx].successive_failed_times = 0;
  } else {
    proxy_[idx].successive_failed_times++;
    proxy_[idx].failed_times++;
    proxy_[idx].last_failed_timestamp = timestamp;
  }

  proxy_[idx].tasks--;
}
}  // namespace crawl
}  // namespace crawler
