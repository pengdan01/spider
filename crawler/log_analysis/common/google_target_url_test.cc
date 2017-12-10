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

TEST(ParseGoogleTargetUrl, cases) {
  struct UrlQuery {
    const char* url;
    const char* target_url;
    const char* parse_target_url;
    bool ret;
  } cases[] = {
    {"http://www.google.com.hk/url?sa=t&rct=j&q=%E9%B2%9C%E8%8A%B1&source=web&cd=1&ved=0CGUQFjAA&url=http%3A%2F%2Fwww.malatown.com.cn%2F&ei=XE19T4KuEOayiQfbiJGfCQ&usg=AFQjCNHKOvUFnBDlhxrXmClax278cQ5f7w&cad=rja", "http://www.malatown.com.cn/", "http://www.malatown.com.cn/", true},  // NOLINT
    {"http://www.google.com.hk/url?sa=t&rct=j&q=%E4%BA%AC%E4%B8%9C&source=web&cd=1&ved=0CCgQFjAA&url=http%3A%2F%2Fwww.360buy.com%2F&ei=Al59T5e0CYa0iQf5nPjACQ&usg=AFQjCNGWezPzLUJN0DTo3PlVWXHMn2ZdDQ&cad=rja", "http://www.360buy.com/", "http://www.360buy.com/", true},  // NOLINT
    {"http://www.google.com.hk/aclk?sa=l&ai=C704h0l19T5q6I4SoiQfZ1byvBOr79JICtoqK5AOOxJiWBAgAEAEoA1DS_oHI_P____8BYJ250IGQBaABqrn2_QPIAQGpApxLraVeRYU-qgQUT9BD6PkitF094nmNEX_M3g5WrGk&sig=AOD64_26k5IXCr5jfWGFHQwG0LcPADlExQ&ved=0CAsQ0Qw&adurl=http://www.flowercn.com/%3Fsid%3Dggxh1&rct=j&q=%E9%B2%9C%E8%8A%B1&c", "http://www.google.com.hk/aclk?sa=l&ai=C704h0l19T5q6I4SoiQfZ1byvBOr79JICtoqK5AOOxJiWBAgAEAEoA1DS_oHI_P____8BYJ250IGQBaABqrn2_QPIAQGpApxLraVeRYU-qgQUT9BD6PkitF094nmNEX_M3g5WrGk&sig=AOD64_26k5IXCr5jfWGFHQwG0LcPADlExQ&ved=0CAsQ0Qw&adurl=http://www.flowercn.com/%3Fsid%3Dggxh1&rct=j&q=%E9%B2%9C%E8%8A%B1&c", "", false},  // NOLINT
    {"http://www.google.com/", "http://www.google.com/", "", false},
    {"http://www.google.com.hk/url?sa=t&rct=j&q=%CA%AE%C1%EA%D5%F2%D7%E2%B7%BF%D0%C5%CF%A2&source=web&cd=2&ved=0CEIQFjAB&url=http%3A%2F%2Fcd.58.com%2Fshilingzhen%2Fzufang%2F&ei=rbsrT8aiJvGZiQfl4tG-Dg&usg=AFQjCNHbkQg7DeJDa-5uB1RjAheEkjOy_Q", "http://cd.58.com/shilingzhen/zufang/", "http://cd.58.com/shilingzhen/zufang/", true},  // NOLINT
    {"http://www.google.co.jp/url?sa=t&rct=j&q=ycbook&source=web&cd=1&ved=0CCgQFjAA&url=http%3A%2F%2Fwww.ycbook.com.cn%2F&ei=yGt-T7i4IeSUiAed4M2nBA&usg=AFQjCNHrarA4sZMh7g6Do8qM221KzJ0OgQ", "http://www.ycbook.com.cn/", "http://www.ycbook.com.cn/", true},  // NOLINT
    {"http://www.google.com.hk/url?q=http://www.bjgcsoft.com/&sa=U&ei=ScpFT-PKAeKSiAfG5ICZAw&ved=0CC4QFjAG&usg=AFQjCNFS9djCov-NrPfu5QSpr8R2RtdqSA", "http://www.bjgcsoft.com/", "http://www.bjgcsoft.com/", true},  // NOLINT
    {"http://www.google.com.hk/url?url=http://www.aipai.com/c5/Pzk2JScnImgnaiQg.html&rct=j&sa=X&ei=Z69PT-SBD_GciAfo4q3YCw&ved=0CDMQuAIwAA&q=%E7%A9%BF%E8%B6%8A%E7%81%AB%E7%BA%BF%E5%85%B3%E4%BA%8EAK%E5%8E%8B%E6%9E%AA%E7%9A%84%E8%A7%86%E9%A2%91&usg=AFQjCNHjz-BBZ5DwRfeu2Lewdp3HWAsE5g", "http://www.aipai.com/c5/Pzk2JScnImgnaiQg.html", "http://www.aipai.com/c5/Pzk2JScnImgnaiQg.html", true},  // NOLINT
    {"http://www.google.com.hk/url?q=http://www.7k7k.com/&sa=U&ei=-9P6T5W5K6m5iAeJpLDVBg&ved=0CBQQFjAA&usg=AFQjCNEXgy-9ZVXrDcppdkso6ddu3QuA8A", "http://www.7k7k.com/", "http://www.7k7k.com/", true},  // NOLINT
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); i++) {
    std::string src;
    bool ret = ParseGoogleTargetUrl(cases[i].url, &src);
    EXPECT_EQ(ret, cases[i].ret);
    EXPECT_STREQ(src.c_str(), cases[i].parse_target_url);
    EXPECT_EQ(GoogleTargetUrl(cases[i].url), cases[i].target_url);
  }
}

// TEST(ParseGoogleTargetUrl, HugeData) {
//   std::vector<std::string> lines;
//   base::file_util::ReadFileToLines("/home/zhengying/wly/c", &lines);
//   ASSERT_GT(lines.size(), 0u);
//   for (int i = 0; i < (int)lines.size(); ++i) {
//     std::string src;
//     bool ret = ParseGoogleTargetUrl(lines[i], &src);
//     if (ret) {
//       std::cout << "SUCC " << src << std::endl;
//     } else {
//       std::cout << "FAIL " << lines[i] << std::endl;
//     }
//   }
// }
}
