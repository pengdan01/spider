#include <stdio.h>
#include <string.h>

#include <iostream>
#include <string>
#include <algorithm>
#include <vector>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_split.h"
#include "base/strings/string_tokenize.h"
#include "base/strings/string_number_conversions.h"
#include "base/mapreduce/mapreduce.h"
#include "web/url/url.h"

DEFINE_int32(top_n, 5, "");

void uniq_url_host(const std::string &host, std::string *uniq_host) {
  CHECK_NOTNULL(uniq_host);
  std::vector<std::string> tokens;
  base::Tokenize(host, ",", &tokens);
  std::sort(tokens.begin(), tokens.end());
  tokens.erase(unique(tokens.begin(), tokens.end()), tokens.end());

  std::random_shuffle(tokens.begin(), tokens.end());
  
  int size = (int)tokens.size();
  for (int i = 0; i < size && i < FLAGS_top_n; ++i) {
    if (i == 0) {
      *uniq_host = tokens[i];
    } else {
      *uniq_host = *uniq_host + "," + tokens[i];
    }
  }
  return;
}

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "");
  CHECK(base::mapreduce::IsReduceApp());
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  CHECK_NOTNULL(task);

  std::string key, value;
  std::vector<std::string> tokens;
  while (task->NextKey(&key)) {
    if (key.empty()) continue;
    int64 total = 0;
    int64 ok_curl = 0;
    int64 ok_200 = 0;
    std::string host_list;
    while (task->NextValue(&value)) {
      tokens.clear();
      base::SplitString(value, "\t", &tokens);
      total += base::ParseIntOrDie(tokens[0]);
      ok_curl += base::ParseIntOrDie(tokens[1]);
      ok_200 += base::ParseIntOrDie(tokens[2]);
      if (host_list.empty()) {
        host_list = tokens[3];
      } else {
        host_list = host_list + "," + tokens[3];
      }
    }
    uniq_url_host(host_list, &host_list); 
    double curl_ok_rate = ok_curl * 1.0 / total;
    double ok_rate = ok_200 * 1.0 / total;
    value = base::StringPrintf("%ld\t%ld\t%f\t%ld\t%f\t%s", total, ok_curl, curl_ok_rate, ok_200, ok_rate,
                               host_list.c_str());
    task->Output(key, value);
  }
  return 0;
}
