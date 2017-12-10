#include <stdio.h>
#include <string.h>

#include <iostream>
#include <string>
#include <set>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"
#include "base/mapreduce/mapreduce.h"

const int kOutLinkMinNum = 5;

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "build index page dict");
  CHECK(base::mapreduce::IsReduceApp());
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  CHECK_NOTNULL(task);

  std::string key;
  std::string value;
  std::vector<std::string> tokens;
  std::set<std::string> buffers;

  while (task->NextKey(&key)) {
    if (key.empty()) {
      LOG(ERROR) << "Record key is empty, ignore it.";
      continue;
    }
    buffers.clear();
    while (task->NextValue(&value)) {
      if (value.empty()) continue;
      buffers.insert(value);
    }
    int num = buffers.size();
    if (num >= kOutLinkMinNum) {
      value = base::StringPrintf("%d", num);
      task->Output(key, value);
    }
  }
  return 0;
}
