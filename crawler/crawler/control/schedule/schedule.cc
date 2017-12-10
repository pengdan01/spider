// Mapper: 把 URL 按照签名分发给 FETCHER_NODE_NUM 个节点
// Reducer: 对每个节点处理的 URL 按负载切分任务
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
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

DEFINE_int32(fetcher_node_num, 0, "number of fetcher servers");
DEFINE_string(host_load_file_path, "", "path of host load file");
DEFINE_uint64(runtime_per_task, 0, "expect runtime(seconds) of fetcher for each task");
DEFINE_int32(default_qps, 3, "default qps of host which is not in host_load_file");

static const char kFieldsNumberAbnormal[] = "kFieldsNumberAbnormal";

void ReadHostLoadFile(std::map<std::string, uint64_t> &host_load) {
  char read_temp_buffer[1000];
  FILE *fp = fopen(FLAGS_host_load_file_path.c_str(), "r");
  if (!fp) {
    LOG(FATAL) << "open " << FLAGS_host_load_file_path << " failed";
  }
  while(fgets(read_temp_buffer, 1000, fp)) {
    char *space = strchr(read_temp_buffer, ' ');
    if (space == NULL) {
      LOG(FATAL) << "host_load_file format error \t" << read_temp_buffer;
    }
    *space = '\0';
    space = strchr(space+1, ' ');
    if (space == NULL) {
      LOG(FATAL) << "host_load_file format error \t" << read_temp_buffer;
    }
    int load = atoi(space+1);
    host_load[read_temp_buffer] = load * FLAGS_runtime_per_task;
  }
  fclose(fp);
}

void Mapper() {
  // key  : url 
  // value: file_type \t importance \t referer \t payload
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  std::string key;
  std::string value;
  std::vector<std::string> fields;
  while (task->NextKeyValue(&key, &value)) {
    fields.clear();
    base::SplitString(value, "\t", &fields);
    if (fields.size() != crawler::kTaskItemNum - 1) {
      task->ReportAbnormalData(kFieldsNumberAbnormal, key, value);
      continue;
    }
    // 按照 url host 分桶
    GURL url(key);
    const std::string &host = url.host();
    uint64_t host_sign = base::CityHash64(host.c_str(), host.length());
    uint64_t reducer_id = host_sign % FLAGS_fetcher_node_num;
    task->OutputWithReducerID(key, value, reducer_id);
  }
}

void Reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  int task_id = task->GetTaskId();
  // key   : url
  // value : file_type \t importance \t referer \t payload
  std::map<std::string, uint64_t> host_load; // host->max_url_number_pre_task
  ReadHostLoadFile(host_load);
  uint64_t default_max_url_per_task = FLAGS_default_qps * FLAGS_runtime_per_task;
  std::map<std::string, uint64_t> host_count;
  std::string key;
  std::string value;
  while (task->NextKey(&key)) {
    while (task->NextValue(&value)) {
      GURL url(key);
      const std::string &host = url.host();
      uint64_t output_task_id;
      if (host_load.find(host) == host_load.end()) {
        output_task_id = host_count[host] / default_max_url_per_task;
      } else {
        output_task_id = host_count[host] / host_load[host];
      }
      ++host_count[host];
      std::string output_file_name = "fetcher_" + base::IntToString(task_id) 
                                    +"_task_" + base::Uint64ToString(output_task_id);
      task->OutputWithFilePrefix(key, value, output_file_name);
    } 
  }
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "crawler schedule");
  if (FLAGS_fetcher_node_num <= 0) {
    LOG(FATAL) << "fetcher node number should larger than 0";
  }
  if (FLAGS_runtime_per_task == 0) {
    LOG(FATAL) << "url number per task";
  }
  if (base::mapreduce::IsMapApp()) {
    Mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    Reducer();
  } else {
    CHECK(false);
  }
}
