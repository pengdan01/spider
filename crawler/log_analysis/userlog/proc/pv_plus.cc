#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <map>
#include <set>

#include "log_analysis/userlog/proc/common.h"
#include "log_analysis/common/log_common.h"
#include "log_analysis/common/error_code.h"

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/encoding/base64.h"
#include "base/encoding/line_escape.h"
#include "base/strings/string_split.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"
#include "base/mapreduce/mapreduce.h"
#include "base/random/pseudo_random.h"
#include "base/hash_function/city.h"
#include "nlp/common/nlp_util.h"

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

  // 不正常的日志（refer列不为空，url列找不到id=4且找不到id=100，这种类型的日志2012/5/17之后就应该没有了）。
  // 请做兼容处理。url列的数据中的id=101的数据代表current_url，与id=0对应。refer列的数据永远是refer。
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
    line = key + "\t" + value;
    if (line.empty()) continue;
    flds.clear();
    base::SplitString(line, "\t", &flds);
    if (flds.size() < 8) {
      continue;
    }

    std::string mid = flds[0];
    if (mid.empty()) {
      task->ReportAbnormalData(log_analysis::kMapInputError, line, "");
      continue;
    }

    std::string server_time = flds[4];
    if (!log_analysis::ConvertTimeFromSecToFormat(flds[4], "%Y%m%d%H%M%S", &server_time)) {
      task->ReportAbnormalData(log_analysis::kMapInputError, line, "");
      continue;
    }

    std::string url_info = flds[6];
    std::string ref_info = flds[7];

    std::string client_ip;
    if (flds.size() >= 14) {
      client_ip = flds[13];
    }

    bool abnormal = false;
    std::map<int, std::string> kv_url;
    if (!url_info.empty()) {
      flds.clear();
      // ; 分割每个 kv 对子
      base::SplitString(url_info, ";", &flds);
      std::vector<std::string> sub;
      for (int i = 0; i < (int)flds.size(); ++i) {
        if (flds[i].empty()) continue;
        sub.clear();
        // kv 对子内部用 | 分割
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
        if (kv_url.find(key) != kv_url.end()
            && kv_url[key] != sub[1]) {
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
        if (flds[i].empty()) continue;
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
        if (kv_ref_url.find(key) != kv_ref_url.end()
            && kv_ref_url[key] != sub[1]) {
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
      task->ReportAbnormalData(log_analysis::kInvalidURL, line, "");
      continue;
    }

    // 解析 url
    if (url_base64.empty()) {
      task->ReportAbnormalData(log_analysis::kInvalidURL, line, "");
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

    // 解析 html title
    std::string html_title;
    it = kv_url.find(kIDCurrentURLTitle);
    if (it != kv_url.end()) {
      std::string str;
      if (log_analysis::Base64ToUTF8(it->second, &str)) {
        html_title = base::LineEscapeString(str);
      }
      // 输出网页 title
      if (!html_title.empty()) {
        task->Output(url, "title\t" + html_title);
      }
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
                              + duration + "\t"
                              + server_time + "\t"
                              + client_ip, reduce_id);
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
      if (flds.size() == 2u && flds.front() == "title") {
        const std::string& url = key;
        const std::string& title = flds[1];
        task->OutputWithFilePrefix(url, title, "title_pv");
      } else if (flds.size() == 7u) {
        task->OutputWithFilePrefix(key, value, "pvlog");
      } else {
        CHECK(false) << key << "\t" << value;
      }
    }
  }
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "pv v2");
  if (base::mapreduce::IsMapApp()) {
    mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    reducer();
  } else {
    CHECK(false);
  }
}
