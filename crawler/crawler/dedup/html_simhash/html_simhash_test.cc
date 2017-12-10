#include "crawler/dedup/html_simhash/html_simhash.h"

#include "base/testing/gtest.h"
#include "base/common/basic_types.h"
#include "base/common/gflags.h"
#include "base/file/file_path.h"
#include "base/file/file_util.h"
#include "base/time/scoped_timer.h"
#include "base/strings/utf_codepage_conversions.h"
#include "extend/simhash/simhash.h"
#include "nlp/common/nlp_util.h"
#include "nlp/idf/idf.h"
#include "nlp/segment/segmenter.h"
#include "crawler/dedup/dom_extractor/content_collector.h"

TEST(HtmlSimHash, CalculatePageHash) {
  simhash::HtmlSimHash html_simhash;
  html_simhash.Init(simhash::HtmlSimHash::kHashAll);

  static struct {
    std::string file_name_1;
    std::string url_1;
    std::string file_name_2;
    std::string url_2;
  } cases[] = {
    {"sina_utf8.html", "http://www.sina.com.cn/news/", "sina_utf8_20120109.html", "http://www.sina.com.cn/"},
    {"jjwxc1.html", "http://www.jjwxc.com/", "jjwxc2.html", "http://www.jjwxc.com/"},
    {"boxilai1.shtml", "http://www.sohu.com/", "boxilai2.shtml", "http://www.sohu.com/"},
    {"baidu.html", "http://www.baidu.com/", "youdao.html", "http://www.youdao.com/"},
    {"keji1.shtml", "http://www.qq.com/", "keji2.htm", "http://www.163.com/"},
    {"liyanhong1.html", "http://www.qq.com/", "liyanhong2.html", "http://www.sohu.com/"},
    {"tieba1", "http://www.baidu.com/", "tieba2", "http://www.baidu.com/"},
    {"nbsp2.html", "http://www.baidu.com/", "nbsp3.html", "http://www.baidu.com/"},
  };

  simhash::PageSignature signature_1, signature_2;
  bool is_bad;
  std::string url_prefix = "crawler/dedup/testdata/";
  for (int id = 0; id < (int) ARRAYSIZE_UNSAFE(cases); ++id) {
    std::string file_name = url_prefix + cases[id].file_name_1;
    std::string html = "";
    std::string page_utf8 = "";
    base::file_util::ReadFileToString(file_name, &html);
    std::string encode_type = base::HTMLToUTF8(html, "", &page_utf8);
    html_simhash.VerifyBadPage(page_utf8, cases[id].url_1, &signature_1, &is_bad);
    file_name = url_prefix + cases[id].file_name_2;
    html = "";
    page_utf8 = "";

    base::file_util::ReadFileToString(file_name, &html);
    encode_type = base::HTMLToUTF8(html, "", &page_utf8);
    html_simhash.VerifyBadPage(page_utf8, cases[id].url_2, &signature_2, &is_bad);

    std::cout << cases[id].file_name_1 << "\t" << cases[id].file_name_2 << std::endl;
    std::cout << cases[id].url_1 << "\t" << cases[id].url_2 << std::endl;
    std::cout << signature_1.domain_hash_ << "\t" << signature_2.domain_hash_ << std::endl;
    std::cout << signature_1.title_hash_ << "\t" << signature_2.title_hash_ << std::endl;
    std::cout << signature_1.content_hash_ << "\t" << signature_2.content_hash_ << std::endl;
    int dist = simhash::CharikarSimHash::HammingDistance(signature_1.content_hash_,
                                                         signature_2.content_hash_);
    std::cout << dist << std::endl;
  }
}

TEST(HtmlSimHash, VerifyBadSignature) {
  simhash::HtmlSimHash html_simhash;
  html_simhash.Init(simhash::HtmlSimHash::kHashAll);
  simhash::PageSignature signature;
  signature.domain_hash_ = 3940308780789797144u;
  signature.title_hash_ = 301811501323704100u;
  signature.content_hash_ = 10411906976202308523u;
  std::string good_url1 = "http://ju.taobao.com/";
  std::string good_url2 = "http://j.taobal.com/";
  std::string bad_url = "http://ju.taobao.com/a/";
  bool is_bad;
  html_simhash.VerifyBadSignature(signature, good_url1, &is_bad);
  EXPECT_EQ(is_bad, false);

  html_simhash.VerifyBadSignature(signature, good_url2, &is_bad);
  EXPECT_EQ(is_bad, false);

  html_simhash.VerifyBadSignature(signature, bad_url, &is_bad);
  EXPECT_EQ(is_bad, true);
}
TEST(HtmlSimHash, VerifyBadPage) {
  simhash::HtmlSimHash html_simhash;
  html_simhash.Init(simhash::HtmlSimHash::kHashAll);

  static struct {
    std::string file_name;
    std::string url;
    bool is_bad;
  } cases[] = {
    // {"paipai.html", "http://auction1.paipai.com/736DC92A00000000007339C20676E3BB%5C", true},
    // {"paipai2.html", "http://auction1.paipai.com/A253F6320000000000223B2F07FB7452", true},
    {"emtoem.html", "http://thi.emtoem.cn:8080/?d=sicnu.emtoem.cn", false},
    {"jonweb.html", "http://www.huashang.jonweb.net/", false},
    // {"core2.html", "http://dougslaterfamily.hobby-site.com/Ancestors_of_Douglas_John_Slater/GBWEB58.HTM",
    // false},
  };

  simhash::PageSignature signature;
  bool is_bad;
  std::string url_prefix = "crawler/dedup/testdata/";
  for (int id = 0; id < (int) ARRAYSIZE_UNSAFE(cases); ++id) {
    std::string file_name = url_prefix + cases[id].file_name;
    std::string html = "";
    std::string page_utf8 = "";
    base::file_util::ReadFileToString(file_name, &html);
    std::string encode_type = base::HTMLToUTF8(html, "", &page_utf8);
    html_simhash.VerifyBadPage(page_utf8, cases[id].url, &signature, &is_bad);
    std::cout << signature.domain_hash_ << "\t"
              << signature.title_hash_ << "\t"
              << signature.content_hash_ << std::endl;
    EXPECT_EQ(is_bad, cases[id].is_bad);
  }
}
/*
  simhash::HtmlSimHash html_simhash;
  static struct {
    std::string term;
  } cases[] = {
    {"北京"},
    {"搜索"},
    {"一切"},
    {"的"},
    {"电脑"},
    {"主页"},
    {"能可不在"},
    {"万博科思"}
  };
  nlp::idf::IdfDicts dicts;

  double idf;
  for (int i = 0; i < (int) ARRAYSIZE_UNSAFE(cases); ++i) {
    idf = dicts.get_idf(cases[i].term, nlp::idf::IdfDicts::kPage);
    LOG(ERROR) << "idf dicts:\t" << idf;
  }
}
*/
/*
TEST(HtmlSimHash, ExtractMainContent) {
  static struct {
    std::string url;
  } cases[] = {
    {"sina_utf8.html"},
    {"dangshi1.htm"},
    {"docin.html"},
    {"baiduwenku.html"},
    {"youku.html"},
    {"tudou.html"},
    {"taobao1"},
    {"tmall.htm"},
    {"kuaibo.html"},
    {"tieba1"},
    {"tieba2"}
  };
  std::string url_prefix = "./extend/simhash/testdata/";
  std::string url = url_prefix + "";
  std::string html = "";
  std::string page_utf8 = "";
  nlp::segment::Segmenter segmenter;
  std::string title;
  std::string content;
  dom_extractor::ContentCollector content_collector;

  for (int i = 0; i < (int) ARRAYSIZE_UNSAFE(cases); ++i) {
    url = url_prefix + cases[i].url;
    html = "";
    page_utf8 = "";
    base::file_util::ReadFileToString(url, &html);
    std::string encode_type = base::HTMLToUTF8(html, "", &page_utf8);

    content_collector.ExtractMainContent(page_utf8, "", &title, &content);
    nlp::util::NormalizeLineInPlaceS(&title);
    nlp::util::NormalizeLineInPlaceS(&content);
    DLOG(INFO) << "url\t" << url;
    DLOG(INFO) << "title:\t" << title;
    DLOG(INFO) << "content:\t" << content;
  }
}
*/
/*
TEST(HtmlSimHash, ExtractHtmlTitleContentTermCount) {
  static struct {
    std::string url;
  } cases [] = {
    {"sina_utf8.html"},
    {"taobao1"},
    {"taobao2"},
    {"dangshi1.htm"},
    {"dangshi2"}
  };

  simhash::HtmlSimHash html_simhash;
  std::string url_prefix = "./extend/simhash/testdata/";
  std::string url;
  std::string html;
  std::string page_utf8 = "";
  std::vector<std::pair<std::string, int64> > term_count;

  for (int i = 0; i < (int) ARRAYSIZE_UNSAFE(cases); ++i) {
    url = url_prefix + cases[i].url;
    html = "";
    term_count.clear();
    base::file_util::ReadFileToString(url, &html);
    std::string encode_type = base::HTMLToUTF8(html, "", &page_utf8);
    std::cout << encode_type << std::endl;

    CHECK(html_simhash.ExtractHtmlTitleContentTermCount(page_utf8, url, &term_count));
    DLOG(INFO) << "Term count for: \t" << url;
    std::vector<std::pair<std::string, int64> >::const_iterator it;
    for (it = term_count.begin(); it != term_count.end(); ++it) {
      DLOG(INFO) << it->first << "\t" << it->second;
    }
  }
}
*/
TEST(HTML, HtmlSimHash) {
  static struct {
    std::string first;
    std::string second;
    bool expected;
  } cases[] = {
    /*
    {"sina_utf8.html", "sina_utf8_20120109.html", true},
    {"boxilai1.shtml", "boxilai2.shtml", true},
    {"mongbasa_utf8.html", "sina_utf8.html", false},
    {"baidu.html", "youdao.html", true},
    {"keji1.shtml", "keji2.htm", true},
    {"liyanhong1.htm", "liyanhong2.shtml", true},
    {"taobao1", "taobao2", true},
    {"longaoqiankun.html", "tianzishenjian.html", true},
    {"tieba1", "tieba2", true},
    {"zhouhongyi_jrj.shtml", "zhouhongyi_sina.shtml", true},
    {"zhouhongyi_jrj.shtml", "zhouhongyi_sohu.shtml", true},
    {"zhouhongyi_sina.shtml", "zhouhongyi_sohu.shtml", true},
    {"yahoo_zz.shtml", "yahoo_cb.shtml", true},
    {"yahoo_zz.shtml", "yahoo_qq.htm", true},
    {"yahoo_zz.shtml", "yahoo_lf.html", true},
    {"yahoo_zz.shtml", "yahoo_tw.shtml", true},
    {"yahoo_cb.shtml", "yahoo_qq.htm", true},
    {"yahoo_cb.shtml", "yahoo_lf.html", true},
    {"yahoo_cb.shtml", "yahoo_tw.shtml", true},
    {"yahoo_qq.htm", "yahoo_lf.html", true},
    {"yahoo_qq.htm", "yahoo_tw.shtml", true},
    {"yahoo_lf.html", "yahoo_tw.shtml", true},
    {"wenku1.html", "wenku2.html", true},
    {"wenku1.html", "wenku3.html", true},
    {"wenku2.html", "wenku3.html", true},
    {"wenku1.html", "wenku4.html", true},
    {"wenku4.html", "wenku5.html", true},
    {"zhidao_1.html", "zhidao_1.html", true},
    {"zangao1.html", "zangao2.html", true},
    {"edoor1.html", "edoor2.html", true},
    {"tmall1.html", "tmall2.html", true},
    {"cnc1.html", "cnc2.html", true},
    {"alibaba_blog.html", "alibaba_blog.html", true},
    {"alibaba_1.html", "alibaba_2.html", true},
    {"baipas1.html", "baipas2.html", true},
    {"561.html", "562.html", true},
    {"qqv1.html", "qqv2.html", true},
    {"bokee1.html", "bokee2.html", true},
    {"taobao_store1.html", "taobao_store2.html", true},
    {"taobao_store1.html", "taobao_store3.html", true},
    {"jyeoo1.html", "jyeoo2.html", true},
    {"hefei1.html", "hefei2.html", true},
    {"tmall_1.html", "tmall_2.html", true},
    {"zjol1.html", "zjol2.html", true},
    {"335911.html", "335912.html", true},
    {"sohu_blog_1.html", "sohu_blog_2.html", true},
    {"maotai1.html", "maotai2.html", true},
    {"zm1.html", "zm1.html", true},
    {"feijiu1.html", "feijiu2.html", true},
    {"niwota1.html", "niwota2.html", true},
    {"eachnet1.html", "eachnet2.html",true},
    {"login1.html", "login2.html", true},
    {"hi1.html", "hi2.html", true},
    {"tmall1.html", "tmall2.html", true},
    {"061.html", "062.html", true},
    {"ynhr1.html", "ynhr2.html", true},
    {"hefei1.html", "hefei2.html", true},
    {"ciwong1.html", "ciwong2.html", true},
    {"pro1.htm", "main.asp", true},
    {"xunlei1.html", "xunlei3.html", true},
    {"qq_xiaoshuo.html", "17k_xiaoshuo.html", true},
    {"qq_xiaoshuo.html", "xxsy_xiaoshuo.html", true},
    {"plu1.html", "plu2.html", true},
    {"xda1.html", "xda2.html", true},
    {"ptfish1.html", "ptfish2.html", true},
    {"gov1.html", "gov2.html", true},
    {"garenahack.html", "352200.html", true},
    {"duowan1.html", "duowan2.html", true},
    {"pcauto1.html", "pcauto2.html", true},
    {"nbsp1.html", "nbsp2.html", true},
    {"nbsp1.html", "nbsp3.html", true},
    {"nbsp1.html", "nbsp4.html", true},
    {"hahawx1.html", "hahawx2.html", true},
    {"lind1.html", "lind2.html", true},
    {"hjen1.html", "hjen2.html", true},
    {"recycle1.html", "recycle2.html", true},
    {"tudou2.html", "tudou3.html", true},
    {"sobar1.html", "sobar2.html", true},
    {"xy1781.html", "xy1782.html", true},
    {"ifeng1.html", "ifeng2.html", true},
    {"zhongyi1.html", "zhongyi2.html", true},
    {"cntv1.html", "cntv2.html", true},
    {"qqv1.html", "qqv2.html", true},
    {"5wei81.html", "5wei82.html", true},
    {"iask1.html", "iask2.html", true},
    {"hljbb1.html", "hljbb2.html", true},
    {"jmnews1.html", "jmnews2.html", true},
    {"wcoat1.html", "wcoat2.html", true},
    */
    {"jjwxc1.html", "jjwxc2.html", true}
  };
  simhash::HtmlSimHash html_simhash;

  std::string html;
  std::string page_utf8;
  std::string url_prefix = "./crawler/dedup/testdata/";

  double t1;
  {
    base::ScopedTimer scoped_timer(&t1);
    for (int id = 0; id < 1; ++id) {
      for (int i = 0; i < (int) ARRAYSIZE_UNSAFE(cases); ++i) {
        uint64 simhash_first;
        uint64 simhash_second;
        html = "";
        base::file_util::ReadFileToString(url_prefix + cases[i].first, &html);
        std::string encode = base::HTMLToUTF8(html, "", &page_utf8);
        CHECK(html_simhash.CalculateSimHash(page_utf8, cases[i].first, &simhash_first));
        html = "";
        base::file_util::ReadFileToString(url_prefix + cases[i].second, &html);
        encode = base::HTMLToUTF8(html, "", &page_utf8);
        CHECK(html_simhash.CalculateSimHash(page_utf8, cases[i].second, &simhash_second));
        int dist = simhash::CharikarSimHash::HammingDistance(simhash_first, simhash_second);
        LOG(ERROR) << dist
                   << "\t" << simhash_first
                   << "\t" << simhash_second
                   << "\t" << cases[i].first
                   << "\tand\t" << cases[i].second;
      }
    }
  }
  LOG(ERROR) << "Total time is \t" << t1;
}


