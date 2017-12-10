#include "log_analysis/common/url_util.h"

#include <vector>
#include <string>

#include "web/url/gurl.h"
#include "web/url_norm/url_norm.h"
#include "base/common/basic_types.h"
#include "base/common/base.h"
#include "base/common/gflags.h"
#include "base/common/logging.h"
#include "base/strings/string_util.h"
#include "log_analysis/common/log_common.h"
#include "extend/regexp/re3/re3.h"

namespace log_analysis {

namespace util {

struct WorthlessUrl {
  const char* url_reg;
  int reason;
};

const WorthlessUrl kWorthlessUrlsBook[] = {
  // douban
  {".*\\.douban\\.com/search.*", kSearchResult, },
  {".*\\.douban.com.*[\\?#]+.*", kSearchResult, },

  {".*/search\\?.+", kSearchResult, },
  {"so\\.v\\.360\\.cn/.*[\\?#]+.*", kSearchResult, },
  {"search\\.51job\\.com/.*[\\?#]+.*", kSearchResult, },
  {"so\\.pps\\.tv/search.*[\\?#]+.*", kSearchResult, },
  {".*\\.vancl\\.com/search.*[\\?#]+.*", kSearchResult, },
  // 微薄
  {"s\\.weibo\\.com/.+", kSearchResult, },
  // taobao's
  {"s\\.taobao\\.com/.*[\\?#]+.*", kSearchResult, },
  {"s8\\.taobao\\.com/.*[\\?#]+.*", kListPage, },
  {"3c\\.taobao\\.com/list\\.htm.*[\\?#]+.*", kListPage, },
  {"list\\.taobao\\.com/.*[\\?#]+.*", kListPage, },
  {"s\\.click\\.taobao\\.com/.*[\\?#]+.*", kUnknown, },
  // paipai
  {".*\\.paipai\\.com/search_list.*[\\?#]+.*", kListPage, },
  // 登录页
  {".*/login/.*[\\?#]+.*", kPrivatePage, },
  {".*/login\\..*[\\?#]+.*", kPrivatePage, },
  {".*\\?action=login.*", kPrivatePage, },
  {".*&action=login.*", kPrivatePage, },
  // 淘宝店的站内搜索
  {".*\\.taobao\\.com/search.*", kSearchResult, },
  {".*\\.tmall\\.com/search.*", kSearchResult, },
  {".*\\.taobao\\.com/.*search=y.*", kSearchResult, },
  {".*\\.tmall\\.com/.*search=y.*", kSearchResult, },
  {"stat\\.simba\\.taobao\\.com/.*[\\?#]+.*", kSearchResult, },
  // google's
  {"www\\.google\\.com\\.hk/.*[\\?#]+.*", kSearchResult, },
  {"www\\.google\\.com/.*[\\?#]+.*", kSearchResult, },
  {"www\\.g\\.cn/.*[\\?#]+.*", kSearchResult, },
  {"translate\\.google\\.com\\.hk/.*[\\?#]+.*", kToolsPage, },
  {"translate\\.google\\.com\\.tw/.*[\\?#]+.*", kToolsPage, },
  // Baidu's
  {"www\\.baidu\\.com/.*[\\?#]+.*", kSearchResult, },
  {"video\\.baidu\\.com/.*[\\?#]+.*", kSearchResult, },
  {"image\\.baidu\\.com/.*[\\?#]+.*", kSearchResult, },
  {"mp3\\.baidu\\.com/.*[\\?#]+.*", kSearchResult, },
  {"map\\.baidu\\.com/.*[\\?#]+.*", kSearchResult, },
  {"fanyi\\.baidu\\.com/.*[\\?#]+.*", kSearchResult, },
  {"cache\\.baidu\\.com/.*[\\?#]+.*", kSearchResult, },
  // soso's
  {"www\\.soso\\.com/.*[\\?#]+.*", kSearchResult, },
  {"video\\.soso\\.com/.*[\\?#]+.*", kSearchResult, },
  {"music\\.soso\\.com/.*[\\?#]+.*", kSearchResult, },
  {".*\\.music\\.soso\\.com/.*[\\?#]+.*", kSearchResult, },
  {"image\\.soso\\.com/.*[\\?#]+.*", kSearchResult, },
  {"map\\.soso\\.com/.*[\\?#]+.*", kSearchResult, },
  // sogou's
  {"www\\.sogou\\.com/.*[\\?#]+.*", kSearchResult, },
  {"news\\.sogou\\.com/.*[\\?#]+.*", kSearchResult, },
  {"www\\.sogou\\.com/.*[\\?#]+.*", kSearchResult, },
  {"mp3\\.sogou\\.com/.*[\\?#]+.*", kSearchResult, },
  {"pic\\.sogou\\.com/.*[\\?#]+.*", kSearchResult, },
  {"v\\.sogou\\.com/.*[\\?#]+.*", kSearchResult, },
  {"map\\.sogou\\.com/.*[\\?#]+.*", kSearchResult, },
  {"zhishi\\.sogou\\.com/.*[\\?#]+.*", kSearchResult, },
  // 广告联盟, 弹窗
  {"p\\.qiyou\\.com/.*[\\?#]+.*", kAds, },
  {"360\\.70e\\.com/.*[\\?#]+.*", kAds, },
  {"37\\.artemisbuzz\\.com/.*[\\?#]+.*", kAds, },
  {"autohome\\.adsame\\.com/.*[\\?#]+.*", kAds, },
  {".*.ifeng.com/.*pop_page.*", kAds, },
  {"adsrich\\.qq\\.com/.*[\\?#]+.*", kAds, },
  {".*\\.sina\\.com\\.cn/.*adfclick.*", kAds, },
  {"allyes\\.com/.*adfclick.*", kAds, },
  {".*\\.dacais\\.com/.*[\\?#]+.*", kAds, },
  {".*\\.gamebean\\.com/.*[\\?#]+.*", kAds, },
  {".*\\.arpg2\\.com/.*[\\?#]+.*", kAds, },
  {".*\\.doubleclick\\.net/.*[\\?#]+.*", kAds, },
  {"union\\.76mi\\.com/.*[\\?#]+.*", kAds, },
  {"myrice\\.com/.*[\\?#]+.*", kAds, },
  {"pro\\.letv\\.com/.*[\\?#]+.*", kAds, },
  {".*\\.whlongda\\.com/.*[\\?#]+.*", kAds, },
  {"p\\.yocc\\.net/.*[\\?#]+.*", kAds, },
  {"p\\.17kuxun\\.com/.+", kAds, },
  {"ccb\\.com\\.7794\\.com/.+", kAds, },
  {"t\\.7794lm\\.com/.+", kAds, },
  {"www\\.37cs\\.com/.+", kAds, },
  {"cs\\.twcczhu\\.com/.+", kAds, },
  {"www\\.zjump\\.cn/.+", kAds, },
  {"juu\\.tg\\.wulinyingxiong\\.net/.+", kAds, },
  {"game\\.ad1111\\.com/.+", kAds, },
  {"tc\\.100tjs\\.com/.+", kAds, },
  {"click\\.union\\.360buy\\.com/.*[\\?#]+.*", kAds, },
  {"www\\.qq937\\.com/.+", kAds, },
  {"aclick9\\.wisemediakcl\\.com/.+", kAds, },
  {"poufo\\.game3737\\.com/.+", kAds, },
  {"www\\.web887\\.com/.+", kAds, },
  {"member\\.198game\\.com/.+", kAds, },
  {"p\\.xp9365\\.com", kAds, },
  {"qb\\.bestooxx\\.com/.+", kAds, },
  {"code\\.ze5\\.com/.+", kAds, },
  {"vda72\\.uuzu\\.com/.+", kAds, },
  {"ggame\\.gy9y\\.com/.+", kAds, },
  {"ad\\.6zzsf\\.com/.*[\\?#]+.*", kAds, },
  {"t\\.iloveyouxi\\.com/.*[\\?#]+.*", kAds, },
  {"bd\\.too9\\.com/.*[\\?#]+.*", kAds, },
  {"nl\\.tg\\.laolinow\\.com/.+", kAds, },
  {"www\\.xiyou55\\.com/.+", kAds, },
  {"cs\\.37see\\.com/.+", kAds, },
  {"www\\.a135\\.net/.+", kAds, },
  {"tongji\\.wan5d\\.com/.+", kAds, },
  {"ad\\.6cqgd\\.com/.+", kAds, },
  {"p4\\.1lo0\\.net/.+", kAds, },
  {"click\\.tanx\\.com", kAds, },
  {"nl\\.tg\\.mailo2\\.com/.+", kAds, },
  {"101a\\.hlwan\\.net/.+", kAds, },
  {"p3\\.1lo0\\.net/.+", kAds, },
  {"ad\\.ylunion\\.com/.*[\\?#]+.*", kAds, },
  // others
  {"warn\\.se\\.360\\.cn/.*[\\?#]+.*", kUnknown, },
  {"www\\.gmail\\.com/.*[\\?#]+.*", kToolsPage, },
  {"www\\.126\\.com/.*[\\?#]+.*", kToolsPage, },
  {"email\\.163\\.com/.*[\\?#]+.*", kToolsPage, },
  {"reg\\.163\\.com/.*[\\?#]+.*", kPrivatePage, },
  {".*\\.mail\\.163\\.com/.*[\\?#]+.*", kPrivatePage, },
  {"mail\\.qq\\.com/.*[\\?#]+.*", kToolsPage, },
  {"aq\\.qq\\.com/.*[\\?#]+.*", kToolsPage, },   // qq 安全中心
  {"mail\\.sohu\\.com/.*[\\?#]+.*", kToolsPage, },
  {"mail\\.sina\\.net/.*[\\?#]+.*", kToolsPage, },
  {"auction1\\.paipai\\.com/.*[\\?#]+.*", kPrivatePage, },
  {"s\\.taobao\\.com/.*[\\?#]+.*", kSearchResult, },  // 搜索结果,
  {"rate\\.taobao\\.com/.*[\\?#]+.*", kPrivatePage, },
  {"trade\\.taobao\\.com/trade/gametrd.*[\\?#]+.*", kPrivatePage, },
  {"trade\\.taobao\\.com/trade/itemlist.*[\\?#]+.*", kPrivatePage, },
  {"photo\\.renren\\.com/.*[\\?#]+.*", kPrivatePage, },
  {"login\\.taobao\\.com/.*[\\?#]+.*", kPrivatePage, },
  {"buy\\.tmall\\.com/error.htm.*", kPrivatePage, },
  {".*\\.yiqisoo\\.com/.*search.*", kSearchResult, },
  {"search\\.360buy\\.com/.*[\\?#]+.*", kSearchResult, },
  {"search\\.china\\.alibaba\\.com/.*[\\?#]+.*", kSearchResult, },
  // 无限组合的静态搜索, 列表
  // 汽车, 商品, 视频等
  {"car\\.autohome\\.com\\.cn/.*list_.*", kSearchResult, },
  {"car\\.autohome\\.com\\.cn/.*detail_.*", kSearchResult, },
  {"www\\.youku\\.com/v_[^/]*list/.*", kListPage, },
  {"list\\.iqiyi\\.com/.*html", kListPage, },
  {"v\\.360\\.cn/[^/]+/(index|list).php.*[\\?#]+.*", kListPage, },
  {"so\\.tv\\.sohu\\.com/list_.*", kListPage, },
  {"www\\.360buy\\.com/products/.+\\.html", kListPage, },
  {".*\\.taobao\\.com/list\\.htm.*[\\?#]+.*", kListPage, },
  {"category\\.dangdang\\.com/.*[\\?#]+.*", kListPage, },
  {".*\\.dangdang\\.com/list_.+", kListPage, },
  {"searchb\\.dangdang\\.com/.*[\\?#]+.*"},
  {".*\\.xunlei\\.com/[^,]+,.*", kListPage, },
  {".*\\.xunlei\\.com/.*list[_/].+", kListPage, },
  {".*\\.tudou\\.com/(albumtop|top)/.+", kListPage, },
  // 文件后缀
  {".*\\.do([\\?#]+.*)?$", kUnknown, },
  {".*\\.doc([\\?#]+.*)?$", kUnknown, },
  {".*\\.mp3([\\?#]+.*)?$", kUnknown, },
  {".*\\.avi([\\?#]+.*)?$", kUnknown, },
  {".*\\.exe([\\?#]+.*)?$", kUnknown, },
  {".*\\.zip([\\?#]+.*)?$", kUnknown, },
  {".*\\.xls([\\?#]+.*)?$", kUnknown, },
  {".*\\.ppt([\\?#]+.*)?$", kUnknown, },
  {".*\\.rar([\\?#]+.*)?$", kUnknown, },
  {".*\\.pdf([\\?#]+.*)?$", kUnknown, },
  {".*\\.dwg([\\?#]+.*)?$", kUnknown, },
  {".*\\.swf([\\?#]+.*)?$", kUnknown, },
  {".*\\.wps([\\?#]+.*)?$", kUnknown, },
  {".*\\.o([\\?#]+.*)?$", kUnknown, },
  {".*\\.cc([\\?#]+.*)?$", kUnknown, },
  {".*\\.c([\\?#]+.*)?$", kUnknown, },
  {".*\\.cpp([\\?#]+.*)?$", kUnknown, },
  {".*\\.h([\\?#]+.*)?$", kUnknown, },
  {".*\\.dll([\\?#]+.*)?$", kUnknown, },
  {".*\\.tar.gz([\\?#]+.*)?$", kUnknown, },
  {".*\\.tgz([\\?#]+.*)?$", kUnknown, },
  {".*\\.jpg([\\?#]+.*)?$", kUnknown, },
  {".*\\.png([\\?#]+.*)?$", kUnknown, },
  {".*\\.gif([\\?#]+.*)?$", kUnknown, },
};

// 线程安全, 可以用静态处理
extend::re3::Re3** init_worthless_url_book_regexp() {
  extend::re3::Re3** regs = new extend::re3::Re3*[arraysize(kWorthlessUrlsBook)];
  for (int i = 0; i < (int)arraysize(kWorthlessUrlsBook); ++i) {
    regs[i] = new extend::re3::Re3(kWorthlessUrlsBook[i].url_reg);
  }
  return regs;
}

static bool IsGarbage(const std::string& url) {
  // RE2 是线程安全的
  static extend::re3::Re3** reg_vec = init_worthless_url_book_regexp();
  CHECK_NOTNULL(reg_vec);

  std::vector<std::string> tokens;
  for (int i = 0; i < (int)arraysize(kWorthlessUrlsBook); ++i) {
    const extend::re3::Re3* reg_exp = reg_vec[i];
    if (extend::re3::Re3::PartialMatchN(url, *reg_exp, &tokens)) {
      return true;
    }
  }

  return false;
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

bool WorthlessUrlForSE(const std::string& url) {
  GURL gurl(url);
  if (!gurl.is_valid()) {
    LOG(ERROR) << "Abandoned URL: " << url << " is invalid.";
    return false;
  }

  if (IsGarbage(url)) {
    VLOG(1) << "Abandoned URL: " << url << " is garbage for search engine";
    return false;
  }

  if (IsSearchResult(url)) {
    VLOG(1) << "Abandoned URL: " << url << " is search result page.";
    return false;
  }

  if (IsAds(url, NULL)) {
    VLOG(1) << "Abandoned URL: " << url << "is ad";
    return false;
  }

  return true;
}

}  // namespace
}  // namespace
