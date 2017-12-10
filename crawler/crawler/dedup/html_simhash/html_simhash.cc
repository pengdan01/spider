#include "crawler/dedup/html_simhash/html_simhash.h"

#include <stack>
#include <vector>
#include <string>
#include <fstream>
#include <unordered_map>

#include "base/common/logging.h"
#include "base/common/gflags.h"
#include "base/strings/string_split.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"
#include "nlp/common/nlp_util.h"
#include "nlp/idf/idf.h"
#include "nlp/segment/segmenter.h"
#include "base/hash_function/city.h"
#include "extend/simhash/simhash.h"
#include "crawler/dedup/dom_extractor/content_collector.h"
#include "web/url/url.h"
#include "web/url/gurl.h"

DEFINE_string(bad_hash, "crawler/dedup/html_simhash/data/bad_hash", "the hash sigature for bad pages");

namespace simhash {
bool HtmlSimHash::Init(const HashModes &mode) {
  hash_mode_ = mode;
  std::ifstream bad_fin(FLAGS_bad_hash);
  CHECK(bad_fin.good()) << "Open bad hash file: " << FLAGS_bad_hash
                        << " fail, use flag --bad_hash specify it";
  std::string line;
  std::vector<std::string> fields;
  while (getline(bad_fin, line)) {
    fields.clear();
    base::SplitString(line, "\t\r\n ", &fields);
    CHECK_EQ(fields.size(), 4u);
    // std::string key = base::StringPrintf("%lu_%lu_%lu", fields[0], fields[1], fields[2]);
    std::string key = fields[0] + "_" + fields[1] + "_" + fields[2];
    bad_page_.insert(std::pair<std::string, std::string>(key, fields[3]));
  }
  CHECK(bad_fin.eof());
  bad_fin.close();
  return true;
}
bool HtmlSimHash::VerifyBadPage(const std::string &page_utf8,
                                    const std::string &url,
                                    PageSignature *signature,
                                    bool *is_bad) {
  *is_bad = false;
  if (hash_mode_ != kHashAll) {
    LOG(ERROR) << "To call VerifyBadPage, hash_mode_ must be kHashAll";
    return false;
  }
  if (bad_page_.size() == 0u) {
    LOG(ERROR) << "To call VerifyBadPage, please provide bad_hash file";
    return false;
  }
  if (!CalculatePageHash(page_utf8, url, signature)) {
    LOG(ERROR) << "CalculatePageHash failed";
    return false;
  }
  if (!VerifyBadSignature(*signature, url, is_bad)) {
    LOG(ERROR) << "VerifyBadSignature failed";
    return false;
  }
  return true;
}
bool HtmlSimHash::VerifyBadSignature(const PageSignature &signature, const std::string &url, bool *is_bad) {
  std::string key = base::StringPrintf("%lu_%lu_%lu",
                                       signature.domain_hash_,
                                       signature.title_hash_,
                                       signature.content_hash_);
  if (bad_page_.find(key) == bad_page_.end()) {
    *is_bad = false;
  } else {
    if (url.length() > bad_page_.find(key)->second.length()) {
      *is_bad = true;
    } else {
      *is_bad = false;
    }
  }
  return true;
}
bool HtmlSimHash::CalculatePageHash(const std::string &page_utf8,
                                    const std::string &url,
                                    PageSignature *signature) {
  CHECK_NOTNULL(signature);
  if (hash_mode_ & kHashDomain) {
    if (!CalcDomainHash(url, &(signature->domain_hash_))) {
    }
  } else {
    // 不需要计算 domain 签名
    signature->domain_hash_ = 0u;
  }

  if (!content_collector_.ExtractMainContent(page_utf8, url, &title_, &content_)) {
    LOG(ERROR) << "ExtractMainContent failed:\t" << url;
    // 返回 false 时, 可能 title_ 和 content_ 都是空串, 也可能前者非空, 后者为空,
    // 不管什么情况, 都继续计算 simhash, 空串计算到的 simhash 是 0
    // return false;
  }
  DLOG(INFO) << "head title:\t" << title_;
  DLOG(INFO) << "main content:\t" << content_;

  if (hash_mode_ & kHashHeadTitle) {
    // 计算 head title 签名
    nlp::util::NormalizeLineInPlaceS(&title_);
    if (!title_.empty()) {
      signature->title_hash_ = base::CityHash64(title_.c_str(), title_.length());
    } else {
      signature->title_hash_ = 0u;
    }
  } else {
    signature->title_hash_ = 0u;
  }

  if (hash_mode_ & kHashMainContent) {
    // 计算正文签名
    nlp::util::NormalizeLineInPlaceS(&content_);
    if (!content_.empty()) {
      std::vector<std::pair<std::string, int64> > term_count;
      std::vector<std::pair<uint64, double> > token_hash_pairs;
      if (!ExtractContentTermCount(content_, &term_count)) {
        LOG(ERROR) << "Fail in ExtractContentTermCount:\t" << url;
        signature->content_hash_ = 0u;
      } else if (!ConstructTokenHashPairs(term_count, &token_hash_pairs)) {
        LOG(ERROR) << "Fail in ConstructTokenHashPairs:\t" << url;
        signature->content_hash_ = 0u;
      } else {
        signature->content_hash_ = simhash::CharikarSimHash::ComputeDocumentSimHash(token_hash_pairs);
      }
    } else {
      signature->content_hash_ = 0u;
    }
  } else {
    signature->content_hash_ = 0u;
  }
  return true;
}
bool HtmlSimHash::CalcDomainHash(const std::string &url, uint64 *hash) {
  // 计算 domain 签名
  if (!url.empty()) {
    // url 非空
    GURL gurl(url);
    std::string host = gurl.host();
    std::string domain;
    std::string tld;
    std::string sub_domain;
    if (!gurl.HostIsIPAddress()) {
      if (!web::ParseHost(host, &tld, &domain, &sub_domain)) {
        // 解析域名失败
        *hash = 0u;
      } else {
        // 解析域名成功
        *hash = base::CityHash64(domain.c_str(), domain.length());
      }
    } else {
      // host 为 IP 地址
      *hash = base::CityHash64(host.c_str(), host.length());
    }
  } else {
    // url 为空
    *hash = 0u;
  }
  return true;
}

bool HtmlSimHash::CalcTitleHash(const std::string &head_utf8, uint64 *hash) {
  return true;
}
bool HtmlSimHash::CalculateSimHash(const std::string &page_utf8, const std::string &url, uint64 *hash) {
  CHECK_NOTNULL(hash);
  std::vector<std::pair<std::string, int64> > term_count;
  std::vector<std::pair<uint64, double> > token_hash_pairs;
  if (!ExtractHtmlTitleContentTermCount(page_utf8, url, &term_count)) {
    LOG(ERROR) << "Fail in ExtractHtmlTitleContentTermCount:\t" << url;
    return false;
  }
  if (!ConstructTokenHashPairs(term_count, &token_hash_pairs)) {
    LOG(ERROR) << "Fail in ConstructTokenHashPairs:\t" << url;
    return false;
  }
  *hash = simhash::CharikarSimHash::ComputeDocumentSimHash(token_hash_pairs);
  return true;
}
bool HtmlSimHash::CalculateSimHashFromTermCount(const std::vector<std::pair<std::string, int64> > &term_count,
                                                const std::string &url,
                                                uint64 *hash) {
  CHECK_NOTNULL(hash);
  std::vector<std::pair<uint64, double> > token_hash_pairs;
  if (!ConstructTokenHashPairs(term_count, &token_hash_pairs)) {
    LOG(ERROR) << "Fail in ConstructTokenHashPairs:\t" << url;
    return false;
  }
  *hash = simhash::CharikarSimHash::ComputeDocumentSimHash(token_hash_pairs);
  return true;
}
bool HtmlSimHash::AcceptLanguage(const std::string &term) {
  if (term == "") {
    return false;
  }
  if (term.at(0) == '\t' || term.at(term.length() - 1) == '\t'
      || term.at(0) == ' ' || term.at(term.length() - 1) == ' ' ) {
    return false;
  }
  base::UTF8CharIterator iter(&term);
  for (; !iter.end(); iter.Advance()) {
    int codepoint = iter.get();
    if ((codepoint < 0x4E00 || codepoint > 0x9FA5) &&  // All Chinese characters.
        (codepoint < 0x0041 || codepoint > 0x005A) &&  // All uppercase English characters.
        (codepoint < 0x0061 || codepoint > 0x007A) &&  // All lowercase English characters.
        codepoint != 12398) {                          // The 'の' in such as 优の良品
      return false;
    }
  }
  return true;
}
bool HtmlSimHash::ExtractHtmlTitleContentTermCount(const std::string &page_utf8, const std::string &url,
                                      std::vector<std::pair<std::string, int64> > *term_count) {
  term_count->clear();

  std::string title;
  std::string content;
  nlp::term::TermContainer terms;
  std::unordered_map<std::string, int64> term_count_map;
  if (!content_collector_.ExtractMainContent(page_utf8, url, &title, &content)) {
    LOG(ERROR) << "ExtractMainContent failed:\t" << url;
    return false;
  }
  DLOG(INFO) << title;
  DLOG(INFO) << content;
  nlp::util::NormalizeLineInPlaceS(&title);
  if (!title.empty()) {
    /*
    CHECK(segmenter_.SegmentT(title, &terms));
    for (int id = 0; id < terms.basic_term_num(); ++id) {
      std::string current_term = terms.basic_term_slice(title, id).as_string();
      // 过滤掉一些无意义字符, 只保留中英文字符
      if (AcceptLanguage(current_term)) {
        term_count_map[current_term] += 5;  // 标题中出现的 term 权重加大
      }
    }
    */
    term_count_map[title] += 10;
  }
  nlp::util::NormalizeLineInPlaceS(&content);
  if (!content.empty()) {
    terms.renew();
    CHECK(segmenter_.SegmentT(content, &terms)) << "Fail to segment: " << content;
    for (int32 id = 0; id < terms.basic_term_num(); ++id) {
      std::string current_term = terms.basic_term_slice(content, id).as_string();
      // 过滤掉一些无意义字符, 只保留中英文字符
      if (AcceptLanguage(current_term)) {
        term_count_map[current_term]++;
      }
    }
  }
  // 把 term count 转入 vector 结构
  std::unordered_map<std::string, int64>::const_iterator it;
  for (it = term_count_map.begin(); it != term_count_map.end(); ++it) {
    term_count->push_back(std::make_pair(it->first, it->second));
  }
  return true;
}
bool HtmlSimHash::ExtractContentTermCount(const std::string &content,
                                              std::vector<std::pair<std::string, int64> > *term_count) {
  term_count->clear();
  nlp::term::TermContainer terms;
  std::unordered_map<std::string, int64> term_count_map;
  if (!content.empty()) {
    terms.renew();
    CHECK(segmenter_.SegmentT(content, &terms)) << "Fail to segment: " << content;
    for (int32 id = 0; id < terms.basic_term_num(); ++id) {
      std::string current_term = terms.basic_term_slice(content, id).as_string();
      // 过滤掉一些无意义字符, 只保留中英文字符
      if (AcceptLanguage(current_term)) {
        term_count_map[current_term]++;
      }
    }
  }
  // 把 term count 转入 vector 结构
  std::unordered_map<std::string, int64>::const_iterator it;
  for (it = term_count_map.begin(); it != term_count_map.end(); ++it) {
    term_count->push_back(std::make_pair(it->first, it->second));
  }
  return true;
}
bool HtmlSimHash::ConstructTokenHashPairs(const std::vector<std::pair<std::string, int64> > &term_count,
                             std::vector<std::pair<uint64, double> > *token_hash_pairs) {
  int64 doc_size = 0;
  int64 term_num =0;
  token_hash_pairs->clear();
  std::vector<std::pair<std::string, int64> >::const_iterator term_it;
  for (term_it = term_count.begin(); term_it != term_count.end(); ++term_it) {
    doc_size += term_it->second;
    term_num++;
  }
  int64 reduce_doc_size = 0;
  int64 terms;
  for (term_it = term_count.begin(); term_it != term_count.end(); ++term_it) {
    uint64 hash = base::CityHash64(term_it->first.c_str(), term_it->first.length());
    double idf = dicts_.get_idf(term_it->first, nlp::idf::IdfDicts::kPage);
    // 如果存在同一个 term 出现很多次, 该 term 将主导 simhash 取值, 此处假设高频 term 是噪声
    if (term_it->second * 1.0 / doc_size > 0.1) {
      terms = 1;
    } else {
      terms = term_it->second;
    }
    token_hash_pairs->push_back(std::make_pair(hash, terms * idf));
    reduce_doc_size += terms;
  }
  std::vector<std::pair<uint64, double> >::iterator it;
  for (it = token_hash_pairs->begin(); it != token_hash_pairs->end(); ++it) {
    it->second /= reduce_doc_size;
  }
  return true;
}
}  // namespace simhash
