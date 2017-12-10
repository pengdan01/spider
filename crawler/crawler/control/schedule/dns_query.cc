#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <set>
#include <vector>
#include <string>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/mapreduce/mapreduce.h"
#include "base/encoding/line_escape.h"
#include "base/hash_function/city.h"
#include "base/strings/string_split.h"
#include "base/strings/string_number_conversions.h"
#include "web/url/gurl.h"

#include "crawler/util/dns_resolve.h"

static const char KDNSServer[] = "180.153.240.21,180.153.240.22,180.153.240.23,180.153.240.24,180.153.240.25";

void Mapper() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  std::string key;
  std::string value;
  std::vector<std::string> fields;
  std::set<std::string> url_hosts;
  while (task->NextKeyValue(&key, &value)) {
    fields.clear();
    base::SplitString(value, "\t", &fields);
    const std::string &url=fields[0];
    GURL gurl(url);
    url_hosts.insert(gurl.host());
  }
  auto it = url_hosts.begin();
  for ( ; it != url_hosts.end(); ++it) {
    task->Output(*it, "1");
  }
}

void Reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  crawler::DnsResolver *dns_resolver = new crawler::DnsResolver(KDNSServer, 1600);
  CHECK_NOTNULL(dns_resolver);
  LOG_IF(FATAL, !dns_resolver->Init()) << "dns_resolver.Init() fail";
  std::string key;
  std::string value;
  std::set<std::string> url_hosts;
  while (task->NextKey(&key)) {
    while (task->NextValue(&value)) {
      url_hosts.insert(key);
    }
  }
  auto it = url_hosts.begin();
  // 构造 dns 需要的结构体
  std::vector<crawler::HostIpInfo> hosts;
  for (; it != url_hosts.end(); ++it) {
    crawler::HostIpInfo ip_info;
    ip_info.host = *it;
    hosts.push_back(ip_info);
  }
  LOG_IF(FATAL, !dns_resolver->ResolveHostBatchMode(&hosts)) << "ResolveHostBatchMode() fail";
  uint64_t failed_dns_no_ip_num = 0;
  uint64_t failed_dns_timeout_num = 0;
  uint64_t retry_failed_dns_no_ip_num = 0;
  uint64_t retry_failed_dns_timeout_num = 0;
  std::vector<crawler::HostIpInfo> retry_hosts;
  for (size_t i = 0; i < hosts.size(); ++i) {
    if (hosts[i].ips.size() == 0UL) {
      // 表明获取 dns 失败
      if (hosts[i].ip_state == crawler::kGetIpFailureNoIp) {
        ++failed_dns_no_ip_num;
      } else {
        ++failed_dns_timeout_num;
        retry_hosts.push_back(hosts[i]);
      }
    } else {
      task->Output(hosts[i].host, hosts[i].ips[0]); // 输出第一个 ip
    }
  }
  if (retry_hosts.size() != 0UL) {
    // sleep 10 秒然后重试
    sleep(10);
    crawler::DnsResolver *retry_dns_resolver = new crawler::DnsResolver(KDNSServer, 1600);
    CHECK_NOTNULL(retry_dns_resolver);
    LOG_IF(FATAL, !retry_dns_resolver->Init()) << "retry_dns_resolver.Init() fail";
    LOG_IF(FATAL, !retry_dns_resolver->ResolveHostBatchMode(&retry_hosts)) 
        << "retry ResolveHostBatchMode() fail";
    for (size_t i = 0; i < retry_hosts.size(); ++i) {
      if (retry_hosts[i].ips.size() == 0UL) {
        // 表明获取 dns 失败
        if (retry_hosts[i].ip_state == crawler::kGetIpFailureNoIp) {
          ++retry_failed_dns_no_ip_num;
        } else {
          ++retry_failed_dns_timeout_num;
        }
      } else {
        task->Output(retry_hosts[i].host, retry_hosts[i].ips[0]); // 输出第一个 ip
      }
    }
  }
  LOG(WARNING) << "total queries number: " << hosts.size();
  LOG(WARNING) << "failed_dns_no_ip_num: " << failed_dns_no_ip_num
               << " failed_dns_timeout_num: " << failed_dns_timeout_num;
  LOG(WARNING) << "total retry number: " << retry_hosts.size();
  LOG(WARNING) << "retry_failed_dns_no_ip_num: " << retry_failed_dns_no_ip_num
               << " retry_failed_dns_timeout_num: " << retry_failed_dns_timeout_num;
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "dns query");
  if (base::mapreduce::IsMapApp()) {
    Mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    Reducer();
  } else {
    CHECK(false);
  }
}
