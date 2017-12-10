#pragma once

#include <string>

#include "crawler/proto/crawled_resource.pb.h"

namespace crawler {

// 爬虫任务文件格式
struct TaskItem {
  std::string url;
  std::string ip;
  int file_type;
  float importance;
  std::string referer;
  std::string use_proxy;
  int retry_times;
  bool is_indexed_baidu;
  bool is_indexed_google;
  bool is_realtime_crawl;
  int32 robots_level;

  TaskItem()
      : file_type(kHTML), importance(0.5), is_indexed_baidu(false),
      is_indexed_google(false), is_realtime_crawl(false), robots_level(0) {
  }
};

const uint64_t kTaskItemNum = 11;

std::string SerializeTaskItem(const TaskItem& item);
void SerializeTaskItemKeyValue(const TaskItem& item, std::string *key, std::string *value);
bool LoadTaskItemFromString(const std::string& str, TaskItem* item);
}  // namespace crawler
