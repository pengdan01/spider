
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

#include "crawler/util/dns_resolve.h"

DEFINE_string(host_load_file_path, "", "host_load_file_path");

static const char KDNSServer[] = "180.153.240.21,180.153.240.22,180.153.240.23,180.153.240.24,180.153.240.25";

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
    host_load[read_temp_buffer] = load;
  }
  fclose(fp);
}

void Mapper() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  std::string key;
  std::string value;
  std::vector<std::string> fields;
  std::map<std::string, uint64_t> url_count;
  while (task->NextKeyValue(&key, &value)) {
    fields.clear();
    base::SplitString(value, "\t", &fields);
    CHECK_EQ(5UL, fields.size());
    const std::string &url=fields[0];
    GURL gurl(url);
    url_count[gurl.host()]++;
  }
  auto it = url_count.begin();
  for ( ; it != url_count.end(); ++it) {
    task->Output(it->first, base::Uint64ToString(it->second));
  }
}

void Reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  std::string key;
  std::string value;
  std::map<std::string, uint64_t> host_count;
  std::map<std::string, uint64_t> host_load;
  ReadHostLoadFile(host_load);
  while (task->NextKey(&key)) {
    while (task->NextValue(&value)) {
      host_count[key] += base::ParseUint64OrDie(value);
    }
  }
  auto it = host_count.begin();
  for ( ; it != host_count.end(); ++it) {
    uint64_t host_load_value = 3;
    if (host_load.find(it->first) != host_load.end()) {
      host_load_value = host_load[it->first];
    }
    double time_consume = (double)it->second / host_load_value;
    task->Output(it->first, base::DoubleToString(time_consume));
  }
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "host stat");
  if (FLAGS_host_load_file_path == "") {
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
