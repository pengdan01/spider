#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/mapreduce/mapreduce.h"
#include "base/strings/string_util.h"
#include "base/strings/string_split.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"
#include "web/url/gurl.h"

const uint32 kSelect_Value_Output_FieldNum = 5;
const uint32 kUpdate_Value_Output_FieldNum = 4;

// reducer 压力控制，根据统计 pv log 得到的 各个 host 的 日 PV 和抓取率 crawle_ratio
// 选取待抓取的 URL
DEFINE_double(crawle_ratio, 1.0, "crawle ratio is calculated from the daily pv of each web host and the "
              "crawle ability of crawler cluster");
DEFINE_int32(max_currency_access_thread, 3, "should be the same as it is set in mapper");
DEFINE_double(lamla, 1.0, "for compress calculate, value is about 1/5 * |max_currency_access_thread|");

// 当一个 站点 的抓取量超过了 日 PV, 是否过滤掉多余的 URL
DEFINE_bool(ignore_overload_urls, true, "");

namespace crawler {
struct UrlRecord {
  std::string url;
  std::string type;
  double score;
  char from;
  std::string refer_or_modified_time;
  // Only valid when |refer_or_modified_time| is a refer url
  char refer_from;
  explicit UrlRecord() {}
};

bool is_greater(const UrlRecord &url1, const UrlRecord &url2) {
  // 看类型: html 排在 css/image 前面 (1,2,3)
  if (url1.type != url2.type) return url1.type < url2.type;
  // 看来源: User seeds > Search Click > PV/Newlink
  if (url1.from != url2.from) {
    if (url1.from == 'E') {
      return true;
    } else if (url2.from == 'E') {
      return false;
    } else if (url1.from == 'S') {
      return true;
    } else if (url2.from == 'S') {
      return false;
    } else if (url1.from == 'A') {
      return true;
    } else if (url2.from == 'A') {
      return false;
    }
  }
  // 看 url 的长度
  return url1.url.size() < url2.url.size();
}

// value 为 selector 的整条输出记录, 格式如下:
// url \t type \t score \t from \t refer_url \t refer_from
// or
// Updater 模块输出, 格式如下:
// url \t type \t score \t from \t last_modified_time
bool BuildStructFromValueString(const std::string &value, UrlRecord *url_record) {
  CHECK_NOTNULL(url_record);
  std::vector<std::string> tokens;
  base::SplitString(value, "\t", &tokens);
  int num = tokens.size();
  if (num != (kSelect_Value_Output_FieldNum+1) && num != (kUpdate_Value_Output_FieldNum+1)) return false;
  if (!base::StringToDouble(tokens[2], &url_record->score)) {
    return false;
  }
  url_record->url = tokens[0];
  url_record->type = tokens[1];
  url_record->from = tokens[3][0];
  url_record->refer_or_modified_time = tokens[4];
  url_record->refer_from = 'K';
  if (num == (kSelect_Value_Output_FieldNum+1)) {
    url_record->refer_from = tokens[5][0];
  }
  return true;
}
}  // end namespace

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "Dispatcher Compress Control Reducer");
  base::mapreduce::ReduceTask * task = base::mapreduce::GetReduceTask();
  CHECK_NOTNULL(task);
  CHECK(FLAGS_crawle_ratio > 0.0);
  CHECK(FLAGS_max_currency_access_thread > 0 && FLAGS_lamla > 0.0);

  std::string key;
  std::string value;
  char flag;
  std::string host;
  std::string last_host = "";
  int64 daily_pv = 0;
  int64 last_daily_pv = 0;
  int64 assigned_url_cnt = 0;
  int64 image_css_cnt = 0;
  int64 will_crawle = 0;
  std::vector<crawler::UrlRecord> tasks;
  std::vector<crawler::UrlRecord> tasks_backup;
  int64 filter_cnt = 0;
  // the mapper output format is: hostA \t PV  or  hostB \t Record
  while (task->NextKey(&key)) {
    base::TrimWhitespaces(&key);
    LOG_IF(FATAL, key.size() < 2) << "Invalid key: " << key;
    flag = key[key.size() - 1];
    LOG_IF(FATAL, flag != 'A' && flag != 'B') << "Flag in Reducer key must be A or B, but is: " << flag;
    if (flag == 'A') {
      // 来至 stat 的数据，只有一对 <key, value> pair, NO need to use while
      if (task->NextValue(&value)) {
        if (!base::StringToInt64(value, &daily_pv)) {
          LOG(ERROR) << "String convert to Int64 fail: " << value;
          continue;
        }
        last_host = key.substr(0, key.size() - 1);
        last_daily_pv = daily_pv;
        continue;
      }
    } else {  // flag == 'B'
      host = key.substr(0, key.size() - 1);
      if (host == last_host) {  // 该抓取 URL 已经有用户访问过
        will_crawle = (int64)(last_daily_pv  / FLAGS_max_currency_access_thread * FLAGS_lamla);
        if (will_crawle == 0) will_crawle = 1;
        // 最多缓存 10 × |will_crawle| 的待抓取 URL, 避免内存超过单 REDUCER 上限
        while (task->NextValue(&value)) {
          base::TrimWhitespaces(&value);
          if (value.empty()) {
            LOG(ERROR) << "value is empty, key: " << key;
            ++filter_cnt;
            continue;
          }
          crawler::UrlRecord record;
          LOG_IF(FATAL, !crawler::BuildStructFromValueString(value, &record)) << "Input record: " << value;
          if (record.type != "1") {  // Image/Css 等附属链接不在压力控制范围之类, 直接输出
            std::string out_value;
            out_value = base::StringPrintf("%s\t%f\t%c\t%s\t%c", record.type.c_str(), record.score,
                                           record.from, record.refer_or_modified_time.c_str(),
                                           record.refer_from);
            task->Output(record.url, out_value);
            ++image_css_cnt;
          } else {
            tasks.push_back(record);
          }
        }
        sort(tasks.begin(), tasks.end(), crawler::is_greater);
        //  抓取量小于实际待抓取量 && 需要过滤掉多余的 URLs
        if (will_crawle < (int64)tasks.size() && FLAGS_ignore_overload_urls == true) {
          filter_cnt += ((int64)tasks.size() - will_crawle);
        } else {
          will_crawle = tasks.size();
        }
        for (int i = 0; i < will_crawle; ++i) {
          std::string out_value;
          out_value = base::StringPrintf("%s\t%f\t%c\t%s\t%c", tasks[i].type.c_str(), tasks[i].score,
                                         tasks[i].from, tasks[i].refer_or_modified_time.c_str(),
                                         tasks[i].refer_from);
          task->Output(tasks[i].url, out_value);
        }
        assigned_url_cnt += will_crawle;
        tasks.clear();
        last_host = "";
      } else {
        // 该待抓取 URL 在 User Log 中没有出现，所以此 URL 可能是一个第一次出现，
        // 也可能是一个很垃圾的链接，根本就没有用户访问过，只能通过其 score 进行
        // 判断是否抓取，先放到 tasks_backup 队列等待处理
       //  while (task->NextValue(&value) && (int64)tasks_backup.size() < FLAGS_max_url_num_per_file) {
        while (task->NextValue(&value)) {
          base::TrimWhitespaces(&value);
          if (value.empty()) continue;
          crawler::UrlRecord record;
          LOG_IF(FATAL, !crawler::BuildStructFromValueString(value, &record)) << "Input record: " << value;
          tasks_backup.push_back(record);
        }
      }
    }
  }
  int64 backup_task_size = (int64)tasks_backup.size();
  sort(tasks_backup.begin(), tasks_backup.end(), crawler::is_greater);
  will_crawle = backup_task_size;
  for (int i = 0; i < will_crawle; ++i) {
    std::string out_value;
    out_value = base::StringPrintf("%s\t%f\t%c\t%s\t%c", tasks_backup[i].type.c_str(), tasks_backup[i].score,
                                   tasks_backup[i].from, tasks_backup[i].refer_or_modified_time.c_str(),
                                   tasks_backup[i].refer_from);
    task->Output(tasks_backup[i].url, out_value);
  }
  tasks_backup.clear();
  assigned_url_cnt += will_crawle;

  LOG(INFO) << "Assign Crawle Url(html)#: " << assigned_url_cnt << ", css/image#: " << image_css_cnt
            << ", filter url for overload of site#:" << filter_cnt;

  return 0;
}
