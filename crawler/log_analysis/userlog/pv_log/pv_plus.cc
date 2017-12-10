#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <map>
#include <set>

#include "log_analysis/common/log_common.h"
#include "log_analysis/common/error_code.h"
#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/encoding/base64.h"
#include "base/strings/string_split.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"
#include "base/mapreduce/mapreduce.h"
#include "base/random/pseudo_random.h"
#include "base/hash_function/city.h"
#include "nlp/common/nlp_util.h"
#include "crawler/api/base.h"

// mid：机器mid值
// product：产品标识
// combo：产品模块标识
// version：产品版本
// server_timestamp:服务器收到数据的UTC时间戳，从1970年1月1日0时0分0秒开始计算的秒数
// proto_version:协议版本号，目前只有v2
// url相关数据的kv对列表：key_id1|base64(v1);key_id2|base64(v2);key_id3|base64(v3) ...
// refer_url相关数据的kv对列表：key_id1|base64(v1);key_id2|base64(v2);key_id3|base64(v3) ... 。注：如果上面url的kv列表中已经包含了refer_url的相关信息，该列就是空串

// 04d5e2e88b74f3bdb5d72014c2d8b643        wd      urlproc 2.8.0.1040      1335282659      2       1|qcyWTw==;0|ac35bc1de674bfd79
// 83691dcdc02ce0f;6|AQA=;4|aHR0cDovL3VzZXIucXpvbmUucXEuY29tLzY4MDQzMD9BRFVJTj0zOTg2MDk2OTgmQURTRVNTSU9OPTEzMzUyMTkwOTYmQURUQUc9Q
// 0xJRU5ULlFRLjQ1MDFfRnJpZW5kVGlwLjAmcHRsYW5nPTIwNTIjIWFwcD00;7|6IW+6K6v576O5aWz4piG5qWa5YS/4piGIC0tIOiFvui

enum {
  kIDCurrentURLMD5            = 0,  // We use 16 bytes hex MD5 string
  kIDWebPageStartTime         = 1,  // measured in the number of seconds since the Unix Epoch (January 1 1970 00:00:00 GMT)
  kIDWebPageDuratrion         = 2,  // measured in seconds
  kIDWebPageOpenedType        = 3,  // enum URLType
  kIDCurrentURL               = 4,
  kIDCurrentURLCharset        = 5,
  kIDCurrentURLAttr           = 6,  // enum URLAttribute, the attribute of current url
  kIDCurrentURLTitle          = 7,
  kIDCurrentURLTitleCharset   = 8,
  kIDQ3WScore                 = 9,
  kIDReferURLMD5              = 100,
  kIDReferURL                 = 101,
  kIDReferURLCharset          = 102,
  kIDAnchorMD5                = 200,
  kIDAnchor                   = 201,
  kIDAnchorCharset            = 202,
  kIDSearchKeywordMD5              = 300,  //The md5 of the search keyword
  kIDSearchKeyword                 = 301,  //The keyword
  kIDSearchEngineType              = 302,  //using enum SearchEngineType, uint8
  kIDSearchKeywordCharset          = 303,  //
  kIDSearchResultWebPageURLMD5     = 304,
  kIDSearchResultWebPageURL        = 305,
  kIDSearchResultWebPageURLCharset = 306,
  kIDSearchResultURLMD5            = 307,
  kIDSearchResultURL               = 308,
};

bool decode_time(const std::string& orig_str, std::string* time_stamp) {
  std::string tmp_str;
  if (!base::Base64Decode(orig_str, &tmp_str)) {
    return false;
  }
  CHECK_EQ(tmp_str.size(), 4u);
  int32 time_number;
  // std::reverse(tmp_str.begin(), tmp_str.end());
  memcpy(&time_number, &(tmp_str[0]), tmp_str.size());
  if (!log_analysis::ConvertTimeFromSecToFormat(base::IntToString(time_number),
                                                "%Y%m%d%H%M%S", &tmp_str)) {
    return false;
  }
  *time_stamp = tmp_str;
  return true;
}

bool decode_attr(const std::string& orig_str, std::string* attr) {
  std::string tmp_str;
  if (!base::Base64Decode(orig_str, &tmp_str)) {
    return false;
  }
  CHECK_EQ(tmp_str.size(), 2u);
  int16 number;
  // std::reverse(tmp_str.begin(), tmp_str.end());
  memcpy(&number, &(tmp_str[0]), tmp_str.size());
  *attr = base::IntToString(number);
  return true;
}

bool GetUrlAndRef(const std::string& line,
                  const std::map<int, std::string>& kv_url, const std::map<int, std::string>& kv_ref_url,
                  std::string* url_md5, std::string* url_base64,
                  std::string* ref_url_md5, std::string* ref_url_base64) {
  url_md5->clear();
  url_base64->clear();
  ref_url_md5->clear();
  ref_url_base64->clear();
  auto it = kv_url.begin();

  // 不解析 ref url 的 md5, ref url 只要 base64 字面即可

  // 0-kIDCurrentURLMD5   4-kIDCurrentURL  100-kIDReferURLMD5   101-kIDReferURL
  //
  // 不正常的日志（url列找不到id=4也找不到id=101）
  if (kv_url.find(kIDReferURL) == kv_url.end()
      && kv_url.find(kIDCurrentURL) == kv_url.end()) {
    return false;
  }

  // 不正常的日志（refer列不为空，url列找不到id=4且找不到id=100，这种类型的日志2012/5/17之后就应该没有了）。请做兼容处理。url列的数据中的id=101的数据代表current_url，与id=0对应。refer列的数据永远是refer。
  if (!kv_ref_url.empty()
      && (kv_url.find(kIDCurrentURL) == kv_url.end()
          && kv_url.find(kIDReferURLMD5) == kv_url.end())) {
    it = kv_url.find(kIDReferURL);
    CHECK(it != kv_url.end()) << line;
    *url_base64 = it->second;
    it = kv_url.find(kIDCurrentURLMD5);
    if (it != kv_url.end()) {
      *url_md5 = it->second;
    }
    
    it = kv_ref_url.find(kIDReferURL);
    CHECK(it != kv_ref_url.end()) << line;
    *ref_url_base64 = it->second;
    // it = kv_ref_url.find(kIDReferURLMD5);
    // CHECK(it != kv_ref_url.end()) << line;
    // *ref_url_md5 = it->second;
    return true;
  }

  // 正常的日志（refer列为空）。url列中id=4就是current_url，id=101就是refer_ur
  if (kv_ref_url.empty()) {
    it = kv_url.find(kIDCurrentURL);
    CHECK(it != kv_url.end()) << line;
    *url_base64 = it->second;
    it = kv_url.find(kIDCurrentURLMD5);
    if (it != kv_url.end()) {
      *url_md5 = it->second;
    }

    it = kv_url.find(kIDReferURL);
    if (it != kv_url.end()) {
      *ref_url_base64 = it->second;
      it = kv_url.find(kIDReferURLMD5);
      // CHECK(it != kv_url.end()) << line;
      // *ref_url_md5 = it->second;
    }
    return true;
  }

  if (!kv_ref_url.empty()) {
    it = kv_url.find(kIDCurrentURL);
    CHECK(it != kv_url.end()) << line;
    *url_base64 = it->second;
    it = kv_url.find(kIDCurrentURLMD5);
    if (it != kv_url.end())  {
      *url_md5 = it->second;
    }

    auto it1 = kv_ref_url.find(kIDCurrentURL);
    auto it2 = kv_ref_url.find(kIDReferURL);
    if (it1 != kv_ref_url.end() && it2 == kv_ref_url.end()) {
      // 正常的日志（refer列不为空，且refer列的id字段为0,4等），url列的id=4代表current_url，refer列的id=4代表refer_url
      *ref_url_base64 = it1->second;
      it = kv_ref_url.find(kIDCurrentURLMD5);
      // CHECK(it != kv_ref_url.end()) << line;
      // *ref_url_md5 = it->second;
    } else if (it1 == kv_ref_url.end() && it2 != kv_ref_url.end()) {
      // 正常的日志（refer列不为空，且refer列的字段为100,101等，这种类型的日志2012/5/17之后就应该没有了）
      *ref_url_base64 = it2->second;
      it = kv_ref_url.find(kIDReferURLMD5);
      // CHECK(it != kv_ref_url.end()) << line;
      // *ref_url_md5 = it->second;
    } else if (it1 == kv_ref_url.end() && it2 == kv_ref_url.end()) {
    } else {
      CHECK(false) << line;
    }
    return true;
  }

  CHECK(false) << line;

  return false;
}

void mapper() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  CHECK(base::mapreduce::IsMapApp());

  int32 reducer_count = task->GetReducerNum();
  CHECK_GT(reducer_count, 0);

  std::string key;
  std::string value;
  std::string line;
  std::vector<std::string> flds;
  while (task->NextKeyValue(&key, &value)) {
    // while (std::getline(std::cin, line)) {
    line = key + "\t" + value;
    if (line.empty()) continue;
    flds.clear();
    base::SplitString(line, "\t", &flds);
    if (flds.size() < 8) {
      task->ReportAbnormalData(log_analysis::kMapInputError, line, "");
      continue;
    }

    std::string mid = flds[0];
    if (mid.empty()) {
      task->ReportAbnormalData(log_analysis::kMapInputError, line, "");
      continue;
    }

    std::string url_info = flds[6];
    std::string ref_info = flds[7];

    bool abnormal = false;
    std::map<int, std::string> kv_url;
    if (!url_info.empty()) {
      flds.clear();
      base::SplitString(url_info, ";", &flds);
      std::vector<std::string> sub;
      for (int i = 0; i < (int)flds.size(); ++i) {
        sub.clear();
        base::SplitString(flds[i], "|", &sub);
        if (sub.size() != 2u) {
          LOG(ERROR)  << base::StringPrintf("invalid field, line: [%s], url_info: [%s], field: [%s]",
                                  line.c_str(), url_info.c_str(), flds[i].c_str());
          task->ReportAbnormalData(log_analysis::kMapInputError, line, "");
          abnormal = true;
          break;
        }
        if (sub[1].size() > 1000) {
          // 异常数据,停止读取
          abnormal = true;
          break;
        }
        int key = 0;
        if (!base::StringToInt(sub[0], &key)) {
          task->ReportAbnormalData(log_analysis::kMapInputError, line, "");
          abnormal = true;
          break;
        }
        if (kv_url.find(key) != kv_url.end()) {
          LOG(ERROR) << base::StringPrintf("duplicated key, line: [%s], url_info: [%s], field: [%s]",
                                  line.c_str(), url_info.c_str(), flds[i].c_str());
          task->ReportAbnormalData(log_analysis::kMapInputError, line, "");
          abnormal = true;
          break;
        }
        kv_url[key] = sub[1];
      }
    }
    if (abnormal) continue;

    std::map<int, std::string> kv_ref_url;
    if (!ref_info.empty()) {
      flds.clear();
      base::SplitString(ref_info, ";", &flds);
      std::vector<std::string> sub;
      for (int i = 0; i < (int)flds.size(); ++i) {
        sub.clear();
        base::SplitString(flds[i], "|", &sub);
        if (sub.size() != 2u) {
            LOG(ERROR) << base::StringPrintf("invalid field, line: [%s], ref_info: [%s], field: [%s]",
                                  line.c_str(), ref_info.c_str(), flds[i].c_str());
          task->ReportAbnormalData(log_analysis::kMapInputError, line, "");
          abnormal = true;
          break;
        }
        if (sub[1].size() > 1000) {
          // 异常数据,停止读取
          abnormal = true;
          break;
        }
        int key = 0;
        if (!base::StringToInt(sub[0], &key)) {
          task->ReportAbnormalData(log_analysis::kMapInputError, line, "");
          abnormal = true;
          break;
        }
        if (kv_ref_url.find(key) != kv_ref_url.end()) {
          LOG(ERROR)  << base::StringPrintf("duplicated key, line: [%s], ref_info: [%s], field: [%s]",
                                  line.c_str(), ref_info.c_str(), flds[i].c_str());
          task->ReportAbnormalData(log_analysis::kMapInputError, line, "");
          abnormal = true;
          break;
        }
        kv_ref_url[key] = sub[1];
      }
    }
    if (abnormal) continue;

    // 解析除 url 外的字段
    auto it = kv_url.find(kIDWebPageStartTime);
    if (it == kv_url.end()) {
      task->ReportAbnormalData(log_analysis::kMapInputError, line, "");
      continue;
    }
    std::string time_stamp;
    if (!decode_time(it->second, &time_stamp)) {
      task->ReportAbnormalData(log_analysis::kMapInputError, line, "");
      continue;
    }

    it = kv_url.find(kIDWebPageDuratrion);
    std::string duration = (it != kv_url.end()) ? it->second : "";
    // 该字段为空, 如果非空,需要看一下如何解析
    if (!duration.empty()) {
      if (duration.size() < 16) { //
        CHECK(false) << duration;
      }
    }
    duration = "0";

    it = kv_url.find(kIDWebPageOpenedType);
    std::string enter_type = (it != kv_url.end()) ? it->second : "";
    // 该字段为空, 如果非空,需要看一下如何解析
    CHECK(enter_type.empty()) << enter_type;
    enter_type = "255";

    it = kv_url.find(kIDCurrentURLAttr);
    std::string attr = "1";
    if (it != kv_url.end()) { 
      if (!decode_attr(it->second, &attr)) {
        attr = "1";
      }
    }

    if (kv_url.find(kIDAnchor) != kv_url.end()
        || kv_url.find(kIDSearchKeyword) != kv_url.end()
        || kv_url.find(kIDSearchResultWebPageURL) != kv_url.end()
        || kv_url.find(kIDSearchResultURL) != kv_url.end()
        ) {
      CHECK(false) << line;
    }
    if (kv_ref_url.find(kIDAnchor) != kv_ref_url.end()
        || kv_ref_url.find(kIDSearchKeyword) != kv_ref_url.end()
        || kv_ref_url.find(kIDSearchResultWebPageURL) != kv_ref_url.end()
        || kv_ref_url.find(kIDSearchResultURL) != kv_ref_url.end()
        ) {
      CHECK(false) << line;
    }

    std::string url;
    std::string url_md5;
    std::string url_base64;
    std::string ref_url;
    std::string ref_url_md5;
    std::string ref_url_base64;
    if (!GetUrlAndRef(line, kv_url, kv_ref_url, &url_md5, &url_base64, &ref_url_md5, &ref_url_base64)) {
      task->ReportAbnormalData(log_analysis::kMapInputError, line, "");
      continue;
    }

    // 解析 url
    if (url_base64.empty()) {
      task->ReportAbnormalData(log_analysis::kMapInputError, line, "");
      continue;
    }
    if (!log_analysis::Base64ToClickUrl(url_base64, &url)) {
      task->ReportAbnormalData(log_analysis::kInvalidURL, url_base64, url_md5);
      continue;
    }
    if (url.empty()) {
      task->ReportAbnormalData(log_analysis::kInvalidURL, url_base64, url_md5);
      continue;
    }

    // 解析 ref_url
    if (!ref_url_base64.empty()) {
      if (!log_analysis::Base64ToClickUrl(ref_url_base64, &ref_url)) {
        ref_url.clear();
      }
    }

    // 输出 md5 -> url 映射表
    if (!url_base64.empty() && !url_md5.empty()) {
      task->Output(url_md5, "md5\t" + url_base64 + "\t" + time_stamp);
    }
    // if (!ref_url_base64.empty() && !ref_url_md5.empty()) {
    //   task->Output(ref_url_md5, "md5\t" + ref_url_base64 + "\t" + time_stamp);
    // }

    // 解析 html title
    std::string html_title;
    it = kv_url.find(kIDCurrentURLTitle);
    if (it != kv_url.end()) {
      std::string str;
      if (log_analysis::Base64ToUTF8(it->second, &str)) {
        html_title = nlp::util::NormalizeLine(str);
      }
      // 输出网页 title
      if (!html_title.empty()) {
        task->Output(url, "title\t" + html_title);
      }
      // if (!html_title.empty())
      // std::cout << html_title << std::endl;
    }

    // 输出 pv log
    // [ mid, time_stamp, url, referer_url, attr, enter_type, duration]
    int32 reduce_id = base::CityHash64(mid.c_str(), mid.size()) % reducer_count;
    std::string compound_key = mid + "\t" + time_stamp;  // mid + time_stamp as compound key
    task->OutputWithReducerID(compound_key,
                              url + "\t"
                              + ref_url + "\t"
                              + attr + "\t"
                              + enter_type + "\t"
                              + duration, reduce_id);

    // std::cout << compound_key + "\t" + url + "\t"
    //                           + ref_url + "\t"
    //                           + attr + "\t"
    //                           + enter_type + "\t"
    //                           + duration << std::endl;
  }
  }

  void reducer() {
    base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
    std::string key;
    std::vector<std::string> flds;
    while (task->NextKey(&key)) {
      std::string value;
      std::map<std::string, std::string> md5_dedup;
      while (task->NextValue(&value)) {
        flds.clear();
        base::SplitString(value, "\t", &flds);
        if (flds.size() == 3u && flds.front() == "md5") {
          // hack 成 md5->url 原始日志的格式
          const std::string& md5 = key;
          const std::string& base64 = flds[1];
          const std::string& time = flds[2];
          std::string new_key = md5 + "\t" + base64;
          if (md5_dedup.find(new_key) != md5_dedup.end()) {
            if (time > md5_dedup[new_key]) {
              md5_dedup[new_key] = time;
            }
          } else {
            md5_dedup[new_key] = time;
          }
        } else if (flds.size() == 2u && flds.front() == "title") {
          const std::string& url = key;
          const std::string& title = flds[1];
          task->OutputWithFilePrefix(url, title, "title");
        } else if (flds.size() == 5u) {
          task->OutputWithFilePrefix(key, value, "pvlog");
        } else {
          CHECK(false) << key << "\t" << value;
        }
      }
      for (auto it = md5_dedup.begin(); it != md5_dedup.end(); ++it) {
        flds.clear();
        base::SplitString(it->first, "\t", &flds);
        CHECK_EQ(flds.size(), 2u);
        const std::string& formatted_time = it->second;
        int64 sec;
        if (!log_analysis::ConvertTimeFromFormatToSec(formatted_time,"%Y%m%d%H%M%S", &sec)) {
          continue;
        }
        task->OutputWithFilePrefix("a",
                                   "b\tc\td\t"
                                   + flds[0] + "\te\t"
                                   + flds[1] + "\t"
                                   + base::Int64ToString(sec), "md5");
      }
    }
  }

  int main(int argc, char *argv[]) {
    base::mapreduce::InitMapReduce(&argc, &argv, "pv v2");
    // base::InitApp(&argc, &argv, "pv v2");
    // mapper();
    if (base::mapreduce::IsMapApp()) {
      mapper();
    } else if (base::mapreduce::IsReduceApp()) {
      reducer();
    } else {
      CHECK(false);
    }
  }
