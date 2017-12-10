#include "crawler/realtime_crawler/cdoc_util.h"

#include <vector>
#include <fstream>
#include "base/common/base.h"
#include "base/strings/utf_codepage_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "crawler/realtime_crawler/extra.h"
#include "crawler/realtime_crawler/crawl_queue.pb.h"
#include "feature/util/nlp_util.h"
#include "feature/web/page/util/html_info_util.h"
#include "base/hash_function/url.h"
#include "base/hash_function/city.h"
#include "base/strings/string_split.h"
#include "sandbox/yuanjinhui/page_temporal/page_temporal.h"
#include "sandbox/jiyanbing/page_miner/news_page_miner.h"

namespace crawler2 {
namespace crawl_queue {

DEFINE_string(news_site_file, "crawler/realtime_crawler/data/newssite.dict", "news site file");


NewsSiteEval* NewsSiteEval::news_site_eval_ = NULL;

NewsSiteEval::NewsSiteEval() {
  loadNewsSite();
}

bool NewsSiteEval::loadNewsSite() {
  std::string input_buffer;
  news_site_data_.clear();
  std::ifstream fin(FLAGS_news_site_file);
  if (!fin) {
    LOG(ERROR) << "open " << FLAGS_news_site_file << "failed";
    return false;
  }
  std::vector<std::string> fields;
  while (std::getline(fin, input_buffer)) {
    fields.clear();
    base::SplitString(input_buffer, "\t", &fields);
    if (fields.size() < 2UL || fields[1].size() <= 0u) {
      LOG(WARNING) << "line: " << input_buffer << ". format error.";
      continue;
    }
    std::string norm_url;
    norm_url = fields[1].substr(7);
    if (norm_url[norm_url.length() - 1] == '/') {
      norm_url.erase(norm_url.length()-1);
    }
    uint64 url_sign = base::CalcUrlSign(norm_url.c_str(), norm_url.length());
    int site_level = -1;
    if (!base::StringToInt(fields[0], &site_level)) {
      continue;
    }
    news_site_data_[url_sign] = site_level;
    VLOG(3) << "site:" << norm_url << "sign:" << url_sign << " level:" << site_level;
  }
  return true;
}

int NewsSiteEval::getNewsScore(uint64 site_sign) {
  NewsSiteType::iterator it_site
      = news_site_data_.find(site_sign);
  if (it_site == news_site_data_.end()) {
    return 0;
  } else {
    return it_site->second + 1;
  }
}

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

bool CDocGenerator::Gen(::feature::HTMLInfo* info, const JobDetail& detail,
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

  // (pengdan): Modified 20130510, feature::web::WebpageFeatureExtractor 对象接口需要
  // 添加一个参数 rich_snippet_mining
  feature::rich_snippet::RichSnippetMining rich_snippet_mining(&extractor_env_);

  feature::web::WebpageFeatureExtractor extractor(&doc, &extractor_env_,
                                                  *res, &rich_snippet_mining);
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
  //if (!detail.anchor().empty()) {
  //  std::string anchor = detail.anchor();
  //  feature::util::NormalizeLineInPlaceS(&anchor);
  //  std::vector<std::string> terms;
  //  crawler2::AnchorInfo* anchor_info = info->add_anchor();
  //  anchor_info->set_freq(1);
  //  CHECK(extractor_env_.segmenter.SegmentS(::base::Slice(anchor), &terms));
  //  for (auto iter = terms.begin(); iter != terms.end(); ++iter) {
  //    anchor_info->add_term(*iter);
  //  }
  //}

  //if (!detail.query().empty()) {
  //  std::string query = detail.query();
  //  feature::util::NormalizeLineInPlaceS(&query);
  //  std::vector<std::string> terms;
  //  crawler2::QueryInfo* query_info = info->add_query();
  //  CHECK(extractor_env_.segmenter.SegmentS(query, &terms));
  //  query_info->set_freq(1);
  //  for (auto iter = terms.begin(); iter != terms.end(); ++iter) {
  //    query_info->add_term(*iter);
  //  }
  //}

  // Add page TemporalComp
  bool temporal_suc = false;
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
    temporal_suc = true;
  }
  time_t cur_time = time(NULL);
  info->mutable_temporal_features()->set_crawler_time(cur_time);

  // 识别页面的新闻属性
  GURL gurl(info->effective_url());
  const std::string &host = gurl.HostNoBrackets();
  uint64 site_sign = base::CalcUrlSign(host.c_str(), host.length());
  NewsSiteEval *news_site_eval = NewsSiteEval::Instance();
  int news_site_score = news_site_eval->getNewsScore(site_sign);
  int mod_news_site_score = news_site_score;
  feature::web::page_miner::PageInfo page_info;
  feature::web::page_miner::PageMiner page_miner;
  if (page_miner.GetPageInfo(&doc, &page_info)) {
    if (page_info.page_type == feature::web::page_miner::PageType::isNewsPage) {
      if (news_site_score == 1 || news_site_score == 2) {
        mod_news_site_score = 1;
      } else if (news_site_score == 3) {
        mod_news_site_score = 2;
      } else if (news_site_score > 3) {
        mod_news_site_score = 3;
      } else {
        mod_news_site_score = 4;
      }
    } else if (page_info.page_type == feature::web::page_miner::PageType::isForumPage) {
      if (news_site_score >= 1 && news_site_score <= 3) {
        mod_news_site_score = 3;
      } else if (news_site_score > 3) {
        mod_news_site_score = 5;
      } else {
        mod_news_site_score = 7;
      }
    } else if (page_info.page_type == feature::web::page_miner::PageType::isHubPage) {
      mod_news_site_score = 7;
    } else {  // 未识别出页面类型
      if (news_site_score >= 1 && news_site_score <= 3) {
        mod_news_site_score = 6;
      } else if (news_site_score > 3) {
        mod_news_site_score = 7;
      } else {
        mod_news_site_score = 0;
      }
    }
  }
  if (!temporal_suc && mod_news_site_score >= 6) {
    mod_news_site_score = 0;
  }
  LOG(INFO) << "news_score " << news_site_score << " " << mod_news_site_score
            << " [" << host << "] " << info->effective_url();
  info->set_news_score(mod_news_site_score);



  // Add CrawlReason
  if (res->brief().has_crawl_reason()) {
    info->mutable_crawl_reason()->CopyFrom(res->brief().crawl_reason());
  }

  return true;
}
}  // namespace crawl_queue
}  // namespace crawler2
