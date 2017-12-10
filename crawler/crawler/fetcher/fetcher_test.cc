#include <stdio.h>
#include <stdlib.h>

#include <iostream>

#include "base/testing/gtest.h"
#include "base/common/logging.h"
#include "base/strings/string_printf.h"
#include "crawler/fetcher/fetcher_thread.h"
#include "crawler/fetcher/multi_fetcher.h"

// TODO(pengdan) : complete g test
//TEST(DnsResolve, ResolveDomainBatchModeTest) {
//  crawler::DnsResolver resolver3("8.8.8.8,8.8.4.4,192.168.11.58", 2000);
//  EXPECT_TRUE(resolver3.Init());
//}

// Test MultiFetcher::LoadSiteControlRules()
TEST(MultiFetcher, LoadSiteControlRulesANDGetSiteCompressParameter) {
  std::string file("crawler/fetcher/testdata/host_load_control.txt");
  std::map<std::string, std::vector<crawler::HostLoadControl>> *host_access_paras;
  host_access_paras = new std::map<std::string, std::vector<crawler::HostLoadControl>> ();
  CHECK_NOTNULL(host_access_paras);
  bool ret = crawler::MultiFetcher::LoadHostControlRules(file, host_access_paras, 3);
  EXPECT_TRUE(ret);
  for (auto it = host_access_paras->begin(); it != host_access_paras->end(); ++it) {
    const std::vector<crawler::HostLoadControl> &rules = it->second;
    for (int i = 0; i < (int)rules.size(); ++i) {
      std::string out = base::StringPrintf("%s %d %d %d:%d-%d:%d", it->first.c_str(),
                                           rules[i].max_concurrent_access, rules[i].max_qps,
                                           rules[i].start_hour,
                                           rules[i].start_min, rules[i].end_hour, rules[i].end_min);
      std::cout << out << std::endl;
    }
  }
  // OK , now test GetSiteCompressParameter
  // 策略 受时间的变化输出也是变化, 这里 注释掉
  /*
  std::string host1("edu.360.cn");
  int max_cur, M, N;
  crawler::MultiFetcher::GetSiteCompressParameter(host_access_paras, host1, &max_cur, &M, &N);
  EXPECT_EQ(max_cur, 5);
  EXPECT_EQ(M, 5);
  EXPECT_EQ(N, 1);
  std::string host2("www.sina.com.cn");
  crawler::MultiFetcher::GetSiteCompressParameter(host_access_paras, host2, &max_cur, &M, &N);
  EXPECT_EQ(max_cur, 8);
  EXPECT_EQ(M, 15);
  EXPECT_EQ(N, 1);
  */
}

// Test MultiFetcher::SimpleFetcher()
TEST(MultiFetcher, SimpleFetcherTest) {
  std::string url1("http://www.mogujie.com/");
  std::string header;
  std::string body;
  bool ret = crawler::MultiFetcher::SimpleFetcher(url1, &header, &body, false);
  EXPECT_TRUE(ret);
  LOG(ERROR) << "done, url: " << url1;
  ret = crawler::MultiFetcher::SimpleFetcher(url1, NULL, &body, false);
  EXPECT_TRUE(ret);
  ret = crawler::MultiFetcher::SimpleFetcher(url1, &header, NULL, false);
  EXPECT_TRUE(ret);

  std::string url2("http://image.baidu.com/i?ct=503316480&z=&tn=baiduimagedetail&word=%BB%B3%D4%D0%D2%BB%B8"
                   "%F6%D4%C2&in=11907&cl=2&lm=-1&st=&pn=10&rn=1&di=12653813565&ln=1964&fr=&fm=hao123&fmq=13"
                   "30256325015_R&ic=&s=&se=&sme=0&tab=&width=&height=&face=&is=&istype=#pn40&-1&di280215428"
                   "70&objURLhttp%3A%2Fè%2Fwww.hongfen.org%2Fu");  // NOLINT
  ret = crawler::MultiFetcher::SimpleFetcher(url2, &header, &body, true);
  EXPECT_TRUE(ret);
  LOG(ERROR) << "done, url: " << url2;

  std::string url3("http://www.baidu.com/s?tn=sitehao123&bs=tttu&f=8&rsv_bp=1&wd=tttu&inputT=0");
  ret = crawler::MultiFetcher::SimpleFetcher(url3, &header, &body, true);
  EXPECT_TRUE(ret);
  LOG(ERROR) << "done, url: " << url3;
  std::string url4("http://user.qzone.qq.com/64649829");
  ret = crawler::MultiFetcher::SimpleFetcher(url4, &header, &body, true);
  EXPECT_TRUE(ret);
  LOG(ERROR) << "done, url: " << url4;
}
