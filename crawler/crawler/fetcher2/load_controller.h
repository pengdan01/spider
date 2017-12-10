#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "base/common/basic_types.h"
#include "base/time/timestamp.h"
#include "base/common/logging.h"
#include "base/random/pseudo_random.h"

namespace crawler2 {

// 压力控制器
class LoadController {
 public:
  struct Options {
    int default_max_connections;
    int default_max_qps;
    // 整个 fetcher 允许的最大连接数
    int max_connections_in_all;

    // 累计抓取次数没达到 min_fetch_times_ 时，不做 QPS 检查
    int min_fetch_times;

    // 在连续失败次数超限 (failed_times_threadhold_) 后，
    // 该 IP 需要等待一段时间 (holdon_duration_after_failed_)
    int min_holdon_duration_after_failed;
    int max_holdon_duration_after_failed;
    int failed_times_threadhold;
    // 在某 IP 上连续失败超过此限制之后，将丢弃所有此 IP 上的 URL
    int max_failed_times;
    uint64 random_seed;

    Options()
        : default_max_connections(5)
        , default_max_qps(3)
        , max_connections_in_all(1000)
        , min_fetch_times(15)
        , min_holdon_duration_after_failed(5000 * 1000)
        , max_holdon_duration_after_failed(5000 * 1000)
        , failed_times_threadhold(10)
        , max_failed_times(100)
        , random_seed(0) {
    }
  };

  // 一个 IP 在一个时间段的压力控制
  struct IpLoadRecord {
    // 时间段是从凌晨开始的分钟数，前闭后闭区间
    int begin_time_in_min;
    int end_time_in_min;

    int max_qps;
    int max_connections;

    // 发起 min_fetch_times 次抓起应该耗费的时间
    // 由 min_fetch_times 和 max_qps 计算得到, 不在配置文件中存储
    int64 min_duration;

    // NOTE: 修改该类定义时，记得在 LoadController 的构造函数里, 修改
    // default_ip_load_ 的初始化代码
  };

  // 参数 holdon_duration_after_failed, 在抓取失败后针对失败 ip 需要休息的时间
  explicit LoadController(const Options& options);

  // 从文件中加载抓取压力控制数据
  bool Load(const std::string &file_name);
  // 从字符串中加载抓取压力控制数据
  bool SetIpLoadRecords(const std::string &file_content);

  // |timestamp| 是调用 base::GetTimestamp() 获取的时间戳, 是 微秒 数 since unix epoch
  const IpLoadRecord *FindIpLoadRecord(const std::string &ip, int64 timestamp) const;

  // 获取系统可用并发连接数
  int GetIdelSlots() const;

  // 获取一个 ip 上的抓取次数
  int GetFetchTimes(const std::string &ip) const;
  // 获取总的抓取次数
  int64 GetAllFetchTimes() const;

  // 判断 url 是否可以抓取
  //
  // 如果可以抓取，则返回 0
  // 如果超过 QPS 限制, 则返回需要等待的时间点
  // 如果连续失败超限，则返回需要等待的时间点
  // 如果上述条件均不满足，但该 ip 的连接数已满, 则返回 -1
  // 如果连续失败数超过上限，则返回 -2, 表示再也不应尝试抓取
  //
  // NOTE:
  // 1. 若无空闲槽位 (GetIdelSlots() 返回 0), 调用该函数会导致程序崩溃
  // timestamp 应该是调用函数时, 用 base::GetTimestamp() 获取的当前时间。
  // 在单测里, timestamp 可以按测试需求设定
  int64 CheckFetch(const std::string &ip, int64 timestamp);

  // 注册/注销 抓取请求
  void RegisterIp(const std::string &ip, int64 timestamp);

  //
  void UnregisterIp(const std::string &ip, int64 timestamp, bool success);
 private:
  // 记录对一个 IP 的抓取状态
  struct IpLoadStatus {
    // 活动连接数 (已发起，但未返回结果)
    int connection_num;

    // 上一次超时后, 第一个请求的发起时间
    int64 first_fetch_time;
    // 上一次超时后, 累计发出的请求数
    int fetch_times;

    // 历史上发出的总请求数
    int64 fetch_times_in_history;

    // 最近一次连续失败的开始时间
    int64 latest_failed_time;
    // 连续失败的次数
    int64 failed_times;

    int64 holdon_duration_after_failed;

    // 当前正在生效的 压力 配置
    // 若该 ip 在不同时间段有不同配置,
    // 在跨越时间段时，压力控制策略可能会有跳变
    const IpLoadRecord *effective_ip_load;

    // NOTE:
    // 每次 fetch_times 刚好超过 min_fetch_times 时,
    // 需要重新设置 |first_fetch_time|, |fetch_times| 和 |fetch_times_in_history|.

    IpLoadStatus() {
      connection_num = 0;
      first_fetch_time = -1;  // -1 表示非法值
      fetch_times = 0;
      fetch_times_in_history = 0;
      latest_failed_time = 0;
      failed_times = 0;
      holdon_duration_after_failed = 0;
      effective_ip_load = NULL;
    }
  };

  static bool CompareIpLoad(const IpLoadRecord &l, const IpLoadRecord &r);

  IpLoadRecord default_ip_load_;

  // ip -> 不同时间段的压力配置, 按 begin_time 增序
  std::unordered_map<std::string, std::vector<IpLoadRecord>> ip_load_records_;

  // ip 的抓取动态
  std::unordered_map<std::string, IpLoadStatus> status_list_;

  int64 started_in_all_;
  int connections_in_all_;
  Options options_;
  base::PseudoRandom random_;
};

}  // namespace crawler2

