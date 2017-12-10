#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>

#include "base/common/basic_types.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "base/file/file_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/common/logging.h"
#include "crawler/crawl/load_controller.h"

namespace crawler2 {
namespace crawl {
LoadController::LoadController(const Options& options)
    : random_(options.random_seed) {
  options_ = options;
  connections_in_all_ = 0;
  started_in_all_ = 0;
  default_ip_load_.begin_time_in_min = 0;
  default_ip_load_.end_time_in_min = 23 * 60 + 59;
  default_ip_load_.max_qps = options.default_max_qps;
  default_ip_load_.max_connections = options.default_max_connections;
  default_ip_load_.min_duration = options.check_frequency / 1.0 / options.default_max_qps * 1000 * 1000;
  CHECK_GE(options.max_failed_times, options.failed_times_threadhold);
  CHECK_GT(options.check_frequency, 1);

  // 加载压力控制配置文件
  const std::string &ip_load_file = options_.ip_load_file;
  if (!ip_load_file.empty()) {
    CHECK(Load(ip_load_file)) << "Load ip load file fail, file: " << ip_load_file;
  }
}

bool LoadController::CompareIpLoad(const IpLoadRecord &l, const IpLoadRecord &r) {
  return l.begin_time_in_min < r.begin_time_in_min;
}

bool LoadController::SetIpLoadRecords(const std::string &file_content) {
  std::vector<std::string> lines;
  base::SplitStringWithOptions(file_content, "\n", true, true, &lines);
  std::vector<std::string> tokens;
  for (auto it = lines.begin(); it != lines.end(); ++it) {
    const std::string &line = *it;
    if (line.empty() || line[0] == '#') continue;
    tokens.clear();
    base::SplitString(line, "\t", &tokens);
    CHECK_EQ(tokens.size(), 4u) << line;

    const std::string &ip = tokens[0];
    // Ignore the ip format check, as the 1st field do NOT have to be a IP, it can also be a host
    // XXX(pengdan): IP 255.255.255.255 会失败
    // LOG_IF(FATAL, ip != "*" && ip != "255.255.255.255" && INADDR_NONE == inet_addr(ip.c_str()))
    //     << "Invalid ip: " << ip;
    IpLoadRecord r;
    r.max_connections = base::ParseIntOrDie(tokens[1]);
    r.max_qps = base::ParseIntOrDie(tokens[2]);

    const std::string &time = tokens[3];
    if (!(time.size() == 11 && time[2] == ':' && time[5] == '-' && time[8] == ':')) {
      // HH:MM-HH:MM
      LOG(ERROR) << "Time format error, must be: HH:MM-HH:MM, but is: " << time;
      return false;
    }
    int sh = base::ParseIntOrDie(time.substr(0, 2));
    int sm = base::ParseIntOrDie(time.substr(3, 2));
    int eh = base::ParseIntOrDie(time.substr(6, 2));
    int em = base::ParseIntOrDie(time.substr(9, 2));
    r.begin_time_in_min = sh * 60 + sm;
    r.end_time_in_min = eh * 60 + em;
    CHECK_LE(r.begin_time_in_min, r.end_time_in_min);
    r.min_duration = options_.check_frequency / 1.0 / r.max_qps * 1000 * 1000;

    ip_load_records_[ip].push_back(r);
  }

  for (auto it = ip_load_records_.begin(); it != ip_load_records_.end(); ++it) {
    std::sort(it->second.begin(), it->second.end(), CompareIpLoad);
    for (int i = 1; i < (int)it->second.size(); ++i) {
      CHECK_GT(it->second[i].begin_time_in_min, it->second[i-1].end_time_in_min)
          << "ip " << it->first << " 的两个压力限制规则的时间段重叠";
    }
  }
  return true;
}

bool LoadController::Load(const std::string &file_name) {
  std::string contents;
  if (!base::file_util::ReadFileToString(file_name, &contents)) {
    LOG(WARNING) << "failed to read file: " << file_name;
  }

  return SetIpLoadRecords(contents);
}

int GetMinuteInDay(int64 timestamp_us) {
  time_t t = timestamp_us / 1000 / 1000;
  struct tm mytm = {0};
  struct tm * ptm = localtime_r(&t, &mytm);
  int hour = ptm->tm_hour;
  int min = ptm->tm_min;
  int min_from_00 = hour * 60 + min;  // 从 00 点开始的 分钟数
  return min_from_00;
}

const LoadController::IpLoadRecord *LoadController::FindIpLoadRecord(
    const std::string &handle_key, int64 timestamp) const {
  // XXX(pengdan): the ip format may like this: ip(or host):proxy_address
  // 180.132.11.90:socks5://127.0,0.1:3000
  // www.baidu.com:socks5://127.0,0.1:3000
  // but when we find the load in the config file, we only use ip(o host)
  std::string ip;
  std::string::size_type pos = handle_key.find(":");
  if (pos == std::string::npos) {
    ip = handle_key;
  } else {
    ip = handle_key.substr(0, pos);
  }

  auto rit = ip_load_records_.find(ip);
  if (rit == ip_load_records_.end()) {
    return &default_ip_load_;
  }

  int min_in_day = GetMinuteInDay(timestamp);
  for (auto it = rit->second.begin(); it != rit->second.end(); ++it) {
    const IpLoadRecord &r = *it;
    if (min_in_day >= r.begin_time_in_min && min_in_day <= r.end_time_in_min) {
      return &r;
    }
  }

  return &default_ip_load_;
}

int LoadController::GetIdelSlots() const {
  int n  = options_.max_connections_in_all - connections_in_all_;
  CHECK_GE(n, 0);
  return n;
}

int64 LoadController::CheckFetch(const std::string &ip, int64 timestamp) {
  // 要求一定有空闲槽位
  CHECK_GT(GetIdelSlots(), 0);

  IpLoadStatus &s = status_list_[ip];

  int64 now_us = timestamp;
  if (s.effective_ip_load == NULL) {
    // 首次涉及该 ip
    s.effective_ip_load = FindIpLoadRecord(ip, now_us);
  }

  if (s.failed_times >= options_.max_failed_times) {
    VLOG(4) << "Too many failures on ip: " << ip;
    return -2;
  }

  // 2. 检查 QPS 过载
  // 仅当抓取次数达到 check_frequency_ 时才检查 QPS 过载
  CHECK_LE(s.fetch_times, options_.check_frequency);
  if (s.fetch_times == options_.check_frequency) {
    CHECK_GE(s.first_fetch_time, 0);
    if (now_us < s.first_fetch_time + s.effective_ip_load->min_duration) {
      VLOG(4) << "qps overlow, ip: " << ip << ", now: " << now_us
              << ", first_fetch_time: " << s.first_fetch_time
              << ", min_duration: " << s.effective_ip_load->min_duration
              << ", fetch times: " << s.fetch_times;

      CHECK_GE(s.first_fetch_time, 0);
      int64 expire_time = s.first_fetch_time + s.effective_ip_load->min_duration;
      return expire_time;
    }
  }

  // 3. 检查连续错误
  if (s.failed_times >= options_.failed_times_threadhold) {
    if (timestamp - s.latest_failed_time < s.holdon_duration_after_failed) {
      VLOG(4) << "hold on after too many failures, ip: "<< ip
              << ", failed times: " << s.failed_times;
      return s.holdon_duration_after_failed + s.latest_failed_time;
    }
  }

  // 4. 检查 并发连接数 过载
  if (s.connection_num >= s.effective_ip_load->max_connections) {
    VLOG(4) << "connection overlow, ip: " << ip
            << ", max connection: " << s.effective_ip_load->max_connections
            << ", connections: " << s.connection_num;
    return -1;
  }

  return 0;
}

// 注册/注销 抓取请求
void LoadController::RegisterIp(const std::string &ip, int64 timestamp) {
  // 发起请求时, 不能过载
  CHECK_EQ(CheckFetch(ip, timestamp), 0L);

  IpLoadStatus &s = status_list_[ip];
  int64 now_us = timestamp;

  // 每抓 check_frequency_ 次, 需要重置状态
  if (s.fetch_times == 0 || s.fetch_times == options_.check_frequency) {
    s.first_fetch_time = now_us;
    s.fetch_times = 0;
    s.effective_ip_load = FindIpLoadRecord(ip, now_us);
  }

  ++s.fetch_times;
  ++s.connection_num;
  ++s.fetch_times_in_history;

  ++connections_in_all_;
  ++started_in_all_;
}

void LoadController::UnregisterIp(const std::string &ip, int64 timestamp, bool success) {
  IpLoadStatus &s = status_list_[ip];
  --s.connection_num;
  --connections_in_all_;

  CHECK_GE(s.connection_num, 0);
  CHECK_GE(s.fetch_times, 0);
  if (!success) {
    s.failed_times++;
    s.latest_failed_time = timestamp;
    s.holdon_duration_after_failed =
        random_.GetInt(options_.min_holdon_duration_after_failed, options_.max_holdon_duration_after_failed);
  } else {
    s.failed_times = 0;
    s.latest_failed_time = 0;
  }
}

int LoadController::GetFetchTimes(const std::string &ip) const {
  auto it = status_list_.find(ip);
  if (it == status_list_.end()) return 0;
  return it->second.fetch_times;
}

int64 LoadController::GetAllFetchTimes() const {
  return started_in_all_;
}

}  // namespace crawl
}  // namespace crawler2

