#include <iostream>
#include <string>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/mapreduce/mapreduce.h"
#include "base/strings/string_util.h"
#include "base/strings/string_split.h"
#include "base/strings/string_number_conversions.h"

const unsigned int kLinkbaseRecordFieldNumber = 15;
const unsigned int kUpdateFieldNumber = 3;

DEFINE_int32(max_update_failed_num, 3, "The max times of update failure allowed. URLs will be removed "
             "from linkbase when update fail counter >= |max_update_failed_num|");
static bool build_record_value(const std::vector<std::string> &vec, std::string *value);

struct UpdateInfo {
  char cmd;
  int64 timestamp;
  UpdateInfo(char cmd, int64 time): cmd(cmd), timestamp(time) {}
};

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "reducer_combine_batch_delta");
  CHECK(base::mapreduce::IsReduceApp());
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  CHECK_NOTNULL(task);

  std::string key;
  std::string value;
  std::string latest_value;

  int64 timestamp = 0;
  std::vector<std::string> tokens;
  std::vector<std::string> latest_tokens;
  std::vector<UpdateInfo> update_infos;
  // 记录格式：
  // source_url \t effective_url \t link_type \t timestamp...
  // source_url \t cmd_type \t timestamp
  while (task->NextKey(&key)) {
    LOG_IF(FATAL, key.empty()) << "Key is empty";
    timestamp = 0;
    update_infos.clear();
    while (task->NextValue(&value)) {
      LOG_IF(FATAL, value.empty()) << "Value is empty";
      tokens.clear();
      base::SplitString(value, "\t", &tokens);
      size_t num = tokens.size();
      if (num != kLinkbaseRecordFieldNumber - 1 && num != kUpdateFieldNumber - 1) {
        LOG(ERROR)  << "Record invalid, fields#: " << num+1 << ", key: " << key << ", value: " << value;
        continue;
      }
      if (num == kUpdateFieldNumber - 1) {  // Update cmd info
        if (tokens[0][0] != 'D' && tokens[0][0] != 'R') {
          LOG(ERROR) << "Invalid Command type ,should be D or R, but is: " << tokens[0][0]
                     << ", key: "<< key << ", value: " << value;
          continue;
        }
        int64 tmp_timestamp;
        if (!base::StringToInt64(tokens[1], &tmp_timestamp)) {
          LOG(ERROR) << "Convert String to Int64 fail, input: " << tokens[1];
          continue;
        }
        update_infos.push_back(UpdateInfo(tokens[0][0], tmp_timestamp));
      } else {  // Link base Record
        int64 tmp_timestamp;
        if (!base::StringToInt64(tokens[2], &tmp_timestamp)) {
          LOG(ERROR) << "Convert string to Int64 fail, string num: " << tokens[2];
          continue;
        }
        if (tmp_timestamp > timestamp) {
          latest_value = value;
          latest_tokens = tokens;
          timestamp = tmp_timestamp;
        }
      }
    }
    if (timestamp == 0) continue;
    if (update_infos.empty()) {  // 该 key 没有更新相关信息, 正常处理
      task->Output(key, latest_value);
      continue;
    }
    bool deadlink = false;
    for (int i = 0; i < (int)update_infos.size(); ++i) {
      const UpdateInfo &info = update_infos[i];
      if (info.timestamp < timestamp) continue;
      if (info.cmd == 'D') {
        deadlink = true;
        break;
      }
      // 'R': Increase the update failed counter
      int update_fail_cnt = latest_tokens[13][0] - '0' + 1;
      if (update_fail_cnt >= FLAGS_max_update_failed_num) {
        deadlink = true;
        break;
      }
      latest_tokens[13] = '0' + update_fail_cnt;  // XXX: update fail counter should not exceed 9
    }
    // Output Record
    if (deadlink == false) {
      LOG_IF(FATAL, !build_record_value(latest_tokens, &latest_value)) << "Failed in Build record value";
      task->Output(key, latest_value);
    }
  }
  return 0;
}

static bool build_record_value(const std::vector<std::string> &vec, std::string *value) {
  CHECK_NOTNULL(value);
  if (vec.empty()) return false;
  *value = vec[0];
  for (int i = 1; i < (int)vec.size(); ++i) {
    *value = *value + "\t" + vec[i];
  }
  return true;
}
