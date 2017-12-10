#pragma once

#include <string>
#include <vector>

#include "third_party/c-ares/ares.h"

#include "base/common/gflags.h"
#include "base/common/basic_types.h"
#include "base/testing/gtest_prod.h"
#include "base/thread/blocking_queue.h"

namespace crawler {

// DNS Server 内部解析超时时间, Bind 默认为 5000 毫秒
DECLARE_int32(dns_inner_timeout_in_ms);
// Server 返回前, DNS 解析最大尝试次数, Bind 默认为 4 次
DECLARE_int32(dns_inner_try_times);
// Server 返回结果是超时时, 再次发起查询的最大重试次数, 为 0 则不重试
DECLARE_int32(dns_outer_try_times);

enum HostStateType {
  kGetIpOK,
  kGetIpFailureTimeOut,
  kGetIpFailureNoIp,
  kCancel
};

struct HostIpInfo {
  std::string host;
  HostStateType ip_state;

  std::vector<std::string> ips;
  int retry;

  HostIpInfo() {}

  explicit HostIpInfo(std::string host):host(host), retry(0) {}

  bool operator<(const HostIpInfo &f2) const {
    return this->host < f2.host;
  }

  bool operator==(const HostIpInfo &f2) const {
    return (this->host == f2.host) && (this->ips == f2.ips);
  }
};

// DNS 查询, 给定 host, 原地返回 host 对应的 ip
// 正常返回 0, 否则返回非 0
//
// DNS server 格式: host[:port][,host[:port]]...
// |window_size_each_server| 控制 client 对每个 DNS server 的查询并发度
// |reasons| 在函数结束前, 返回每个 server 的查询状态. 可以设置为 NULL
// 通过指定 |dns_outer_try_times| FLAGS 控制 Server 外的重试查询次数.
//
// XXX(huangboxiang): 将原有的 DnsResolver 对象去除, 直接使用函数接口,
// 原因是 DNS 查询的 c-ares 库非多线程安全, 导致 DnsResolver 对象无法被多线程共享.
// 快速查询时需要外部实现多线程,重试等逻辑. 而原有 API 无法支持查询和重试重叠进行,
// 且在少量 server down 机情况下失败率很高, 也无法适应慢节点存在的情况, 影响了查询效率.
// 新的解决方案 解决了上面提到的所有问题
int32 DNSQueries(const std::string& dns_servers,
                 int32 window_size_each_server,
                 std::vector<HostIpInfo> *hosts,
                 std::vector<std::string> *reasons);
}  // namespace crawler
