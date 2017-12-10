#include <string>
#include <iostream>

#include "base/common/logging.h"
#include "base/common/basic_types.h"
#include "base/strings/string_util.h"
#include "base/encoding/base64.h"
#include "base/encoding/cescape.h"
#include "base/encoding/url_encode.h"
#include "base/strings/utf_codepage_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"
#include "web/url/url.h"
#include "web/url/url_util.h"
#include "base/time/time.h"
#include "nlp/common/nlp_util.h"

#include "log_analysis/common/log_common.h"

namespace log_analysis {
struct SearchEngine {
  const char* host_prefix;
  const char* host_postfix;
  const char* path;
  const char* query_key;
  const char* search_engine;
};

// 当 host_prefix  非空，用其去匹配 URL host 的前缀
// 当 host_postfix 非空，用其去匹配 URL host 的后缀
// 这两个字段不能同时为 非空，否则函数行为未定义
static const SearchEngine kGeneralSearchBook[] = {
  {"www.google.",           NULL, "/search",       "q",     kGoogle},
  {"www.google.",           NULL, "/",             "q",     kGoogle},
  {"www.baidu.",            NULL, "/s",            "wd",    kBaidu},
  {"www.baidu.",            NULL, "/s",            "word",  kBaidu},
  {"www.baidu.",            NULL, "/baidu",        "wd",    kBaidu},
  {"www.baidu.",            NULL, "/baidu",        "word",  kBaidu},
  {"baidu.com",             NULL, "/s",            "wd",    kBaidu},
  {"baidu.com",             NULL, "/s",            "word",  kBaidu},
  {"baidu.com",             NULL, "/baidu",        "wd",    kBaidu},
  {"baidu.com",             NULL, "/baidu",        "word",  kBaidu},
  {"www.sogou.",            NULL, "/web",          "query", kSogou},
  {"www.sogou.",            NULL, "/sogou",        "query", kSogou},
  {"www.sogou.",            NULL, "/sohu",         "query", kSogou},
  {"www.bing.com",          NULL, "/search",       "q",     kBing},
  {"cn.bing.com",           NULL, "/search",       "q",     kBing},
  {"cn.bing.com",           NULL, "/",             "q",     kBing},
  {"cn.bing.com",           NULL, "/results.aspx", "q",     kBing},
  {"cnweb.search.live.com", NULL, "/results.aspx", "q",     kBing},
  {"search.live.com",       NULL, "/results.aspx", "q",     kBing},
  {"go.microsoft.com",      NULL, "/fwlink/",      "q",     kBing},
  {"auto.search.msn.com",   NULL, "/response.asp", "q",     kBing},
  {"auto.search.msn.com",   NULL, "/search",       "q",     kBing},
  {"g.msn.com",             NULL, "/8SEZHCN030000TBR/TOOLBRSMenu1",     "q",     kBing},
  {"www.soso.com",          NULL, "/q",            "w",     kSoso},
  {NULL,    "search.yahoo.com",   "/search",       "p",     kYahoo},
  {"www.youdao.com",        NULL, "/search",       "q",     kYoudao},
};

static const SearchEngine kVerticalSearchBook[] = {
  {"tuan.etao.com",       NULL, "/list.htm",    "q",     kTaobaoTuan},
  {"price.etao.com",      NULL, "/search.htm",  "q",     kTaobaoPrice},
  {"news.baidu.com",      NULL, "/ns",          "word",  kBaiduNews},
  {"mp3.baidu.com",       NULL, "/m",           "word",  kBaiduMp3},
  {"image.baidu.com",     NULL, "/i",           "word",  kBaiduImage},
  {"image.baidu.com",     NULL, "/",            "q",     kBaiduImage},
  {"video.baidu.com",     NULL, "/v",           "word",  kBaiduVideo},
  {"video.baidu.com",     NULL, "/s",           "word",  kBaiduVideo},
  {"zhidao.baidu.com",    NULL, "/q",           "word",  kBaiduZhidao},
  {"baike.baidu.com",     NULL, "/w",           "word",  kBaiduBaike},
  {"baike.baidu.com",     NULL, "/search/word", "word",  kBaiduBaike},
  {"baike.baidu.com",     NULL, "/search",      "word",  kBaiduBaike},
  {"wenku.baidu.com",     NULL, "/search",      "word",  kBaiduWenku},
  {"dict.baidu.com",      NULL, "/s",           "wd",    kBaiduDict},
  {"blog.soso.com",       NULL, "/qz.q",        "w",     kSosoBlog},
  {"game.youdao.com",     NULL, "/search",      "q",     kYoudaoGame},
  {NULL,    "taobao.com",       "/search",      "q",     kTaobao},
  {NULL,    "etao.com",         "/search",      "q",     kTaobao},
};

struct Ads {
  const char* host_prefix;
  const char* host_postfix;
  const char* path;
  const char* src;
};

// 当 host_prefix  非空，用其去匹配 URL host 的前缀
// 当 host_postfix 非空，用其去匹配 URL host 的后缀
// 这两个字段不能同时为 非空，否则函数行为未定义
static const Ads kAdsBook[] = {
  {"www.baidu.",          NULL,  "/baidu.php",        "Baidu"},
  {"www.baidu.",          NULL,  "/adrc.php",         "Baidu"},
  {"www.baidu.",          NULL,  "/cb.php",           "Baidu"},
  {"www.baidu.",          NULL,  "/cpro.php",           "Baidu"},
  {"www.google.",         NULL,  "/aclk",             "Google"},
};

static bool ParseSearchQuery(const std::string& orig_url,
                             const SearchEngine code_book[],
                             int code_book_item_num,
                             std::string* query,
                             std::string* search_engine) {
  query->clear();
  search_engine->clear();
  int item_num = code_book_item_num;

  // google 首页的 url 比较特殊，query 出现在 ref 中
  // 为了能够将其纳入统一的处理体系，这里将 # 改为 ?
  // TODO(zhengying): 需要监控规则的变化
  std::string url = orig_url;
  if (url.find("http://www.google.com.hk/#") == 0) {
    url[25] = '?';
  }

  GURL gurl(url);
  if (!gurl.has_host() || !gurl.has_path() || !gurl.has_query()) {
    DLOG(INFO) << "host/path/query incomplete in url: " << url;
    return false;
  }

  // chec if url match the search engine book
  std::string host = gurl.host();
  std::string path = gurl.path();
  base::LowerString(&path);
  std::vector<const SearchEngine*> found_items;
  for (int i = 0; i < item_num; ++i) {
    const SearchEngine& item = code_book[i];
    DCHECK(item.host_prefix == NULL || item.host_postfix == NULL);
    if (NULL != item.host_prefix) {
      if (host.find(item.host_prefix) == 0 &&
          path == item.path) {
        found_items.push_back(&item);
      }
    } else if (NULL != item.host_postfix) {
      if (host.find(item.host_postfix) != std::string::npos &&
          host.find(item.host_postfix) + strlen(item.host_postfix) == host.size()
          && path == item.path) {
        found_items.push_back(&item);
      }
    }
  }
  if (found_items.empty()) {
    DLOG(INFO) << StringPrintf("host [%s] in url [%s] not match search engine book",
                               host.c_str(), url.c_str());
    return false;
  }

  // parse url query, to get the field related to search query
  std::string search_query;
  url_parse::Component url_query = gurl.parsed_for_possibly_invalid_spec().query;
  const char* spec = gurl.spec().c_str();
  url_parse::Component key, value;
  const SearchEngine* found_item = NULL;
  while (url_parse::ExtractQueryKeyValue(spec, &url_query, &key, &value)) {
    std::string url_key = base::Slice(spec+key.begin, key.len).as_string();
    base::LowerString(&url_key);
    for (int i = 0; i < (int)found_items.size() && found_item == NULL; ++i) {
      // if (strncmp(&spec[key.begin], found_items[i]->query_key, key.len) == 0) {
      if (url_key == found_items[i]->query_key) {
        found_item = found_items[i];
        search_query.assign(&spec[value.begin], value.len);
        // DLOG(INFO) << StringPrintf("found value [%s] for key [%s] in url [%s]",
        //                            search_query.c_str(), found_item->query_key, url.c_str());
      }
    }
    if (found_item != NULL) {
      break;
    }
  }

  if (found_item == NULL) {
    DLOG(INFO) << StringPrintf("key not found in query of url [%s]", url.c_str());
    return false;
  }
  // decode the search query, and convert to UTF8
  std::string unquote_query;
  if (!base::DecodeUrlComponentWithBestEffort(search_query.c_str(), search_query.length(), &unquote_query)) {
    DLOG(WARNING) << "failed to decode query: " << search_query;
    return false;
  }
  std::string utf8_search_query;
  const char* final_code = base::HTMLToUTF8(unquote_query, "", &utf8_search_query);
  if (final_code == NULL) {
    DLOG(WARNING) << StringPrintf("[CHECK] failed to convert to utf8 [%s]", search_query.c_str());
    return false;
  } else {
    DLOG(INFO) << StringPrintf("query [%s], encoding [%s]", utf8_search_query.c_str(), final_code);
  }
  nlp::util::NormalizeLineCopy(utf8_search_query, query);
  if (!query->empty()) {
    *search_engine = found_item->search_engine;
    return true;
  } else {
    return false;
  }
}

bool IsGeneralSearch(const std::string& url,
                     std::string* query,
                     std::string* search_engine) {
  std::string q;
  std::string s;
  CHECK((query && search_engine)
        || (!query && !search_engine));

  bool ret = ParseSearchQuery(url, kGeneralSearchBook,
                              arraysize(kGeneralSearchBook), &q, &s);
  if (ret && query) {
    *query = q;
    *search_engine = s;
  }
  return ret;
}

bool IsVerticalSearch(const std::string& url,
                     std::string* query,
                     std::string* vertical_search) {
  std::string q;
  std::string s;
  CHECK((query && vertical_search)
        || (!query && !vertical_search));

  bool ret = ParseSearchQuery(url, kVerticalSearchBook,
                              arraysize(kVerticalSearchBook), &q, &s);
  if (ret && query) {
    *query = q;
    *vertical_search = s;
  }
  return ret;
}

bool IsAds(const std::string& orig_url, std::string* src) {
  if (src != NULL) {
    *src = "";
  }

  int item_num = arraysize(kAdsBook);

  std::string url = orig_url;

  GURL gurl(url);
  if (!gurl.has_host() || !gurl.has_path() || !gurl.has_query()) {
    DLOG(INFO) << "host/path/query incomplete in url: " << url;
    return false;
  }

  // chec if url match the search engine book
  std::string host = gurl.host();
  std::string path = gurl.path();
  const Ads* found_item = NULL;
  for (int i = 0; i < item_num; ++i) {
    const Ads& item = kAdsBook[i];
    DCHECK(item.host_prefix == NULL || item.host_postfix == NULL);
    if (NULL != item.host_prefix) {
      if (host.find(item.host_prefix) == 0 &&
          path == item.path) {
        found_item = &item;
        break;
      }
    } else if (NULL != item.host_postfix) {
      if (host.find(item.host_postfix) != std::string::npos &&
          host.find(item.host_postfix) + strlen(item.host_postfix) == host.size()
          && path == item.path) {
        found_item = &item;
        break;
      }
    }
  }
  if (NULL == found_item) {
    DLOG(INFO) << StringPrintf("host [%s] in url [%s] not match search engine book",
                               host.c_str(), url.c_str());
    return false;
  }

  if (src != NULL) {
    *src = found_item->src;
  }
  return true;
}

bool IsGeneralSearch(const char* se) {
  if (se == kBaidu  || strcasecmp(se, kBaidu) == 0  ||
      se == kGoogle || strcasecmp(se, kGoogle) == 0 ||
      se == kBing   || strcasecmp(se, kBing) == 0   ||
      se == kSoso   || strcasecmp(se, kSoso) == 0   ||
      se == kSogou  || strcasecmp(se, kSogou) == 0  ||
      se == kYahoo  || strcasecmp(se, kYahoo) == 0) {
    return true;
  } else {
    return false;
  }
}

bool IsGeneralSearchClick(const std::string& orig_target_url, const std::string& referer_url,
                                 std::string* query, std::string* search_engine) {
  std::string qry, se;
  // referer 不是通用搜索搜索页面
  if (!IsGeneralSearch(referer_url, &qry, &se)) {
    return false;
  }

  // 目标页是通用/垂直搜索引擎
  if (IsGeneralSearch(orig_target_url, NULL, NULL)
      || IsVerticalSearch(orig_target_url, NULL, NULL)) {
    return false;
  }

  // 目标页是广告
  if (IsAds(orig_target_url, NULL)) {
    return false;
  }

  // 对百度 google bing 的搜索结果做进一步的过滤
  int type = -1;
  if (se == kGoogle) {
    type = 0;
  } else if (se == kBaidu) {
    type = 1;
  } else if (se == kBing) {
    type = 2;
  }

  if (type >= 0) {
    // 对于在自家搜索引擎上搜自家的情况,不走下面的域名过滤
    bool special_case = false;
    if ((type == 0 && (qry.find("谷歌") != std::string::npos || qry.find("google") != std::string::npos))
        || (type == 1 && (qry.find("百度") != std::string::npos
                          || qry.find("baidu") != std::string::npos
                          || qry.find("hao123") != std::string::npos))
        || (type == 2 && (qry.find("必应") != std::string::npos || qry.find("bing") != std::string::npos))) {
      special_case = true;
    }
    // 滤除 target url 是频道页的情况
    if (!special_case) {
      std::string ref_domain;
      if (type == 0) {
        ref_domain = ".google.";
      } else if (type == 1) {
        ref_domain = ".baidu.";
      } else if (type == 2) {
        ref_domain = ".bing.";
      } else {
        CHECK(false);
      }

      std::string target_url = orig_target_url;
      if (type == 0) {
        target_url = GoogleTargetUrl(orig_target_url);
      }
      if (type == 1) {
        target_url = BaiduTargetUrl(orig_target_url);
      }

      GURL gurl(target_url);
      std::string target_host = gurl.host();
      std::string target_query = gurl.query();
      // ref 是百度, 点击 贴吧结果, 由于无法分辨是否在搜索结果中,就不过滤
      if ((type == 1 && target_host == "tieba.baidu.com")) {
        // 无搜索词的贴吧结果予以过滤
        std::size_t pos1 = target_query.find("kw=&");
        std::size_t pos2 = target_query.find("kw=");
        if (pos1 != std::string::npos
            || (pos2 != std::string::npos && pos2+3 == target_query.size())) {
          return false;
        }
      } else {
        // 目标 host 中包含 ref domain, 且 url query 不为空, 是垂直搜索
        if (target_host.find(ref_domain) != std::string::npos && !target_query.empty()) {
          return false;
        }
      }

      // 百度搜索, 过滤点击首页和登录页
      if (type == 1
          && (target_host == "www.baidu.com"
              || target_host == "passport.baidu.com"
              || target_host == "www.hao123.com")) {
        return false;
      }

      // 谷歌搜索, 只要目标 host 中包含 ref domain, 不管 url query 是否为空 都要过滤
      if (type == 0 && target_host.find(ref_domain) != std::string::npos) {
        return false;
      }
      if (type == 0 && target_host == "webcache.googleusercontent.com") {
        return false;
      }
    }
  }
  if (query) *query = qry;
  if (search_engine) *search_engine = se;
  return true;
}

}  // namespace log_analysis
