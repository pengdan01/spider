#include "base/common/base.h"
#include "base/common/gflags.h"
#include "base/common/logging.h"
#include "base/hash_function/city.h"
#include "base/mapreduce/mapreduce.h"
#include "crawler/api/base.h"
#include "base/strings/string_printf.h"
#include "base/random/pseudo_random.h"
#include "base/strings/string_split.h"
// #include "crawler/exchange/task_data.h"

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "");
  CHECK(base::mapreduce::IsMapApp());
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  std::string key, value;
  std::vector<std::string> tokens;
  
  while (task->NextKeyValue(&key, &value)) {
    tokens.clear();
    base::SplitStringWithMoreOptions(value, "\t", false, false, 1, &tokens);
    CHECK_EQ(tokens.size(), 2u);
    std::string output_key = tokens[0];
    std::string output_value = tokens[1];

    tokens.clear();
    base::SplitString(value, "\t", &tokens);
    CHECK(tokens.size() == 7u || tokens.size() == 10u) << value;

    task->Output(output_key, output_value);

//     crawler::TaskItem item;
//     std::string output_key, output_value;
//     if (!LoadTaskItemFromString(value, &item)) {
//       LOG(ERROR) << "Failed to parse: " << value;
//       continue;
//     }
// 
//     crawler::SerializeTaskItemKeyValue(item, &output_key, &output_value);
//     task->Output(output_key, output_value);
  }
  return 0;
}
