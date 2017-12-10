#pragma once

#include <string>
#include <map>
#include "base/container/dense_hash_map.h"

namespace crawler {
class Evaluation {
 public:
  void Init(const std::string& urlfile,
            const std::string& websitefile);
  void Evaluate(const std::string& report_stats);
  void SaveWebsiteReport(const std::string& file);
  void SaveUrlReport(const std::string& file);
  float success_rate() const;
 private:
  void LoadTopnURLsOrDie(const std::string& file);
  void LoadTopnSitesOrDie(const std::string& file);
  void CheckStatusFile(const std::string& file);

  ::base::dense_hash_map<std::string, int> topn_urls_;
  ::base::dense_hash_map<std::string, std::pair<int, int>> topn_websites_;
  int topn_url_cnt_;
  int success_;
};
}  // namespace crawler
