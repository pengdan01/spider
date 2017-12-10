#include <vector>
#include <unordered_map>

#include "log_analysis/common/log_common.h"
#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_codepage_conversions.h"
#include "base/hash_function/city.h"
#include "base/mapreduce/mapreduce.h"
#include "base/common/base.h"
#include "nlp/common/nlp_util.h"
//#include "nlp/snippet/offline/util.h"
#include "crawler/api/base.h"
#include "web/html/utils/url_extractor.h"

int get_pn(const std::string& url) {
  std::vector<std::string> flds;
  base::SplitString(url, "?", &flds);
  CHECK_EQ(flds.size(), 2u);
  std::string query = flds[1];
  base::SplitString(url, "&", &flds);
  for (int i = 0; i < (int)flds.size(); ++i) {
    size_t pos = flds[i].find("pn=");
    if (pos == 0) {
      flds[i].erase(0, 3);
      return base::ParseIntOrDie(flds[i]);
    }
  }
  return 0;
}

void mapper() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();

  std::string key;
  std::string value;
  std::vector<std::string> urls;
  std::vector<std::string> snippets;
  web::utils::AnchorInfoVec anchors;
  // int32 reducer_count = task->GetReducerNum();
  while (task->NextKeyValue(&key, &value)) {
    std::string query, se, charset, page;
    if (!log_analysis::ParseSearchQuery(key, &query, &se, &charset)) {
      LOG(ERROR) << "failed to parse: " << key;
      continue;
    }
    if (NULL == base::HTMLToUTF8(value, "", &page)) {
      LOG(ERROR) << "failed to parse page: " << key;
      continue;
    }
    anchors.clear();
    web::utils::ExtractURLs(page.c_str(), key.c_str(), NULL, NULL, NULL, &anchors);
    for (int i = 0; i < (int)anchors.size(); ++i) {
      const std::string& anchor = anchors[i].first;
      if (anchor.find("www.baidu.com/s") != std::string::npos ||
          anchor.find("cache.baidu.com") != std::string::npos) {
        continue;
      }
        task->Output(query, anchors[i].first);
    }
    // int rank_start = get_pn(key);
    // urls.clear();
    // snippets.clear();
    // snippet::parse_baidu_snippet(page, &urls, &snippets);
    // int32 reduce_id = base::CityHash64(query.c_str(), query.size()) % reducer_count;
    // for (int i= 0; i < (int)urls.size(); ++i) {
    //   std::string comp_key = base::StringPrintf("%s\t%03d", query.c_str(), rank_start + i);
    //   task->OutputWithReducerID(comp_key, urls[i], reduce_id);
    // }
  }
  return;
}

void reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  std::string key;
  while (task->NextKey(&key)) {
    std::string value;
    while (task->NextValue(&value)) {
      task->Output(key, value);
    }
  }
  return;
}

int main(int argc, char *argv[]) {
  // LOG(ERROR) << get_pn("http://www.baidu.com/s?rsv_spt=3&wd=2012%E5%B9%B4%E4%BA%94%E4%B8%80%E4%BC%91%E5%81%87&rsv_bp=0");
  // base::InitApp(&argc, &argv, "query count");
  base::mapreduce::InitMapReduce(&argc, &argv, "query count");
  if (base::mapreduce::IsMapApp()) {
    mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    reducer();
  } else {
    CHECK(false);
  }
}
