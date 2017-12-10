#include <string>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/string_tokenize.h"
#include "base/strings/string_number_conversions.h"
#include "base/mapreduce/mapreduce.h"

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "Add simhash");
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  CHECK_NOTNULL(task);

  std::string key;
  std::string value;
  char flag;
  std::string last_url;
  std::string last_simhash;
  std::string last_value;
  std::vector<std::string> tokens;
  int64 timestamp = 0;
  int64 tmp_timestamp = 0;
  int type = 0;
  // 输入: Asource_url \t hash_code  or Bsource_url \t Record (hash code absence)
  // We do a JOIN operation here
  while (task->NextKey(&key)) {
    LOG_IF(FATAL,key.size() < 2) << "Invalid Record, key: " << key;
    flag = key[key.size()-1];
    LOG_IF(FATAL, flag != 'A' && flag != 'B') << "Error Input: " << key;
    key = key.substr(0, key.size() - 1);
    if (flag == 'A') {
      if (task->NextValue(&value)) {
        last_url = key;
        last_simhash = value;
      }
    } else {
      timestamp = 0;
      last_value = "";
      while (task->NextValue(&value)) {
        int num = base::Tokenize(value, "\t", &tokens);
        if (num != 12) {
          LOG(ERROR) << "Value should be 12, but field#: " << num;
          continue;
        }
        type = tokens[1][0] - '0';
        if (!base::StringToInt64(tokens[2], &tmp_timestamp)) {
          LOG(ERROR) << "Faild in Convert timestamp to int64, timestamp: " << tokens[2];
          continue;
        }
        if (tmp_timestamp > timestamp) {
          last_value = value;
          timestamp = tmp_timestamp;
        }
      }
      if (last_value.empty()) continue;
      if (type == 1) {  // page
        if (key != last_url) continue;
        value = last_value + "\t" + last_simhash + "\t0";
        task->Output(key, value);
        last_url = "";
        last_simhash = "";
      } else {
        value = last_value + "\t0\t0";
        task->Output(key, value);
      }
    }
  }
  return 0;
}
