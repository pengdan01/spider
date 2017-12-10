//
// Created by yumeng on 2017/3/20.
//

#include <sstream>

#include "base/thread/thread_util.h"
#include "storage/base/process_util.h"
#include "base/strings/string_split.h"
#include "base/common/sleep.h"
#include "amonitor_reporter.h"
#include "base/strings/string_printf.h"
#include "base/time/timestamp.h"
#include "spider/crawler/util.h"

DEFINE_string(amonitor_address, "11.226.2.7", "amonitor proxy服务的ip+端口, 80端口可不写");

// 先声明 未公开的仅限内部使用的 若干 counters API
namespace google {
extern void GetAllCounters(std::vector<CommandLineFlagInfo> *);
extern void GetAllCountersInGroup(const char *, std::vector<CommandLineFlagInfo> *);
extern bool FindCounter(const char *, const char *, CommandLineFlagInfo *res);
namespace internal {
extern int64 GetRawInt64CounterValue(void *c);
extern double GetRawDoubleCounterValue(void *c);
}  // namespace internal
}  // namespace google

spider::AmonitorReporter::AmonitorReporter() : is_already_started_(false), is_stop_(false) {

}

void spider::AmonitorReporter::Init(const std::string &show_service_name) {
  if (this->is_already_started_.exchange(true)) {
    // 老的值也是true，说明已经初始化过了，这里就不要重复初始化了
    return;
  }
  this->show_service_name_ = show_service_name;
  // init http client
  curl_global_init(CURL_GLOBAL_ALL);
  counter_groups_.clear();
  counter_groups_.push_back("job_manager");
  counter_groups_.push_back("resource_handler");
  counter_groups_.push_back("kafka_client");
  counter_groups_.push_back("general_crawler");
  this->worker_thread_.Start(NewCallback(this, &AmonitorReporter::WorkerRun));
  LOG(INFO) << "Start amonitor reporter thread, with service name: " << this->show_service_name_;
}

void spider::AmonitorReporter::Stop() {
  is_stop_ = true;
  this->worker_thread_.Join();
  curl_global_cleanup();
}

spider::AmonitorReporter &spider::AmonitorReporter::GetInstance() {
  static AmonitorReporter reporter;
  return reporter;
}

bool CommandLineFlagInfo2Counter(const google::CommandLineFlagInfo &flag,
                                 std::string *group, std::string *name,
                                 bool *is_float, int64 *v, double *f) {
  std::vector<std::string> tokens;
  base::SplitStringUsingSubstr(flag.name, "__", &tokens);
  if (tokens.size() != 2 || tokens[0].empty() || tokens[1].empty()) {
    LOG(ERROR) << "failed to parse group/name for counter '" << flag.name << "'";
    return false;
  }
  group->assign(tokens[0]);
  name->assign(tokens[1]);

  if (flag.type == "Int64Counter") {
    *is_float = false;
    *v = google::internal::GetRawInt64CounterValue(const_cast<void *>(flag.flag_ptr));
  } else if (flag.type == "DoubleCounter") {
    *is_float = true;
    *f = google::internal::GetRawDoubleCounterValue(const_cast<void *>(flag.flag_ptr));
  } else {
    LOG(ERROR) << "invalid counter type for counter '" << flag.name << "', type: " << flag.type;
    return false;
  }

  return true;
}

std::string spider::AmonitorReporter::GetLastCounterValue() {
  std::map<std::string, std::string> result;

  for (auto iter = counter_groups_.begin(); iter != counter_groups_.end(); ++iter) {
    std::vector<google::CommandLineFlagInfo> flags;
    // 指定分组名，只上报xss_spider分组下的counter，其它的不报，防止误报
    google::GetAllCountersInGroup(iter->c_str(), &flags);
    if (flags.empty()) {
      continue;
    }
    for (int i = 0; i < (int) flags.size(); ++i) {
      google::CommandLineFlagInfo &flag = flags[i];
      std::string group;
      std::string name;
      std::string value;
      bool is_float;
      int64 v;
      double f;
      if (!CommandLineFlagInfo2Counter(flag, &group, &name, &is_float, &v, &f)) {
        continue;
      }
      if (is_float) {
        value = base::StringPrintf("%g", f);
      } else {
        value = base::StringPrintf("%lu", v);
      }
      std::string group_and_name = group + "__" + name;
      result[group_and_name] = value;
    }
  }

  return Map2String(result);
}

void spider::AmonitorReporter::WorkerRun() {
  thread::SetCurrentThreadName("AmonitorReport");
  const std::string amonitor_url = "http://" + FLAGS_amonitor_address + "/report_body";

  // 上报amonitor时，使用client ip要针对docker容器做一些处理
  std::string host_id;
  const char* docker_host_ip = getenv("DOCKER_HOST_IP");
  const char* docker_container_name = getenv("DOCKER_CONTAINER_NAME");
  if (docker_host_ip != NULL && docker_container_name != NULL) {
    // 如果在docker容器内，则使用docker宿主机ip + docker容器名来做唯一标识
    const std::string local_ip = docker_host_ip;
    const std::string pid = docker_container_name;
    host_id = local_ip + "-" + pid;
  } else {
    host_id = base::GetLocalIP();
  }
  while (!is_stop_) {
    base::SleepForSeconds(1);
    int64_t currentTimestamp = base::GetTimestamp() / 1000;
    std::ostringstream oss;
    oss << "{\n";
    oss << "\"host\": \"" << host_id << "\",\n";
    oss << "\"app_name\": \"" << this->show_service_name_ << "\",\n";
    oss << "\"tm\": " << currentTimestamp << ",\n";
    oss << "\"body\": [\n";
    int key_value_cnt = 0;
    for (auto iter = counter_groups_.begin(); iter != counter_groups_.end(); ++iter) {
      std::vector<google::CommandLineFlagInfo> flags;
      // 指定分组名，只上报xss_spider分组下的counter，其它的不报，防止误报
      google::GetAllCountersInGroup(iter->c_str(), &flags);
      if (flags.empty()) {
        // 没有合适的计数器，直接跳出循环，TODO: 是否可以考虑直接返回？因为下一次循环也不会有计数器了
        continue;
      }
      for (int i = 0; i < (int) flags.size(); ++i) {
        google::CommandLineFlagInfo &flag = flags[i];
        std::string group;
        std::string name;
        std::string value;
        bool is_float;
        int64 v;
        double f;
        if (!CommandLineFlagInfo2Counter(flag, &group, &name, &is_float, &v, &f)) {
          continue;
        }
        if (is_float) {
          value = base::StringPrintf("%g", f);
        } else {
          value = base::StringPrintf("%lu", v);
        }

        if (key_value_cnt > 0) {
          oss << ", ";
        }
        key_value_cnt++;

        std::string group_and_name = group + "__" + name;
        oss << "{\n";
        oss << "\"name\": \"" << group_and_name << "\",\n";
        oss << "\"value\": \"" << value << "\",\n";
        oss << "\"metric_type\": \"STATUS_METRIC\",\n";
        oss << "\"unit\": \"-\"}";

        // 如果是total_开头的计数器，说明是累积的值，这些把这些累积的值的增量算出来，再上报
        if (base::StartsWith(name, "total_", true)) {
          double delta;
          if (is_float) {
            delta = std::max(0.0, f - this->last_double_counter_values[group_and_name]);
            this->last_double_counter_values[group_and_name] = f;
          } else {
            delta = std::max(0L, v - this->last_int64_counter_values[group_and_name]);
            this->last_int64_counter_values[group_and_name] = v;
          }
          std::string delta_name = "delta_" + name.substr(6);
          oss << ", ";
          oss << "{\n";
          oss << "\"name\": \"" << group << "__" << delta_name << "\",\n";
          oss << "\"value\": \"" << delta << "\",\n";
          oss << "\"metric_type\": \"STATUS_METRIC\",\n";
          oss << "\"unit\": \"-\"}";
        }
      }
    }

    oss << "]}";
    if (key_value_cnt == 0) {
      continue;
    }
    std::string json = oss.str();
    ReportToAmonitor(json, amonitor_url);
  }
  LOG(INFO) << "the [AmonitorReport] thread exit";
}

void spider::AmonitorReporter::ReportToAmonitor(const std::string &json, const std::string &url) {
  CURL *curl = curl_easy_init();

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2000);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5000);

  /* Perform the request, res will get the return code */
  CURLcode res = curl_easy_perform(curl);
  /* Check for errors */
  if (res != CURLE_OK) {
    LOG(WARNING) << "amonitor proxy request fail, url: " << url << ", error: " << curl_easy_strerror(res);
  } else {
    LOG_EVERY_N(INFO, 60) << "amonitor proxy request successfully";
  }
  curl_easy_cleanup(curl);
}
