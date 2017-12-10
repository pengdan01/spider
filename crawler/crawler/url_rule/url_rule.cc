#include "crawler/url_rule/url_rule.h"

#include "web/url/gurl.h"
#include "web/url_norm/url_norm.h"
#include "base/common/basic_types.h"
#include "base/common/base.h"
#include "base/common/gflags.h"
#include "base/common/logging.h"
#include "log_analysis/common/log_common.h"

namespace crawler {

std::string ReplaceAlias(const std::string& url) {
  web::UrlNorm url_norm;
  GURL gurl(url);
  CHECK(gurl.is_valid());

  if (gurl.host() == "wwv.ranwen.com") {
    std::string new_url = ::base::StringReplace(url, "wwv.ranwen.com", "www.ranwen.com", false);
    return new_url;
  }

  std::string new_url;
  if (!url_norm.NormalizeAlias(url, &new_url)) {
    return url;
  } else {
    return new_url;
  }
}

struct WorthlessURL {
  const char* host;
  int reason;
};

const WorthlessURL kWorthlessPageHost[] = {
  {"warn.se.360.cn", kClientPage,},
  {"so.v.360.cn", kSearchResult,},
  {"search.51job.com", kSearchResult,},
  {"translate.google.com.hk", kToolsPage},
  {"translate.google.com.tw", kToolsPage,},
  {"auction1.paipai.com", kPrivatePage,},
  {"s.taobao.com", kSearchResult,},  // 搜索结果,
  {"rate.taobao.com", kPrivatePage,},
  {"s8.taobao.com", kListPage,},
  {"list.taobao.com", kListPage,},
  {"s.click.taobao.com", kUnknown,},
  {"p.qiyou.com", kAds,},   // 广告联盟,
  {"video.baidu.com", kSearchResult,},
  {"image.baidu.com", kSearchResult,},
  {"mp3.baidu.com", kSearchResult,},
  {"map.baidu.com", kSearchResult,},
  {"www.soso.com", kSearchResult,},
  {"video.soso.com", kSearchResult,},
  {"music.soso.com", kSearchResult,},
  {"image.soso.com", kSearchResult,},
  {"map.soso.com", kSearchResult,},
  {"www.sogou.com", kSearchResult,},
  {"news.sogou.com", kSearchResult,},
  {"www.sogou.com", kSearchResult,},
  {"mp3.sogou.com", kSearchResult,},
  {"pic.sogou.com", kSearchResult,},
  {"v.sogou.com", kSearchResult,},
  {"map.sogou.com", kSearchResult,},
  {"zhishi.sogou.com", kSearchResult,},
  {"photo.renren.com", kPrivatePage,},
};

int IsWorthlessPage4SE(const std::string& url) {
  GURL gurl(url);
  CHECK(gurl.is_valid());

  const size_t list_size = arraysize(kWorthlessPageHost);
  for (size_t i = 0; i < list_size; ++i) {
    if (gurl.host() == kWorthlessPageHost[i].host
        && (gurl.path().length() > 1
            || gurl.query().length() > 1)) {
      return true;
    }
  }

  return false;
}

struct ResourceTypeMap {
  const char* postfix;
  int type;
};

static const ResourceTypeMap kPathPostfixBlackList[] = {
  {".do", kUnknown},
  {".doc", kUnknown},
  {".mp3", kAudio},
  {".avi", kVideo},
  {".exe", kUnknown},
  {".zip", kUnknown},
  {".xls", kUnknown},
  {".ppt", kUnknown},
  {".rar", kUnknown},
  {".pdf", kUnknown},
  {".dwg", kUnknown},
  {".swf", kAudio},
  {".wps", kUnknown},
  {".o", kUnknown},
  {".cc", kText},
  {".cpp", kText},
  {".cc", kText},
  {".h", kText},
  {".dll", kUnknown},
  {"tag.gz", kUnknown},
};

int URLResourceType(const std::string& url) {
  const size_t list_size = arraysize(kPathPostfixBlackList);
  for (size_t i = 0; i < list_size; ++i) {
    if (::base::EndsWith(url, kPathPostfixBlackList[i].postfix, false)) {
      return kPathPostfixBlackList[i].type;
    }
  }
  
  return kHTML;
}

bool IsSearchResult(const std::string& url) {
  if (log_analysis::IsVerticalSearch(url, NULL, NULL)) {
    return true;
  }

  if (log_analysis::IsGeneralSearch(url, NULL, NULL)) {
    return true;
  }

  std::string domain, insite;
  if (log_analysis::IsSiteInternalSearch(url, &domain, &insite)) {
    return true;
  }
  
  return false;
}

bool TidyURL(const std::string& url, std::string* new_url) {
  CHECK(new_url);

  GURL gurl(url);
  if (!gurl.is_valid()) {
    VLOG(1) << "URL: " << url << " is invalid.";
    return false;
  }
  
  if (IsSearchResult(url)) {
    VLOG(1) << "URL: " << url << " is search result page.";
    return false;
  }

  if (kHTML != URLResourceType(url)) {
    VLOG(1) << "URL: " << url << " is not webpage.";
    return false;
  }

  if (kNotWorthless != IsWorthlessPage4SE(url)) {
    VLOG(1) << "URL: " << url << " is worthless for search engine";
    return false;
  }

  if (log_analysis::IsAds(url, NULL)) {
    VLOG(1) << "URL: " << url << "is ad";
    return false;
  }

  // 替换
  web::UrlNorm norm;
  std::string tmp = url;
  norm.NormalizeAlias(tmp, new_url);
  tmp = *new_url;
  norm.NormalizeSlashAndDot(tmp, new_url);
  tmp = *new_url;
  norm.RemoveUselessQuery(tmp, new_url);
  tmp = *new_url;
  *new_url = ReplaceAlias(tmp);
  return true;
}
}  // namespace crawler
