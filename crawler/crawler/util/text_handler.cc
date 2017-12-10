#include "crawler/util/text_handler.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <iostream>
#include <algorithm>

#include "base/encoding/cescape.h"
#include "base/common/logging.h"
#include "base/strings/string_util.h"
#include "web/url/gurl.h"
#include "extend/regexp/re3/re3.h"
#include "extend/regexp/re2/re2.h"

namespace crawler {

bool TextHandler::CescapeWholeFile(const char *fname, std::string *str) {
  CHECK_NOTNULL(fname);

  struct stat file_stat;
  LOG_IF(FATAL, stat(fname, &file_stat) != 0) << "call stat(fname, &file_stat) fail.";

  int64 file_size = file_stat.st_size;

  char *file_buf = new char[5 * file_size];
  CHECK_NOTNULL(file_buf);

  FILE * fp = fopen(fname, "rb");
  CHECK_NOTNULL(fp);

  int tmp = fread(file_buf, 1, file_size, fp);
  CHECK_GE(tmp, 0);
  file_buf[file_size] = 0;
  fclose(fp);
  char *dest = file_buf + file_size + 1;

  int dest_len = base::CEscapeString(file_buf, file_size, dest, 4*file_size-1);
  PLOG_IF(FATAL, dest_len < 0);

  *str = dest;
  delete []file_buf;
  return true;
}

bool TextHandler::CescapeWholeFileUtf8Safe(const char *fname, std::string *str) {
  CHECK_NOTNULL(fname);
  CHECK_GT(strlen(fname), 0U);
  struct stat file_stat;
  LOG_IF(FATAL, stat(fname, &file_stat) < 0) << "call stat(fname,  &file_stat) fail.";
  int64 file_size = file_stat.st_size;

  char *file_buf = new char[file_size + 1];
  CHECK_NOTNULL(file_buf);

  FILE * fp = fopen(fname, "rb");
  CHECK_NOTNULL(fp);

  int tmp = fread(file_buf, 1, file_size, fp);
  CHECK_GE(tmp, 0);
  file_buf[file_size] = 0;
  fclose(fp);
  *str = base::Utf8SafeCEscape(std::string(file_buf));
  delete []file_buf;
  return true;
}

bool TextHandler::UnCnescapeWholeFile(const std::string &str, const char *fname) {
  CHECK_NOTNULL(fname);

  char *buf = new char[strlen(str.c_str()) * 2];
  CHECK_NOTNULL(buf);
  int64 bytes = base::UnescapeCEscapeBytes(str.c_str(), buf);
  LOG_IF(FATAL, bytes < 0) << "call base::UnescapeCEscapeBytes() fail.";

  FILE * fp = fopen(fname, "wb");
  CHECK_NOTNULL(fp);

  fwrite(buf, 1, strlen(buf), fp);
  fclose(fp);
  delete []buf;
  return true;
}

bool TextHandler::UnCnescapeWholeFileUtf8Safe(const std::string &str, const char *fname) {
  CHECK_NOTNULL(fname);

  std::string dest = base::UnescapeCEscapeString(str);

  FILE *fp = fopen(fname, "wb");
  CHECK_NOTNULL(fp);

  fwrite(dest.c_str(), 1, strlen(dest.c_str()), fp);
  fclose(fp);
  return true;
}

void TextHandler::ExtractLink(const std::string &link, const std::string &source,
                              std::vector<std::string> *urls, const Re3 &UrlPattern) {
  CHECK_NOTNULL(urls);
  base::Slice input(source);
  std::string url, valid_url;
  GURL base_url(link);

  while (Re3::FindAndConsume(&input, UrlPattern, &url)) {
    GURL new_url = base_url.Resolve(url);
    if (new_url.is_valid()) {
      valid_url = new_url.spec();
      if (base::EndsWith(valid_url, ".exe", false)) {
        DVLOG(3) << "url ends with '.exe', ignored: " << valid_url;
        continue;
      }
      if (base::StartsWith(valid_url, "javascript:", false)) {
        DVLOG(3) << "url starts with 'javascript:', ignored: " << valid_url;
        continue;
      }
      // 去掉最后面的多余的 '#'
      if (valid_url[valid_url.size()-1] == '#') {
        valid_url = valid_url.substr(0, valid_url.size()-1);
      }
      if (valid_url[valid_url.size()-1] == '/') {
        valid_url = valid_url.substr(0, valid_url.size()-1);
      }
      urls->push_back(valid_url);
    } else {
      DVLOG(3) << "invalid url, ignored: " << new_url;
    }
  }
  if (urls->size() > 1) {  // sort and unique
    sort(urls->begin(), urls->end());
    urls->erase(unique(urls->begin(), urls->end()), urls->end());
  }
}

void ExtractLink2(const std::string &link, const std::string &source,
                               std::vector<std::string> *urls, const extend::re2::RE2 &UrlPattern) {
  CHECK_NOTNULL(urls);
  base::Slice input(source);
  std::string url, valid_url;
  GURL base_url(link);

  while (extend::re2::RE2::FindAndConsume(&input, UrlPattern, &url)) {
    GURL new_url = base_url.Resolve(url);
    if (new_url.is_valid()) {
      valid_url = new_url.spec();
      if (base::EndsWith(valid_url, ".exe", false)) {
        DVLOG(3) << "url ends with '.exe', ignored: " << valid_url;
        continue;
      }
      if (base::StartsWith(valid_url, "javascript:", false)) {
        DVLOG(3) << "url starts with 'javascript:', ignored: " << valid_url;
        continue;
      }
      // 去掉最后面的多余的 '#'
      if (valid_url[valid_url.size()-1] == '#') {
        valid_url = valid_url.substr(0, valid_url.size()-1);
      }
      if (valid_url[valid_url.size()-1] == '/') {
        valid_url = valid_url.substr(0, valid_url.size()-1);
      }
      urls->push_back(valid_url);
    } else {
      DVLOG(3) << "invalid url, ignored: " << new_url;
    }
  }
  if (urls->size() > 1) {  // sort and unique
    sort(urls->begin(), urls->end());
    urls->erase(unique(urls->begin(), urls->end()), urls->end());
  }
}

void TextHandler::ExtractHtml(const std::string & link, const std::string &source,
                               std::vector<std::string> *urls) {
  // 需要经过两层转义，即：匹配'\'需要在正则中使用'\\\\'
  // const Re3 UrlPattern("<[aA]\\s+.*href=\"(http://\\S+/?)\"[^>]*>");
  // const extend::re2::RE2 UrlPattern("<[aA]\\s+.*href=\"(\\S+/?)\"[^>]*>");
  const extend::re2::RE2 UrlPattern("<[aA]\\s[^>]*href=\"(\\S+/?)\"[^>]*>");
  ExtractLink2(link, source, urls, UrlPattern);
}

void TextHandler::ExtractCss(const std::string & link, const std::string &source,
                              std::vector<std::string> *urls) {
  // 需要经过两层转义，即：匹配'\'需要在正则中使用'\\\\'
  const Re3 UrlPattern("<link\\s+[^>]*href=\"(\\S+\\.css.*?)\"[^>]*>");
  ExtractLink(link, source, urls, UrlPattern);
}

void TextHandler::ExtractImg(const std::string & link, const std::string &source,
                              std::vector<std::string> *urls) {
  // 需要经过两层转义，即：匹配'\'需要在正则中使用'\\\\'
  const Re3 UrlPattern("<img\\s+[^>]*src=\"([^\"]*)\"[^>]*>");
  ExtractLink(link, source, urls, UrlPattern);
}

void TextHandler::Extract_Html_Css_Image(const std::string & link, const std::string &source,
                                         std::vector<std::string> *html, std::vector<std::string> *css,
                                         std::vector<std::string> *image) {
  // 需要经过两层转义，即：匹配'\'需要在正则中使用'\\\\'
  const Re3 HtmlPattern("<[aA]\\s+[^>]*href=\"(\\S+/?)\"[^>]*>");
  ExtractLink(link, source, html, HtmlPattern);
  const Re3 CssPattern("<link\\s+[^>]*href=\"(\\S+\\.css.*?)\"[^>]*>");
  ExtractLink(link, source, css, CssPattern);
  const Re3 ImagePattern("<img\\s+[^>]*src=\"([^\"]*)\"[^>]*>");
  ExtractLink(link, source, image, ImagePattern);
}

}  // end namespace
