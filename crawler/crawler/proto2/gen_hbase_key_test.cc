#include "base/common/logging.h"
#include "base/common/gflags.h"
#include "base/common/basic_types.h"
#include "base/testing/gtest.h"
#include "crawler/proto2/gen_hbase_key.h"

TEST(Transcode, ReverseUrl) {
  struct TestCase {
    std::string url;
    bool success;
    std::string rurl;
  } cases[] = {
    // 空 url
    {"http://", false, ""},
    {"", false, ""},
    {"http", false, ""},
    {"://", false, ""},

    // 正常域名
    {"http://www.sina.com", true, "http://com.sina.www"},   // 反转 url 不会在最后添加 斜杠
    {"http://www.sina.com/", true, "http://com.sina.www/"},
    {"http://www.sina.com/test.path", true, "http://com.sina.www/test.path"},
    {"http://www.sina.com:8899/test", true, "http://com.sina.www:8899/test"},
    {"http://www.sina.com:test/test", true, "http://com.sina.www:test/test"},

    {"http:/www.sina.com/test.path", false, ""},
    {"http:/www.sina.com:8899/test", false, ""},
    {"http:/www.sina.com:test/test", false, ""},

    {"http://user:pass@www.sina.com:8899/test", true, "http://com.sina.www:8899@user:pass/test"},
    {"http://user@www.sina.com:8899/test", true, "http://com.sina.www:8899@user/test"},
    {"http://user@www.sina.com/test", true, "http://com.sina.www@user/test"},
    {"http://user@www.sina.com:port/test", true, "http://com.sina.www:port@user/test"},

    // 协议字段非法
    {"://www.sina.com", false, ""},
    {"a-b://www.sina.com/", false, ""},
    {"a_b://www.sina.com/", false, ""},
    {"a d://www.sina.com/test.path", false, ""},
    {" ://www.sina.com:8899/test", false, ""},

    // 支持各种协议
    {"JavaScript://www.sina.com:test/test", true, "JavaScript://com.sina.www:test/test"},
    {"about://www.sina.com:test/test", true, "about://com.sina.www:test/test"},
    {"thunder://www.sina.com:test/test", true, "thunder://com.sina.www:test/test"},
    // 其实只要协议串只包含数字和字母即可
    {"abcd1234://www.sina.com:test/test", true, "abcd1234://com.sina.www:test/test"},

    {"http://../test", true, "http://../test"},
    {"http://./test", true, "http://./test"},
    {"http://..com/test", true, "http://com../test"},
    {"http://com../test", true, "http://..com/test"},
    {"http://123/test", true, "http://123/test"},
    {"http://123:8080/test", true, "http://123:8080/test"},

    // host 只有数字和 .
    {"http://1.11.111.1111", true, "http://1.11.111.1111"},
    {"http://2.22.222.2222", true, "http://2.22.222.2222"},
    {"http://3.33.333.3333", true, "http://3.33.333.3333"},
    {"http://4.44.444.4444", true, "http://4.44.444.4444"},
    {"http://5.55.555.5555", true, "http://5.55.555.5555"},
    {"http://6.66.666.6666", true, "http://6.66.666.6666"},
    {"http://7.77.777.7777", true, "http://7.77.777.7777"},
    {"http://8.88.888.8888", true, "http://8.88.888.8888"},
    {"http://9.99.999.9999", true, "http://9.99.999.9999"},
    {"http://0.00.000.0000", true, "http://0.00.000.0000"},

    {"http://1.11.111.1111/", true, "http://1.11.111.1111/"},
    {"http://1234567890/index.html", true, "http://1234567890/index.html"},
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
    auto test_case = cases[i];
    std::string rurl;
    ASSERT_EQ(crawler2::ReverseUrl(test_case.url, &rurl), test_case.success) << test_case.url;
    if (test_case.success) {
      ASSERT_EQ(test_case.rurl, rurl);
    }
  }
}


DEFINE_string(test_url, "", "");

TEST(Transcode, GenHbaseKey) {
  struct TestCase {
    std::string url;
    std::string hbase_key;
    std::string new_hbase_key;
  } cases[] = {
    {"http://www.test.com/", "06408-http://com.test.www/", "061928-http://com.test.www/"},
    {"http://www.test.com/index.html",  "00659-http://com.test.www/index.html",
      "288275-http://com.test.www/index.html"},
    {"http://www.sina.com/index.html",  "02615-http://com.sina.www/index.html",
      "053015-http://com.sina.www/index.html"},
    {"http://1.2.3.4", "15726-http://1.2.3.4", "044846-http://1.2.3.4"},
    {"http://1.2.3.4/", "10825-http://1.2.3.4/", "365737-http://1.2.3.4/"},
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
    auto test_case = cases[i];
    std::string hbase_key;
    crawler2::GenHbaseKey(test_case.url, &hbase_key);
    ASSERT_EQ(hbase_key, test_case.hbase_key);
    std::string new_hbase_key;
    crawler2::GenNewHbaseKey(test_case.url, &new_hbase_key);
    ASSERT_EQ(new_hbase_key, test_case.new_hbase_key);
    std::string updated_hbase_key;
    ASSERT_TRUE(crawler2::UpdateHbaseKey(hbase_key, &updated_hbase_key)) << hbase_key;
    ASSERT_EQ(new_hbase_key, updated_hbase_key);
    ASSERT_NE(new_hbase_key, hbase_key);
    LOG(INFO) << test_case.url << "\t" << new_hbase_key << "\t" << hbase_key;
    ASSERT_TRUE(crawler2::IsNewHbaseKey(new_hbase_key));
    ASSERT_TRUE(!crawler2::IsNewHbaseKey(hbase_key)) << hbase_key;
  }

  if (!FLAGS_test_url.empty()) {
    std::string hbase_key;
    crawler2::GenHbaseKey(FLAGS_test_url, &hbase_key);
    LOG(ERROR) << FLAGS_test_url << "\t" << hbase_key;
  }
}

