#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>
#include "feature/web/extractor/api/extractor.h"
#include "feature/site/site_feature_extractor.h"
#include "feature/web/page/html_features.pb.h"
#include "crawler/realtime_crawler/crawl_queue.pb.h"

namespace crawler2 {
namespace crawl_queue {

typedef std::unordered_map<uint64_t, int> NewsSiteType;


class NewsSiteEval {
 public:
  static NewsSiteEval *Instance() {
    if (news_site_eval_ == NULL) {
      news_site_eval_ = new NewsSiteEval;
    }
    return news_site_eval_;
  }
  int getNewsScore(uint64 site_sign);
 private:
  NewsSiteEval();
  bool loadNewsSite();

 private:
  NewsSiteType news_site_data_;
  static NewsSiteEval* news_site_eval_;
};


class CDocGenerator {
 public:
  bool Init(const std::string& error_titles);
  bool Gen(::feature::HTMLInfo* info, const JobDetail& detail,
           Resource* res, std::string* title);
 private:
  bool ShouldBeDropped(const std::string &str) const {
    return wrong_title_.find(str) != wrong_title_.end();
  }

  feature::web::ExtractorEnv extractor_env_;
  feature::site::SiteFeatureExtractor site_extractor_;
  std::unordered_set<std::string> wrong_title_;
};
}  // namesapce crawl_queue
}  // namespace crawler2
