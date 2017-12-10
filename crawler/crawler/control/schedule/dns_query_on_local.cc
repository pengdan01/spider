#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <pthread.h>
#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/encoding/line_escape.h"
#include "base/hash_function/city.h"
#include "base/strings/string_split.h"
#include "base/strings/string_number_conversions.h"
#include "web/url/gurl.h"

#include "crawler/util/dns_resolve.h"

DEFINE_string(hosts_list, "", "list of hosts to be query");
DEFINE_string(output_file, "", "output file");
DEFINE_int32(retry_time, 3, "DNS query retry times");
DEFINE_int32(thread_num, 10, "DNS query thread num");
DEFINE_int32(window_size, 100, "DNS query window size");
static const int KDNSServerNum = 5;
static const char KDNSServer[] = "180.153.240.21,180.153.240.22,180.153.240.23,180.153.240.24,180.153.240.25";
// static const char KDNSServer[] = "180.153.240.25";
static const char kGoogleDNSServer[] = "8.8.8.8";
//static const int KDNSRetryTime = 1;
//static const int kDNSQueryThread = 8;

void ReadData(std::vector<crawler::HostIpInfo> *hosts) {
  std::string input_buffer;
  std::ifstream fin(FLAGS_hosts_list.c_str());
  if (!fin) {
    LOG(FATAL) << "open " << FLAGS_hosts_list << " failed";
  }
  while(std::getline(fin, input_buffer)) {
    size_t tab_pos = input_buffer.find("\t");
    if (tab_pos == std::string::npos) {
      LOG(FATAL) << "hosts format error " << input_buffer;
    }
    crawler::HostIpInfo ip_info;
    ip_info.host = input_buffer.substr(0, tab_pos);
    hosts->push_back(ip_info);
  }
  fin.close();
}

void DnsResolve(std::vector<crawler::HostIpInfo> &hosts, // (TODO) should be const
                FILE *output_dns_file,
                int32 dns_resolver_window_size,
                bool last_try,
                std::vector<crawler::HostIpInfo> *retry_hosts,
                uint64 *failed_dns_no_ip_num,
                uint64 *failed_dns_timeout_num) {
  if (hosts.size() == 0UL) {
    return;
  }
  // 构建 dns 解析对象，开始解析
  std::vector<std::string> dns_server;
  std::string output_dns_host_list;
  base::SplitString(KDNSServer, ",", &dns_server);
  crawler::DnsResolver *dns_resolver;
  int32 s_idx = rand()%KDNSServerNum;
  LOG(INFO) << "thread connect to DNS Server " << s_idx;
  if (last_try) {
    LOG(INFO) << "last try, use Google DNS Server";
    //dns_resolver = new crawler::DnsResolver(kGoogleDNSServer, dns_resolver_window_size);
    dns_resolver = new crawler::DnsResolver(dns_server[s_idx], dns_resolver_window_size);
  } else {
    dns_resolver = new crawler::DnsResolver(dns_server[s_idx], dns_resolver_window_size);
  }
  CHECK_NOTNULL(dns_resolver);
  LOG_IF(FATAL, !dns_resolver->Init()) << "dns_resolver.Init() fail";
  LOG_IF(FATAL, !dns_resolver->ResolveHostBatchMode(&hosts)) << "ResolveHostBatchMode() fail";
  // 统计解析结果
  *failed_dns_no_ip_num = 0;
  *failed_dns_timeout_num = 0;
  for (size_t i = 0; i < hosts.size(); ++i) {
    if (hosts[i].ips.size() == 0UL) {
      // 表明获取 dns 失败
      if (hosts[i].ip_state == crawler::kGetIpFailureNoIp) {
        ++(*failed_dns_no_ip_num);
      } else {
        ++(*failed_dns_timeout_num);
        retry_hosts->push_back(hosts[i]);
      }
    } else {
      output_dns_host_list = hosts[i].ips[0];
      for (size_t ip_i = 1; ip_i < hosts[i].ips.size(); ++ip_i) {
        output_dns_host_list += "\t" + hosts[i].ips[ip_i];
      }
      fprintf(output_dns_file, "%s\t%s\n",hosts[i].host.c_str(), output_dns_host_list.c_str()); // 输出第一个 ip
    }
  }
}

struct DNSQueryThreadStruct {
  FILE *output_file_fp; // linux 保证写锁
  int32 dns_resover_window_size;
  std::vector<crawler::HostIpInfo> *hosts;
  std::vector<crawler::HostIpInfo> *retry_hosts;
  bool last_try; // 是否是最后一次try，如果是 用google的服务器
};

void *dns_query_thread(void *args) {
  DNSQueryThreadStruct *param = (DNSQueryThreadStruct*)args;
  uint64_t failed_dns_no_ip_num = 0;
  uint64_t failed_dns_timeout_num = 0;
  DnsResolve(*param->hosts, param->output_file_fp, param->dns_resover_window_size, param->last_try,
             param->retry_hosts, &failed_dns_no_ip_num, &failed_dns_timeout_num);
  LOG(WARNING) << " total query: " << param->hosts->size() 
               << " failed_dns_no_ip_num " << failed_dns_no_ip_num
               << " failed_dns_timeout_num " << failed_dns_timeout_num;
  return NULL;
}

void DNSQuery() {
  std::vector<crawler::HostIpInfo> hosts;
  std::vector<crawler::HostIpInfo> *thread_hosts = new std::vector<crawler::HostIpInfo>[FLAGS_thread_num];
  std::vector<crawler::HostIpInfo> *thread_retry_hosts = new std::vector<crawler::HostIpInfo>[FLAGS_thread_num];
  CHECK_NOTNULL(thread_hosts);
  CHECK_NOTNULL(thread_retry_hosts);
  FILE *output_fp = fopen(FLAGS_output_file.c_str(), "w");
  if (!output_fp) {
    LOG(FATAL) << "open output file failed";
  }

  // 读取数据
  ReadData(&hosts);
  // 对 hosts 分桶
  srand (unsigned(time(NULL)));

  pthread_t *pid = new pthread_t[FLAGS_thread_num];
  DNSQueryThreadStruct *thread_param = new DNSQueryThreadStruct[FLAGS_thread_num];
  // 循环重试次
  for (int try_i = 0; try_i < FLAGS_retry_time; ++try_i) {
    int thread_num = FLAGS_thread_num;
    thread_num /= (try_i+1);
    if (thread_num <=0) {
      thread_num = 1;
    }
    LOG(INFO) << "Start dns query try : " << try_i;
    for (int thread_i = 0; thread_i < thread_num; ++thread_i) {
      thread_retry_hosts[thread_i].clear();
      thread_hosts[thread_i].clear();
    }
    random_shuffle(hosts.begin(), hosts.end());
    uint64_t hosts_num = hosts.size();
    for (uint64_t host_i = 0; host_i < hosts_num; ++host_i){
      int thread_id = host_i % thread_num;
      thread_hosts[thread_id].push_back(hosts[host_i]);
    }

    // 创建线程
    for (int i = 0; i < thread_num; ++i) {
      thread_param[i].output_file_fp = output_fp;
      thread_param[i].dns_resover_window_size = FLAGS_window_size;
      if (try_i == FLAGS_retry_time-1) {
        thread_param[i].dns_resover_window_size = FLAGS_window_size/2;
      }
      thread_param[i].hosts = thread_hosts+i;
      thread_param[i].retry_hosts = thread_retry_hosts+i;
      if (try_i == FLAGS_retry_time-1) {
        thread_param[i].last_try = true;
      } else {
        thread_param[i].last_try = false;
      }
      pthread_create(pid+i, NULL, dns_query_thread, (void *)(thread_param+i));
    }

    // 等待
    for (int i = 0; i < thread_num; ++i) {
      pthread_join(pid[i], NULL);
    }
    
    hosts.clear();
    // add retry hosts to hosts vector
    for (int thread_i = 0; thread_i < thread_num; ++thread_i) {
      for (size_t v_i = 0; v_i < thread_retry_hosts[thread_i].size(); ++v_i) {
        hosts.push_back(thread_retry_hosts[thread_i][v_i]);
      }
    }
    if (try_i == FLAGS_retry_time-1) {
      FILE *fp = fopen("time_out_host","w");
      for (int thread_i = 0; thread_i < thread_num; ++thread_i) {
        for (size_t v_i = 0; v_i < thread_retry_hosts[thread_i].size(); ++v_i) {
          fprintf(fp, "%s\n", thread_retry_hosts[thread_i][v_i].host.c_str());
        }
      }
      fclose(fp);
    }
    // retry 睡眠 5 秒
    sleep(5);
  }
  fclose(output_fp);
  delete []thread_param;
  delete []pid;
  delete []thread_hosts;
}

int main(int argc, char *argv[]) {
  base::InitApp(&argc, &argv, "dns query on local");
  DNSQuery();
}

