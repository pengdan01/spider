#include <iostream>
#include <string>
#include <vector>

#include "base/common/base.h"
#include "base/mapreduce/mapreduce.h"
#include "base/common/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_split.h"
#include "base/strings/string_tokenize.h"
#include "base/strings/string_number_conversions.h"

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "");
  auto task = base::mapreduce::GetReduceTask();

  std::string key, value;
  std::vector<std::string> tokens;
  // key: host, value: curl_ok_page# \t valid_page#
  while (task->NextKey(&key)) {
    std::string source_url;
    int64 timestamp = 0;
    while (task->NextValue(&value)) {
      tokens.clear();
      base::SplitString(value, "\t", &tokens);
      int64 tmp_timestamp = base::ParseInt64OrDie(tokens[1]);
      if (tmp_timestamp > timestamp) {
        source_url = tokens[0];
        timestamp = tmp_timestamp; 
      }
    }
    if (timestamp != 0) {
      value = base::StringPrintf("%s\t%ld", source_url.c_str(), timestamp);
      task->Output(key, value);
    }
  }
  return 0;
}
