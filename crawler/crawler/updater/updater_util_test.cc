#include "crawler/updater/updater_util.h"

#include "base/testing/gtest.h"
#include "base/common/logging.h"
#include "base/file/file_util.h"

TEST(UpdaterUtil, ExtractParameterValueFromHeader) {
  std::string head1("HTTP/1.1 200 OK\r\nDate: Wed, 14 Mar 2012 10:55:46 GMT\r\nServer: Apache/2\r\n"
                    "X-Powered-By: PHP/5.2.17\r\nSet-Cookie: PHPSESSID=513d645d0acaaeb750023d4b2f4c"
                    "b0e8; path=/; domain=.livedildoaction.com\r\nExpires: Thu, 19 Nov 1981 08:52:0"
                    "0 GMT\r\nCache-Control: no-store, no-cache, must-revalidate, post-checkr\n\r\n");
  bool status;
  std::string value;
  status = crawler::ExtractParameterValueFromHeader(head1, "Date", &value);
  EXPECT_TRUE(status);
  EXPECT_EQ(value, "Wed, 14 Mar 2012 10:55:46 GMT");

  // Case Sensitive
  status = crawler::ExtractParameterValueFromHeader(head1, "date", &value);
  EXPECT_FALSE(status);

  std::string head2("HTTP/1.1 200 OK\r\nContent-Length: 3006\r\nContent-Type: text/html\r\nContent-E"
                    "ncoding: gzip\r\nLast-Modified: Thu, 22 Dec 2011 04:45:20 GMT\r\nAccept-Ranges:"
                    " bytes\r\nETag: \"0b02a8264c0cc1:1ae6\"\r\nVary: Accept-Encoding\r\nServer: Micros"
                    "oft-IIS/6.0\r\nX-Powered-By: ASP.NET\r\nDate: Thu, 16 Feb 2012 17:10:55");
  status = crawler::ExtractParameterValueFromHeader(head2, "Last-Modified", &value);
  EXPECT_TRUE(status);
  EXPECT_EQ(value, "Thu, 22 Dec 2011 04:45:20 GMT");

  std::string head3;
  base::FilePath path("crawler/updater/testdata/header_escaped");
  EXPECT_TRUE(base::file_util::ReadFileToString(path, &head3));

  status = crawler::ExtractParameterValueFromHeader(head3, "Last-Modified", &value);
  EXPECT_TRUE(status);
  EXPECT_EQ(value, "Thu, 22 Dec 2011 04:45:20 GMT");
  status = crawler::ExtractParameterValueFromHeader(head3, "ETag", &value);
  EXPECT_TRUE(status);
  EXPECT_EQ(value, "\"0b02a8264c0cc1:1ae6\"");
}
