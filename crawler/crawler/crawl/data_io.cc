#include "crawler/crawl/data_io.h"

#include <fstream>
#include <sstream>
#include <unordered_set>
#include <deque>
#include <vector>
#include <string>

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "web/url/url.h"

namespace crawler2 {
namespace crawl {

void LoadHttpsFiles(const std::string &file_name, std::unordered_set<std::string> *https_urls) {
  std::ifstream is(file_name.c_str());
  CHECK(is.good()) << "failed to open file: " << file_name;

  std::string line;
  while (std::getline(is, line)) {
    base::TrimWhitespaces(&line);
    https_urls->insert(line);
  }
  CHECK(is.eof());
  is.close();
}

void LoadClientRedirectFile(const std::string& file_name,
                            std::unordered_map<std::string, std::string>* client_redirect_urls) {
  std::ifstream is(file_name.c_str());
  CHECK(is.good()) << "failed to open file: " << file_name;

  std::string line;
  std::vector<std::string> tokens;
  while (std::getline(is, line)) {
    base::TrimWhitespaces(&line);
    tokens.clear();
    base::SplitString(line, "\t", &tokens);
    CHECK_EQ(tokens.size(), 2u) << line;
    GURL url1(tokens[0]);
    GURL url2(tokens[1]);
    CHECK(url1.is_valid() && url1.has_host()) << line;
    CHECK(url2.is_valid() && url2.has_host()) << line;
    // client_redirect_urls->insert(std::make_pair<std::string, std::string>(tokens[0], tokens[1]));
    client_redirect_urls->insert(std::make_pair(tokens[0], tokens[1]));
  }
  CHECK(is.eof());
  is.close();
}

}  // namespace crawl
}  // namespace crawler22

