#include "crawler/evaluation/evaluation.h"

#include <string>
#include <fstream>
#include "base/common/basic_types.h"
#include "base/common/base.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/common/logging.h"
#include "base/common/gflags.h"
#include "base/container/dense_hash_map.h"
#include "web/url/gurl.h"
#include "crawler/exchange/report_status.h"

namespace crawler {

void Evaluation::Init(const std::string& urlfile, const std::string& websitefile) {
  LoadTopnURLsOrDie(urlfile);
  LoadTopnURLsOrDie(websitefile);
}

void Evaluation::LoadTopnURLsOrDie(const std::string& file) {
  int cnt = 0;
  std::string line;
  std::ifstream is(file.c_str());
  CHECK(is.good());
  while (std::getline(is, line) && cnt < topn_url_cnt_) {
    std::string output;
    ::base::TrimString(line, "\t", &output);
    topn_urls_.insert(std::make_pair(line, 0));
    cnt++;
  }

  CHECK(is.eof());
}

void Evaluation::LoadTopnSitesOrDie(const std::string& file) {
  std::ifstream is(file.c_str());
  CHECK(is.good());
  std::string line;
  while (std::getline(is, line)) {
    std::string output;
    ::base::TrimString(line, "\t", &output);
    topn_websites_.insert(std::make_pair(line, std::make_pair(0, 0)));
  }

  CHECK(is.eof());
}

void Evaluation::CheckStatusFile(const std::string& file) {
  // 逐个比较 status 文件当中 url 的状态
  std::string line;
  std::ifstream is(file.c_str());
  CHECK(is.good());
  while (std::getline(is, line)) {
    std::string output;
    ::base::TrimString(line, "\t", &output);
    ReportStatus status;
    CHECK(crawler::LoadReportStatusFromString(&status, output));
    if (status.success && topn_urls_.find(status.url) != topn_urls_.end()) {
      topn_urls_[status.url] = 1;
    }

    GURL url(status.url);
    if (!url.is_valid()) {
      continue;
    }

    if (topn_websites_.find(url.host()) != topn_websites_.end()) {
      if (status.success) {
        topn_websites_[url.host()].first += 1;
      } else {
        topn_websites_[url.host()].second += 1;
      }
    }
  }

  CHECK(is.eof());

  for (auto iter = topn_urls_.begin(); iter != topn_urls_.end(); ++iter) {
    if (iter->second > 0) {
      success_++;
    }
  }
}

void Evaluation::Evaluate(const std::string& report_file) {
  CheckStatusFile(report_file);
}

float Evaluation::success_rate() const {
  CHECK(topn_url_cnt_ > 0);
  return success_ / topn_url_cnt_;
}

void Evaluation::SaveWebsiteReport(const std::string& file) {
  std::fstream fs(file.c_str(), std::fstream::out);
  CHECK(fs.good());
  for (auto iter = topn_websites_.begin(); iter != topn_websites_.end(); ++iter) {
    float success_rate = (iter->second.first + iter->second.second > 0) ? (iter->second.first/(float)(iter->second.first + iter->second.second)) : 0.0f;
    fs << iter->first << "\t" << iter->second.first
       << "\t" << iter->second.second
       << success_rate << std::endl;
  }
}

void Evaluation::SaveUrlReport(const std::string& file) {
  std::fstream fs(file.c_str(), std::fstream::out);
  CHECK(fs.good());
  for (auto iter = topn_urls_.begin(); iter != topn_urls_.end(); ++iter) {
    if (iter->second > 0) {
      fs << iter->first << "\t" << 1 << std::endl;
    } else {
      fs << iter->first << "\t" << 0 << std::endl;
    }
  }
}
}  // namespace crawer
