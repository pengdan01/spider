#include <string>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/mapreduce/mapreduce.h"
#include "base/strings/string_util.h"
#include "base/strings/string_tokenize.h"
#include "base/strings/string_number_conversions.h"
#include "crawler/proto/crawled_resource.pb.h"

DEFINE_int32(max_update_failed_num, 3, "The max times of update failure allowed. URLs will be removed "
             "from both linkbase and web page base when update fail counter >= |max_update_failed_num|");
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
  std::string out_value;
  std::vector<std::string> tokens;
  char flag;

  std::vector<UpdateInfo> update_info;
  crawler::CrawledResource pb;
  int64 timestamp = 0;
  // 网页数据输入的 key 为 rurl,  value = 'A'为一个序列化的 protocal buffer 的 string
  // 更新数据输入的 key 为 rurl,  value = 'B'cmd_type \t timestamp
  while (task->NextKey(&key)) {
    if (key.empty()) continue;
    timestamp = 0;
    out_value = "";
    update_info.clear();
    while (task->NextValue(&value)) {
      flag = value[0];
      LOG_IF(FATAL, !(flag == 'A' || flag == 'B')) << "Start flag in value shoud be A or B, value: " << value;
      value = value.substr(1);
      if (flag == 'A') {
        if (!pb.ParseFromString(value)) {
          LOG(ERROR) << "pb.ParseFromString(value) fail";
          continue;
        }
        int64 tmp_timestamp = pb.crawler_info().crawler_timestamp();
        if (tmp_timestamp > timestamp) {
          timestamp = tmp_timestamp;
          out_value = value;
        }
      } else {  // flag == 'B' Update info
        tokens.clear();
        int num = base::Tokenize(value, "\t", &tokens);
        LOG_IF(FATAL, num != 2) << "Update Info record format error, field# should be 3, input: "
                                 << key + "\t" + value;
        int64 tmp_timestamp;
        if (!base::StringToInt64(tokens[1], &tmp_timestamp)) {
          LOG(ERROR) << "Fail: Convert String to Int64, input: " << tokens[1];
          continue;
        }
        update_info.push_back(UpdateInfo(tokens[0][0], tmp_timestamp));
      }
    }
    if (timestamp == 0) continue;
    if (update_info.empty()) {
      task->Output(key, out_value);
    } else {
      bool dead_link = false;
      for (int i = 0; i < (int)update_info.size(); ++i) {
        const UpdateInfo &update = update_info[i];
        if (update.timestamp < timestamp) continue;  // 已经过期的更新信息, 直接丢弃
        if (update.cmd == 'D') {
          dead_link = true;
          break;
        }
        if (update.cmd == 'R') {
          int update_fail_cnt = 0;
          if (pb.crawler_info().has_update_fail_cnt()) {
            update_fail_cnt = pb.crawler_info().update_fail_cnt();
          }
          update_fail_cnt++;
          if (update_fail_cnt >= FLAGS_max_update_failed_num) {
            dead_link = true;
            break;
          }
          pb.mutable_crawler_info()->set_update_fail_cnt(update_fail_cnt);
        }
      }
      if (dead_link == false) {
        LOG_IF(FATAL, !pb.SerializeToString(&out_value)) << "Fail: pb.SerializeToString()";
        task->Output(key, out_value);
      } else {
        LOG(INFO) << "Dead link: " << key << ", will delete from web page base";
      }
    }
  }
  return 0;
}
