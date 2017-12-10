#include "crawler2/general_crawler/cdoc_util.h"

#include <vector>
#include "base/common/base.h"
#include "base/strings/utf_codepage_conversions.h"
#include "crawler/realtime_crawler/extra.h"
#include "crawler/realtime_crawler/crawl_queue.pb.h"
#include "feature/util/nlp_util.h"
#include "feature/web/page/util/html_info_util.h"
#include "sandbox/yuanjinhui/page_temporal/page_temporal.h"

namespace crawler3 {
bool CDocGenerator::Init(const std::string& path) {
  std::vector<std::string> lines;
  if (!base::file_util::ReadFileToLines(base::FilePath(path), &lines)) {
    LOG(ERROR) << "Failed to Load ErrorTitle files: " << path;
    return false;
  }

  for (auto iter = lines.begin(); iter != lines.end(); ++iter) {
    if (iter->empty()) {
      continue;
    }

    wrong_title_.insert(*iter);
  }

  return true;
}

bool CDocGenerator::Gen(::feature::HTMLInfo* info, const crawler2::crawl_queue::JobDetail& detail,
                        crawler2::Resource* res, std::string* title) {
  CHECK_NOTNULL(title);
  CHECK_NOTNULL(res);
  if (!res->content().has_utf8_content()) {
    std::string content_utf8;
    const char *codepage = base::HTMLToUTF8(
        res->content().raw_content(),
        res->brief().response_header(), &content_utf8);
    res->mutable_content()->set_utf8_content(content_utf8);
    if (codepage) {
      res->mutable_brief()->set_original_encoding(codepage);
    }
  }

  ::web::HTMLDoc doc(res->content().utf8_content().c_str(),
                     res->brief().url().c_str());
  ::web::HTMLParser parser;
  parser.Init();
  parser.Parse(&doc, ::web::HTMLParser::kParseAll);

  feature::web::WebpageFeatureExtractor extractor(&doc, &extractor_env_,
                                                  *res);
  extractor.Extract(info, true);
  site_extractor_.Extract(info->url(), info);

  if (::feature::util::GetTitleFromHTMLInfo(*info, title)) {
    if (ShouldBeDropped(*title)) {
      LOG(INFO) << "drop URL[" << info->url()
                << "] because of its title[" << title << "] in list.";
      return false;
    }
  }

  res->mutable_content()->set_html_title(*title);

  // 获取 anchor 信息
  if (!detail.anchor().empty()) {
    std::string anchor = detail.anchor();
    feature::util::NormalizeLineInPlaceS(&anchor);
    std::vector<std::string> terms;
    crawler2::AnchorInfo* anchor_info = info->add_anchor();
    anchor_info->set_freq(1);
    CHECK(extractor_env_.segmenter.SegmentS(::base::Slice(anchor), &terms));
    for (auto iter = terms.begin(); iter != terms.end(); ++iter) {
      anchor_info->add_term(*iter);
    }
  }

  if (!detail.query().empty()) {
    std::string query = detail.query();
    feature::util::NormalizeLineInPlaceS(&query);
    std::vector<std::string> terms;
    crawler2::QueryInfo* query_info = info->add_query();
    CHECK(extractor_env_.segmenter.SegmentS(query, &terms));
    query_info->set_freq(1);
    for (auto iter = terms.begin(); iter != terms.end(); ++iter) {
      query_info->add_term(*iter);
    }
  }

  // Add page TemporalComp
  feature::web::page_temporal::TemporalInfo temporal_info;
  feature::web::page_temporal::PageTemporal page_temporal;
  if (!page_temporal.GetTemporalInfo(&doc, &temporal_info)) {
    LOG(INFO) << "Failed in GetTemporalInfo() for doc, url: " << info->url();
  } else {
    VLOG(5) << "done in GetTemporalInfo() for doc, url: " << info->url();
    feature::HTMLTemporalFeatures *temporal_feature = info->mutable_temporal_features();
    temporal_feature->set_author_year(temporal_info.year);
    temporal_feature->set_author_month(temporal_info.month);
    temporal_feature->set_author_day(temporal_info.day);
    temporal_feature->set_author_hour(temporal_info.hour);
    temporal_feature->set_author_minute(temporal_info.minute);
  }

  // Add CrawlReason
  if (res->brief().has_crawl_reason()) {
    info->mutable_crawl_reason()->CopyFrom(res->brief().crawl_reason());
  }

  return true;
}
}  // namespace crawler2
