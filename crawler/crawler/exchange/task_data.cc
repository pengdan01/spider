#include <vector>
#include <string>

#include "base/strings/string_split.h"
#include "base/strings/string_number_conversions.h"
#include "crawler/exchange/task_data.h"

namespace crawler {

bool LoadTaskItemFromString(const std::string& str, TaskItem* item) {
  if (item == NULL) {
    return false;
  }
  // input format: url \t file_type \t importance \t referer \t payload
  std::vector<std::string> fields;
  base::SplitString(str, "\t", &fields);
  if (kTaskItemNum != fields.size()) {
    LOG(INFO) << "kTaskItemNum " << kTaskItemNum << " != fields.size()" << fields.size();
    return false;
  }

  int file_type;
  double importance;
  int retry_times;
  if (!base::StringToInt(fields[2], &file_type)) {
    return false;
  }
  if (!base::StringToDouble(fields[3], &importance)) {
    return false;
  }
  if (!base::StringToInt(fields[6], &retry_times)) {
    return false;
  }
  int32 baidu_index, google_index, realtime_crawl, level;
  if (!base::StringToInt(fields[7], &baidu_index) ||
      !base::StringToInt(fields[8], &google_index) ||
      !base::StringToInt(fields[9], &realtime_crawl) ||
      !base::StringToInt(fields[10], &level)) {
    return false;
  }

  item->url = fields[0];
  item->ip = fields[1];
  item->file_type = file_type;
  item->importance = importance;
  item->referer = fields[4];
  item->use_proxy = fields[5];
  item->retry_times = retry_times;
  item->is_indexed_baidu = static_cast<bool>(baidu_index);
  item->is_indexed_google = static_cast<bool>(google_index);
  item->is_realtime_crawl = static_cast<bool>(realtime_crawl);
  item->robots_level = level;
  return true;
}

std::string SerializeTaskItem(const TaskItem& item) {
  std::string output = item.url + "\t";                     // URL
  output += item.ip + "\t";
  output += base::IntToString(item.file_type) + "\t";       // file_type
  output += base::DoubleToString(item.importance) + "\t";   // importance
  output += item.referer + "\t";                            // referer
  output += item.use_proxy + "\t";                            // use_proxy
  output += base::IntToString(item.retry_times) + "\t";            // retry_times
  output += base::IntToString(static_cast<int32>(item.is_indexed_baidu)) + "\t";
  output += base::IntToString(static_cast<int32>(item.is_indexed_google)) + "\t";
  output += base::IntToString(static_cast<int32>(item.is_realtime_crawl)) + "\t";
  output += base::IntToString(item.robots_level);
  return output;
}

void SerializeTaskItemKeyValue(const TaskItem& item, std::string *key, std::string *value) {
  CHECK_NOTNULL(key);
  CHECK_NOTNULL(value);
  *key = item.url;  // url as key;
  *value = "";
  *value += item.ip + "\t";
  *value += base::IntToString(item.file_type) + "\t";       // file_type
  *value += base::DoubleToString(item.importance) + "\t";   // importance
  *value += item.referer + "\t";                            // referer
  *value += item.use_proxy + "\t";                            // user_proxy
  *value += base::IntToString(item.retry_times) + "\t";            // retry_times
  *value += base::IntToString(static_cast<int32>(item.is_indexed_baidu)) + "\t";
  *value += base::IntToString(static_cast<int32>(item.is_indexed_google)) + "\t";
  *value += base::IntToString(static_cast<int32>(item.is_realtime_crawl)) + "\t";
  *value += base::IntToString(item.robots_level);
}

}  // namespace crawler
