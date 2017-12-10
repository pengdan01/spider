#include "crawler/util/text_handler.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>

#include "base/encoding/cescape.h"
#include "base/file/file_util.h"
#include "base/testing/gtest.h"

TEST(TextHandler, TestCescapeWholeFile) {
  const char *fpr = "crawler/util/testdata/read.txt";
  const char *fpw = "crawler/util/testdata/write.txt";
  std::string source;
  std::string dest;
  bool ret = crawler::TextHandler::CescapeWholeFile(fpr, &source);
  EXPECT_TRUE(ret == true);
  dest = source;
  bool ret2 = crawler::TextHandler::UnCnescapeWholeFile(dest, fpw);
  EXPECT_TRUE(ret2 == true);

  struct stat file_stat;
  stat(fpr, &file_stat);
  int64 file_size1 = file_stat.st_size;

  stat(fpw, &file_stat);
  int64 file_size2 = file_stat.st_size;

  EXPECT_TRUE(file_size1 == file_size2);

  std::string source2 = base::Utf8SafeCEscape(source);
  std::string source3 = base::UnescapeCEscapeString(source2);
  EXPECT_TRUE(source == source3);

  bool ret3 = crawler::TextHandler::CescapeWholeFileUtf8Safe(fpr, &source);
  EXPECT_TRUE(ret3 == true);
  std::cout << source << std::endl;
  // bool ret4 = crawler::TextHandler::UnCnescapeWholeFileUtf8Safe(source,fpw);
}

TEST(TextHandler, TestExtractLinksFromUnEscapedString) {
  const char *fpr= "crawler/util/testdata/read.txt";
  FILE *fp = fopen(fpr, "r");
  EXPECT_TRUE(fp != NULL);
  struct stat file_stat;
  stat(fpr, &file_stat);
  uint64 file_size = file_stat.st_size;
  char *tmp = new char[file_size +1];
  memset(tmp, 0, file_size +1);

  EXPECT_TRUE(file_size == fread(tmp, 1, file_size, fp));

  std::string source(tmp);
  delete [] tmp;
  std::cout << "\n" << source << "\n";
  std::vector<std::string> urls;

  std::string link("http://www.265.com/");
  crawler::TextHandler::ExtractHtml(link, source, &urls);
  // EXPECT_EQ(urls.size(), 6U);
  for (int i = 0; i < static_cast<int>(urls.size()); ++i) {
    std::cout << urls[i] << std::endl;
  }
  urls.clear();

  crawler::TextHandler::ExtractCss(link, source, &urls);
  // EXPECT_EQ(urls.size(), 5U);
  for (int i = 0; i < static_cast<int>(urls.size()); ++i) {
    std::cout << urls[i] << std::endl;
  }
  urls.clear();

  crawler::TextHandler::ExtractImg(link, source, &urls);
  // EXPECT_EQ(urls.size(), 4U);
  for (int i = 0; i < static_cast<int>(urls.size()); ++i) {
    std::cout << urls[i] << std::endl;
  }
}

TEST(TextHandler, TestHao123) {
  // const char *fpr= "crawler/util/testdata/hao123.utf8.html";
  const char *fpr= "crawler/util/testdata/read.txt";
  std::string content;
  base::file_util::ReadFileToString(fpr, &content);

  std::string referer("http://www.hao123.com/");
  std::vector<std::string> urls;
  crawler::TextHandler::ExtractHtml(referer, content, &urls);

  for (int i = 0; i < static_cast<int>(urls.size()); ++i) {
    LOG(INFO) << urls[i];
  }
}

