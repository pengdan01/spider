
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#include "web/url/gurl.h"
#include "crawler/exchange/task_data.h"
DEFINE_string(ip_load_file_path, "", "ip_load_file_path");
DEFINE_uint64(default_ip_load, 3, "default ip load value");
void ReadIPLoadFile(std::map<std::string, uint64_t> &ip_load) {
  std::string read_buffer;
  std::vector<std::string> fields;
  std::ifstream fin(FLAGS_ip_load_file_path);
  if (!fin) {
    LOG(FATAL) << "open " << FLAGS_ip_load_file_path << " failed";
  }
  while(std::getline(fin, read_buffer)) {
    fields.clear();
    base::SplitString(read_buffer, "\t", &fields);
    CHECK_EQ(4UL, fields.size());
    int load = base::ParseIntOrDie(fields[2]);
    ip_load[fields[0]] += load;
  }
  fin.close();
}

void Mapper() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  std::string key;
  std::string value;
  std::vector<std::string> fields;
  std::map<std::string, uint64_t> ip_count;
  while (task->NextKeyValue(&key, &value)) {
    fields.clear();
    base::SplitString(value, "\t", &fields);
    CHECK_EQ(crawler::kTaskItemNum-1, fields.size());
    const std::string &ip=fields[0];
    ip_count[ip]++;
  }
  auto it = ip_count.begin();
  for ( ; it != ip_count.end(); ++it) {
    task->Output(it->first, base::Uint64ToString(it->second));
  }
}

void Reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  std::string key;
  std::string value;
  std::map<std::string, uint64_t> ip_count;
  std::map<std::string, uint64_t> ip_load;
  ReadIPLoadFile(ip_load);
  while (task->NextKey(&key)) {
    while (task->NextValue(&value)) {
      ip_count[key] += base::ParseUint64OrDie(value);
    }
  }
  auto it = ip_count.begin();
  for ( ; it != ip_count.end(); ++it) {
    uint64_t ip_load_value = FLAGS_default_ip_load;
    if (ip_load.find(it->first) != ip_load.end()) {
      ip_load_value = ip_load[it->first];
    }
    double time_consume = (double)it->second / ip_load_value;
    task->Output(it->first, base::DoubleToString(time_consume) + "\t" +
                 base::Uint64ToString(it->second) + "\t" +
                 base::Uint64ToString(ip_load_value));
  }
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "ip stat");
  if (FLAGS_ip_load_file_path == "") {
    LOG(FATAL) << "please set host_load_file_path";
  }
  if (base::mapreduce::IsMapApp()) {
    Mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    Reducer();
  } else {
    CHECK(false);
  }
}
