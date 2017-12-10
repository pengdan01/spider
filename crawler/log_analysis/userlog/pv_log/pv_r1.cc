#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <set>
#include <fstream>

#include "log_analysis/common/error_code.h"
#include "log_analysis/common/log_common.h"
#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_printf.h"
#include "base/encoding/line_escape.h"
#include "base/mapreduce/mapreduce.h"
#include "base/random/pseudo_random.h"
#include "base/time/timestamp.h"
#include "nlp/common/nlp_util.h"
#include "crawler/api/base.h"

// static const std::string kNoRefer = "NOREFER";
DEFINE_string(md5_url_path, "", "hdfs path of md5->url files");
DEFINE_string(pv_log_path,  "", "hdfs path of pv log");
DEFINE_string(hot_md5_path,  "", "local file path of high pv url md5 map");
DEFINE_string(skip_md5_path,  "", "local file path of high pv but error url's md5, should be skipped");

void load_md5_map(std::unordered_map<std::string, std::string>* map, const std::string& file_path) {
  std::ifstream in(file_path.c_str());
  CHECK(in) << file_path;
  std::string line;
  std::vector<std::string> flds;
  while (std::getline(in, line)) {
    base::TrimTrailingWhitespaces(&line);
    if (line.empty()) continue;
    flds.clear();
    base::SplitString(line, "\t", &flds);
    CHECK_EQ(flds.size(), 2u) << line;
    (*map)[flds[0]] = flds[1];
  }
  if (in.eof()) in.close();
}

void mapper() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  base::PseudoRandom rand(base::GetTimestamp());

  std::unordered_map<std::string, std::string> local_md5_map;
  if (!FLAGS_hot_md5_path.empty()) {
    load_md5_map(&local_md5_map, FLAGS_hot_md5_path);
  }

  std::unordered_map<std::string, std::string> skip_md5_map;
  if (!FLAGS_skip_md5_path.empty()) {
    load_md5_map(&skip_md5_map, FLAGS_skip_md5_path);
  }

  base::mapreduce::InputSplit split = task->GetInputSplit();
  const std::string& file_name = split.hdfs_path;
  int input_type;
  if (file_name.find(FLAGS_md5_url_path) != std::string::npos) {
    input_type = 0;
  } else if (file_name.find(FLAGS_pv_log_path) != std::string::npos) {
    input_type = 1;
  } else {
    LOG(FATAL) << "unknown input type, file: " << file_name;
  }

  std::string key;
  std::string value;
  std::string line;
  std::vector<std::string> flds;
  while (task->NextKeyValue(&key, &value)) {
    if (input_type == 1) {  // streaming input
      line = key + "\t" + value;
      if (line.empty()) continue;
      flds.clear();
      base::SplitString(line, "\t", &flds);
      if (flds.size() < 9) {
        task->ReportAbnormalData(log_analysis::kMapInputError, key, value);
        continue;
      }
      std::string& mid = flds[0];
      if (mid.empty()) {
        task->ReportAbnormalData(log_analysis::kMapInputError, key, value);
        continue;
      }
      std::string& enter_type = flds[1];
      std::string& attr = flds[6];
      std::string& duration = flds[8];
      std::string time_stamp;
      // token_url 日志的第 8 列是用户浏览 URL 的时间
      // 第 10 列是日志上传到服务器的时间。 因此选用前者
      if (!log_analysis::ConvertTimeFromSecToFormat(flds[7], "%Y%m%d%H%M%S", &time_stamp)) {
        task->ReportAbnormalData(log_analysis::kMapInputError, key, value);
        continue;
      }

      std::string& url_md5 = flds[2];
      std::string& ref_md5 = flds[4];
      if (url_md5.empty()) { 
        task->ReportAbnormalData(log_analysis::kInvalidURL, key, value);
        continue;
      }
      std::string url;
      if (local_md5_map.find(url_md5) != local_md5_map.end()) {
        url = local_md5_map[url_md5];
      }

      // 放在这个辞典中的url都是无效url, 而且有着很大的 pv. 发给reduce会造成严重的分桶问题,如
      // "aHR0cDovL3B0bG9naW4yLnFxLmNvbS9qdW1wPz9XPw==  http://ptlogin2.qq.com/jump??W?"
      if (skip_md5_map.find(url_md5) != skip_md5_map.end()) {
        // 如果目标 url 是此类 url, 跳过该记录
        task->ReportAbnormalData(log_analysis::kInvalidURL, key, value);
        continue;
      }
      if (skip_md5_map.find(ref_md5) != skip_md5_map.end()) {
        // 如果 ref 是此类 url, 仅将 ref 清空即可
        ref_md5.clear();
        continue;
      }

      std::string ref_url;
      if (!ref_md5.empty() && local_md5_map.find(ref_md5) != local_md5_map.end()) {
        ref_url = local_md5_map[ref_md5];
      }
      if (ref_md5.empty() || !ref_url.empty()) { // 没有 ref, 或已经查到 ref url
        if (url.empty()) {  // 需要查询 url_md5 对应的字面
          task->Output(url_md5,
                       mid  + "\t" + time_stamp + "\t" + ref_url + "\t" +
                       attr + "\t" + enter_type + "\t" + duration + "\tOnlyForDest");  // for dest url query
        } else {  // 不需要查询，直接输出到reduce，可以跳过第二轮
          // 第三轮的输入 [ mid, time_stamp, url, ref_url, '', '', attr, enter_type, duration]
          task->Output(mid,
                       time_stamp  + "\t" + url + "\t" + ref_url + "\t\t\t" +
                       attr + "\t" + enter_type + "\t" + duration);  // for pv log r3
        }
      } else {  // 需要查询 ref
        if (url.empty()) {  // 需要进入第二轮查询 url_md5 对应的字面 
          CHECK(!ref_md5.empty() && !url_md5.empty()) << "both md5 should be not empty";
          task->Output(ref_md5,
                       mid  + "\t" + time_stamp + "\t" + url_md5 + "\t" +
                       attr + "\t" + enter_type + "\t" + duration + "\tForRef");  // for dest and ref
          task->Output(url_md5,
                       mid  + "\t" + time_stamp + "\t" + ref_md5 + "\t" +
                       attr + "\t" + enter_type + "\t" + duration + "\tForDest");  // for dest and ref
        } else {  // 只需要查询 ref_url 就可以进入第三轮
          task->Output(ref_md5,
                       mid  + "\t" + time_stamp + "\t" + url + "\t" +
                       attr + "\t" + enter_type + "\t" + duration + "\tOnlyForRef");  // for ref url query
        }
      }
    } else {
      const std::string& base64 = key;
      std::string click_url;
      if (!log_analysis::Base64ToClickUrl(base64, &click_url)) {
        continue;
      }
      flds.clear();
      base::SplitString(value, "\t", &flds);
      std::vector<std::string> sub;
      for (int i = 0; i < (int)flds.size(); ++i) {
        sub.clear();
        base::SplitString(flds[i], ":", &sub);
        // md5, time
        CHECK_EQ(sub.size(), 2u);
        // md5, url, time
        task->Output(sub[0], click_url + "\t" + sub[1]);
      }
    }
  }
  return;
}

void reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();

  std::unordered_map<std::string, std::string> local_md5_map;
  if (!FLAGS_hot_md5_path.empty()) {
    load_md5_map(&local_md5_map, FLAGS_hot_md5_path);
  }

  int64 total_bytes = 0;
  int64 key_num = 0;

  std::string key;
  std::string line;
  std::vector<std::string> flds;
  while (task->NextKey(&key)) {
    std::string value;
    std::map<std::string, std::string> url_candidates;

    std::set<std::string> dedup_r2;  // output for r2
    std::set<std::string> dedup_r3;  // output for r3

    total_bytes += key.size();
    key_num++;

    int64 value_num = 0;
    int64 value_bytes = 0;

    while (task->NextValue(&value)) {
      value_num++;
      value_bytes += value.size();
      total_bytes += value.size();

      // LOG_EVERY_N(INFO, 100) <<
      //     base::StringPrintf("key num: %ld, total_bytes: %ld, value_num: %ld, value_bytes: %ld",
      //                        key_num, total_bytes, value_num, value_bytes);

      line = key + "\t" + value;
      flds.clear();
      base::SplitString(line, "\t", &flds);
      if (flds.size() == 8) {
        if (flds.back() == "OnlyForDest" || flds.back() == "OnlyForRef") {
          dedup_r3.insert(line);
        } else if (flds.back() == "ForDest" || flds.back() == "ForRef") {
          dedup_r2.insert(line);
        } else {
          LOG(FATAL) << base::StringPrintf("invalid line: [%s], field [%d]",
                                           line.c_str(), (int)flds.size());
        }
      } else if (flds.size() == 9) {
        dedup_r3.insert(line);
      } else if (flds.size() == 3) {
        // md5, url, time
        // 按时间戳从小到大排序 url
        url_candidates[flds[2]] = flds[1];
        const std::string& url = flds[1];
        if (local_md5_map.find(flds[0]) != local_md5_map.end()) {
          if (flds[0] == "2e8873d882b4eb4b73b35f5214d2e90d" && url == "http://www.sohu.com./") {
            LOG(ERROR) << "special case: " << flds[0] << " " << url;
            flds[1] = "http://www.sohu.com/";
          } else {
            LOG_IF(FATAL, local_md5_map[flds[0]] != url) <<
                base::StringPrintf("line [%s]. for md5 [%s], cached url is [%s], readin url is [%s]",
                                   line.c_str(), flds[0].c_str(), local_md5_map[flds[0]].c_str(), url.c_str());
          }
        }
      } else {
        LOG(FATAL) << base::StringPrintf("invalid line: [%s], field [%d]",
                                         line.c_str(), (int)flds.size());
      }
    }
 
    std::string url;
    if (!url_candidates.empty()) {
      // 选择时间戳最靠后的记录
      url = url_candidates.rbegin()->second;
      
      if (url_candidates.size() > 1u) {
        // 出现 url 冲突
        // 报告 url 冲突
        std::string debug_str;
        for (auto it = url_candidates.begin(); it != url_candidates.end(); ++it) {
          if (!debug_str.empty()) {
            debug_str += "\t";
          }
          debug_str = debug_str + it->second + ":" + it->first;
        }
        task->ReportAbnormalData(log_analysis::kMD5Conflict, key, debug_str);
      }
    }
    // LOG(INFO) << base::StringPrintf("key num: %ld, total_bytes: %ld", key_num, total_bytes);
    if (value_num > 100000) {
      LOG(INFO) << base::StringPrintf("key: [%s], url: [%s], value_num: %ld, value_bytes: %ld",
                                      key.c_str(), url.c_str(), value_num, value_bytes);
      task->ReportAbnormalData("HotSiteMd5", key,
                               base::StringPrintf("url:[%s], value_num:[%ld]", url.c_str(), value_num));
    }

    for (auto it = dedup_r2.begin(); it != dedup_r2.end(); ++it) {
      flds.clear();
      base::SplitString(*it, "\t", &flds);
      // flds[0] = url;
      LOG_IF(FATAL, flds.size() != 8 || (flds.back() != "ForDest" && flds.back() != "ForRef"))
          << base::StringPrintf("invalid line: [%s], field [%d]", it->c_str(), (int)flds.size());

      // output: [mid, time_stamp, md5, url ] for next round merge
      if (!url.empty()) {
        task->Output(flds[1], flds[2] + "\t" + flds[0] + "\t" + url);
      } else {
        task->ReportAbnormalData(log_analysis::kInvalidURL, key, "");
      }

      // 没有做任何 md5->url 的替换，而是留到下一轮的 reduce 阶段进行
      if (flds.back() == "ForDest") {
        // From: [url_md5, mid, time_stamp, ref_md5, attr, enter_type, duration, SIGN]
        // To:   [ mid, time_stamp, url_md5, ref_md5, '', '', attr, enter_type, duration, SIGN]
        task->Output(flds[1],
                     log_analysis::JoinFields(flds, "\t" , "203") + "\t\t\t"
                     + log_analysis::JoinFields(flds, "\t", "456") + "\tstill_md5");
      } else {
        // From: [ref_md5, mid, time_stamp, url_md5, attr, enter_type, duration, SIGN]
        // To:   [ mid, time_stamp, url_md5, ref_md5, '', '', attr, enter_type, duration]
        task->Output(flds[1],
                     log_analysis::JoinFields(flds, "\t" , "230") + "\t\t\t"
                     + log_analysis::JoinFields(flds, "\t", "456") + "\tstill_md5");
      }
    }

    // LOG(INFO) << "output for round2 finished.";

    for (auto it = dedup_r3.begin(); it != dedup_r3.end(); ++it) {
      flds.clear();
      base::SplitString(*it, "\t", &flds);
      if (flds.size() == 9) {
        task->Output(flds[0], log_analysis::JoinFields(flds, "\t" ,"12345678"));
      } else if (flds.size() == 8) {
        flds[0] = url;
        if (flds.back() == "OnlyForDest") {
          if (url.empty()) {
            task->ReportAbnormalData(log_analysis::kInvalidURL, key, "");
            continue;
          }
          // From: [url_md5, mid, time_stamp, ref_url, attr, enter_type, duration, SIGN]
          // To:   [ mid, time_stamp, url, ref_url, '', '', attr, enter_type, duration]
          task->Output(flds[1],
                       log_analysis::JoinFields(flds, "\t" , "203") + "\t\t\t"
                       + log_analysis::JoinFields(flds, "\t", "456"));
        } else if (flds.back() == "OnlyForRef") {
          // From: [ref_md5, mid, time_stamp, url, attr, enter_type, duration, SIGN]
          // To:   [mid, time_stamp, url, ref_url, '', '', attr, enter_type, duration]
          task->Output(flds[1],
                       log_analysis::JoinFields(flds, "\t" , "230") + "\t\t\t"
                       + log_analysis::JoinFields(flds, "\t", "456"));
        } else {
          LOG(FATAL) << base::StringPrintf("invalid line: [%s], field [%d]",
                                           it->c_str(), (int)flds.size());
        }
      } else {
        LOG(FATAL) << base::StringPrintf("invalid line: [%s], field [%d]",
                                         it->c_str(), (int)flds.size());
      }
    }
    // LOG(INFO) << "output for round3 finished.";
  }
  return;
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "pv log r1");
  CHECK(!FLAGS_md5_url_path.empty() && !FLAGS_pv_log_path.empty())
      << "specify md5_url_path and pv_log_path";
  if (base::mapreduce::IsMapApp()) {
    mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    reducer();
  } else {
    CHECK(false);
  }
}
