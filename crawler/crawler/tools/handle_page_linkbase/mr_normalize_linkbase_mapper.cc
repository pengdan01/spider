#include <string>
#include <map>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/encoding/cescape.h"
#include "base/encoding/line_escape.h"
#include "base/strings/string_tokenize.h"
#include "base/strings/string_util.h"
#include "base/strings/string_printf.h"
#include "base/mapreduce/mapreduce.h"
#include "base/hash_function/fast_hash.h"
#include "crawler/api/base.h"

const int kValueHashMapFieldNum = 1;
const int kValueLinkBaseFiledNum = 14;

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "Add simhash");
  CHECK(base::mapreduce::IsMapApp());
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  CHECK_NOTNULL(task);

  std::string key;
  std::string value;
  std::vector<std::string> tokens;

  int reduce_num = task->GetReducerNum();

  std::string content_type;
  char resource_type;
  std::string http_header;
  std::string out;

  // 数据输入的 key 为 source_url, value 为一个 hash number
  // linkbase record 
  while (task->NextKeyValue(&key, &value)) {
    if (key.empty() || value.empty()) continue;
    int num = base::Tokenize(value, "\t", &tokens);
    if (num == kValueHashMapFieldNum) {  // From mapfile: normalized_url \t simhash
      /*
      if(crawler::ReverseUrl(key, &key, false)) { 
        task->OutputWithReducerID(key + "A", value, base::CityHash64(key.data(), key.size()) % reduce_num);
      }
      */
    } else if (num == kValueLinkBaseFiledNum) {
      const std::string &effective_url = tokens[0];
      resource_type = tokens[1][0];
      const std::string &timestamp = tokens[2];
      const std::string &score = tokens[3];
      const std::string &rs_code = tokens[4];
      const std::string &file_time = tokens[5];
      const std::string &total_time = tokens[6];
      const std::string &redirect_cnt = tokens[7];
      const std::string &download_speed = tokens[8];
      const std::string &content_length_donwload= tokens[9];
      const std::string &content_type = tokens[10];
      const std::string &http_header = tokens[11];
      out = base::StringPrintf("%s\t%c\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s",
                               effective_url.c_str(),
                               resource_type, timestamp.c_str(), score.c_str(), rs_code.c_str(),
                               file_time.c_str(), total_time.c_str(), redirect_cnt.c_str(),
                               download_speed.c_str(),
                               content_length_donwload.c_str(), content_type.c_str(), http_header.c_str());
      task->OutputWithReducerID(key + "B", out,
                                base::CityHash64(key.data(), key.size()) % reduce_num);
    } else {
      LOG(ERROR) << "Invalid record format, ,field#: " << num << ", input: " << key + "\t" + value;
    }
  }
  return 0;
}
