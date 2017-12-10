#include "log_analysis/common/log_common.h"
#include <queue>
#include <utility>
#include "base/common/basic_types.h"
#include "base/testing/gtest.h"
#include "base/time/time.h"
#include "base/strings/string_split.h"
#include "base/strings/string_number_conversions.h"
#include "base/hash_function/md5.h"
#include "nlp/common/nlp_util.h"
#include "base/encoding/line_escape.h"

namespace log_analysis {
TEST(Userlog, SplitString) {
  std::string line = "\t\t\t\t\t\t";
  std::vector<std::string> flds;
  base::SplitString(line, "\t", &flds);
  ASSERT_EQ(flds.size(), 7u);
  for (int i = 0; i < (int)flds.size(); ++i) {
    ASSERT_TRUE(flds[i].empty());
  }
}

TEST(Userlog, ParseTime) {
  std::string time_in_sec = "1328185139";
  std::string new_format;
  ASSERT_TRUE(ConvertTimeFromSecToFormat(time_in_sec, "%Y%m%d%H%M%S", &new_format));
  ASSERT_EQ(new_format, "20120202201859");

  int64 sec;
  ASSERT_TRUE(ConvertTimeFromFormatToSec(new_format, "%Y%m%d%H%M%S", &sec));
  ASSERT_EQ(sec, 1328185139);

  int64 new_sec = sec + 1;
  ASSERT_TRUE(ConvertTimeFromSecToFormat(base::Int64ToString(new_sec).c_str(), "%Y%m%d%H%M%S", &new_format));
  ASSERT_EQ(new_format, "20120202201900");
}

TEST(Userlog, JoinFields) {
  std::vector<std::string> flds;
  for (char i = 'a'; i <= 'g'; ++i) {
    std::string str;
    str.push_back(i);
    flds.push_back(str);
  }
  // abcdefg
  ASSERT_EQ(JoinFields(flds, "", "0"), "a");
  ASSERT_EQ(JoinFields(flds, "", "3"), "d");
  ASSERT_DEATH(JoinFields(flds, "", "33"), "");
  ASSERT_EQ(JoinFields(flds, "", "10"), "ba");
  ASSERT_EQ(JoinFields(flds, "\t", "10"), "b\ta");
  ASSERT_EQ(JoinFields(flds, "\t", "1023"), "b\ta\tc\td");
  ASSERT_EQ(JoinFields(flds, "\t", "0"), "a");
}

TEST(Userlog, SearchID) {
  std::string line = "aaGooglexx201201010101";
  std::string md51 = MD5String(line);
  line = "aaGooglexx201201010102";
  std::string md52 = MD5String(line);
  ASSERT_NE(md51, md52);
}

TEST(Userlog, aa) {
  std::vector<std::string> a;
  a.push_back("");
  std::cout << a.size() << std::endl;
}

TEST(Userlog, priority_queue) {
  std::priority_queue<std::pair<int64, std::string>> top_n;
  top_n.push(std::make_pair(-5, "z"));
  if (top_n.size() > 3) top_n.pop();
  top_n.push(std::make_pair(-4, "x"));
  if (top_n.size() > 3) top_n.pop();
  top_n.push(std::make_pair(-3, "y"));
  if (top_n.size() > 3) top_n.pop();
  top_n.push(std::make_pair(-2, "a"));
  if (top_n.size() > 3) top_n.pop();
  top_n.push(std::make_pair(-1, "b"));
  if (top_n.size() > 3) top_n.pop();
  ASSERT_EQ(top_n.top().first, -3);
  ASSERT_EQ(top_n.top().second, "y");
  std::vector<std::string> queries;
  while (!top_n.empty()) {
    queries.push_back(top_n.top().second);
    top_n.pop();
  }
  ASSERT_EQ(queries.size(), 3u);
  ASSERT_EQ(queries[0], "y");
  ASSERT_EQ(queries[1], "x");
  ASSERT_EQ(queries[2], "z");
}
}  // namespace log_analysis
