#include "log_analysis/common/log_common.h"
#include "base/common/basic_types.h"
#include "base/testing/gtest.h"
#include "base/time/time.h"
#include "base/strings/string_split.h"
#include "base/file/file_util.h"
#include "base/hash_function/md5.h"
#include "nlp/common/nlp_util.h"
#include "base/encoding/line_escape.h"

namespace log_analysis {
TEST(ParseVerticalQuery, cases) {
  struct UrlQuery {
    const char* url;
    const char* query;
    const char* site;
  } cases[] = {
    {"http://so.tv.sohu.com/mts?c=1&wd=%25u5B8B%25u6167%25u4E54", "宋慧乔", "Sohu"},
    {"http://so.tv.sohu.com/mts?box=1&wd=%208%u6BEB%u7C73", "8毫米", "Sohu"},
    {"http://so.iqiyi.com/so/q_%20%E5%BF%85%E8%83%9C%E5%A5%89%E9%A1%BA%E8%8B%B1_f_2", "必胜奉顺英", "Qiyi"},
    {"http://www.iqiyi.com/dianying/20100819/n30547.html?pltfm=11&furl=http://so.iqiyi.com/so/q_%E5%90%8C%E6%80%A7%E6%81%8B%E5%A5%B3%E7%94%B5%E5%BD%B1_f_2&type=fltl&area=phtr&pos=ctnvw", "同性恋女电影", "Qiyi"},  // NOLINT
    {"http://hz.youku.com/red/click.php?tp=1&cp=4003338&cpp=1000449&url=http://www.soku.com/search_video/q_%E5%AF%86%E7%88%B1", "密爱", "Youku"},   // NOLINT
    {"http://www.soku.com/search_video/q_%E2%80%96VeeKo%E2%80%96_orderby_1_page_4", "‖veeko‖", "Youku"},
    {"http://www.soku.com/search_video/q_vogue+%E8%8A%B1%E7%B5%AE_orderby_1_page_11", "vogue 花絮", "Youku"},
    {"http://so.tv.sohu.com/mts?&wd=%u51EF%u8389", "凯莉", "Sohu"},
    {"http://so.tv.sohu.com/mts?c=1&wd=%u5B8B%u6167%u4E54", "宋慧乔", "Sohu"},
    {"http://so.tv.sohu.com/mts?box=1&wd=%u4E0E%u6211%u540C%u7720", "与我同眠", "Sohu"},
    {"http://so.tudou.com/nisearch.do?kw=%BA%F3%B9%AC", "后宫", "Tudou"},

    // 阅读类
    {"http://sosu.qidian.com/searchresult.aspx?searchkey=%E8%AF%9B%E4%BB%99&searchtype=%E7%BB%BC%E5%90%88", "诛仙", "Qidian"},  // NOLINT
    {"http://novel.hongxiu.com/shuku/searchresult.aspx?iftitle=1&query=%D6%EF%CF%C9", "诛仙", "Hongxiu"},
    {"http://s.readnovel.com/web/search.php?searchconditions=0&keywords=%D6%EF%CF%C9&button=%CB%D1+%CB%F7", "诛仙", "Readnovel"},  // NOLINT
    // 电商类
    {"http://search.360buy.com/Search?keyword=%CD%F8%BC%FEJWNR2000", "网件jwnr2000", "360buy"},
    {"http://searchb.dangdang.com/?key=%B2%CC%BF%B5%D3%C0", "蔡康永", "Dangdang"},
    {"list.tmall.com/search_product.htm?q=u%C5%CC&user_action=initiative", "u盘", "Tmall"},

    // 软件类
    {"http://www.hackol.com/search.asp?m=0&s=0&word=qq%D3%CE%CF%B7", "qq游戏", "Hackol"},
    {"http://www.skycn.com/search.php?ss_name=%CE%A2%C8%EDoffice&sf=index", "微软office", "Skycn"},
    {"http://search.newhua.com/search_list.php?searchname=360%E5%AE%89%E5%85%A8%E5%8D%AB%E5%A3%AB", "360安全卫士", "Newhua"},  // NOLINT
    {"http://www.xiazaiba.com/word/%CB%A2%BB%FA_%BE%AB%C1%E9", "刷机_精灵", "Xiazaiba"},

    // 游戏类
    {"http://so2.4399.com/search/search.php?k=%B3%E8%CE%EF%C1%AC%C1%AC%BF%B42.5%B0%E6", "宠物连连看2.5版", "4399"},  // NOLINT
    {"http://so.7k7k.com/game/%E5%86%92%E9%99%A9%E5%B2%9B.htm", "冒险岛", "7k7k"},
    {"http://search.17173.com/jsp/game.jsp?charset=gbk&keyword=%C9%F1%CF%C9%B4%AB", "神仙传", "17173"},
    {"http://so.yxdown.com/s_%25u6781%25u54C1%25u98DE%25u8F66_soft.html", "极品飞车", "Yxdown"},
    {"http://ks.pcgames.com.cn/?q=%C8%FD%B9%FA%D6%BE&key=cms", "三国志", "Pcgames"},
    {"http://youxi.zol.com.cn/index.php?c=Se&keyword=%D5%F7%CD%BE", "征途", "Zol"},
  };
  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); i++) {
    std::string query;
    std::string site;
    IsSiteInternalSearch(cases[i].url, &query, &site);
    std::cout << query << std::endl;
    EXPECT_EQ(query, cases[i].query) << query;
    EXPECT_EQ(site, cases[i].site);
  }
}

DEFINE_string(video_query_file, "log_analysis/common/data/qiyi", "video query for test");

TEST(ParseVerticalQuery, huge) {
  // TODO(gaojianhuang)
  // 拿一些实际数据,跑一下解析,看一下
  // 1. 正则的命中情况, 是否要改进正则
  // 2. query 解析的结果,是否有编码问题
  std::vector<std::string> lines;
  base::file_util::ReadFileToLines(FLAGS_video_query_file, &lines);
  std::string query;
  std::string site;
  int error_num = 0, success_num = 0;
  for (int i = 0; i < (int)lines.size(); ++i) {
    if (IsSiteInternalSearch(lines[i], &query, &site)) {
      // std::cout << site << "\t" << query <<std::endl;
      success_num++;
    } else {
      error_num++;
      // std::cout << "error case" << std::endl;
    }
  }
  std::cout << "success_num: " << success_num << "\terror_num: " << error_num << "\t";
}
}
