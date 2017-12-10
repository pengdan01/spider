#include <iostream>
#include <string>

#include "log_analysis/common/log_common.h"
#include "base/common/logging.h"
#include "base/common/basic_types.h"
#include "base/strings/string_util.h"
#include "base/strings/string_split.h"
#include "base/encoding/base64.h"
#include "base/encoding/cescape.h"
#include "base/encoding/url_encode.h"
#include "base/strings/utf_codepage_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"
#include "nlp/common/nlp_util.h"
#include "web/url/gurl.h"
#include "web/url/url_util.h"
#include "base/time/time.h"
#include "extend/regexp/re3/re3.h"

namespace log_analysis {
struct VerticalSite {
  const char* reg;
  int group_id;
  const char* site;
};

static const VerticalSite kVerticalSiteBook[] = {
  // 视频类
  {"so\\.iqiyi\\.com/so/q(_|=|/)([^\\?&_]+)", 1, "Qiyi"},
  {"www\\.soku\\.com/search_video/(type_tag_)?q(_|=)([^\\?&_]+)(&searchdomain=.*)?", 2, "Youku"},
  {"www\\.soku\\.com&searchType=video&(tag_type_)?q(_|=)([^\\?&_]+)", 2, "Youku"},
  {"so\\.tv\\.sohu\\.com/mts\\?(.*)(cat|area|wd)=([^\\?&_]+)", 2, "Sohu"},
  {"so\\.tudou\\.com/n?isearch(/|.do\\?.*kw=)([^\\?&_]+)", 1, "Tudou"},

  // 阅读类
  {"sosu\\.qidian\\.com/searchresult\\.aspx\\?(.*)searchkey=([^\\?&]+)", 1, "Qidian"},
  {"novel\\.hongxiu\\.com/shuku/searchresult\\.aspx\\?(.*)query=([^\\?&]+)", 1, "Hongxiu"},
  {"s\\.readnovel\\.com/web/search\\.php\\?(.*)keywords=([^\\?&]+)", 1, "Readnovel"},

  // 电商类
  {"search\\.360buy\\.com/(.*)\\?(.*)keyword=([^\\?&]+)", 2, "360buy"},
  {"searchb\\.dangdang\\.com/(.*)\\?(.*)key=([^\\?&]+)", 2, "Dangdang"},
  {"tmall\\.com/([^\\?]+)?(.*)q=([^\\?&]+)", 2, "Tmall"},

  // 软件类
  {"www\\.hackol\\.com/search\\.asp\\?(.*)word=([^\\?&]+)", 1, "Hackol"},
  {"www\\.skycn\\.com/search\\.php\\?(.*)ss_name=([^\\?&]+)", 1, "Skycn"},
  {"search\\.newhua\\.com/search_list\\.php\\?(.*)searchname=([^\\?&]+)", 1, "Newhua"},
  {"www\\.xiazaiba\\.com/word/([^\\?/&]+)", 0, "Xiazaiba"},

  // 游戏类
  {"so2\\.4399\\.com/search/search\\.php\\?(.*)k=([^\\?&]+)", 1, "4399"},
  {"so\\.7k7k\\.com/game/([^\\.]+).htm", 0, "7k7k"},
  {"search\\.17173\\.com/jsp/game\\.jsp\\?(.*)keyword=([^\\?&]+)", 1, "17173"},
  {"so\\.yxdown\\.com/s_([^_]+)_soft\\.html", 0, "Yxdown"},
  {"ks\\.pcgames\\.com\\.cn/\\?q=([^\\?&]+)", 0, "Pcgames"},
  {"youxi\\.zol\\.com\\.cn/index\\.php\\?(.*)keyword=([^\\?&]+)", 1, "Zol"},
};

extend::re3::Re3** init_regexp() {
  extend::re3::Re3** regs = new extend::re3::Re3*[arraysize(kVerticalSiteBook)];
  for (int i = 0; i < (int)arraysize(kVerticalSiteBook); ++i) {
    regs[i] = new extend::re3::Re3(kVerticalSiteBook[i].reg);
  }
  return regs;
}

// 处理双重 url encode 的情况
static bool DecodeQuery(const std::string& orig_query, std::string* decoded_query) {
  decoded_query->clear();
  if (base::DecodeUrlComponentWithBestEffort(orig_query.c_str(), orig_query.length(), decoded_query)) {
    int num = 0;
    for (int i = 0; i < (int)decoded_query->size(); ++i) {
      if ((*decoded_query)[i] == '%') num++;
    }
    if (num > int(decoded_query->size() * 0.2)) { // may be double url-encoded  // NOLINT
      std::string str;
      if (base::DecodeUrlComponentWithBestEffort(decoded_query->c_str(), decoded_query->length(), &str)) {
        *decoded_query = str;
      }
    }
    std::string utf8;
    if (NULL == base::HTMLToUTF8(*decoded_query, "", &utf8)) {
      decoded_query->clear();
      DLOG(ERROR) << "faile to convert to utf8: " << decoded_query;
      return false;
    }
    *decoded_query = utf8;
    return true;
  } else {
    return false;
  }
}

static bool DecodeSohuQuery(const std::string& orig_query, std::string* decoded_query) {
  decoded_query->clear();
  std::vector<std::string> chars;
  base::SplitStringWithOptions(orig_query, "%", true, true, &chars);
  if (chars.empty()) return false;
  bool first_is_chinese = (orig_query[0] == '%');
  std::string utf8;
  for (int i = 0; i < (int)chars.size(); ++i) {
    const std::string &buf = chars[i];
    if (i == 0 && !first_is_chinese) {
      *decoded_query += buf;
      continue;
    }
    if (buf.size() < 2) continue;
    if (buf[0] == 'u') {
      if (buf.size() < 5u) {
        DLOG(ERROR) << "hex to int failed: " << chars[i];
        return false;
      }
      int code;
      if (!base::HexStringToInt(buf.substr(1, 4), &code)) {
        DLOG(ERROR) << "hex to int failed: " << chars[i];
        return false;
      }
      wchar_t tmp = code;
      base::WideToUTF8(&tmp, 1, &utf8);
      *decoded_query += utf8;
      if (buf.size() > 5u) {
        *decoded_query += buf.substr(5);
      }
      continue;
    }
    if (buf.substr(0, 3) == "25u") {
      if (buf.size() < 7u) {
        DLOG(ERROR) << "hex to int failed: " << chars[i];
        return false;
      }
      int code;
      if (!base::HexStringToInt(buf.substr(3, 4), &code)) {
        DLOG(ERROR) << "hex to int failed: " << chars[i];
        return false;
      }
      wchar_t tmp = code;
      base::WideToUTF8(&tmp, 1, &utf8);
      *decoded_query += utf8;
      if (buf.size() > 7u) {
        *decoded_query += buf.substr(7);
      }
      continue;
    }
    int code;
    if (!base::HexStringToInt(buf.substr(0, 2), &code)) {
      DLOG(ERROR) << "hex to int failed: " << chars[i];
      return false;
    }
    wchar_t tmp = code;
    base::WideToUTF8(&tmp, 1, &utf8);
    *decoded_query += utf8;
    if (buf.size() > 2u) {
      *decoded_query += buf.substr(2);
    }
  }
  return true;
}

bool IsSiteInternalSearch(const std::string& orig_url, std::string* query,
                            std::string* site) {
  // RE2 是线程安全的
  static extend::re3::Re3** reg_vec = init_regexp();

  query->clear();

  std::vector<std::string> groups;
  std::string decoded;

  for (int i = 0; i < (int)arraysize(kVerticalSiteBook); ++i) {
    const VerticalSite& book = kVerticalSiteBook[i];
    const extend::re3::Re3* reg_exp = reg_vec[i];
    if (extend::re3::Re3::PartialMatchN(orig_url, *reg_exp, &groups)) {
      DCHECK((int)groups.size() >= book.group_id+1);
      const std::string& value = groups[book.group_id];
      bool ret = false;
      if (strncmp(book.site, "Sohu", 4) == 0 ||
          strncmp(book.site, "Yxdown", 6) == 0) {
        ret = DecodeSohuQuery(value, &decoded);
      } else {
        ret = DecodeQuery(value, &decoded);
      }
      if (ret) {
        *site = book.site;
        *query = decoded;
        nlp::util::NormalizeLineInPlaceS(query);
        if (query->empty()) {
          site->clear();
          return false;
        } else {
          return true;
        }
      } else {
        DLOG(ERROR) << "parse query failed: " << value;
      }
    }
    // else if (strncmp(book.site, "Sohu", 4) == 0) {
    //   // 在看特定 case 时,输出不匹配 case,以检查正则的正确性
    //   LOG(ERROR) << base::StringPrintf("parse reg failed: url[%s] reg[%s]",
    //                                    orig_url.c_str(), book.site);
    // }
  }
  return false;
}
}
