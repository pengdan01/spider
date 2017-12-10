#include <string>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/string_tokenize.h"
#include "base/strings/string_number_conversions.h"
#include "base/mapreduce/mapreduce.h"

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "mr_updater_reducer");
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  CHECK_NOTNULL(task);

  std::string key;
  std::string value;
  std::vector<std::string> tokens;

  int resource_type;
  std::string last_modified_time;
  std::string score;
  // source_url \t resource_type \t  timestamp \t  score \t  time
  while (task->NextKey(&key)) {
    if (key.empty()) continue;
    int64 num_timestamp = 0;
    while (task->NextValue(&value)) {
      int num = base::Tokenize(value, "\t", &tokens);
      if (num != 4) {
        LOG(ERROR) << "value field# must be 4, key: " << key << ", vaule: " << value;
        continue;
      }
      resource_type = tokens[1][0] - '0';
      if (resource_type != 1) {
       LOG(ERROR) << "Should be page, ignore key: " << key << ", value: " << value;
       continue;
      }
      int64 tmp_timestamp;
      if (!base::StringToInt64(tokens[1], &tmp_timestamp)) {
        LOG(ERROR) << "Faild in Convert timestamp to int64, timestamp: " << tokens[1];
        continue;
      }
      if (tmp_timestamp > num_timestamp) {
        score = tokens[2];
        last_modified_time = tokens[3];
        num_timestamp = tmp_timestamp;
      }
    }
    if (num_timestamp == 0) continue;
    // M: 标示 该记录来之更新模块
    value = "1\t" + score + "\tM\t" + last_modified_time;
    task->Output(key, value);
  }
  return 0;
}
