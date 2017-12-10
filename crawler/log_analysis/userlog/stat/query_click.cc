#include <vector>
#include <unordered_map>

#include "log_analysis/common/log_common.h"
#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"
#include "base/file/file_util.h"
#include "base/mapreduce/mapreduce.h"
#include "base/hash_function/city.h"
#include "nlp/common/nlp_util.h"
#include "crawler/api/base.h"

// void mapper() {
//   std::vector<std::string> lines;
//   base::file_util::ReadFileToLines("luanma", &lines);
//   CHECK_GT(lines.size(), 0u);
//   std::unordered_set<std::string> hash;
//   for (int i = 0; i < (int)lines.size(); ++i) {
//     hash.insert(lines[i]);
//   }
//   LOG(INFO) << "total " << hash.size();
// 
//   base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
//   std::string key;
//   std::string value;
//   std::string line;
//   std::vector<std::string> flds;
//   while (task->NextKeyValue(&key, &value)) {
//     line = key + "\t" + value;
//     if (line.empty()) continue;
//     flds.clear();
//     base::SplitString(line, "\t", &flds);
//     if (flds.size() >= 7) {  // new log
//       if (flds[4].empty() || flds[6].empty()) {
//         continue;
//       }
//       const std::string& md5 = flds[4];
//       const std::string& base64_text = flds[6];
//       std::string decoded_text;
//       std::string non_utf8;
//       std::string normalized;
//       int ret = log_analysis::Base64ToUTF8(base64_text, &non_utf8, &decoded_text);
//       if (ret == log_analysis::kSucc) {
//         if (crawler::NormalizeUrl(decoded_text, &normalized)) {
//           if (hash.find(normalized) != hash.end()) {
//             task->Output(md5, base64_text + "\t" + normalized);
//           }
//         }
//       }
//     }
//   }
//   return;
// }
// 
// void reducer() {
//   base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
//   std::string key;
//   std::vector<std::string> flds;
//   while (task->NextKey(&key)) {
//     std::string value;
//     std::unordered_set<std::string> dedup;
//     while (task->NextValue(&value)) {
//       task->Output(key, value);
//     }
//   }
//   return;
// }

bool is_vertical_site(const std::string& url, std::string* site) {
  if (url.find("iqiyi.com") != std::string::npos ||
      url.find("youku.com") != std::string::npos ||
      url.find("tudou.com") != std::string::npos ||
      url.find("www.ku6.com") != std::string::npos ||
      url.find("tv.sohu.com") != std::string::npos) {
    *site = "Video";
    return true;
  }

  if (url.find("qidian.com") != std::string::npos ||
      url.find("hongxiu.com") != std::string::npos ||
      url.find("readnovel.com") != std::string::npos ||
      url.find("www.quanben.com") != std::string::npos ||
      url.find("www.ranwen.com") != std::string::npos ||
      url.find("www.xs8.cn") != std::string::npos ||
      url.find("www.fmx.cn") != std::string::npos ||
      url.find("www.kanshu.com") != std::string::npos) {
    *site = "Book";
    return true;
  }

  if (url.find("hackol.com") != std::string::npos ||
      url.find("skycn.com") != std::string::npos ||
      url.find("newhua.com") != std::string::npos ||
      url.find("xiazaiba.com") != std::string::npos ||
      url.find("crsky.com") != std::string::npos) {
    *site = "Software";
    return true;
  }

  if (url.find("hackol.com") != std::string::npos ||
      url.find("skycn.com") != std::string::npos ||
      url.find("newhua.com") != std::string::npos ||
      url.find("xiazaiba.com") != std::string::npos ||
      url.find("crsky.com") != std::string::npos) {
    *site = "Software";
    return true;
  }

  if (url.find("4399.com") != std::string::npos ||
      url.find("7k7k.com") != std::string::npos ||
      url.find("17173.com") != std::string::npos ||
      url.find("yxdown.com") != std::string::npos ||
      url.find("youxi.zol.com") != std::string::npos ||
      url.find("games.sina.com.cn") != std::string::npos ||
      url.find("www.pcgames.com.cn") != std::string::npos ||
      url.find("games.qq.com") != std::string::npos ||
      url.find("pcgames.com") != std::string::npos) {
    *site = "Game";
    return true;
  }

  if (url.find("4399.com") != std::string::npos ||
      url.find("7k7k.com") != std::string::npos ||
      url.find("17173.com") != std::string::npos ||
      url.find("yxdown.com") != std::string::npos ||
      url.find("youxi.zol.com") != std::string::npos ||
      url.find("games.sina.com.cn") != std::string::npos ||
      url.find("www.pcgames.com.cn") != std::string::npos ||
      url.find("games.qq.com") != std::string::npos ||
      url.find("pcgames.com") != std::string::npos) {
    *site = "Game";
    return true;
  }

  if (url.find("zhidao.baidu.com") != std::string::npos ||
      url.find("iask.sina.com.cn") != std::string::npos ||
      url.find("wenwen.soso.com") != std::string::npos) {
    *site = "QA";
    return true;
  }

  
  return false;
}

DEFINE_string(pv_log_path, "", "hdfs path of pv log");
DEFINE_string(search_log_path, "", "hdfs path of search log");

void mapper_output(base::mapreduce::MapTask *task, const std::string& query,
                   const std::string& site, const std::string& dedup_id) {
  static int32 reducer_count = task->GetReducerNum();
  CHECK_GT(reducer_count, 0);

  if (site.empty()) {
    task->Output(query, dedup_id);
  } else {
    int32 reduce_id = base::CityHash64(query.c_str(), query.size()) % reducer_count;
    std::string compound_key = query + "\t" + site;
    task->OutputWithReducerID(compound_key, dedup_id, reduce_id);
  }
}

void mapper() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();

  base::mapreduce::InputSplit split = task->GetInputSplit();
  const std::string& file_name = split.hdfs_path;
  int input_type;
  if (!FLAGS_pv_log_path.empty() && file_name.find(FLAGS_pv_log_path) != std::string::npos) {
    input_type = 0;
  } else if (!FLAGS_search_log_path.empty() && file_name.find(FLAGS_search_log_path) != std::string::npos) {
    input_type = 1;
  } else {
    LOG(FATAL) << "unknown input type, file: " << file_name;
  }

  std::string key;
  std::string value;
  std::string line;
  std::vector<std::string> flds;
  std::string query;
  std::string site;
  std::string vertical_site;
  std::string charset;
  bool suc;
  while (task->NextKeyValue(&key, &value)) {
    line = key + "\t" + value;
    if (line.empty()) continue;
    flds.clear();
    base::SplitString(line, "\t", &flds);
    std::string query;
    std::string dedup_id;
    if (input_type == 0) {  // pv log
      if (flds.size() > 5) {
        dedup_id = flds[0];
        const std::string& url = flds[2];
        const std::string& ref_url = flds[3];
        if (ref_url.empty()) continue;
        bool ret = is_vertical_site(url, &vertical_site);
        if (ret) {
          suc = log_analysis::ParseSearchQuery(ref_url, &query, &site, &charset);
          if (suc) {
            mapper_output(task, query, vertical_site, dedup_id);
          }
        }
      }
    } else {  // search log
      if (flds.size() > 4) {
        query = flds[3];
        dedup_id = flds[0];
        if (!query.empty() && !dedup_id.empty()) {
          mapper_output(task, query, "", dedup_id);
        }
      }
    }
  }
  return;
}

void reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  std::string key;
  std::vector<std::string> flds;
  while (task->NextKey(&key)) {
    std::string value;
    std::unordered_set<std::string> dedup;
    while (task->NextValue(&value)) {
      dedup.insert(value);
    }
    if (dedup.size() > 0) {
      task->Output(key, base::IntToString(dedup.size()));
    }
  }
  return;
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "query count");
  if (base::mapreduce::IsMapApp()) {
    mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    reducer();
  } else {
    CHECK(false);
  }
}
