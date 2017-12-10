#include <iostream>
#include <fstream>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_printf.h"
#include "crawler/fetcher/fetcher_thread.h"
#include "crawler/fetcher/multi_fetcher.h"

DEFINE_string(url_file, "crawler/tools/tmp_task/test_url", "");
int main(int argc, char *argv[]) {
  base::InitApp(&argc, &argv, "");
  std::string filename(FLAGS_url_file);
  std::ifstream myfin(filename);
  std::string line;
  CHECK(myfin.good());
  std::string header;
  std::string body;
  std::string url;
  while (getline(myfin, line)) {
    url = line; 
    bool ret = crawler::MultiFetcher::SimpleFetcher(url, &header, &body, false);
    if (ret) {
      LOG(ERROR) << "done, url: " << url; 
    }
  }
  CHECK(myfin.eof());
  return 0;
}

