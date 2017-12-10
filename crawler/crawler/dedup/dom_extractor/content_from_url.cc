#include "base/file/file_util.h"
#include "base/file/file_path.h"
#include "base/common/base.h"
#include "base/strings/utf_codepage_conversions.h"
#include "crawler/dedup/dom_extractor/content_collector.h"

DEFINE_string(url, "", "");

int main(int argc, char* argv[]) {
  base::InitApp(&argc, &argv, "content_from_url");

  dom_extractor::ContentCollector content_collector;
  std::string html = "";
  std::string page_utf8 = "";

  base::file_util::ReadFileToString(FLAGS_url, &html);
  std::string encode_type = base::HTMLToUTF8(html, "", &page_utf8);

  std::string title;
  std::string content;
  content_collector.ExtractMainContent(page_utf8, "", &title, &content);
  std::cout << "title:\t" << title << std::endl;
  std::cout << "content:\t" << content << std::endl;

  return 0;
}
