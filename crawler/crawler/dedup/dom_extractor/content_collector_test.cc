#include "crawler/dedup/dom_extractor/content_collector.h"

#include "base/testing/gtest.h"
#include "base/common/basic_types.h"
#include "base/file/file_path.h"
#include "base/file/file_util.h"
#include "base/time/scoped_timer.h"
#include "base/strings/utf_codepage_conversions.h"
#include "extend/simhash/simhash.h"
#include "nlp/common/nlp_util.h"

TEST(CotentCollector, ExtractMainContent) {
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
    {"zhidao5.html", "http://www.baidu.com/", "zhidao5.html", "http://www.baidu.com/"},
    {"jonweb.html", "www.jonweb.com", "jonweb.html", "www.jonweb.com"},
  };

  std::string title;
  std::string content;
  dom_extractor::ContentCollector content_collector;
  std::string url_prefix = "crawler/dedup/testdata/";
  for (int id = 0; id < (int) ARRAYSIZE_UNSAFE(cases); ++id) {
    std::string file_name = url_prefix + cases[id].file_name_1;
    std::string html = "";
    std::string page_utf8 = "";
    base::file_util::ReadFileToString(file_name, &html);
    std::string encode_type = base::HTMLToUTF8(html, "", &page_utf8);
    content_collector.ExtractMainContent(page_utf8, "", &title, &content);
    std::cout << "title:\t" << title << std::endl;
    std::cout << "content:\t" << content << std::endl;

    std::string file_name2 = url_prefix + cases[id].file_name_2;
    html = "";
    page_utf8 = "";
    base::file_util::ReadFileToString(file_name2, &html);
    encode_type = base::HTMLToUTF8(html, "", &page_utf8);
    content_collector.ExtractMainContent(page_utf8, "", &title, &content);
    std::cout << "title:\t" << title << std::endl;
    std::cout << "content:\t" << content << std::endl;
  }
}
