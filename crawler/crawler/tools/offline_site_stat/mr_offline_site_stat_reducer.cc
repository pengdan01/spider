#include <stdio.h>
#include <string.h>

#include <iostream>
#include <string>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"
#include "base/mapreduce/mapreduce.h"

const char flag1 = 'A';
const char flag2 = 'B';
const std::string kPrefix1("specific_site");
const std::string kPrefix2("top_n");

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "stat");
  CHECK(base::mapreduce::IsReduceApp());
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  CHECK_NOTNULL(task);

  std::string key;
  std::string value;
  std::vector<std::string> tokens;
  char flag =' ';
  std::string prefix;

  while (task->NextKey(&key)) {
    if (key.empty()) {
      LOG(ERROR) << "Record key is empty, ignore it.";
      continue;
    }
    int64 cnt = 0;
    while (task->NextValue(&value)) {
      if (value.size() < 1) continue;
      flag = value[0];
      if (flag != flag1 && flag != flag2) {
        LOG(ERROR) << "Invalid value, should start with A or B ,but: " << value;
        continue;
      }
      value = value.substr(1);
      int64 tmp_num = 0;
      if (!base::StringToInt64(value, &tmp_num)) {
        LOG(ERROR) << "StringToInt64() fail, str: " << value;
        continue;
      }
      cnt += tmp_num;
    }
    if (flag == flag1) {
      prefix = kPrefix1;
    } else {
      prefix = kPrefix2;
    }
    value = base::StringPrintf("%ld", cnt);
    task->OutputWithFilePrefix(key, value, prefix);
  }
  return 0;
}
