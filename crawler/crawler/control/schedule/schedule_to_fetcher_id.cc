// Mapper: 读入hash表 按 value 把 url 分配到指定id
// Reducer: 对每个节点处理的 URL 按负载切分任务
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/mapreduce/mapreduce.h"
#include "base/encoding/line_escape.h"
#include "base/hash_function/city.h"
#include "base/strings/string_split.h"
#include "base/strings/string_number_conversions.h"

#include "crawler/exchange/task_data.h"

DEFINE_int32(fetcher_node_num, 0, "number of fetcher servers");
DEFINE_int32(reducer_num, 0, "reducer number");
DEFINE_string(ip_to_fetcher_id_file_path, "", "path of ip to fetcher id file");
DEFINE_uint64(url_num_per_task, 0, "expect url num for each task");

static const char kFieldsNumberAbnormal[] = "kFieldsNumberAbnormal";

void ReadIPToFetcherID(std::map<std::string, int> &ip_to_fetcher_id) {
  std::string read_buffer;
  std::vector<std::string> fields;
  std::ifstream fin(FLAGS_ip_to_fetcher_id_file_path);
  if (!fin) {
    LOG(FATAL) << "open " << FLAGS_ip_to_fetcher_id_file_path << " failed";
  }
  while (std::getline(fin, read_buffer)) {
    fields.clear();
    base::SplitString(read_buffer, "\t", &fields);
    int fetcher_id = base::ParseIntOrDie(fields[1]);
    ip_to_fetcher_id[fields[0]] = fetcher_id;
  }
  fin.close();
}

void Mapper() {
  // key  : url 
  // value: ip \t file_type \t importance \t referer \t payload
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  srand(time(NULL));

  int reducer_num = base::mapreduce::GetMapTask()->GetReducerNum();
  int avg_reducer_per_fetcher = reducer_num / FLAGS_fetcher_node_num;

  std::map<std::string, int> ip_to_fetcher_id;
  // read ip to fetcher id
  ReadIPToFetcherID(ip_to_fetcher_id);

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
    // 按照 url ip 分桶
    int fetcher_id;
    const std::string &host_ip = fields[0];
    if (ip_to_fetcher_id.find(host_ip) != ip_to_fetcher_id.end()) {
      fetcher_id = ip_to_fetcher_id[host_ip];
    } else {
      CHECK(false) << "cannot found ip: " << host_ip << " in ip_to_fetcher_id";
    }
    int rand_num = rand();
    task->OutputWithReducerID(base::IntToString(rand_num), key+"\t"+value,
                              fetcher_id + (rand() % avg_reducer_per_fetcher) * FLAGS_fetcher_node_num);
  }
}

void Reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  int task_id = task->GetTaskId();
  int fetcher_id = task_id % FLAGS_fetcher_node_num;
  // key   : url
  // value : file_type \t importance \t referer \t payload
  std::string key;
  std::string value;
  std::string output_key;
  std::string output_value;
  std::vector<int64> fetcher_url_num;
  int64 current_url_num = 0;
  while (task->NextKey(&key)) {
    while (task->NextValue(&value)) {
      ++current_url_num;
      std::string output_file_name = "fetcher_" + base::IntToString(fetcher_id) 
                                    +"_task_" + base::Uint64ToString(current_url_num/FLAGS_url_num_per_task);
      size_t tab_pos = value.find("\t");
      output_key = value.substr(0, tab_pos);
      output_value = value.substr(tab_pos+1, value.length()-tab_pos-1);
      task->OutputWithFilePrefix(output_key, output_value, output_file_name);
    } 
  }
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "crawler schedule");
  if (FLAGS_fetcher_node_num <= 0) {
    LOG(FATAL) << "fetcher node number should larger than 0";
  }
  if (FLAGS_url_num_per_task == 0) {
    LOG(FATAL) << "url_num_per_task should be larger than 0";
  }
  if (FLAGS_reducer_num == 0) {
    LOG(FATAL) << "please set reducer_num flags";
  }
  if (base::mapreduce::IsMapApp()) {
    int reducer_num = base::mapreduce::GetMapTask()->GetReducerNum();
    if (reducer_num % FLAGS_fetcher_node_num != 0) {
      LOG(FATAL) << "reducer_num % FLAGS_fetcher_node_num should be 0";
    }
    Mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    Reducer();
  } else {
    CHECK(false);
  }
}

