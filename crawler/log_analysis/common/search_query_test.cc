#include "log_analysis/common/log_common.h"
#include "base/common/basic_types.h"
#include "base/testing/gtest.h"
#include "base/time/time.h"
#include "base/strings/string_split.h"
#include "base/hash_function/md5.h"
#include "nlp/common/nlp_util.h"
#include "base/encoding/line_escape.h"
#include "base/strings/utf_codepage_conversions.h"
#include "base/encoding/url_encode.h"

namespace log_analysis {
TEST(ParseGeneralSearchQuery, cases) {
  struct UrlQuery {
    const char* url;
    const char* query;
    const char* search_engine;
  } cases[] = {
    {"http://www.baidu.com/s?wd=%EC%C5%CE%E8%B9%D2&opt-webpage=on&ie=gbk", "炫舞挂", kBaidu},
    {"http://www.baidu.com/baidu?cl=3&tn=baidutop10&fr=top1000&wd=%B0%B5%B9%E2%3A%B3%CD%B7%A3&rsv_spt=2&issp=2", "暗光:惩罚", kBaidu}, // NOLINT
    {"http://www.baidu.com/s?wd=a67.com&opt-webpage=on&ie=gbk", "a67.com", kBaidu},
    {"http://www.baidu.com/s?cl=3&wd=郝邵文", "郝邵文", kBaidu},
    {"http://www.baidu.com/s?cl=3&wd=郝邵  文", "郝邵 文", kBaidu},  // NormalizeLine // NOLINT
    {"http://www.baidu.com/s?wd=%CF%E7%B4%E5%B0%AE%C7%E9+%D0%A1%D2%B9%C7%FA&rsv_bp=0&rsv_spt=3", "乡村爱情 小夜曲", kBaidu},  // NOLINT
    {"http://news.baidu.com/ns?word=%B8%DF%CC%FA+%B1%B1%BE%A9+%D6%D0%D1%EB&tn=news&from=news&ie=gb2312&bs=%BA%C3%BA%A2%D7%D3&sr=0&cl=2&rn=20&ct=1&prevct=no", "", ""}, // NOLINT
    {"http://video.baidu.com/v?word=%D3%DE%C8%CB%BD%DA&ct=301989888&rn=20&pn=0&db=0&s=0&fbl=800", "", ""}, // NOLINT
    {"http://zhidao.baidu.com/q?word=%BA%C3%BA%A2%D7%D3&lm=0&fr=search&ct=17&pn=0&tn=ikaslist&rn=10", "", ""}, // NOLINT
    {"http://mp3.baidu.com/m?word=%CD%AC%D7%C0%B5%C4%C4%E3&lm=-1&f=ms&tn=baidump3&ct=134217728&lf=&rn=", "", ""}, // NOLINT
    {"http://image.baidu.com/i?ct=201326592&cl=2&nc=1&lm=-1&st=-1&tn=baiduimage&istype=2&fm=index&pv=&z=0&word=%BA%A9%B6%B9%CF%C8%C9%FA&s=0", "", ""}, // NOLINT
    {"http://video.baidu.com/s?f=1002&id=251558&n=25&word=%B9%AC&ns=2&se=0123", "", ""},
    {"http://baike.baidu.com/w?ct=17&lm=0&tn=baiduWikiSearch&pn=0&rn=10&word=%C0%EE%D0%A1%E8%B4&submit=search", "", ""}, // NOLINT
    {"http://wenku.baidu.com/search?word=%B5%E7%D7%D3%BE%AF%B2%EC%BC%BC%CA%F5&lm=0&od=0&fr=top_home", "", ""}, // NOLINT
    {"http://www.google.com.hk/search?q=%BB%AA%CB%B6&opt-webpage=on&client=aff-360daohang&hl=zh-CN&ie=gb2312&newwindow=1", "华硕", kGoogle},  // NOLINT
    {"http://www.google.com.hk/search?hl=en&newwindow=1&safe=strict&q=hello%20world&bav=on.2,or.r_gc.r_pw.,cf.osb&biw=1920&bih=1096&um=1&ie=UTF-8&tbm=isch&source=og&sa=N&tab=wi&ei=B_V7T4jLPKuUiQeQwNCSCQ", "hello world", kGoogle}, // NOLINT
    {"http://www.google.com/uds/afs??W?", "", ""},
    {"http://www.google.com.hk/#hl=zh-CN&newwindow=1&safe=strict&site=&q=%E4%B9%A1%E6%9D%91%E7%88%B1%E6%83%85+%E5%B0%8F%E5%A4%9C%E6%9B%B2&btnK=Google+%E6%90%9C%E7%B4%A2&oq=&aq=&aqi=&aql=&gs_sm=&gs_upl=&bav=on.2,or.r_gc.r_pw.,cf.osb&fp=5ad759e8c55eb699&biw=1920&bih=1035",  "乡村爱情 小夜曲", kGoogle},  // NOLINT
    {"http://www.sogou.com/sogou?pid=Af81002&query=7k7k小游戏&p=50040104&sourceid=sugg&w=01015004&oq=7k7k&ri=0", "7k7k小游戏", kSogou},  // NOLINT
    {"http://www.sogou.com/sogou?query=%D3%C5%BF%A8&pid=A8KFZ-5163&iv=6.1.0.6700", "优卡", kSogou},  // NOLINT
    {"http://www.sogou.com/web?query=苏丹中国工人遭袭图片&p=02210102&fhintidx=0", "苏丹中国工人遭袭图片", kSogou},  // NOLINT
    {"http://www.sogou.com/web??W?", "", ""},
    {"http://cn.bing.com/search?FORM=DNSAS&q=www.ssjj.com", "www.ssjj.com", kBing},
    {"http://www.bing.com/search?q=facebook&src=IE-SearchBox&FORM=IE8SRC", "facebook", kBing},
    {"http://cn.bing.com/search?q=%E5%B0%8F%E7%B1%B3%E6%89%8B%E6%9C%BA&src=IE-SearchBox&FORM=IE8SRC", "小米手机", kBing},  // NOLINT
    {"http://cn.bing.com/explore?FORM=Z9LH5", "", ""},
    {"http://www.soso.com/q?w=vagaa&unc=k400054&cid=union.s.wh&TSoSite=%CB%D1%CB%D1", "vagaa", kSoso},  // NOLINT
    {"http://www.soso.com/q?w=58%CD%AC%B3%C7&unc=k400054&cid=union.s.wh&TSoSite=%CB%D1%CB%D1", "58同城", kSoso},  // NOLINT
    {"http://tw.search.yahoo.com/search?p=facebook%E7%99%BB%E5%85%A5&fr=yfp-s&ei=utf-8&v=0", "facebook登入", kYahoo},  // NOLINT
    {"http://s.taobao.com/search?q=%C9%B4%BD%ED&commend=all&source=suggest&ssid=s5-e-p1", "", ""},  // NOLINT
    {"http://s.etao.com/search?initiative_id=setao_20120404&q=%B9%FE%C0%EF", "", ""},
    {"http://daogou.etao.com/search?q=%B5%E7%CA%D3&t=0", "", ""},
    {"http://tuan.etao.com/list.htm?q=%B5%E7%C4%D4+%C9%CF%BA%A3", "", ""},
    {"http://price.etao.com/search.htm?q=F22+%D5%BD%B6%B7%BB%FA", "", ""},  // NormalizeLine  // NOLINT
    {"http://www.sogou.com/sogou?sourceid=hint&bh=1&hintidx=3&query=media+player+11%CF%C2%D4%D8&&pid=sogou-netb-e9412ee564384b98&duppid=1&p=40040702&dp=1&w=01020600&interV=", "media player 11下载", kSogou},  // NOLINT
    {"http://www.youdao.com/search?q=%E5%A4%96%E6%9D%A5%E5%AA%B3%E5%A6%87%E6%9C%AC%E5%9C%B0%E9%83%8E", "外来媳妇本地郎", kYoudao},  // NOLINT
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); i++) {
    std::string charset;
    std::string search_engine;
    std::string query;
    IsGeneralSearch(cases[i].url, &query, &search_engine);
    EXPECT_STREQ(query.c_str(), cases[i].query);
    EXPECT_STREQ(search_engine.c_str(), cases[i].search_engine);
  }
}

TEST(ParseVerticalSearchQuery, cases) {
  struct UrlQuery {
    const char* url;
    const char* query;
    const char* search_engine;
  } cases[] = {
    {"http://www.baidu.com/s?wd=%EC%C5%CE%E8%B9%D2&opt-webpage=on&ie=gbk", "", ""},
    {"http://www.baidu.com/baidu?cl=3&tn=baidutop10&fr=top1000&wd=%B0%B5%B9%E2%3A%B3%CD%B7%A3&rsv_spt=2&issp=2", "", ""}, // NOLINT
    {"http://www.baidu.com/s?wd=a67.com&opt-webpage=on&ie=gbk", "", ""},
    {"http://www.baidu.com/s?cl=3&wd=郝邵文", "", ""},
    {"http://www.baidu.com/s?cl=3&wd=郝邵  文", "", ""},  // NormalizeLine // NOLINT
    {"http://www.baidu.com/s?wd=%CF%E7%B4%E5%B0%AE%C7%E9+%D0%A1%D2%B9%C7%FA&rsv_bp=0&rsv_spt=3", "", ""},  // NOLINT
    {"http://news.baidu.com/ns?word=%B8%DF%CC%FA+%B1%B1%BE%A9+%D6%D0%D1%EB&tn=news&from=news&ie=gb2312&bs=%BA%C3%BA%A2%D7%D3&sr=0&cl=2&rn=20&ct=1&prevct=no", "高铁 北京 中央", kBaiduNews}, // NOLINT
    {"http://video.baidu.com/v?word=%D3%DE%C8%CB%BD%DA&ct=301989888&rn=20&pn=0&db=0&s=0&fbl=800", "愚人节", kBaiduVideo}, // NOLINT
    {"http://zhidao.baidu.com/q?word=%BA%C3%BA%A2%D7%D3&lm=0&fr=search&ct=17&pn=0&tn=ikaslist&rn=10", "好孩子", kBaiduZhidao}, // NOLINT
    {"http://mp3.baidu.com/m?word=%CD%AC%D7%C0%B5%C4%C4%E3&lm=-1&f=ms&tn=baidump3&ct=134217728&lf=&rn=", "同桌的你", kBaiduMp3}, // NOLINT
    {"http://image.baidu.com/i?ct=201326592&cl=2&nc=1&lm=-1&st=-1&tn=baiduimage&istype=2&fm=index&pv=&z=0&word=%BA%A9%B6%B9%CF%C8%C9%FA&s=0", "憨豆先生", kBaiduImage}, // NOLINT
    {"http://video.baidu.com/s?f=1002&id=251558&n=25&word=%B9%AC&ns=2&se=0123", "宫", kBaiduVideo},
    {"http://baike.baidu.com/w?ct=17&lm=0&tn=baiduWikiSearch&pn=0&rn=10&word=%C0%EE%D0%A1%E8%B4&submit=search", "李小璐", kBaiduBaike}, // NOLINT
    {"http://wenku.baidu.com/search?word=%B5%E7%D7%D3%BE%AF%B2%EC%BC%BC%CA%F5&lm=0&od=0&fr=top_home", "电子警察技术", kBaiduWenku}, // NOLINT
    {"http://www.google.com.hk/search?q=%BB%AA%CB%B6&opt-webpage=on&client=aff-360daohang&hl=zh-CN&ie=gb2312&newwindow=1", "", ""},  // NOLINT
    {"http://www.google.com.hk/search?hl=en&newwindow=1&safe=strict&q=hello%20world&bav=on.2,or.r_gc.r_pw.,cf.osb&biw=1920&bih=1096&um=1&ie=UTF-8&tbm=isch&source=og&sa=N&tab=wi&ei=B_V7T4jLPKuUiQeQwNCSCQ", "", ""}, // NOLINT
    {"http://www.google.com/uds/afs??W?", "", ""},
    {"http://www.google.com.hk/#hl=zh-CN&newwindow=1&safe=strict&site=&q=%E4%B9%A1%E6%9D%91%E7%88%B1%E6%83%85+%E5%B0%8F%E5%A4%9C%E6%9B%B2&btnK=Google+%E6%90%9C%E7%B4%A2&oq=&aq=&aqi=&aql=&gs_sm=&gs_upl=&bav=on.2,or.r_gc.r_pw.,cf.osb&fp=5ad759e8c55eb699&biw=1920&bih=1035",  "", ""},  // NOLINT
    {"http://www.sogou.com/sogou?pid=Af81002&query=7k7k小游戏&p=50040104&sourceid=sugg&w=01015004&oq=7k7k&ri=0", "", ""},  // NOLINT
    {"http://www.sogou.com/sogou?query=%D3%C5%BF%A8&pid=A8KFZ-5163&iv=6.1.0.6700", "", ""},  // NOLINT
    {"http://www.sogou.com/web?query=苏丹中国工人遭袭图片&p=02210102&fhintidx=0", "", ""},  // NOLINT
    {"http://www.sogou.com/web??W?", "", ""},
    {"http://cn.bing.com/search?FORM=DNSAS&q=www.ssjj.com", "", ""},
    {"http://www.bing.com/search?q=facebook&src=IE-SearchBox&FORM=IE8SRC", "", ""},
    {"http://cn.bing.com/search?q=%E5%B0%8F%E7%B1%B3%E6%89%8B%E6%9C%BA&src=IE-SearchBox&FORM=IE8SRC", "", ""},  // NOLINT
    {"http://cn.bing.com/explore?FORM=Z9LH5", "", ""},
    {"http://www.soso.com/q?w=vagaa&unc=k400054&cid=union.s.wh&TSoSite=%CB%D1%CB%D1", "", ""},  // NOLINT
    {"http://www.soso.com/q?w=58%CD%AC%B3%C7&unc=k400054&cid=union.s.wh&TSoSite=%CB%D1%CB%D1", "", ""},  // NOLINT
    {"http://tw.search.yahoo.com/search?p=facebook%E7%99%BB%E5%85%A5&fr=yfp-s&ei=utf-8&v=0", "", ""},  // NOLINT
    {"http://s.taobao.com/search?q=%C9%B4%BD%ED&commend=all&source=suggest&ssid=s5-e-p1", "纱巾", kTaobao},  // NOLINT
    {"http://s.etao.com/search?initiative_id=setao_20120404&q=%B9%FE%C0%EF", "哈里", kTaobao},
    {"http://daogou.etao.com/search?q=%B5%E7%CA%D3&t=0", "电视", kTaobao},
    {"http://tuan.etao.com/list.htm?q=%B5%E7%C4%D4+%C9%CF%BA%A3", "电脑 上海", kTaobaoTuan},
    {"http://price.etao.com/search.htm?q=F22+%D5%BD%B6%B7%BB%FA", "f22 战斗机", kTaobaoPrice},  // NormalizeLine  // NOLINT
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); i++) {
    std::string charset;
    std::string search_engine;
    std::string query;
    IsVerticalSearch(cases[i].url, &query, &search_engine);
    ASSERT_STREQ(query.c_str(), cases[i].query) << cases[i].url;
    ASSERT_STREQ(search_engine.c_str(), cases[i].search_engine);
  }
}

TEST(IsAds, cases) {
  struct UrlQuery {
    const char* url;
    const char* src;
  } cases[] = {
    {"http://www.baidu.com/baidu.php?url=JTnK00K0hmQxKpFIdsgzE3lsb0an32RXJDvvDAs5z8IRZCl2Z_IT3Rf6iaIZ8wdz98WPkZxxwSX3v--vtfJZ8vi5XWa4of1RR-sNhDDlmBm-c-oqBad5foBgLZxX.7b_jCX17N6uG6S9S5Le3A2th1XGyAp7BEuYqhkf.U1Yk0ZDq1tJeJ0KY5TO28l60pyYqnWcd0ZTq0ATqmh3kn0KdpHd1mydsUZK_mgwhULFVnH0snsKspyfqnfKWpyfqn1T40AdY5HD0pvbqn0KzIjY", kBaidu},  // NOLINT
    {"http://www.baidu.com/adrc.php?t=00jy00c00fAUq0m0TW0h0cyPFfjDcMd50000000000000000xndhc6.THY60A3q0ZnqnjfknycsPymLnAw-nH-WP6Kd5HTLrjNAPWIKwRDYrHmLrDcdfYR3nH61fHf1P1b3fYmd0ADquZCsUyPlnZIrwDN3yyGKnNGJmLIpwdN3HdIPnbGVnyG9NdD4H-wFnbqDiHNrwAVVmY92I-GZUAVfNDNVyNfsX79lfydbw104ih4yI7KHyMuWUy_4HRPpXhwgiMuWUy_4ih4aXy7RnZ92URuom1P", kBaidu},  // NOLINT
    {"http://www.baidu.com/cb.php?c=IgF_pyfqnHf4nWfzP0KYTjYk0A7b5HndPHRdPsKbuHY1P10LPjc0TAq15HDdPjRsP6K15yPbm16zmyw9PhmznWu-ryD0uZfqnHn1n1msP1nYrfKdThsqpZwYTjCEQvPEuAqLUB44ULNbmyt8mvqVQvPGuA-9UBquULNbmyqDpyPYgLPoXyP8Qh-8uAN3QhN3ufKzujYk0AFV5H00TZcqn0KdpyfqnHfvnj0znfKEpyfqnHnzrj60mv-b5HDvrjTL0AqLUWYkn10dn0KWThnqnWfLnW0", kBaidu}, // NOLINT
    {"http://www.google.com.hk/aclk?sa=l&ai=CyZgTxjx9T9mbBOGqiQeT0biIB-r79JICtoqK5AOOxJiWBAgAEAEoA1DS_oHI_P____8BYJ250IGQBaABqrn2_QPIAQGpAhlTYOjSSIU-qgQUT9C9bxOlNYXNxbmNmE6VHjR0foo&sig=AOD64_14maZCyFI6an3xFe7Y6k9Z_EAh7A&ved=0CAsQ0Qw&adurl=http://www.flowercn.com/%3Fsid%3Dggxh1&rct=j&q=%E9%B2%9C%E8%8A%B1&c", "Google"}, // NOLINT
    {"http://www.baidu.com/baidu?xxx", ""},
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); i++) {
    std::string src;
    IsAds(cases[i].url, &src);
    EXPECT_STREQ(src.c_str(), cases[i].src);
  }
}

TEST(ParseSearchQueryFromURL, cases) {
  struct Base64UTF8 {
    const char* base64_text;
    const char* utf8_text;
  } cases[] = {
    {"5rGJ546L55qE5paH5pys546LNzYwMOmAmueUqOeJiA==", "汉王的文本王7600通用版"},
    {"urrN9bXEzsSxvs31NzYwMM2o08Ow5g==", "汉王的文本王7600通用版"},
    {"d3d3LmJhaWR1LmNvbQ==", "www.baidu.com"},
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); i++) {
    std::string utf8;
    EXPECT_TRUE(Base64ToUTF8(cases[i].base64_text, &utf8));
    EXPECT_STREQ(utf8.c_str(), cases[i].utf8_text);
  }
}

TEST(IsGeneralSearchClick, cases) {
  struct  {
    const char* dest_url;
    const char* search_engine;
    const char* query;
    bool is_or_not;
  } cases[] = {
    {"http://www.google.com/videohp?hl=zh-CN", "Google", "百度", false},
    {"http://www.hao123.com/index1.htm", "Baidu", "开心网", false},
    {"http://www.hao123.com/index1.htm", "Baidu", "hao123", true},
    {"http://zhidao.baidu.com/q?ct=17&pn=0&tn=ikaslist&rn=10&word=%20&fr=wwwt", "Baidu", "优酷", false},
    {"http://zhidao.baidu.com/q?ct=24&cm=16&tn=ucframework", "Baidu", "新浪", false},
    {"http://www.hao123.com/error/error-404.html", "Baidu", "新浪", false},
    {"http://www.google.com.tw/imgres?imgurl=http://www.marykaydir.cn/images/1.jpg&imgrefurl=http://www.marykaydir.cn/&usg=__A8yhsubR173lraKLypDnKn0FOxU=&h=454&w=667&sz=32&hl=zh-TW&start=11&zoom=1&tbnid=Vid5VqF7uU02RM:&tbnh=94&tbnw=138&ei=UfKDT6HmBYzjmAX_hvDOBw&prev=/search%3Fq%3D%25E7%2599%25BE%25E5%25BA%25A6%26um%3D1%26hl%3Dzh-TW%26newwindow%3D1%26sa%3DN%26rlz%3D1T4ADFA_zh-TWCN474CN476%26tbm%3Disch%26prmd%3Divnsz&um=1&itbs=1", "Google", "百度", false},  // NOLINT
    {"http://www.google.com.hk/intl/zh-CN/ads/", "Google", "淘宝", false},
    {"http://www.google.com.hk/imgres?q=%E8%82%BE%E7%BB%93%E7%9F%B3&um=1&hl=zh-CN&inlang=zh-CN&newwindow=1&safe=strict&client=aff-cs-360se&hs=qgJ&sa=N&tbm=isch&tbnid=B7N98P-Z6FBsiM:&imgrefurl=http://www.hudong.com/wiki/%25E8%2582%25BE%25E7%25BB%2593%25E7%259F%25B3&docid=X4wvJyFS-2OrmM&imgurl=http://a3.att.hudong.com/00/88/01300000162256121369886350168_s.jpg&w=300&h=235&ei=TUV-T8i2M8SYiAfMrtjdAw&zoom=1&iact=hc&vpx=571&vpy=183&dur=863&hovh=188&hovw=240&tx=116&ty=124&sig=102761408862627600888&page=1&tbnh=128&tbnw=163&start=0&ndsp=21&ved=1t:429,r:3,s:0,i:77&biw=1220&bih=642", "Google", "肾结石", false},  // NOLINT
    {"http://baike.baidu.com/searchword/?word=%BA%B8%BD%D3&pic=1", "Baidu", "网易", false},
    {"http://cn.bing.com/account/general?ru=%2fsearch%3fq%3d%25e7%2599%25be%25e5%25ba%25a6%26FORM%3dAWRE&sh=4", "Bing", "百度", false},  // NOLINT
    {"http://ditu.google.cn/maps?client=aff-cs-360se&oe=UTF-8&q=%E7%99%BE%E5%BA%A6&hl=zh-CN&prmdo=1&um=1&ie=UTF-8&ei=V89WT-atF-awiQf4heXoCA&sa=X&oi=mode_link&ct=mode&cd=2&ved=0CAsQ_AUoAQ", "Google", "百度", false},  // NOLINT

    {"http://www.google.com.hk/url?q=http://www.baidu.com/&rct=j&sa=U&ei=ve04T836C66yiQf-v_jwAQ&ved=0CDQQFjAA&q=%E7%99%BE%E5%BA%A6&usg=AFQjCNG39p4YcIF_Jqu4Y8oBqy8kn2dUhw", "Google", "百度", true},  // NOLINT
    {"http://121.32.136.21:8080/notice_new.htm?02038039371@163.gd", "Baidu", "淘宝", true}
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); i++) {
    // 基于 query 和 search engine, 构造 ref_url
    std::string search_engine = cases[i].search_engine;
    std::string query = cases[i].query;
    std::string dest_url = cases[i].dest_url;
    std::string ref_url;
    if (search_engine == log_analysis::kBaidu) {
      std::string new_query;
      CHECK(base::UTF8ToCodepage(query, base::kCodepageGB18030, base::OnStringConversionError::FAIL, &new_query));  // NOLINT
      ref_url = "http://www.baidu.com/s?wd=" + base::EncodeUrlComponent(new_query.c_str());
    } else if (search_engine == log_analysis::kGoogle) {
      ref_url = "http://www.google.com.hk/search?q=" + base::EncodeUrlComponent(query.c_str());
    } else if (search_engine == log_analysis::kBing) {
      ref_url = "http://www.bing.com/search?q=" + base::EncodeUrlComponent(query.c_str());
      // LOG(ERROR) << ref_url;
    } else {
      CHECK(false) << "only test Baidu, Google, Bing";
    }
    if (i == 11) LOG(ERROR) << GoogleTargetUrl(dest_url);
    EXPECT_EQ(cases[i].is_or_not, log_analysis::IsGeneralSearchClick(dest_url, ref_url, NULL, NULL)) << i;
  }
}

TEST(IsGeneralSearchClick, test) {
  struct UrlQuery {
    const char* url;
    const char* ref_url;
    bool succ;
  } cases[] = {
    // 点击的是相关搜索, 应该被干掉
    {"http://www.baidu.com/s?wd=%EC%C5%CE%E8%B9%D2&opt-webpage=on&ie=gbk", "http://www.baidu.com/s?wd=%EC%C5%CE%E8%B9%D2&opt-webpage=on&ie=gbk", false},  // NOLINT

    {"http://www.sina.com.cn/", "http://www.baidu.com/s?wd=%EC%C5%CE%E8%B9%D2&opt-webpage=on&ie=gbk", true},

    // 搜"炫舞挂"点百度图片, 不是一个合理点击
    {"http://image.baidu.com/?wd=%EC%C5%CE%E8%B9%D2", "http://www.baidu.com/s?wd=%EC%C5%CE%E8%B9%D2&opt-webpage=on&ie=gbk", false},  // NOLINT

    // 搜索"图片", 出百度图片的结果, 这种情况鱼龙混杂, 统一过滤掉
    {"http://image.baidu.com/i?tn=baiduimage&ct=201326592&lm=-1&cl=2&fr=ala0&word=%CD%BC%C6%AC", "http://www.baidu.com/s?wd=%CD%BC%C6%AC", false},  // NOLINT
    //{"http://www.baidu.com/link?url=a8d0d94de53f3d4a576afc3be182ad9ca6e8c0962218c9e338d199d3aad45625220ea6e968dca7db7bf605ce", "http://www.baidu.com/s?wd=qiyi&rsv_bp=0&rsv_spt=3&inputT=889", false},  // NOLINT

    // 非搜索
    {"http://news.sina.com.cn/c/sd/2012-05-16/005524421084.shtml", "http://www.sina.com.cn/", false},  // NOLINT
    {"http://baike.baidu.com/view/47227.htm", "http://www.baidu.com/s?wd=hello+world&rsv_bp=0&rsv_spt=3&inputT=2102", true},  // NOLINT
    // secondary search
    {"http://www.baidu.com/s?bs=hello+world&f=8&rsv_bp=1&rsv_spt=3&wd=%C4%E3%BA%C3&inputT=3463", "http://www.baidu.com/s?wd=hello+world&rsv_bp=0&rsv_spt=3&inputT=2102", false},  // NOLINT
    // serch query 不包含百度,点击百度
    {"http://www.baidu.com/", "http://www.baidu.com/s?wd=hello+world&rsv_bp=0&rsv_spt=3&inputT=2102", false},  // NOLINT
    // serch query 包含百度,点击百度
    {"http://www.baidu.com/", "http://www.baidu.com/s?wd=%B0%D9%B6%C8&rsv_bp=0&rsv_spt=3&inputT=4568", true},  // NOLINT
    {"http://zhidao.baidu.com/question/78058439.html", "http://www.baidu.com/s?bs=hello+world&f=8&rsv_bp=1&rsv_spt=3&wd=hello+world&inputT=3814", true},  // NOLINT
    // 垂直
    {"http://image.baidu.com/i?tn=baiduimage&ct=201326592&lm=-1&cl=2&fr=ala1&word=%CF%CA%BB%A8%CD%BC%C6%AC","http://www.baidu.com/s?wd=%CF%CA%BB%A8%CD%BC%C6%AC&rsv_bp=0&rsv_spt=3&inputT=11815",false},  // NOLINT
    {"http://baike.baidu.com/view/47227.htm","http://cn.bing.com/search?q=hello+world&qs=n&form=QBLH&pq=hello+world&sc=8-6&sp=-1&sk=",true},  // NOLINT
    // bing's secondary search
    {"http://cn.bing.com/search?q=helloworld%E6%B2%B9%E8%80%97%E8%AE%A1%E7%AE%97%E5%99%A8&qs=AS&form=QBRE&pq=hello+world&sc=8-11&sp=6&sk=AS5", "http://cn.bing.com/search?q=hello+world&qs=n&form=QBLH&pq=hello+world&sc=8-6&sp=-1&sk=", false}, // NOLINT
    // google's secondary search
    {"http://www.google.com.hk/#hl=en&newwindow=1&safe=strict&q=awk&oq=awk&aq=f&aqi=&aql=&gs_l=serp.3...44424.46510.0.46813.13.7.0.0.0.0.0.0..0.0...0.0.ae2hIuKhBhs&bav=on.2,or.r_gc.r_pw.,cf.osb&fp=f0801d346be8106&biw=1920&bih=1096", "http://www.google.com.hk/#hl=en&newwindow=1&safe=strict&site=&source=hp&q=hello+world&oq=hello+world&aq=f&aqi=&aql=&gs_l=hp.3...290180.291884.0.292024.11.8.0.0.0.0.0.0..0.0...0.0.GPxhW3QOLgY&bav=on.2,or.r_gc.r_pw.,cf.osb&fp=f0801d346be8106&biw=1920&bih=1096", false}, // NOLINT
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); i++) {
    ASSERT_EQ(IsGeneralSearchClick(cases[i].url, cases[i].ref_url, NULL, NULL), cases[i].succ) << i;
  }
}
}
