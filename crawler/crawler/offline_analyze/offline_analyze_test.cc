#include <iostream>
#include <string>

#include "base/common/base.h"
#include "base/testing/gtest.h"
#include "base/common/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/string_tokenize.h"
#include "base/strings/string_split.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_codepage_conversions.h"
#include "base/file/file_util.h"
#include "web/html/utils/url_extractor.h"
#include "crawler/proto/crawled_resource.pb.h"
#include "crawler/offline_analyze/offline_analyze_util.h"
#include "web/url/url.h"
#include "web/url_norm/url_norm.h"

// TEST(offline_analyze, tmptest) {
//   const std::string pattern("*/*.*");
//   std::string path1("/");
//   EXPECT_TRUE(!base::MatchPattern(path1, pattern));
//   std::string path2("/aa/a.html");
//   EXPECT_TRUE(base::MatchPattern(path2, pattern));
//   std::string path3("/a/");
//   EXPECT_TRUE(!base::MatchPattern(path3, pattern));
//   std::string empty("");
//   std::vector<std::string> tokens;
//   base::SplitString(empty, "\t", &tokens);
//   LOG(INFO) << "size: " << tokens.size();
//   EXPECT_EQ(tokens.size(), 1U);
//   std::string url("mail.bwpsll.com:88");
//   GURL gurl(url);
//   LOG(ERROR) << "scheme: " << gurl.scheme();
//   EXPECT_TRUE(!web::has_scheme(url));
//   std::string url2("JavascripT:mail.bwpsll.com:88/gs.js");
//   EXPECT_TRUE(web::has_scheme(url2));
//   std::string url3("JvascripT:mail.bwpsll.com:88/gs.js");
//   EXPECT_TRUE(!web::has_scheme(url3));
//   std::string url4("http://www.baidu.com.///");
//   GURL gurl2(url4);
//   if (gurl2.is_valid()) {
//     LOG(ERROR) << "Url is valid: " << url4;
//   } else {
//     LOG(ERROR) << "Url is not valid: " << url4;
//   }
//   LOG(ERROR) << "INFO:" << gurl2.spec();
//   LOG(ERROR) << "INFO host:" << gurl2.host();
//   LOG(ERROR) << "INFO path:" << gurl2.path();
//   int x;
//   base::StringToInt("0900", &x);
//   LOG(ERROR) << "09 to int:" << x; 
//   long t = time(NULL);
//   struct tm *ptm = localtime(&t);
//   LOG(ERROR) << "hour: " << ptm->tm_hour;
//   LOG(ERROR) << "min: " << ptm->tm_min;
//   
//   web::UrlNorm url_norm;
//   std::string url10("http://028.teambuy.com.cn/tuangou/info.php?chno=10&Bigid=4000&infoID=449013");
//   if (!url_norm.NormalizeClickUrl(url10, &url10)) {
//     LOG(ERROR) << "Fail: url_norm.NormalizeClickUrl(url, &url), url: " << url10;
//   }
//   LOG(ERROR) << "After norm, url: " << url10;
// }
// 
// TEST(offline_analyze, offline_analyze_util_url_preprocess) {
//   const std::string postfix(".exe .zip");
//   const std::string prefix("javascript mailto");
//   std::vector<std::string> postfixs;
//   std::vector<std::string> prefixs;
//   base::Tokenize(postfix, " ", &postfixs);
//   base::Tokenize(prefix, " ", &prefixs);
// 
//   std::vector<std::pair<std::string, std::string> > anchor;
//   anchor.push_back(std::make_pair("http://www.sohu.com/", "1"));
//   anchor.push_back(std::make_pair("http://www.sohu.com/#", "2, the same as 1"));
//   anchor.push_back(std::make_pair("http://www.sohu.com#", "3, the same as 1"));
//   anchor.push_back(std::make_pair("javascript://www.sohu.com/#", "4, ignore"));
//   anchor.push_back(std::make_pair("JavascrIpt://www.sohu.com/#", "5, ignore"));
//   anchor.push_back(std::make_pair("mailto://www.sohu.com/#", "6, ignore"));
//   anchor.push_back(std::make_pair("http://www.sohu.com/a.exe", "7, ignore"));
//   anchor.push_back(std::make_pair("http://www.sohu.com/a.zip", "8, ignore"));
//   anchor.push_back(std::make_pair("http://81.duote.org:8080/matschool.zip", "9, ignore"));
//   anchor.push_back(std::make_pair("http://81.duote.org:8080/matschool.zip   ", "10, ignore"));
// 
//   std::vector<std::string> outlinks;
//   crawler::url_preprocess(&outlinks, anchor, postfixs, prefixs);
// 
//   EXPECT_EQ(3U, outlinks.size());
//   std::cout << "Tips, thos three links should the same\n";
//   for (int i = 0; i < (int)outlinks.size(); ++i) {
//     std::cout << "after preprocess, url is: " << outlinks[i] << std::endl;
//   }
// }

TEST(offline_analyze, web_utils_ExtractURLs) {
  std::string page_url("http://tech.techweb.com.cn/");
  std::string page_data;
  // const std::string filepath("crawler/offline_analyze/testdata/byr");
  const std::string filepath("/home/pengdan/workplace/wly/tmp/index.html");
  CHECK(base::file_util::ReadFileToString(filepath, &page_data));
  std::vector<std::string> link_css;
  std::vector<std::string> link_script;
  std::vector<web::ImageInfo> images;
  std::vector<std::pair<std::string, std::string> > anchor;
  std::cout << "ExtratURL input: page_data size is: " << page_data.size()
            << ", page_url is: " << page_url << std::endl;
  web::utils::ExtractURLs(page_data.c_str(), page_url.c_str(), &link_css, &link_script, &images, &anchor);

  // for (int i = 0; i < (int)link_css.size(); ++i) {
  //   LOG(INFO) << "type: css, url: " << link_css[i];
  // }
  // for (int i = 0; i < (int)link_script.size(); ++i) {
  //   LOG(INFO) << "type: script, url: " << link_script[i];
  // }
  for (int i = 0; i < (int)anchor.size(); ++i) {
    LOG(ERROR) << "url: " << anchor[i].first << ", anchor text: " << anchor[i].second;
  }
  // unsigned int cnt = 0;
  // for (int i = 0; i < (int)images.size(); ++i) {
  //   const web::ImageInfo &i_info = images[i];
  //   if (!i_info.width_specified || !i_info.height_specified) {
  //     ++cnt;
  //   }
  //   LOG(INFO) << "type: image, url: " << i_info.url << ", w_spec: " << i_info.width_specified
  //             << ", w_value: " << i_info.width << ", h_spec: " << i_info.height_specified
  //             << ", h_value: " << i_info.height;
  // }
}

// TEST(offline_analyze, HTMLToUTF8WithBestEffort) {
//   std::string url;
//   std::string http_header;
//   std::string raw_html;
//   std::string prefix("crawler/offline_analyze/testdata/techweb");
//   CHECK(base::file_util::ReadFileToString(prefix + ".url", &url));
//   CHECK(base::file_util::ReadFileToString(prefix + ".header", &http_header));
//   CHECK(base::file_util::ReadFileToString(prefix + ".html", &raw_html));
// 
//   int skipped_bytes;
//   int nonascii_bytes;
//   std::string res;
//   const char *code_page = base::ConvertHTMLToUTF8WithBestEffort(
//       url, http_header, raw_html, &res, &skipped_bytes, &nonascii_bytes);
//   LOG(ERROR) << "code page: " << code_page; 
//   LOG(ERROR) << "utf8 page: " << res; 
// }
