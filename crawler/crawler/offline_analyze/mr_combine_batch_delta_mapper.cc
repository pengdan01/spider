#include <string>
#include <map>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_util.h"
#include "base/mapreduce/mapreduce.h"
#include "crawler/proto/crawled_resource.pb.h"
#include "crawler/selector/crawler_selector_util.h"
#include "crawler/api/base.h"
#include "web/url/url.h"
#include "web/url_norm/url_norm.h"

const int64 kLocalBufferSetSize = 5000;

static void HandleProtoBuffer(base::mapreduce::MapTask *task);
static void HandleUpdateInfo(base::mapreduce::MapTask *task);

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "combine batch and delta");
  CHECK(base::mapreduce::IsMapApp());
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  CHECK_NOTNULL(task);

  const std::string &filepath = task->GetInputSplit().hdfs_path;
  CHECK(!filepath.empty());
  LOG(INFO) << "Input file path: " << filepath;
  if (base::MatchPattern(filepath, "*/update*")) {
    HandleUpdateInfo(task);
  } else {
    HandleProtoBuffer(task);
  }
  return 0;
}

// |timestamp| is the timestamp when this page is crawled
// |value| is a serialized string of protocal buffer
// |rurl| is the reverse url of source url
struct ProtocalBufferWithTimestamp {
  int64 timestamp;
  std::string rurl;
  std::string value;
  ProtocalBufferWithTimestamp(): timestamp(0) {}
  ProtocalBufferWithTimestamp(const std::string &value, const std::string &rurl,
                              int64 timestamp): timestamp(timestamp), rurl(rurl), value(value) {}
};
static void HandleProtoBuffer(base::mapreduce::MapTask *task) {
  int32 reduce_num = task->GetReducerNum();
  int32 task_id = 0;
  int64 count = 0;
  int64 total = 0;

  std::string key;
  std::string value;
  std::string normal_url;  // 没有反转的 url
  std::string reason;

  web::UrlNorm url_norm;

  crawler::CrawledResource pb;
  std::map<std::string, ProtocalBufferWithTimestamp> local_buffer;
  std::map<std::string, ProtocalBufferWithTimestamp>::iterator it;
  // 网页数据输入的 key 为 source_url 的反转形式, value 为一个 proto buffer
  while (task->NextKeyValue(&key, &value)) {
    LOG_IF(FATAL, key.empty() || value.empty()) << "key or value is empty";
    ++total;
    // 对 反转过 的 url 进行复原
    if (!web::ReverseUrl(key, &normal_url, false)) {
      LOG(ERROR) << "base::ReverseUrl() fail, url: " << key;
      continue;
    }
    // 使用严格的过滤规则
    if (crawler::WillFilterAccordingRules(normal_url, &reason, true)) {
      ++count;
      LOG_EVERY_N(WARNING, 200) << reason << ", count: " << google::COUNTER;
      continue;
    }
    if (!url_norm.NormalizeClickUrl(normal_url, &normal_url)) {
      LOG(ERROR) << "NormalizeClickUrl() fail, url: " << normal_url;
      continue;
    }
    if (!web::ReverseUrl(normal_url, &key, false)) {
      LOG(ERROR) << "base::ReverseUrl() fail 2, url: " << normal_url;
      continue;
    }
    if (!pb.ParseFromString(value)) {
      LOG(ERROR) << "pb.ParseFromString(value) fail";
      continue;
    }
    pb.set_source_url(normal_url);
    if (!pb.SerializeToString(&value)) {
      LOG(ERROR) << "pb.SerializeToString() fail";
      continue;
    }
    ProtocalBufferWithTimestamp tmp_var(value, key, pb.crawler_info().crawler_timestamp());
    it = local_buffer.find(normal_url);
    if (it != local_buffer.end()) {  // 已经存在 local_buffer
      if (it->second.timestamp < tmp_var.timestamp) {
        it->second = tmp_var;
      }
    } else {
      // the first element of map is normal key(not reverted)
      local_buffer.insert(std::make_pair(normal_url, tmp_var));
    }
    // 是否超过 local buffer 允许的最大 size
    if ((int32)local_buffer.size() > kLocalBufferSetSize) {
      for (it = local_buffer.begin(); it != local_buffer.end(); ++it) {
        // 计算 Reduce Id 是对正常的 url (没有反转) 进行
        if (crawler::AssignReduceId(it->first, reduce_num, &task_id, false)) {
          // output key is reversed url
          task->OutputWithReducerID((*it).second.rurl, "A" + (*it).second.value, task_id);
        } else {
          LOG(ERROR) << "crawler::AssignReduceId() fail, url: " << it->first;
        }
      }
      local_buffer.clear();
    }
  }
  // Finally, output all the data in local_buffer
  for (it = local_buffer.begin(); it != local_buffer.end(); ++it) {
    // 计算 Reduce Id 是对正常的 url (没有反转) 进行
    if (crawler::AssignReduceId(it->first, reduce_num, &task_id, false)) {
      // output key is reversed url
      task->OutputWithReducerID((*it).second.rurl, "A" + (*it).second.value, task_id);
    } else {
      LOG(ERROR) << "crawler::AssignReduceId() fail, url: " << it->first;
    }
  }
  local_buffer.clear();
  LOG(INFO) << "Total url#: " << total << ", filtered url#: " << count;
}

// 处理 更新相关信息
static void HandleUpdateInfo(base::mapreduce::MapTask *task) {
  int32 reduce_num = task->GetReducerNum();
  int32 task_id = 0;
  int64 count = 0;
  int64 total = 0;

  std::string key;
  std::string value;
  std::string rurl;  // 反转的 url
  std::string reason;
  // Record Format: Normal_url \t command_type \t timestamp
  while (task->NextKeyValue(&key, &value)) {
    LOG_IF(FATAL, key.empty() || value.empty()) << "key or value is empty";
    ++total;
    // 对 url 进行反转
    if (!web::ReverseUrl(key, &rurl, false)) {
      LOG(ERROR) << "base::ReverseUrl() fail, url: " << key;
      continue;
    }
    // 使用严格的过滤规则
    if (crawler::WillFilterAccordingRules(key, &reason, true)) {
      ++count;
      LOG_EVERY_N(WARNING, 200) << reason << ", count: " << google::COUNTER;
      continue;
    }
    // 计算 Reduce Id 是对正常的 url (没有反转) 进行
    if (crawler::AssignReduceId(key, reduce_num, &task_id, false)) {
      // output key is reversed url
      task->OutputWithReducerID(rurl, "B" + value, task_id);
    } else {
      LOG(ERROR) << "crawler::AssignReduceId() fail, url: " << key;
    }
  }
  LOG(INFO) << "Total update url#: " << total << ", filtered url#: " << count;
}
