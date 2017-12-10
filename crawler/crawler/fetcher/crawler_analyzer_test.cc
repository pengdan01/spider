#include <iostream>
#include <string>

#include "base/common/base.h"
#include "base/testing/gtest.h"
#include "base/common/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/string_tokenize.h"
#include "base/file/file_util.h"
#include "crawler/fetcher/crawler_analyzer.h"
#include "crawler/selector/crawler_selector_util.h"

TEST(crawler_fetcher, IsVIPHost) {
  /*
  std::string host0("www.baidu.com");
  EXPECT_TRUE(crawler::IsVIPHost(host0));
  std::string host1("www.hao123.com");
  EXPECT_TRUE(crawler::IsVIPHost(host1));
  std::string host2("www.idu.o");
  EXPECT_TRUE(!crawler::IsVIPHost(host2));
  std::string host3("alitalk.alibaba.com.cn");
  EXPECT_TRUE(crawler::IsVIPHost(host3));
  */
}

TEST(crawler_fetcher, IsVIPUrl) {
  EXPECT_TRUE(crawler::IsVIPUrl("http://www.sohu.com", 1, 'S', "", 'K'));
}
