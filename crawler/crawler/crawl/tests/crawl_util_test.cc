#include <iostream>
#include <string>

#include "base/common/base.h"
#include "base/testing/gtest.h"
#include "base/common/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/string_tokenize.h"
#include "base/file/file_util.h"

#include "crawler/crawl/crawl_util.h"

TEST(crawl_util, IsAjaxUrl) {
  static struct {
    const char *url;
    bool result;
  } cases[] = {
    {"", false},
    {"http://tb.himg.baidu.com/sys/portrait/item/ff1ca1ced7d4b5bcd7d4d1ddd818", false},
    {"http://bbs.byr.cn/#!board/Job", true},
    {"http://bbs.byr.cn/#!", true},
  };
  for (int i = 0; i < (int)ARRAYSIZE_UNSAFE(cases); ++i) {
    EXPECT_EQ(crawler2::crawl::IsAjaxUrl(cases[i].url), cases[i].result);
  }
}

TEST(crawl_util, TransformAjaxUrl) {
  static struct {
    const char *url;
    const char *result;
  } cases[] = {
    {"http://bbs.byr.cn/#!board/Job", "http://bbs.byr.cn/?_escaped_fragment_=board/Job"},
    {"http://bbs.byr.cn/#!", "http://bbs.byr.cn/"},
    {"http://www.newsmth.net/nForum/#!article/ITExpress/1262857", "http://www.newsmth.net/nForum/?_escaped_fragment_=article/ITExpress/1262857"}, // NOLINT
  };
  for (int i = 0; i < (int)ARRAYSIZE_UNSAFE(cases); ++i) {
    std::string new_url;
    crawler2::crawl::TransformAjaxUrl(cases[i].url, &new_url);
    EXPECT_EQ(new_url, cases[i].result);
  }
}
