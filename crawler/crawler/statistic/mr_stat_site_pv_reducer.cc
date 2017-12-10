#include <iostream>
#include <string>

#include "base/common/base.h"
#include "base/mapreduce/mapreduce.h"
#include "base/common/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"

DEFINE_int32(pv_log_num, 1, "the day number of pv logs, we will calculate the daily pv of each host");

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "");
  CHECK(base::mapreduce::IsReduceApp());
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  CHECK_NOTNULL(task);

  CHECK_GE(FLAGS_pv_log_num, 0);

  std::string key;
  std::string value;
  int64 pv_count;
  // 记录格式： host\tcount
  while (task->NextKey(&key)) {
    if (key.empty()) continue;
    pv_count = 0;
    while (task->NextValue(&value)) {
      int64 tmp_count;
      if (!base::StringToInt64(value, &tmp_count)) {
        LOG(WARNING) << "Convert string to int64 fail: " << value;
        continue;
      }
      pv_count += tmp_count;
    }
    // 对历史累计的 日 PV 和 当前新计算的 日 PV 取均值
    int64 average_pv = (int64)(pv_count / (FLAGS_pv_log_num + 1));
    if (average_pv >= 6) {
      std::string out = base::StringPrintf("%ld", average_pv);
      task->Output(key, out);
    }
  }
  return 0;
}
