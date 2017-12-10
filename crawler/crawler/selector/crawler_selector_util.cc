#include "crawler/selector/crawler_selector_util.h"

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <string>

#include "web/url/url.h"
#include "web/url_norm/url_norm.h"
#include "base/common/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_tokenize.h"
#include "base/strings/string_number_conversions.h"
#include "log_analysis/common/log_common.h"
#include "crawler/api/base.h"
#include "crawler/proto/crawled_resource.pb.h"

namespace crawler {

struct FilterRule {
  const char* host_prefix;
  const char* host_postfix;
  const char* path;
  // XXX: 当 query 取值为 NULL 时, 表示只要目标 URL 的 query 为非空, 就算 query 匹配成功
  const char* query;
  const char* search_engine;
};

// host 在黑名单中的 URL 将 被过滤掉
static const char* kHostBlackList[] = {"ptlogin2.qq.com", "ptlogin2.3366.com", "img03.taobaocdn.com",
  "cache.baidu.com",
  "ptlogin2.paipai.com", "webcache.googleusercontent.com", "snapshot.soso.com", "newscache.baidu.com"};

// host 在受限名单中的 URL 中, 会过滤掉所有非首页的 URLs
static const char* kHostRestrictList[] = {"weibo.com", "www.kaixin001.com", "p.777wyx.com", "g.zx915.com",
  "hero.qzoneapp.com"};

static const char* kPathPostfixBlackList[] = {".do", ".doc", ".mp3", ".avi", ".exe", ".zip",
  ".xls", ".ppt", ".rar", ".pdf", ".dwg",
  ".swf", ".wps", ".o", ".cc", ".cpp", ".cc", ".h", ".dll"};
static const char* kPathPostfixBlackList2[] = {"search.htm", "search.php",
  "search.asp", "search.html", "redirects", ".do",
  ".doc", ".mp3", ".avi", ".exe", ".zip", ".xls", ".ppt", ".rar", ".pdf", ".dwg", ".swf", ".wps", ".o", ".cc",
  ".cpp", ".cc", ".h", ".dll"};

static const char* kFilterQueryArray[] = {"login", "register", "url=", "redirecturl=", "targeturl=",
  "action=", "logout", "cdnurl=", "order_id=", "trade_id=", "tradeid=", "attachmentid="};
static const char* kFilterQueryArray2[] = {"login", "register", "keyword=", "keywords=", "kw=", "key=", "wd=",
  "word=", "query=", "q=", "search=", "url=", "redirecturl=", "targeturl=", "cdnurl=", "action=",
  "logout", "order_id=", "trade_id=", "tradeid=", "attachmentid="};

// 当 host_prefix  非空，用其去匹配 URL host 的前缀
// 当 host_postfix 非空，用其去匹配 URL host 的后缀
// 这两个字段不能同时为 非空，否则函数行为未定义
static const FilterRule kFilterRuleBook[] = {
  // 需要登录
  {NULL,                 ".com",               "/*login*",                  "*",          "Login"},
  {NULL,                 ".cn",                "/*login*",                  "*",          "Login"},
  {NULL,                 ".taobao.com",        "/account/*",                "*",          "Login"},
  {NULL,                 ".taobao.com",        "/auction/*",                "*",          "Login"},
  {NULL,                 ".taobao.com",        "/user/order_detail*",       "*",          "Login"},
  {NULL,                 ".tmall.com",         "/detail/orderDetail.*",     "*",          "Login"},
  {NULL,                 ".alibaba.com",       "/order/*",                  "*",          "Login"},
  {NULL,                 ".alibaba.com",       "/member/signin*",           "*",          "Login"},
  {NULL,                 ".alibaba.com",       "/favorites/add_to_*",       "*",          "Login"},
  {NULL,                 ".alibaba.com",       "/offer/post/fill_*",        "*",          "Login"},
  {NULL,                 ".alipay.com",        "/standard/payment/*",       "*",          "Login"},
  {NULL,                 ".qq.com",            "/cn2/findpsw/*",            "*",          "Login"},
  {NULL,                 ".profile.live.com",  "*",                         "*",          "Login"},
  // 垃圾游戏界面
  {"p.777wyx.com",        NULL,                "*",                         "*",          "RubishGame"},
  // Google 广告
  {"www.google.",         NULL,                "/aclk",                     "*",          "Google"},
  {"map.baidu.com",       NULL,                "/",                         NULL,         "Baidu"},
  // Baidu 广告
  {"www.baidu.",          NULL,                "/cpro.php",                 NULL,         "Baidu"},
  {"www.baidu.",          NULL,                "/baidu.php",                NULL,         "Baidu"},
  {"www.baidu.",          NULL,                "/adrc.php",                 NULL,         "Baidu"},
  {"www.baidu.",          NULL,                "/cb.php",                   NULL,         "Baidu"},
  {"passport.baidu.com",  NULL,                "/",                         NULL,         "Baidu"},
  {"v.sogou.",            NULL,                "/mlist/*",                  "*",          "Sogou"},
  // Sogou 广告
  {"www.sogou.com",       NULL,                "/bill_search",              "*",          "SogouAds"},
  {"map.soso.com",        NULL,                "/",                         NULL,         "Soso"},
  // Soso 广告
  {"jzclick.soso.com",    NULL,                "/click",                    "*",         "SosoAds"},
  {"s.click.taobao.com",  NULL,                "/t_3",                      "*",          "Taobao"},
  {"trade.taobao.com",    NULL,                "/trade/*",                  "*",          "Taobao"},
  {NULL,                  ".sina.com.cn",      "/search*",                  NULL,         "Sina"},
  {NULL,                  ".sina.com.cn",      "/question/ask_new*",        NULL,         "Sina"},
  {"www.uqude.com",       NULL,                "/content/getSolr.action",   "*",          "Uqude"},
  {"www.uqude.com",       NULL,                "/search*",                  "*",          "Uqude"},
  {"link.admin173.com",   NULL,                "/index.php",                NULL,         "Link173"},
  {"www.kaixin001.com",   NULL,                "/login/*",                  "*",          "Kaixin"},
  {"www.168dushi.com.",   NULL,                "/czfy*",                    "*",          "168dushi"},
  {"car.autohome.com",    NULL,                "/price/list-*",             "*",          "Autohome"},
  {NULL,                  ".auto.sohu.com",    "/searchterm.sip",           NULL,         "Sohu"},
  {"product.it.sohu.com",     NULL,            "/search/*",                 "*",          "Sohu"},
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
  {"translate.google",    NULL,                "*",                         NULL,         "GoogleTranslate"},
  // Baidu 推广
  {"e.baidu.com",         NULL,                "*",                         NULL,         "BaiduTuiguang"},
  // Bing 翻译
  {"www.microsofttranslator.com",  NULL,       "*",                         NULL,         "BingFanyi"},
  // Bing 快照
  {"cc.bingj.com",        NULL,                "/cache.*",                  "*",          "BingCache"},
  // Bing 广告
  {"adredir.adcenter.bing.",       NULL,       "/redir",                    "*",          "BingAds"},
  {"www.content4ads.com",          NULL,       "/live.php",                 "*",          "BingAds"},

  // Sogou 快照
  {"www.sogou.com",       NULL,                "/websnapshot",              "*",          "SogouCache"},
  // 百度知道问题分类页
  {"zhidao.baidu.com",    NULL,                "/browse/*",                 "*",          "ZhidaoBrowse"},
  // youdao 广告
  {"clkservice.youdao.com",       NULL,        "/clk/request.s",            "*",          "YoudaoAds"},
  // youdao 快照
  {"www.youdao.com",       NULL,        "/cache",            "*",          "YoudaoCache"},
};

static const FilterRule kFilterRuleBook2[] = {
  {NULL,                 ".com",               "/*login*",                  "*",          "Login"},
  {NULL,                 ".cn",                "/*login*",                  "*",          "Login"},
  {NULL,                 ".taobao.com",        "/account/*",                "*",          "Login"},
  {NULL,                 ".taobao.com",        "/auction/*",                "*",          "Login"},
  {NULL,                 ".taobao.com",        "/user/order_detail*",       "*",          "Login"},
  {NULL,                 ".tmall.com",         "/detail/orderDetail.*",     "*",          "Login"},
  {NULL,                 ".alibaba.com",       "/order/*",                  "*",          "Login"},
  {NULL,                 ".alibaba.com",       "/member/signin*",           "*",          "Login"},
  {NULL,                 ".alibaba.com",       "/favorites/add_to_*",       "*",          "Login"},
  {NULL,                 ".alibaba.com",       "/offer/post/fill_*",        "*",          "Login"},
  {NULL,                 ".alipay.com",        "/standard/payment/*",       "*",          "Login"},
  {NULL,                 ".qq.com",            "/cn2/findpsw/*",            "*",          "Login"},
  {NULL,                 ".profile.live.com",  "*",                         "*",          "Login"},
  {"www.google.",         NULL,                "/search",                   "*",          "Google"},
  {"www.google.",         NULL,                "/",                         "*",          "Google"},
  {"www.google.",         NULL,                "/aclk",                     "*",          "Google"},
  {"www.baidu.",          NULL,                "/cpro.php",                 NULL,         "Baidu"},
  {"www.baidu.",          NULL,                "/baidu.php",                NULL,         "Baidu"},
  {"www.baidu.",          NULL,                "/adrc.php",                 NULL,         "Baidu"},
  {"www.baidu.",          NULL,                "/cb.php",                   NULL,         "Baidu"},
  {"passport.baidu.com",  NULL,                "/",                         NULL,         "Baidu"},
  {"zhidao.baidu.",       NULL,                "/q",                        "*",          "Baidu"},
  {"map.baidu.com",       NULL,                "/",                         NULL,         "Baidu"},
  {"news.baidu.com",      NULL,                "/ns",                       "*",          "Baidu"},
  {"v.sogou.",            NULL,                "/mlist/*",                  "*",          "Sogou"},
  // Sogou 广告
  {"www.sogou.com",       NULL,                "/bill_search",              "*",          "SogouAds"},
  {"www.soso.com",        NULL,                "/q",                        "*",          "Soso"},
  {"wenwen.soso.com",     NULL,                "/z/Search.e",               "*",          "Soso"},
  {"map.soso.com",        NULL,                "/",                         NULL,         "Soso"},
  // Soso 广告
  {"jzclick.soso.com",    NULL,                "/click",                    "*",         "SosoAds"},
  {"search.yahoo.com",    NULL,                "/search;*",                 "*",          "Yahoo"},
  {"s8.taobao.com",       NULL,                "/search",                   "*",          "Taobao"},
  {"trade.taobao.com",    NULL,                "/trade/*",                  "*",          "Taobao"},
  {"s.click.taobao.com",  NULL,                "/t_3",                      "*",          "Taobao"},
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
  // Baidu 推广
  {"e.baidu.com",         NULL,                "*",                         NULL,         "BaiduTuiguang"},
  // Bing 翻译
  {"www.microsofttranslator.com",  NULL,       "*",                         NULL,         "BingFanyi"},
  // Bing 快照
  {"cc.bingj.com",        NULL,                "/cache.*",                  "*",          "BingCache"},
  // Bing 广告
  {"adredir.adcenter.bing.",       NULL,       "/redir",                    "*",          "BingAds"},
  {"www.content4ads.com",          NULL,       "/live.php",                 "*",          "BingAds"},
  // Sogou 快照
  {"www.sogou.com",       NULL,                "/websnapshot",              "*",          "SogouCache"},
  // 百度知道问题分类页
  {"zhidao.baidu.com",    NULL,                "/browse/*",                 "*",          "ZhidaoBrowse"},
  // youdao 广告
  {"clkservice.youdao.com",       NULL,        "/clk/request.s",            "*",          "YoudaoAds"},
  // youdao 快照
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
static bool IsUrlPathValid2(const std::string &input_path) {
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
  int item_num = arraysize(kPathPostfixBlackList2);
  for (int i = 0; i < item_num && !found; ++i) {
    if (base::EndsWith(path, kPathPostfixBlackList2[i], false)) {
      DLOG(INFO) << "Invalid url path: path in kPathPostfixBlackList2, path: " << path;
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
static bool IsUrlQueryValid2(const std::string &query) {
  if (query.empty()) return true;
  if ((int)query.size() > kMaxQueryLength) {
    DLOG(INFO) << "Invalid url query: Exceed Max Query length(1000), query: " << query;
    return false;
  }

  std::string myquery(query);
  base::LowerString(&myquery);
  std::string::size_type pos = 0;
  bool found = false;
  int item_num = arraysize(kFilterQueryArray2);
  for (int i = 0; i < item_num && !found; ++i) {
    pos = myquery.find(kFilterQueryArray2[i]);
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
                           const std::string &query, std::string *search_engine) {
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
  if (NULL != search_engine) *search_engine = found_item->search_engine;
  return true;
}
static bool IsSearchResult2(const std::string &host, const std::string &path,
                           const std::string &query, std::string *search_engine) {
  int item_num = arraysize(kFilterRuleBook2);
  const FilterRule* found_item = NULL;
  for (int i = 0; i < item_num; ++i) {
    const FilterRule& item = kFilterRuleBook2[i];
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
  if (NULL != search_engine) *search_engine = found_item->search_engine;
  return true;
}

// |strict_rules| 为 true 时, 使用跟严格的过滤规则
bool WillFilterAccordingRules(const std::string &orig_url, std::string *reason, bool strict_rules) {
  std::string url(orig_url);
  if (!web::has_scheme(url)) {
    url = "http://" + url;
  }
  // Check Max length of URL allowed
  if ((int)url.size() > crawler::kMaxLinkSize) {
    if (reason != NULL) {
      *reason = "url exceeds max url lenght limit(2KB), url: " + url;
    }
    return true;
  }
  // Bad Url format like: http://%20www.sina.com.cn or http://.www.sohu.com/
  if (base::StartsWith(url, "http://.", false) || base::StartsWith(url, "http://%", false) ||
      base::StartsWith(url, "http://+", false)) {
    if (reason != NULL) {
      *reason = "url bad format, start with +, -, . or %, url: " + url;
    }
    return true;
  }
  GURL gurl(url);
  // Check Http Schema
  if (!gurl.SchemeIs("http")) {
    if (reason != NULL) {
      *reason = "Not pass http schema check: only support http, url: " + url;
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
  std::string search_engine;
  if (strict_rules == true) {
    // Check IsHostInBlackList or not
    if (IsHostInBlackList(host)) {
      if (reason != NULL) {
        *reason = "Not pass IsHostInBlackList check: host in blacklist, host: " + host;
      }
      return true;
    }
    // Check Url Path is valid or not
    if (!IsUrlPathValid2(path)) {
      if (reason != NULL) {
        *reason = "Not pass check: IsUrlPathValid(), path: " + path;
      }
      return true;
    }
    // Check Url Query is valid or not
    if (!IsUrlQueryValid2(query)) {
      if (reason != NULL) {
        *reason = "Not pass check: IsUrlQueryValid(), query: " + query;
      }
      return true;
    }
    // Check if Url match the search engine page book
    if (IsSearchResult2(host, path, query, &search_engine)) {
      if (reason != NULL) {
        *reason = "Not pass check: !IsSearchResult(), url: " + url + ", search engine: " + search_engine;
      }
      return true;
    }
  } else {
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
    if (IsSearchResult(host, path, query, &search_engine)) {
      if (reason != NULL) {
        *reason = "Not pass check: !IsSearchResult(), url: " + url + ", search engine: " + search_engine;
      }
      return true;
    }
  }
  return false;
}

static bool IsVIPUrl_Internal(const std::string &url, char from) {
  if (url.empty()) return false;
  // 来之 Search Click 的 Page 时 VIP
  if (from == 'S') return true;
  // nAvi boost
  if (from == 'A') return true;
  // 来之 First-page result URl
  // XXX(pengdan): 这类数据一般时批量生成的, 用户通过 user_input_url 配置将这路数据进入爬虫系统
  if (from == 'E') return true;
  // 站点首页
  // XXX(pengdan): 还没有找到 泛域名的 模式, 这里先放着
  GURL gurl(url);
  const std::string &path = gurl.path();
  if (path == "/") return true;

  return false;
}
bool IsVIPUrl(const std::string &url, int type, char from, const std::string &parent_url, char parent_from) {
  LOG_IF(FATAL, !(type == 1 || type == 2 || type == 3)) << "Invalid url type, should be 1 or 2 or 3";
  if (type == 1) {
    if (IsVIPUrl_Internal(url, from)) return true;
    // 搜索引擎结果页中提取出的 URL
    if (log_analysis::IsGeneralSearch(parent_url, NULL, NULL) ||
        log_analysis::IsVerticalSearch(parent_url, NULL, NULL)) return true;
    return false;
  } else {
    if (IsVIPUrl_Internal(parent_url, parent_from)) return true;
    // 对于 css/image 类型, parent_from 使用 'V' 表示 parent url 为 VIP url
    // 来之 VIP url 的 css/image 也为 VIP 资源
    return parent_from == 'V';
  }
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

// 是否是 有价值的链接
bool IsInvaluableLink(const std::string &url, const std::string &parent) {
  return false;
}


struct ImageLink {
  const char *host;
  const char *path;
};
static const ImageLink kInvaluableImageDict[] = {
  {"tb.himg.baidu.com", "/sys/portrait/item/*"},
};
bool IsValuableImageLink(const std::string &image_url) {
  GURL gurl(image_url);
  if (gurl.is_valid()) return false;
  const std::string &host = gurl.host();
  const std::string &path= gurl.path();
  bool flag = true;
  for (int i = 0; i < (int)arraysize(kInvaluableImageDict); ++i) {
    const std::string &host_pattern = kInvaluableImageDict[i].host;
    const std::string &path_pattern = kInvaluableImageDict[i].path;
    if (base::MatchPattern(host, host_pattern) && base::MatchPattern(path, path_pattern)) {
      flag = false;
      break;
    }
  }
  return flag;
}

}  // end namespace
