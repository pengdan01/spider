#include "crawler/util/ip_scope.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>

#include "base/testing/gtest.h"

TEST(IP_SCOPE, IPString2IntTest) {
  const char *ip_str = "1.0.0.1";
  uint32 ip_int = 16777217;  // (1 << 24)  + 1
  uint32 ip_output;
  crawler::ConvertIp2Int(std::string(ip_str), ip_output);
  EXPECT_TRUE(ip_int == ip_output);
}

TEST(IP_SCOPE, BuildIPScopeFromFileTest) {
  std::string filename("crawler/util/testdata/country-ipv4.lst-partical");
  std::vector<crawler::EdgeNode> ip_scopes;
  EXPECT_TRUE(BuildIPScopeFromFile(filename.c_str(), &ip_scopes) == true);
  for (size_t i = 0; i < ip_scopes.size() - 1; i += 2) {
    printf("%u, %u\n", ip_scopes[i].ip, ip_scopes[i+1].ip);
  }
  printf("total_scope_num: %u\n", (uint32_t)ip_scopes.size());
}

TEST(IP_SCOPE, InCurrentIPScopeTest) {
  std::string filename("crawler/util/testdata/country-ipv4.lst-partical");
  std::vector<crawler::EdgeNode> ip_scopes;
  EXPECT_TRUE(BuildIPScopeFromFile(filename.c_str(), &ip_scopes) == true);
  uint32 in = 1728395263;
  EXPECT_TRUE(InCurrentIPScope(&ip_scopes, in) == true);
  uint32 notin = 1728396288;
  EXPECT_TRUE(InCurrentIPScope(&ip_scopes, notin) == false);

  std::string in_str = "103.5.56.1";
  EXPECT_TRUE(InCurrentIPScope(&ip_scopes, in_str.c_str()) == true);
  std::string out_str = "1.0.3.1";
  EXPECT_TRUE(InCurrentIPScope(&ip_scopes, out_str.c_str()) == false);
}

TEST(IP_SCOPE, GetIpFromDomain) {
  // std::string domain1("www.baidu.com");
  std::string domain1("search.baby.sina.com.cn");
  std::vector<std::string> ips;
  bool ret = crawler::GetIpFromDomain(domain1, &ips);
  EXPECT_TRUE(ret);
  for (size_t i = 0; i < ips.size(); ++i) {
    printf("domain: %s, ip: %s\n", domain1.c_str(), ips[i].c_str());
  }
  std::string domain2("club.city.travel.sohu.com");
  ret = crawler::GetIpFromDomain(domain2, &ips);
  EXPECT_TRUE(ret);
  for (size_t i = 0; i < ips.size(); ++i) {
    printf("domain: %s, ip: %s\n", domain2.c_str(), ips[i].c_str());
  }
}

TEST(IP_SCOPE, IsDomainInCountry) {
  std::string filename("crawler/util/testdata/country-ipv4.lst");
  std::vector<crawler::EdgeNode> ip_scopes;
  EXPECT_TRUE(BuildIPScopeFromFile(filename.c_str(), &ip_scopes) == true);

  bool ret;
  std::string domain1("www.google.com");
  ret = IsDomainInCountry(domain1, &ip_scopes);
  EXPECT_TRUE(ret == false);

  std::string domain2("www.hao123.com");
  ret = IsDomainInCountry(domain2, &ip_scopes);
  EXPECT_TRUE(ret == true);

  std::string domain3("www.sexynaked.info");
  ret = IsDomainInCountry(domain3, &ip_scopes);
  EXPECT_TRUE(ret == false);

  std::string domain4("answers.yahoo.com");
  ret = IsDomainInCountry(domain4, &ip_scopes);
  EXPECT_TRUE(ret == false);
}

