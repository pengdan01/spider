#include "crawler/util/dns_resolve.h"

#include <stdio.h>
#include <stdlib.h>

#include <iostream>

#include "base/testing/gtest.h"
#include "base/testing/gtest_prod.h"
#include "base/common/logging.h"

static void printHostIpInfo(const std::vector<crawler::HostIpInfo> *infos) {
  CHECK(infos != NULL && infos->empty() != true);
  for (std::vector<crawler::HostIpInfo>::size_type i = 0; i < infos->size(); ++i) {
    if (!(*infos)[i].ips.empty()) {
      for (std::vector<std::string>::size_type j = 0; j < (*infos)[i].ips.size(); ++j) {
        printf("domain: %s, ip: %s\n", (*infos)[i].host.c_str(), (*infos)[i].ips[j].c_str());
      }
    } else {
      printf("domain: %s, no ips.\n", (*infos)[i].host.c_str());
    }
  }
}

// TEST(DnsResolverTest, GetResolvConfNameServer) {
//   crawler::DnsResolver resolver("8.8.8.8", 100);
//   EXPECT_TRUE(resolver.Init());
//   // a normal resolve.conf
//   const std::string resolv_file("crawler/util/testdata/dns_resolve/resolv.conf");
//   std::string server_str;
//   resolver.GetResolvConfNameServer(&server_str, resolv_file);
//   EXPECT_EQ(server_str, "192.168.0.222,192.168.11.1");
//
//   // a empty resolve.conf
//   const std::string resolv_file2("crawler/util/testdata/dns_resolve/resolv_empty.conf");
//   resolver.GetResolvConfNameServer(&server_str, resolv_file2);
//   EXPECT_TRUE(server_str.empty());
//
//   // a resolve.conf with first line comment with '#'
//   const std::string resolv_file3("crawler/util/testdata/dns_resolve/resolv_one.conf");
//   resolver.GetResolvConfNameServer(&server_str, resolv_file3);
//   EXPECT_EQ(server_str, "192.168.11.1");
// }

TEST(DnsResolve, ResolveDomainBatchModeTest) {
  std::vector<crawler::HostIpInfo> domains, domains2, domains3;

  crawler::HostIpInfo d1("www.baidu.com");
  crawler::HostIpInfo d2("www.sohu.com");
  crawler::HostIpInfo d3("www.google.com");
  crawler::HostIpInfo d4("www.sina.com.cn");
  crawler::HostIpInfo d5("www.tom.com");
  crawler::HostIpInfo d6("www.youku.com");
  crawler::HostIpInfo d7(".com");
  domains.push_back(d1);
  domains.push_back(d2);
  domains.push_back(d3);
  domains.push_back(d4);
  domains.push_back(d5);
  domains.push_back(d6);
  domains.push_back(d7);

  domains2 = domains;
  domains3 = domains;
/*  Need a lot of time to run this test case, so comment the follow 2 cases
  // witch no parameter
  crawler::DnsResolver resolver;
  EXPECT_TRUE(resolver.Init());
  EXPECT_TRUE(resolver.ResolveHostBatchMode(&domains));
  printHostIpInfo(&domains);

  // witch one parameter
  crawler::DnsResolver resolver2("192.168.11.58");
  EXPECT_TRUE(resolver2.Init());
  EXPECT_TRUE(resolver2.ResolveHostBatchMode(&domains2));
  printHostIpInfo(&domains2);
*/
  // witch two parameter
  std::vector<std::string> reasons;
  EXPECT_EQ(0, crawler::DNSQueries("8.8.8.8,8.8.4.4,192.168.11.58", 200, &domains3, &reasons));
  for (int i = 0; i < (int32)reasons.size(); ++i) {
    EXPECT_STREQ(reasons[i].c_str(), "success");
  }
//   crawler::DnsResolver resolver3("8.8.8.8,8.8.4.4,192.168.11.58", 200);
//   EXPECT_TRUE(resolver3.Init());
//   EXPECT_TRUE(resolver3.ResolveHostBatchMode(&domains3));
  printHostIpInfo(&domains3);
}
