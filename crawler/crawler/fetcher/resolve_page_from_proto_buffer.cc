#include <iostream>
#include <fstream>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "crawler/proto/crawled_resource.pb.h"

void DisplayCrawleInfo(const crawler::CrawledResource &);

int main(int argc, char *argv[]) {
  if (argc != 2) {
    LOG(ERROR) << "Usage %s input_file" << argv[0];
    return 1;
  }
  crawler::CrawledResource info;
  FILE * fp = fopen(argv[1], "rb");
  CHECK_NOTNULL(fp);
  uint32 size = 0;
  while (!feof(fp)) {
    if (sizeof(size) != fread(&size, 1, sizeof(size), fp)) break;
    // std::cout << "pb size: " << size << std::endl;
    char *tmp = new char[size + 1];
    uint32 n = fread(tmp, 1, size, fp);
    if (n != size) break;
     tmp[n] = 0;
    // std::cout << "tmp buffer: " << tmp << std::endl;
    if (!info.ParseFromArray(tmp, n)) {
      std::cout << "parse fail\n";
    }
    DisplayCrawleInfo(info);
    delete [] tmp;
  }

  return 0;
}


void DisplayCrawleInfo(const crawler::CrawledResource &info) {
  std::cout << info.DebugString();
  // std::cout << info.source_url() << std::endl;
  // std::cout << info.content_raw() << std::endl;
  // std::cout << info.content_utf8_converted() << std::endl;
}
