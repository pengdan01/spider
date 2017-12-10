#include <vector>

#include "base/testing/gtest.h"
#include "crawler/url_rule/url_rule.h"

TEST(URLRule, IsSearchResult) {
  ASSERT_TRUE(crawler::IsSearchResult("http://www.baidu.com/s?wd=h"));
  ASSERT_TRUE(crawler::IsSearchResult("http://s.taobao.com/search?q=%BE%BB%BB%AF%C6%F7"));
  // ASSERT_TRUE(crawler::IsSearchResult("http://www.google.com/#hl=en&newwindow=1&sclient=psy-ab&q=hello"));
  // ASSERT_TRUE(crawler::IsSearchResult("http://mp3.baidu.com/&word=h"));
}

TEST(URLRule, Tidy) {
  std::string new_url;
  ASSERT_FALSE(crawler::TidyURL("http://www.baidu.com/s?wd=h", &new_url));
  ASSERT_FALSE(crawler::TidyURL("http://s.taobao.com/search?q=%BE%BB%BB%AF%C6%F7", &new_url));
  // ASSERT_FALSE(crawler::TidyURL("http://www.google.com/#hl=en&newwindow=1&sclient=psy-ab&q=hello", &new_url));
  // ASSERT_FALSE(crawler::TidyURL("http://mp3.baidu.com/&word=h", &new_url));
}


