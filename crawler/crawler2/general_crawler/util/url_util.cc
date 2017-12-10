#include "crawler2/general_crawler/util/url_util.h"

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <string>

#include "web/url/url.h"
#include "web/url_norm/url_norm.h"
#include "base/encoding/url_encode.h"
#include "base/common/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_tokenize.h"
#include "base/strings/string_split.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/timestamp.h"
#include "extend/regexp/re3/re3.h"
#include "log_analysis/common/log_common.h"
#include "third_party/jsoncpp/include/json/json.h"

namespace crawler3 {

using extend::re3::Re3;

struct FilterRule {
  const char* host_prefix;
  const char* host_postfix;
  const char* path;
  // XXX: 当 query 取值为 NULL 时, 表示只要目标 URL 的 query 为非空, 就算 query 匹配成功
  const char* query;
  const char* rule_type;
};

// host 在黑名单中的 URL 将被过滤掉
static const char* kHostBlackList[] = {
  "ptlogin2.qq.com", "ptlogin2.3366.com", "img03.taobaocdn.com",
  "cache.baidu.com", "ptlogin2.paipai.com", "webcache.googleusercontent.com",
  "snapshot.soso.com", "newscache.baidu.com"
};

// host 在受限名单中的 URL 中, 会过滤掉所有非首页的 URLs
static const char* kHostRestrictList[] = {
  "weibo.com", "www.kaixin001.com", "p.777wyx.com", "g.zx915.com",
  "hero.qzoneapp.com"
};

static const char* kPathPostfixBlackList[] = {
  "search.htm", "search.php", "search.asp", "search.html", "redirects", ".do",
  ".doc", ".mp3", ".avi", ".exe", ".zip", ".xls", ".ppt", ".rar", ".pdf", ".dwg",
  ".swf", ".wps", ".o", ".cc", ".cpp", ".cc", ".h", ".dll"
};

static const char* kFilterQueryArray[] = {
  "login", "register", "wd=",
  "word=", "query=", "q=", "search=", "url=", "redirecturl=", "targeturl=", "cdnurl=", "action=",
  "logout", "order_id=", "trade_id=", "tradeid=", "attachmentid="
};

// 当 host_prefix  非空，用其去匹配 URL host 的前缀
// 当 host_postfix 非空，用其去匹配 URL host 的后缀
// 这两个字段不能同时为 非空，否则函数行为未定义
static const FilterRule kFilterRuleBook[] = {
  {NULL,                 ".com",               "/*login*",                  "*",          "Login"},
  {NULL,                 ".cn",                "/*login*",                  "*",          "Login"},
  {NULL,                 ".profile.live.com",  "*",                         "*",          "Login"},
  // taobao & alibaba
  {NULL,                 ".taobao.com",        "/account/*",                "*",          "Login"},
  {NULL,                 ".taobao.com",        "/auction/*",                "*",          "Login"},
  {NULL,                 ".taobao.com",        "/user/order_detail*",       "*",          "Login"},
  {NULL,                 ".tmall.com",         "/detail/orderDetail.*",     "*",          "Login"},
  {NULL,                 ".alibaba.com",       "/order/*",                  "*",          "Login"},
  {NULL,                 ".alibaba.com",       "/member/signin*",           "*",          "Login"},
  {NULL,                 ".alibaba.com",       "/favorites/add_to_*",       "*",          "Login"},
  {NULL,                 ".alibaba.com",       "/offer/post/fill_*",        "*",          "Login"},
  {"s8.taobao.com",       NULL,                "/search",                   "*",          "Taobao"},
  {"trade.taobao.com",    NULL,                "/trade/*",                  "*",          "Taobao"},
  {"s.click.taobao.com",  NULL,                "/t_3",                      "*",          "Taobao"},
  // qq
  {NULL,                 ".alipay.com",        "/standard/payment/*",       "*",          "Login"},
  {NULL,                 ".qq.com",            "/cn2/findpsw/*",            "*",          "Login"},
  // google
  {"www.google.",         NULL,                "/search",                   "*",          "Google"},
  {"www.google.",         NULL,                "/",                         "*",          "Google"},
  {"www.google.",         NULL,                "/aclk",                     "*",          "Google"},
  // baidu
  {"www.baidu.",          NULL,                "/cpro.php",                 NULL,         "Baidu"},
  {"www.baidu.",          NULL,                "/baidu.php",                NULL,         "Baidu"},
  {"www.baidu.",          NULL,                "/adrc.php",                 NULL,         "Baidu"},
  {"www.baidu.",          NULL,                "/cb.php",                   NULL,         "Baidu"},
  {"zhidao.baidu.",       NULL,                "/q",                        "*",          "Baidu"},
  {"map.baidu.com",       NULL,                "/",                         NULL,         "Baidu"},
  {"news.baidu.com",      NULL,                "/ns",                       "*",          "Baidu"},
  {"passport.baidu.com",  NULL,                "/",                         NULL,         "Baidu"},
  {"e.baidu.com",         NULL,                "*",                         NULL,         "BaiduTuiguang"},
  {"zhidao.baidu.com",    NULL,                "/browse/*",                 "*",          "ZhidaoBrowse"},
  // sogou
  {"www.sogou.com",       NULL,                "/bill_search",              "*",          "SogouAds"},
  {"v.sogou.",            NULL,                "/mlist/*",                  "*",          "Sogou"},
  {"www.sogou.com",       NULL,                "/websnapshot",              "*",          "SogouCache"},
  // soso
  {"www.soso.com",        NULL,                "/q",                        "*",          "Soso"},
  {"wenwen.soso.com",     NULL,                "/z/Search.e",               "*",          "Soso"},
  {"map.soso.com",        NULL,                "/",                         NULL,         "Soso"},
  {"jzclick.soso.com",    NULL,                "/click",                    "*",         "SosoAds"},
  // yahoo
  {"search.yahoo.com",    NULL,                "/search;*",                 "*",          "Yahoo"},
  // sina
  {NULL,                  ".sina.com.cn",      "/search*",                  NULL,         "Sina"},
  {"www.uqude.com",       NULL,                "/content/getSolr.action",   "*",          "Uqude"},
  {"link.admin173.com",   NULL,                "/index.php",                NULL,         "Link173"},
  {"www.kaixin001.com",   NULL,                "/login/",                   "*",          "Kaixin"},
  {"www.168dushi.com.",   NULL,                "/czfy*",                    "*",          "168dushi"},
  {"car.autohome.com",    NULL,                "/price/list-*",             "*",          "Autohome"},
  {NULL,                  ".auto.sohu.com",    "/searchterm.sip",           NULL,         "Sohu"},
  {"product.it.sohu.com",     NULL,            "/search/*",                 "*",          "Sohu"},
  {"search.360buy.com",   NULL,                "/search",                   NULL,         "360buy"},
  {NULL,                  ".hao123.com",       "/index*",                   "*",          "Hao123"},
  {NULL,                  ".hao123.net",       "/index*",                   "*",          "Hao123"},
  {"search.51job.com",    NULL,                "/list*",                    NULL,         "51job"},
  {"movie.xunlei.com",    NULL,                "/person/*",                 "*",          "Xunlei"},
  {"bbs.ifeng.",          NULL,                "*",                         "action=*",   "Ifeng"},
  {"huilitongxie.com",    NULL,                "/",                         "gallery*",   "Huilitongxie"},
  {NULL,                  "enet.com.cn",       "/price/plist*",             "*",          "Enet"},
  {"www.52dpe.com",       NULL,                "/",                         "gallery*",   "52dpe"},
  {NULL,                  "pctowap.com",       "/dir/*",                    "*",          "Pctowap"},
  {NULL,                  "5173.com",          "/search/*",                 "*",          "5173"},
  // chinadaily.chinadaily ---> www.chinadaily.com.cn
  {"chinadaily.chinadaily.",  NULL,            "*",                         "*",          "ChinaDaily"},
  // news.whnew.com ---> www.whnew.com/
  {"news.whnews.cn",      NULL,                "*",                         "*",          "Whnews"},
  {"whnews.cn",           NULL,                "*",                         "*",          "Whnews"},
  {"whccr.com",           NULL,                "*",                         "*",          "Whnews"},
  // dgvan.zjol.com.cn ---> itcpn.zjol.com.cn
  {"dgvan.zjol.com.cn",   NULL,                "*",                         "*",          "ItcpnZjol"},
  {"v.360.cn",            NULL,                "*/list.php",                "cat=*",      "Search360"},
  {"v.360.cn",            NULL,                "*/index.php",               "cat=*",      "Search360"},
  {"www.newegg.",         NULL,                "/Search.*",                 "*",          "Newegg"},
  {"mq.qq.com",           NULL,                "*",                         NULL,         "Mqqq"},
  {"so.tudou.com",        NULL,                "/nisearch*",                "*",          "Soutudo"},
  {NULL,                  "mail.163.com",      "*",                         NULL,         "163Mail"},
  {"t.qq.com",            NULL,                "/p/t/*",                    "*",          "TengxunWeibo"},
  {"translate.google",    NULL,                "*",                         NULL,        "GoogleTranslate"},
  // bing
  {"www.microsofttranslator.com",  NULL,       "*",                         NULL,         "BingFanyi"},
  {"cc.bingj.com",        NULL,                "/cache.*",                  "*",          "BingCache"},
  {"adredir.adcenter.bing.",       NULL,       "/redir",                    "*",          "BingAds"},
  {"www.content4ads.com",          NULL,       "/live.php",                 "*",          "BingAds"},
  // youdao
  {"clkservice.youdao.com",       NULL,        "/clk/request.s",            "*",          "YoudaoAds"},
  {"www.youdao.com",       NULL,        "/cache",            "*",          "YoudaoCache"},
};


static bool IsUrlPathValid(const std::string &input_path) {
  std::string path(input_path);
  if (path.empty()) return false;
  std::vector<std::string> tokens;
  int num = base::Tokenize(path, "/", &tokens);
  if (num >= kMaxPathDepth) {
    DLOG(INFO) << "Invalid url path: exceed kMaxPathDepth(8), path: " << path;
    return false;
  }
  base::LowerString(&path);
  bool found = false;
  int item_num = arraysize(kPathPostfixBlackList);
  for (int i = 0; i < item_num && !found; ++i) {
    if (base::EndsWith(path, kPathPostfixBlackList[i], false)) {
      DLOG(INFO) << "Invalid url path: path in kPathPostfixBlackList, path: " << path;
      found = true;
      break;
    }
  }
  return !found;
}

static bool IsUrlQueryValid(const std::string &query) {
  if (query.empty()) return true;
  if ((int)query.size() > kMaxQueryLength) {
    DLOG(INFO) << "Invalid url query: Exceed Max Query length(1000), query: " << query;
    return false;
  }

  std::string myquery(query);
  base::LowerString(&myquery);
  std::string::size_type pos = 0;
  bool found = false;
  int item_num = arraysize(kFilterQueryArray);
  for (int i = 0; i < item_num && !found; ++i) {
    pos = myquery.find(kFilterQueryArray[i]);
    if (pos != std::string::npos) {
      DLOG(INFO) << "Invalid url query: contain not_allowed tokens, query: " << query;
      found = true;
      break;
    }
  }
  return !found;
}

static bool IsHostInBlackList(const std::string &host) {
  int item_num = arraysize(kHostBlackList);
  bool found = false;
  for (int i = 0; i < item_num && !found; ++i) {
    if (host == kHostBlackList[i]) {
      found = true;
      break;
    }
  }
  return found;
}

static bool IsPassHostInRestrictCheck(const std::string &url) {
  GURL gurl(url);
  const std::string &host = gurl.host();
  int item_num = arraysize(kHostRestrictList);
  bool not_found = true;
  for (int i = 0; i < item_num; ++i) {
    if (base::MatchPattern(host, kHostRestrictList[i])) {
      not_found = false;
      break;
    }
  }
  return not_found || (gurl.path() == "/" && gurl.query().empty());
}

static bool IsSearchResult(const std::string &host, const std::string &path,
                           const std::string &query, std::string *rule_type) {
  int item_num = arraysize(kFilterRuleBook);
  const FilterRule* found_item = NULL;
  for (int i = 0; i < item_num; ++i) {
    const FilterRule& item = kFilterRuleBook[i];
    CHECK(!(item.host_prefix == NULL && item.host_postfix == NULL));
    if (NULL != item.host_prefix) {
      if (base::StartsWith(host, item.host_prefix, false) && base::MatchPattern(path, item.path)) {
        if ((item.query == NULL && !query.empty())
            || (item.query != NULL && base::MatchPattern(query, item.query))) {
          found_item = &item;
          break;
        }
      }
    } else {
      if (base::EndsWith(host, item.host_postfix, false) && base::MatchPattern(path, item.path)) {
        if ((item.query == NULL && !query.empty())
            || (item.query != NULL && base::MatchPattern(query, item.query))) {
          found_item = &item;
          break;
        }
      }
    }
  }
  if (NULL == found_item) return false;
  if (NULL != rule_type) *rule_type = found_item->rule_type;
  return true;
}

bool WillFilterAccordingRules(const std::string &orig_url, std::string *reason) {
  std::string url(orig_url);
  if (!web::has_scheme(url)) {
    url = "http://" + url;
  }
  // Check Max length of URL allowed
  if ((int)url.size() > crawler3::kMaxLinkSize) {
    if (reason != NULL) {
      *reason = "Url exceeds max url lenght limit(1KB), url: " + url;
    }
    return true;
  }
  // Bad Url format like: http://%20www.sina.com.cn or http://.www.sohu.com/
  if (base::StartsWith(url, "http://.", false) || base::StartsWith(url, "http://%", false) ||
      base::StartsWith(url, "http://+", false)) {
    if (reason != NULL) {
      *reason = "Url bad format, start with +, -, . or %, url: " + url;
    }
    return true;
  }
  GURL gurl(url);
  if (!gurl.is_valid()) {
    if (reason != NULL) {
      *reason = "Not pass gurl.is_valid() check: , url: " + url;
    }
    return true;
  }

  // Check Http Schema
  const std::string &scheme = gurl.scheme();
  if (scheme != "http" && scheme != "https") {
    if (reason != NULL) {
      *reason = "Not pass scheme check: only support http or https, url: " + url;
    }
    return true;
  }
  //  是否通过 受限制 Host check
  if (!IsPassHostInRestrictCheck(url)) {
    if (reason != NULL) {
      *reason = "Not pass IsPassHostInRestrictCheck: url: " + url;
    }
    return true;
  }
  const std::string &host = gurl.host();
  const std::string &path = gurl.path();
  const std::string &query = gurl.query();

  std::string rule_type;
  // Check IsHostInBlackList or not
  if (IsHostInBlackList(host)) {
    if (reason != NULL) {
      *reason = "Not pass IsHostInBlackList check: host in blacklist, host: " + host;
    }
    return true;
  }
  // Check Url Path is valid or not
  if (!IsUrlPathValid(path)) {
    if (reason != NULL) {
      *reason = "Not pass check: IsUrlPathValid(), path: " + path;
    }
    return true;
  }
  // Check Url Query is valid or not
  if (!IsUrlQueryValid(query)) {
    if (reason != NULL) {
      *reason = "Not pass check: IsUrlQueryValid(), query: " + query;
    }
    return true;
  }
  // Check if Url match the search engine page book
  if (IsSearchResult(host, path, query, &rule_type)) {
    if (reason != NULL) {
      *reason = "Not pass check: !IsSearchResult(), url: " + url + ", type: " + rule_type;
    }
    return true;
  }
  return false;
}

bool IsGeneralSearchFirstNPage(const std::string &url, int N) {
  CHECK(!url.empty() && N > 0);
  if (!log_analysis::IsGeneralSearch(url, NULL, NULL)) {
    LOG(WARNING) << "Not Pass: IsGeneralSearch()" << url;
    return false;
  }
  GURL gurl(url);
  const std::string &host = gurl.host();
  const std::string &query = gurl.query();
  // Google
  if (host == "www.google.com.hk") {
    int max_start = (N-1) * 10;
    std::string::size_type pos = query.find("start=");
    if (pos == std::string::npos) return true;
    std::string::size_type pos2 = pos + std::string("start=").size();
    std::string::size_type pos3 = pos + std::string("start=").size();
    while (pos3 < query.size() && query[pos3] != '&') ++pos3;
    std::string start_str = query.substr(pos2, pos3-pos2);
    if (start_str.empty()) return true;
    int start_num;
    if (!base::StringToInt(start_str, &start_num)) {
      LOG(WARNING) << "StringToInt32() fail, str: " << start_str;
      return false;
    }
    if (start_num <= max_start) return true;
    return false;
  }
  // Baidu
  if (host == "www.baidu.com") {
    int max_pn = (N-1) * 10;
    std::string::size_type pos = query.find("pn=");
    if (pos == std::string::npos) return true;
    std::string::size_type pos2 = pos + std::string("pn=").size();
    std::string::size_type pos3 = pos + std::string("pn=").size();
    while (pos3 < query.size() && query[pos3] != '&') ++pos3;
    std::string pn_str = query.substr(pos2, pos3-pos2);
    if (pn_str.empty()) return true;
    int pn_num;
    if (!base::StringToInt(pn_str, &pn_num)) {
      LOG(WARNING) << "StringToInt32() fail, str: " << pn_str;
      return false;
    }
    if (pn_num <= max_pn) return true;
    return false;
  }
  // Bing
  if (host == "www.bing.com") {
    int max_first = (N-1) * 10 + 1;
    std::string::size_type pos = query.find("first=");
    if (pos == std::string::npos) return true;
    std::string::size_type pos2 = pos + std::string("first=").size();
    std::string::size_type pos3 = pos + std::string("first=").size();
    while (pos3 < query.size() && query[pos3] != '&') ++pos3;
    std::string first_str = query.substr(pos2, pos3-pos2);
    if (first_str.empty()) return true;
    int first_num;
    if (!base::StringToInt(first_str, &first_num)) {
      LOG(WARNING) << "StringToInt32() fail, str: " << first_str;
      return false;
    }
    if (first_num <= max_first) return true;
    return false;
  }
  // Sogou
  if (host == "www.sogou.com") {
    int max_page = N;
    std::string::size_type pos = query.find("page=");
    if (pos == std::string::npos) return true;
    std::string::size_type pos2 = pos + std::string("page=").size();
    std::string::size_type pos3 = pos + std::string("page=").size();
    while (pos3 < query.size() && query[pos3] != '&') ++pos3;
    std::string page_str = query.substr(pos2, pos3-pos2);
    if (page_str.empty()) return true;
    int page_num;
    if (!base::StringToInt(page_str, &page_num)) {
      LOG(WARNING) << "StringToInt32() fail, str: " << page_str;
      return false;
    }
    if (page_num <= max_page) return true;
    return false;
  }
  // Soso
  if (host == "www.soso.com") {
    int max_page = N;
    std::string::size_type pos = query.find("pg=");
    if (pos == std::string::npos) return true;
    std::string::size_type pos2 = pos + std::string("pg=").size();
    std::string::size_type pos3 = pos + std::string("pg=").size();
    while (pos3 < query.size() && query[pos3] != '&') ++pos3;
    std::string page_str = query.substr(pos2, pos3-pos2);
    if (page_str.empty()) return true;
    int page_num;
    if (!base::StringToInt(page_str, &page_num)) {
      LOG(WARNING) << "StringToInt32() fail, str: " << page_str;
      return false;
    }
    if (page_num <= max_page) return true;
    return false;
  }

  return false;
}

bool IsVerticalSearchFirstNPage(const std::string &url, int N) {
  CHECK(!url.empty() && N > 0);
  if (!log_analysis::IsVerticalSearch(url, NULL, NULL)) {
    LOG(WARNING) << "Not Pass: IsVerticalSearch()" << url;
    return false;
  }
  GURL gurl(url);
  const std::string &host = gurl.host();
  const std::string &query = gurl.query();
  // Baidu news|zhidao|baike|wenku
  if (host == "news.baidu.com" || host == "zhidao.baidu.com" ||
      host == "baike.baidu.com" || host == "wenku.baidu.com") {
    int max_start = (N-1) * 10;
    if (host == "news.baidu.com") {
      max_start = (N-1) * 20;
    }
    std::string::size_type pos = query.find("pn=");
    if (pos == std::string::npos) return true;
    std::string::size_type pos2 = pos + std::string("pn=").size();
    std::string::size_type pos3 = pos + std::string("pn=").size();
    while (pos3 < query.size() && query[pos3] != '&') ++pos3;
    std::string start_str = query.substr(pos2, pos3-pos2);
    if (start_str.empty()) return true;
    int start_num;
    if (!base::StringToInt(start_str, &start_num)) {
      LOG(WARNING) << "StringToInt32() fail, str: " << start_str;
      return false;
    }
    if (start_num <= max_start) return true;
    return false;
  }
  return false;
}

static const char *kBlackHoleHosts[] = {"edu.360.cn"};
static bool IsBlackHoleHost(const std::string &host) {
  bool flag = false;
  for (int i = 0; i < (int)arraysize(kBlackHoleHosts); ++i) {
    if (host == kBlackHoleHosts[i]) {
      flag = true;
      break;
    }
  }
  return flag;
}

bool IsBlackHoleLink(GURL *target_url, GURL *parent_url) {
  CHECK(target_url != NULL && parent_url != NULL);
  const std::string &host = parent_url->host();
  const std::string &query = parent_url->query();
  if (query.empty() || !IsBlackHoleHost(host)) return false;
  const std::string &host2 = target_url->host();
  const std::string &query2 = target_url->query();
  if (host2 != host || query2.empty()) return false;
  return true;
}

bool RewriteTaobaoItemDetail(std::string *url) {
  CHECK_NOTNULL(url);
  GURL gurl(*url);
  if (!gurl.is_valid()) {
    return false;
  }
  const std::string &host = gurl.host();
  const std::string &path = gurl.path();
  const std::string &query = gurl.query();

  if (host != "item.taobao.com" && host != "detail.tmall.com") {
    return false;
  }
  // http://detail.tmall.com/item.htm?spm=a220m.1000858.1000725.1.oVlJKL&id=16600551819&is_b=1&cat_id=50025829&q=
  // http://detail.tmall.com/venus/spu_detail.htm?spm=a220m.1000858.1000725.1.rAlpKo&spu_id=211354662&cat_id=50024401&entryNum=0&loc=&mallstItemId=16483650814&rn=3d5898734ffae91eaa80a5f4c1db5b62&rewcatid=50024401&from=&disp=g&active=1
  if (path != "/item.htm" && path != "/venus/spu_detail.htm") {
    return false;
  }
  if (query.empty()) {
    return false;
  }
  std::vector<std::string> parts;
  base::SplitString(query, "&", &parts);
  if (path == "/item.htm") {
    for (auto it = parts.begin(); it != parts.end(); ++it) {
      if (base::StartsWith(*it, "id=", false)) {
        url->assign("http://" + host + path + "?" + *it);
        return true;
      }
    }
  } else if (path == "/venus/spu_detail.htm") {
    std::string new_query;
    for (auto it = parts.begin(); it != parts.end(); ++it) {
      if (base::StartsWith(*it, "spu_id=", false) ||
          base::StartsWith(*it, "mallstItemId=", false)) {
        new_query.append(*it + "&");
      }
    }
    url->assign("http://" + host + path + "?" + new_query.substr(0, new_query.length()-1));
    return true;
  }
  return false;
}

bool ConvertTaobaoUrlFromPCToMobile(const std::string &pc_url, std::string *m_url) {
  CHECK_NOTNULL(m_url);
  std::string url(pc_url);
  if (!RewriteTaobaoItemDetail(&url)) {
    DLOG(ERROR) << "RewriteTaobaoItemDetail() fail, url: " << url;
    return false;
  }
  // 转换成手机版本 url
  GURL gurl(url);
  const std::string &host = gurl.host();
  const std::string &path= gurl.path();
  const std::string &query = gurl.query();
  if (query.empty()) {
    DLOG(ERROR) << "query is empty, url: " << url;
    return false;
  }
  // 获取 host
  std::string m_host;
  std::string itemId;
  if (host == "item.taobao.com") {
    m_host = "a.m.taobao.com";
  } else if (host == "detail.tmall.com") {
    m_host = "a.m.tmall.com";
  } else {
    DLOG(ERROR) << "not support host, host: " << host;
    return false;
  }
  // 获取商品 id
  if (path == "/item.htm") {
    std::string::size_type pos = query.rfind("=");
    if (pos == std::string::npos) {
      DLOG(ERROR) << "did not find '=' in query, url: " << url;
      return false;
    }
    itemId = query.substr(pos+1);
  } else if (path == "/venus/spu_detail.htm") {
    std::string::size_type pos = query.rfind("mallstItemId=");
    if (pos == std::string::npos) {
      DLOG(ERROR) << "did not find 'mallstItemId=' in query, url: " << url;
      return false;
    }
    pos += strlen("mallstItemId=");
    std::string::size_type pos0 = pos;
    while (pos != query.length() && isdigit(query[pos])) pos++;
    if (pos == pos0) {
      DLOG(ERROR) << "item id not valid, url: " << url;
      return false;
    }
    itemId = query.substr(pos0, pos-pos0);
  } else {
    DLOG(ERROR) << "not support path, url: " << url;
    return false;
  }

  m_url->assign("http://" + m_host + "/i" + itemId + ".htm");
  return true;
}

bool RewriteTaobaoItemList(std::string *url) {
  CHECK_NOTNULL(url);
  GURL gurl(*url);
  if (!gurl.is_valid()) {
    return false;
  }
  const std::string &host = gurl.host();
  std::string path = gurl.path();
  const std::string &query = gurl.query();
  const std::string &ref = gurl.ref();

  if (host != "list.taobao.com" && host != "nvzhuang.taobao.com") {
    return false;
  }
  if (!base::MatchPattern(path, "/itemlist/*.htm") && !base::MatchPattern(path, "/market/*.htm") &&
      !base::MatchPattern(path, "/market/*.php")) {
    return false;
  }

  if (base::MatchPattern(path, "/market/*/searchAuction.htm")) {
    if (!ref.empty()) {
      url->assign(ref.substr(1));
      DLOG(ERROR) << "ref: " << ref;
      return RewriteTaobaoItemList(url);
    }
  }

  if (query.empty()) {
    return false;
  }
  std::set<std::string> ordered_parts;
  std::vector<std::string> parts;
  if (ref.empty()) {
    base::SplitString(query, "&", &parts);
  } else {
    base::SplitString(ref.substr(1), "&", &parts);
  }
  for (auto it = parts.begin(); it != parts.end(); ++it) {
    if (base::StartsWith(*it, "spm=", false)) {
      continue;
    }
    ordered_parts.insert(*it);
  }
  url->assign("http://" + host + path + "?");

  for (auto it = ordered_parts.begin(); it != ordered_parts.end(); ++it) {
    if (it == ordered_parts.begin()) {
      url->append(*it);
    } else {
      url->append("&" + *it);
    }
  }
  return true;
}


void BuildNextNUrl(const std::string &url, int n,
                   std::vector<std::string> *next_n_url,
                   int each_page_item_num) {
  CHECK_GT(n, 0);
  CHECK_GT(each_page_item_num, 0);
  CHECK_NOTNULL(next_n_url);
  GURL gurl(url);
  const std::string &host = gurl.host();
  const std::string &path = gurl.path();
  const std::string &query = gurl.query();
  std::vector<std::string> parts;
  std::string new_query;
  base::SplitString(query, "&", &parts);
  for (int i = 0; i < (int)parts.size(); ++i) {
    if (!base::StartsWith(parts[i], "s=", false)) {
      new_query.append(parts[i] + "&");
    }
  }
  int s = 0;
  for (int i = 0; i < n; ++i) {
    s = (i + 1) * each_page_item_num;
    next_n_url->push_back("http://"+ host + path + "?" + new_query + base::StringPrintf("s=%d", s));
  }
}

// 由于淘吧最多显示 前 100 页, 为了获取尽可能多的商品页, 采用不同的排序方法:
// 默认 销量 人气 信用 最新 总价
void BuildTaobaoDifferentSortType(const std::string &url, std::vector<std::string> *urls) {
  CHECK_NOTNULL(urls);
  GURL gurl(url);
  const std::string &host = gurl.host();
  const std::string &path = gurl.path();
  const std::string &query= gurl.query();

  std::vector<std::string> parts;
  base::SplitString(query, "&", &parts);
  std::string new_query;
  for (int i = 0 ; i < (int)parts.size(); ++i) {
    if (!base::StartsWith(parts[i], "sort=", false)) {
      new_query.append(parts[i] + "&");
    }
  }
  urls->push_back("http://" + host + path + "?" + new_query + "sort=biz30day");
  urls->push_back("http://" + host + path + "?" + new_query + "sort=coefp");
  urls->push_back("http://" + host + path + "?" + new_query + "sort=ratesum");
  urls->push_back("http://" + host + path + "?" + new_query + "sort=_oldstart");
  urls->push_back("http://" + host + path + "?" + new_query + "sort=bid");
}

// 由于天猫最多显示 前 100 页, 为了获取尽可能多的商品页, 采用不同的排序方法:
// 默认排序, 按照销量排序, 按照价格升(降)序
void BuildTmallDifferentSortType(const std::string &url, std::vector<std::string> *urls) {
  CHECK_NOTNULL(urls);
  GURL gurl(url);
  const std::string &host = gurl.host();
  const std::string &path = gurl.path();
  const std::string &query= gurl.query();

  std::vector<std::string> parts;
  base::SplitString(query, "&", &parts);
  std::string new_query;
  for (int i = 0 ; i < (int)parts.size(); ++i) {
    if (!base::StartsWith(parts[i], "sort=", false)) {
      new_query.append(parts[i] + "&");
    }
  }
  urls->push_back("http://" + host + path + "?" + new_query + "sort=d");
  urls->push_back("http://" + host + path + "?" + new_query + "sort=p");
  urls->push_back("http://" + host + path + "?" + new_query + "sort=pd");
}

// 淘宝 && Tmall
bool BuildTaobaoCommentLink(const std::string &url, std::string *comment_url, int latest_comment_num) {
  CHECK_NOTNULL(comment_url);
  static const Re3 m_version_taobao("http://a.m.taobao.com/i(\\d+).html?");
  static const Re3 m_version_tmall("http://a.m.tmall.com/i(\\d+).html?");
  std::string itemId;
  if (!Re3::FullMatch(url, m_version_taobao, &itemId) &&
      !Re3::FullMatch(url, m_version_tmall, &itemId)) {
    LOG(ERROR) << "url must be mobile-version taobao item, but is: " << url;
    return false;
  }
  std::string tmp_comment_url = base::StringPrintf("http://%s/ajax/rate_list.do?item_id=%s&p=1&ps=%d",
                                                   GURL(url).host().c_str(), itemId.c_str(),
                                                   latest_comment_num);
  comment_url->assign(tmp_comment_url);
  return true;
}

// 商品详情
bool BuildTaobaoProductDetailLink(const std::string &url, std::string *detail_url) {
  CHECK_NOTNULL(detail_url);
  static const Re3 m_version_taobao("http://a.m.taobao.com/i(\\d+).html?");
  static const Re3 m_version_tmall("http://a.m.tmall.com/i(\\d+).html?");
  std::string itemId;
  if (!Re3::FullMatch(url, m_version_taobao, &itemId) &&
      !Re3::FullMatch(url, m_version_tmall, &itemId)) {
    LOG(ERROR) << "url must be mobile-version taobao item, but is: " << url;
    return false;
  }
  std::string tmp_url = base::StringPrintf("http://%s/ajax/desc_list.do?item_id=%s",
                                                   GURL(url).host().c_str(), itemId.c_str());
  detail_url->assign(tmp_url);
  return true;
}

// 商品参数
bool BuildTaobaoProductParamLink(const std::string &url, std::string *param_url) {
  CHECK_NOTNULL(param_url);
  static const Re3 m_version_taobao("http://a.m.taobao.com/i(\\d+).html?");
  static const Re3 m_version_tmall("http://a.m.tmall.com/i(\\d+).html?");
  std::string itemId;
  if (!Re3::FullMatch(url, m_version_taobao, &itemId) &&
      !Re3::FullMatch(url, m_version_tmall, &itemId)) {
    LOG(ERROR) << "url must be mobile-version taobao item, but is: " << url;
    return false;
  }
  std::string tmp_url = base::StringPrintf("http://%s/ajax/param.do?item_id=%s",
                                                   GURL(url).host().c_str(), itemId.c_str());
  param_url->assign(tmp_url);
  return true;
}

static void Parse1(const Json::Value &value, ::crawler3::url::TaobaoProductInfo *item) {
  // timestamp
  item->set_timestamp(base::GetTimestamp());
  // link
  const std::string &link = value["href"].asString();
  item->set_link(link);
  // itemId
  const std::string &itemId = value["itemId"].asString();
  item->set_item_id(itemId);
  // tradeNum
  const std::string &tradeNum = value["tradeNum"].asString();
  item->set_trade_num(tradeNum);
  // title
  const std::string &title = value["fullTitle"].asString();
  item->set_title(title);
  // price
  const std::string &price = value["price"].asString();
  item->set_price(price);
  // currentPrice
  const std::string &currentPrice= value["currentPrice"].asString();
  item->set_current_price(currentPrice);
  // imageLink
  const std::string &imageLink= value["image"].asString();
  item->set_image_link(imageLink);

  // sellerId
  const std::string &sellerId = value["sellerId"].asString();
  item->set_seller_id(sellerId);
  // commend
  const std::string &commend = value["commend"].asString();
  item->set_commend(commend);
  // loc
  const std::string &loc = value["loc"].asString();
  item->set_loc(loc);
}

static void Parse2(const Json::Value &value, ::crawler3::url::TaobaoProductInfo *item) {
  // timestamp
  item->set_timestamp(base::GetTimestamp());
  // link
  std::string link = value["link"].asString();
  RewriteTaobaoItemDetail(&link);
  item->set_link(link);
  // itemId
  const std::string &itemId = value["itemId"].asString();
  item->set_item_id(itemId);
  // tradeNum
  int tradeNum = value["salesCount"].asInt();
  item->set_trade_num(base::StringPrintf("%d", tradeNum));
  // title
  std::string title = value["title"].asString();
  std::string tmp_title;
  if (base::DecodeUrlComponent(title.c_str(), &tmp_title)) {
    item->set_title(tmp_title);
  } else {
    DLOG(ERROR) << "decode title fail, title:" << title;
  }
  // price
  const std::string &price = value["price"].asString();
  item->set_price(price);
  // currentPrice
  const std::string &currentPrice= value["priceOld"].asString();
  item->set_current_price(currentPrice);
  // imageLink
  const std::string &imageLink= value["img"].asString();
  item->set_image_link(imageLink);

  // sellerId
  item->set_seller_id("");
  // commend
  item->set_commend("");
  // loc
  item->set_loc("");
}

bool ParseJsonFormatPage(const std::string &json_page,
                         std::vector< ::crawler3::url::TaobaoProductInfo> *items,
                         int *page_num) {
  CHECK_NOTNULL(items);
  CHECK_NOTNULL(page_num);
  if (json_page.empty()) {
    DLOG(ERROR) << "json file empty";
    return false;
  }
  // 淘宝返回的 json 格式有两种:
  // 1. 正常的 json 文件格式, 不需要对文件作任何预处理;
  // 2. 需要取出文件开始和结束的一对小括号()
  int format_type;
  std::string page(json_page);
  int32 page_size = (int32)page.size();
  int32 rstart = page_size-1;
  while (rstart >=0 && (page[rstart] == '\n' || page[rstart] == '\t' ||
                        page[rstart] == '\r' || page[rstart] == ' ')) --rstart;
  if (rstart < 0) {
    DLOG(ERROR) << "file format error, rstart < 0";
    return false;
  }
  char flag = page[rstart];
  if (flag == '}') {
    format_type = 0;
  } else if (flag == ')') {
    format_type = 1;
    // 预处理, 取出 () 前后包含的内容
    std::string::size_type pos = page.find('(');
    if (pos == std::string::npos) {
      DLOG(ERROR) << "json format error, did not find (";
      return false;
    }
    int32 len = (page_size-1) - (pos+1);
    if (len <= 0) {
      DLOG(ERROR) << "json format error, len <=0 after pre-process";
      return false;
    }
    page = json_page.substr(pos+1, len);
  } else {
    DLOG(ERROR) << "json file format error, last char of file must be } or ), but is: " << flag;
    return false;
  }

  // 开始解析 Json 网页
  Json::Reader reader;
  Json::Value root;
  if (!reader.parse(page, root, false)) {
    DLOG(ERROR) << "parse fail";
    return false;
  }
  if (root.type() != Json::objectValue) {
    return false;
  }
  crawler3::url::TaobaoProductInfo item;
  std::string page_str;
  if (format_type == 0) {
    Json::Value itemlist = root["itemList"];
    if (itemlist.type() != Json::arrayValue) {
      return false;
    }
    int size = itemlist.size();
    DLOG(ERROR) << "items num=" << size;
    for (int i = 0; i < size; ++i) {
      item.Clear();
      const Json::Value &val = itemlist[i];
      if (val.type() == Json::nullValue) {
        continue;
      }
      Parse1(val, &item);
      items->push_back(item);
    }
    page_str = root["page"]["totalPage"].asString();
  } else if (format_type == 1) {
    Json::Value itemlist = root["item"]["list"];
    if (itemlist.type() != Json::arrayValue) {
      return false;
    }
    int size = itemlist.size();
    DLOG(ERROR) << "items num=" << size;
    for (int i = 0; i < size; ++i) {
      item.Clear();
      const Json::Value &val = itemlist[i];
      if (val.type() == Json::nullValue) {
        continue;
      }
      Parse2(val, &item);
      items->push_back(item);
    }
    page_str = root["page"]["pageSize"].asString();
  } else {
    LOG(FATAL) << "unknown format type: " << format_type;
  }

  if (page_str.empty() || !base::StringToInt(page_str, page_num)) {
    LOG(ERROR) << "convert page_num to int fail, page: " << page_str;
    *page_num = -1;
  }
  return true;
}

bool GetPageNumFromTmallListPage(const std::string &page_utf8, int32 *total_page_num) {
  CHECK_NOTNULL(total_page_num);
  if (page_utf8.empty()) {
    return false;
  }
  const std::string &page = page_utf8;
  static const Re3 page_num_pattern("\\s*共(\\d+)页");
  std::vector<std::string> page_num_info(10, "");
  if (!Re3::PartialMatchN(page, page_num_pattern, &page_num_info)) {
    DLOG(ERROR) << "Re3::PartialMatchN() fail";
    return false;
  }
  DLOG(ERROR) << "page_num: " << page_num_info[0];
  if (!base::StringToInt(page_num_info[0], total_page_num)) {
    DLOG(ERROR) << "base::StringToInt() fail";
    return false;
  }
  return true;
}

bool GetPageNumFromJingDongListPage(const std::string &jingdong_page_utf8, int32 *total_page_num) {
  CHECK_NOTNULL(total_page_num);
  if (jingdong_page_utf8.empty()) {
    return false;
  }
  const std::string &page = jingdong_page_utf8;
  static const Re3 page_num_pattern("<a href=\"[-\\d]+.html\" >(\\d+)</a><a href=\"[-\\d]+.html\" class=\"next\" >下一页<b></b></a>");  // NOLINT
  std::vector<std::string> page_num_info(10, "");
  if (!Re3::PartialMatchN(page, page_num_pattern, &page_num_info)) {
    DLOG(ERROR) << "Re3::PartialMatchN() fail";
    return false;
  }
  DLOG(ERROR) << "page_num: " << page_num_info[0];
  if (!base::StringToInt(page_num_info[0], total_page_num)) {
    DLOG(ERROR) << "base::StringToInt() fail";
    return false;
  }
  return true;
}

bool GetPageNumFromSuNingListPage(const std::string &suning_page_utf8, int32 *total_page_num) {
  CHECK_NOTNULL(total_page_num);
  if (suning_page_utf8.empty()) {
    return false;
  }
  const std::string &page = suning_page_utf8;
  static const Re3 page_num_pattern("<span><i id=\"pageThis\">\\d+</i>/<i id=\"pageTotal\">(\\d+)</i></span>");  // NOLINT
  std::vector<std::string> page_num_info(10, "");
  if (!Re3::PartialMatchN(page, page_num_pattern, &page_num_info)) {
    DLOG(ERROR) << "Re3::PartialMatchN() fail";
    return false;
  }
  DLOG(ERROR) << "page_num: " << page_num_info[0];
  if (!base::StringToInt(page_num_info[0], total_page_num)) {
    DLOG(ERROR) << "base::StringToInt() fail";
    return false;
  }
  return true;
}

bool GetJingDongNextUrl(const std::string &first_url, int next_n_page, std::vector<std::string> *urls) {
  CHECK_NOTNULL(urls);
  CHECK_GT(next_n_page, 0);
  if (first_url.empty()) {
    return false;
  }
  const std::string &url = first_url;
  static const char *extra = "-0-0-0-0-0-0-0-1-1-";
  if (url.find(extra) != std::string::npos) {
    return false;
  }

  // 对于大家电, 翻页 url 的构造略有不同, 可能是临时的
  std::string last_part;
  //  大家电翻页 url
  // http://www.360buy.com/products/737-794-798-0-0-0-0-0-0-0-1-1-4-1-72-33.html
  if (base::MatchPattern(url, "http://www.360buy.com/products/737-794-*.html")) {
    for (int i = 2; i < 2 + next_n_page; ++i) {
      last_part = base::StringPrintf("%s%d-1-72-33.html", extra, i);
      urls->push_back(base::StringReplace(url, ".html", last_part, false));
    }
  } else {
    for (int i = 2; i < 2 + next_n_page; ++i) {
      last_part = base::StringPrintf("%s%d.html", extra, i);
      urls->push_back(base::StringReplace(url, ".html", last_part, false));
    }
  }
  return true;
}

// Build from: http://search.suning.com/emall/strd.do?ci=289505&cityId=9173
//  |
//  |
// \_/
// To:
// http://search.suning.com/emall/strd.do?ci=289505&cityId=9173&cp=NextPageIndex&il=0&si=5&st=14&iy=-1
bool GetSuNingNextUrl(const std::string &first_url, int next_n_page, std::vector<std::string> *urls) {
  CHECK_NOTNULL(urls);
  CHECK_GT(next_n_page, 0);
  static const Re3 suning_list_url("http://search.suning.com/emall/strd.do\\?ci=\\d+&cityId=\\d+");
  static const Re3 suning_list_url2("http://search.suning.com/emall/s?trd.do\\?ci=\\d+");
  static const Re3 suning_list_url3("http://search.suning.com/emall/pcd.do\\?ci=\\d+");
  static const Re3 suning_list_url4("http://search.suning.com/emall/search.do\\?keyword=.*");
  if (!Re3::FullMatch(first_url, suning_list_url) && !Re3::FullMatch(first_url, suning_list_url2) &&
      !Re3::FullMatch(first_url, suning_list_url3) && !Re3::FullMatch(first_url, suning_list_url4)) {
    LOG(ERROR) << "Not match suning list url pattern, url: " << first_url;
    return false;
  }
  const std::string &query = GURL(first_url).query();
  if (query.find("&il=0&si=5&st=14&iy=-1") != std::string::npos) {
    return false;
  }

  for (int i = 1; i < 1 + next_n_page; ++i) {
     urls->push_back(base::StringPrintf("%s&cp=%d&il=0&si=5&st=14&iy=-1", first_url.c_str(), i));
  }
  return true;
}

static const Re3 suning_product_url_pattern("http://www.suning.com/emall/prd_(\\d+)_(\\d+)_-\\d_(\\d+)_\\.html");  // NOLINT
static const Re3 jingdong_product_url_pattern("http://www.360buy.com/product/([0-9]+).html");

bool BuildJingDongPriceImageLink(const std::string &url, std::string *price_image_url) {
  CHECK_NOTNULL(price_image_url);
  std::string product_id;
  if (!Re3::FullMatch(url, jingdong_product_url_pattern, &product_id)) {
    return false;
  }
  price_image_url->assign("http://jprice.360buyimg.com/price/gp" + product_id + "-1-1-3.png");
  return true;
}

// http://www.suning.com/emall/prd_10052_10051_-7_1966545_.html   - ->
//  |
//  |
// \_/
// http://www.suning.com/emall/SNProductStatusView?storeId=10052&catalogId=10051&productId=1966545&cityId=9017
bool BuildSuNingPriceLink(const std::string &url, std::string *price_url) {
  CHECK_NOTNULL(price_url);
  std::string storeId, catalogId, productId;
  if (!Re3::FullMatch(url, suning_product_url_pattern, &storeId, &catalogId, &productId)) {
    LOG(ERROR) << "Not match suning product item url pattern, url: " << url;
    return false;
  }
  static const char *cityId = "9017";  // 北京
  price_url->assign(base::StringPrintf("http://www.suning.com/emall/SNProductStatusView?storeId=%s&catalogId=%s&productId=%s&cityId=%s", storeId.c_str(), catalogId.c_str(), productId.c_str(), cityId)); // NOLINT
  return true;
}

bool BuildJingDongCommentLink(const std::string &url, std::string *comment_url) {
  CHECK_NOTNULL(comment_url);
  std::string product_id;
  if (!Re3::FullMatch(url, jingdong_product_url_pattern, &product_id)) {
    return false;
  }
  comment_url->assign("http://club.360buy.com/clubservice/newproductcomment-" + product_id + "-0-0.html");
  return true;
}

// http://www.suning.com/emall/prd_10052_10051_-7_1966545_.html
//  |
//  |
// \_/
// http://www.suning.com/emall/snmemtest_10051_10052_1966545_-7_000000000102556416_all_Product_0_.html  <==>
// http://www.suning.com/emall/snmemtest_10051_10052_1966545_-7_0_all_Product_0_.html

bool BuildSuNingCommentLink(const std::string &url, std::string *comment_url) {
  CHECK_NOTNULL(comment_url);
  std::string storeId, catalogId, productId;
  if (!Re3::FullMatch(url, suning_product_url_pattern, &storeId, &catalogId, &productId)) {
    LOG(ERROR) << "Not match suning product item url pattern, url: " << url;
    return false;
  }
  comment_url->assign(base::StringPrintf("http://www.suning.com/emall/snmemtest_%s_%s_%s_-7_0_all_Product_0_.html", catalogId.c_str(), storeId.c_str(), productId.c_str())); // NOLINT
  return true;
}

// 对图片进行转换, 目的: 抓取原始 size 的图片
// FROM: http://q.i02.wimg.taobao.com/bao/uploaded/i1/846481524/T2dgK_XmxXXXXXXXXX_!!846481524.jpg_70x70.jpg
// TO:   http://q.i02.wimg.taobao.com/bao/uploaded/i1/846481524/T2dgK_XmxXXXXXXXXX_!!846481524.jpg
void ConvertTaobaoImageUrl(const std::string &raw, std::string *new_url) {
  CHECK_NOTNULL(new_url);
  GURL gurl(raw);
  const std::string &host = gurl.host();
  const std::string &path = gurl.path();
  std::string::size_type pos = path.find(".");
  if (pos == std::string::npos) {
    new_url->assign(raw);
  } else {
    new_url->assign("http://" + host + path.substr(0, pos) + path.substr(pos, 4));
  }
}

}  // end namespace
