#pragma once

#include <string>
#include <unordered_set>
#include "feature/web/extractor/api/extractor.h"
#include "feature/site/site_feature_extractor.h"
#include "feature/web/page/html_features.pb.h"
#include "crawler/realtime_crawler/crawl_queue.pb.h"

namespace crawler3 {
class CDocGenerator {
 public:
  bool Init(const std::string& error_titles);
  bool Gen(::feature::HTMLInfo* info, const crawler2::crawl_queue::JobDetail& detail,
           crawler2::Resource* res, std::string* title);
 private:
  bool ShouldBeDropped(const std::string &str) const {
    return wrong_title_.find(str) != wrong_title_.end();
  }

  feature::web::ExtractorEnv extractor_env_;
  feature::site::SiteFeatureExtractor site_extractor_;
  std::unordered_set<std::string> wrong_title_;
};
}  // namespace crawler2
