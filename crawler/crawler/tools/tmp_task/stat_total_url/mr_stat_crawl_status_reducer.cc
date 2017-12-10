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

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "");
  CHECK(base::mapreduce::IsReduceApp());
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  CHECK_NOTNULL(task);

  std::string key;
  while (task->NextKey(&key)) {
    task->Output(key, "");
  }
  return 0;
}
