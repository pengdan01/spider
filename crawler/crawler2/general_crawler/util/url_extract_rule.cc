#include "crawler2/general_crawler/util/url_extract_rule.h"

#include <iostream>
#include <fstream>

#include "base/common/gflags.h"
#include "base/common/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/string_split.h"

namespace crawler3 {
namespace util {

void LoadUrlExtractRule(const std::string &file, std::vector<RulePattern> *rules) {
  CHECK_NOTNULL(rules);
  std::ifstream myfin(file);
  CHECK(myfin.good()) << "file name: " << file;
  std::string line;
  std::vector<std::string> fields;
  while (getline(myfin, line)) {
    base::TrimWhitespaces(&line);
    if (line.empty() || line[0] == '#') {
      continue;
    }
    fields.clear();
    base::SplitString(line, "\t", &fields);
    CHECK_EQ(fields.size(), 3u);
    rules->push_back(RulePattern(fields[0], fields[1], fields[2]));
  }
  CHECK(myfin.eof());
  LOG(ERROR) << "Load rules[" << rules->size() << "] from file: " << file;
  return;
}
}
}
