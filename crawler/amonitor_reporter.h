#pragma once

//
// 将程序里面的一些计数器，周期上报给amonitor
//
// Created by yumeng on 2017/3/20.
//

#include <string>
#include <map>
#include <vector>
#include <cstdatomic>

#include "base/common/logging.h"
#include "base/thread/thread.h"
#include "base/common/closure.h"
#include "base/common/gflags.h"
#include "curl/curl.h"

namespace spider {

class AmonitorReporter {
 public:
  // 调用这个函数来初始化，多次调用只有第一次生效
  void Init(const std::string& show_service_name);
  // 停止上报数据
  void Stop();

  std::string GetLastCounterValue();

 private:
  void WorkerRun();
  void ReportToAmonitor(const std::string &json, const std::string &url);

 public:
  static AmonitorReporter &GetInstance();

 private:
  thread::Thread worker_thread_; // 真正周期上报的线程
  std::atomic<bool> is_already_started_; // 是否工作线程已经开启
  std::vector<std::string> counter_groups_; // 需要上报的那些分组名
  bool is_stop_; // 标识是否已停止上报
  std::string show_service_name_; // 上报给amonitor时使用的名字
  std::map<std::string, int64> last_int64_counter_values; // 记录上一次的计数器的结果
  std::map<std::string, double> last_double_counter_values; // 记录上一次的计数器的结果

 DISALLOW_IMPLICIT_CONSTRUCTORS(AmonitorReporter); // 禁止外部创建及copy
};

} // namespace spider 
