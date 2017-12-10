// 输入 host -> ip 映射表；UV 数据； host_load 数据
// 为 UV 数据填入 IP 字段，将 host_load 转换为 ip_load

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/mapreduce/mapreduce.h"
#include "base/encoding/line_escape.h"
#include "base/hash_function/city.h"
#include "base/strings/string_split.h"
#include "base/strings/string_number_conversions.h"
#include "web/url/gurl.h"
#include "crawler/exchange/task_data.h"

DEFINE_string(host_ip_path, "", "host to ip map");
DEFINE_string(crawler_task_input_path, "", "crawler task input data path");

static const uint64_t KHostLoadFieldNum = 4;

void HostIPMapper() {
  std::string key;
  std::string value;
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  int reduce_num = task->GetReducerNum();
  while (task->NextKeyValue(&key, &value)) {
    for (int i = 0; i < reduce_num; ++i) {
      task->OutputWithReducerID(key + "\tA", value, i);
    }
  }
}

void CrawlerTaskMapper() {
  srand(time(NULL));
  std::string key;
  std::string value;
  std::vector<std::string> fields;
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  int reduce_num = task->GetReducerNum();
  while (task->NextKeyValue(&key, &value)) {
    fields.clear();
    base::SplitString(value, "\t", &fields);
    CHECK_EQ(fields.size(), crawler::kTaskItemNum-1);
    GURL url(key);
    task->OutputWithReducerID(url.host() + "\tB", key+"\t"+value, rand() % reduce_num);
  }
}

void Reducer() {
  base::mapreduce::ReduceTask* task = base::mapreduce::GetReduceTask();
  std::string dict_key;
  std::vector<std::string> host_ips;
  std::string key;
  srand(time(NULL));
  while (task->NextKey(&key)) {
    if (key[key.size()-1] == 'A') {
      std::string value;
      while (task->NextValue(&value)) {}
      host_ips.clear();
      base::SplitStringWithOptions(value, "\t", true, true, &host_ips);
      dict_key = key.substr(0, key.size() - 2);
    } else if (key[key.size()-1] == 'B') {
      std::string data_key = key.substr(0, key.size()-2);
      if (data_key != dict_key || host_ips.empty()) {
        LOG(ERROR) << "no ip found, key: " << data_key;
        std::string value;
        while (task->NextValue(&value)) {
          LOG_EVERY_N(ERROR, 1000) << "no ip found, value: " << value;
        }
        continue;
      }
      std::string value;
      std::string crawler_output_key;
      std::string crawler_output_value;
      while (task->NextValue(&value)) {
        crawler::TaskItem task_item;
        CHECK(LoadTaskItemFromString(value, &task_item));
        task_item.ip = host_ips[rand()%host_ips.size()];
        crawler::SerializeTaskItemKeyValue(task_item, &crawler_output_key, &crawler_output_value);
        task->OutputWithFilePrefix(crawler_output_key, crawler_output_value, "crawler_task");
        LOG_EVERY_N(INFO, 10000) << "Output: " << crawler_output_key << "\t" << crawler_output_value;
      }
    } else {
      LOG(FATAL) << "Invalid data key. " << key;
    }
  }
}

void Mapper() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  base::mapreduce::InputSplit split = task->GetInputSplit();
  const std::string& hdfs_path = split.hdfs_path;
  if (hdfs_path.find(FLAGS_host_ip_path) != std::string::npos) {
    HostIPMapper();
  } else {
    // 为了支持调度的输入数据路径包含通配符以及多路径，这里不使用 hdfs_path 路径匹配
    // 而是在 CrawlerTaskMapper 内部 CHECK 每一行的列个数
    CrawlerTaskMapper();
  }
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "url to ip");
  if (FLAGS_host_ip_path == "" ||
      FLAGS_crawler_task_input_path == "") {
    LOG(FATAL) << "please set host_ip_path, crawler_task_input_path";
  }
  if (base::mapreduce::IsMapApp()) {
    Mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    Reducer();
  } else {
    CHECK(false);
  }
}
