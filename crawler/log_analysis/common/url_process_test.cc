
#include "log_analysis/common/log_common.h"
#include "base/common/basic_types.h"
#include "base/testing/gtest.h"
#include "base/time/time.h"
#include "base/strings/string_split.h"
#include "base/file/file_util.h"
#include "base/hash_function/md5.h"
#include "nlp/common/nlp_util.h"
#include "base/encoding/line_escape.h"

namespace log_analysis {

TEST(AbandonUrlQuery, cases) {
  struct UrlQuery {
    const char* url;
    const char* normed_url;
  } cases[] = {
    {"http://www.google.com.hk/url?sa=t&rct=j&q=%E9%B2%9C%E8%8A%B1&source=web&cd=1&ved=0CGUQFjAA&url=http%3A%2F%2Fwww.malatown.com.cn%2F&ei=XE19T4KuEOayiQfbiJGfCQ&usg=AFQjCNHKOvUFnBDlhxrXmClax278cQ5f7w&cad=rja", "http://www.google.com.hk/url"},  // NOLINT
    {"http://www.Baidu.com", "http://www.baidu.com/"},
    {"http://www.Baidu.com/baidu.php#aa?bb", "http://www.baidu.com/baidu.php"},
    {"http://192.168.11.137", "http://192.168.11.137/"},
    {"http://www.itrust.org.cn/yz/pjwx.asp?wm=1786197705", "http://www.itrust.org.cn/yz/pjwx.asp"},
  };
  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); i++) {
    std::string src;
    bool ret = abandon_url_query(cases[i].url, &src);
    EXPECT_STREQ(src.c_str(), cases[i].normed_url);
    if (src == "") {
      EXPECT_TRUE(!ret);
    } else {
      EXPECT_TRUE(ret);
    }
  }
}

TEST(Base64ToClickUrl, cases) {
  static const struct {
    const char* base64;
    const char* click_url;
    bool  succ;
  } cases[] = {
    {"aHR0cDovL3d3dy5iYWlkdS5jb20K", "http://www.baidu.com/", true},
    {"aHR0cDovL3d3dy5iYWlkdS5jb20vcz93ZD0lQ0IlQUIlQzklQUIlQzclRjIK", "http://www.baidu.com/s?wd=%CB%AB%C9%AB%C7%F2", true},  // NOLINT
    {"aHR0cDovL3d3dy5iYWlkdS5jb20/P1c/Cg==", "http://www.baidu.com??W?", false},
    {"aHR0cDovL3d3dy5iYWlkdS5jb20/P0w/Cg==", "http://www.baidu.com??L?", false},
    {"aHR0cDovL3d3dy5iYWlkdS5jb20/P1U/Cg==", "http://www.baidu.com??U?", false},
    {"aHR0cDovL3B0bG9naW4yLnFxLmNvbS9qdW1wPz9XPw==", "http://ptlogin2.qq.com/jump??W?", false},
    {"1", "", false},
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); i++) {
    std::string click_url;
    bool ret = Base64ToClickUrl(cases[i].base64, &click_url);
    ASSERT_EQ(ret, cases[i].succ);
    if (ret) {
      ASSERT_EQ(click_url, cases[i].click_url);
    }
  }
}

// 用于对一个文件中的 base64 数据批量跑出对应的 url
// TEST(sss,ss) {
//   std::vector<std::string> lines;
//   std::vector<std::string> flds;
//   base::file_util::ReadFileToLines("/home/zhengying/wly/c", &lines);
//   for (int i = 0; i < (int)lines.size(); ++i) {
//     const std::string& line = lines[i];
//     flds.clear();
//     base::SplitString(line, "\t", &flds);
//     CHECK_EQ(flds.size(), 2u);
//     std::string url;
//     if (Base64ToClickUrl(flds[1], &url)) {
//       std::cout << flds[0] << "\t" << url << std::endl;
//     } else {
//       std::cout << flds[0] << "\tERROR" << std::endl;
//     }
//   }
// }
}  // namespace log_analysis
