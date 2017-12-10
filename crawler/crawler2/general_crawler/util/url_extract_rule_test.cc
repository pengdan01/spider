#include "crawler2/general_crawler/util/url_extract_rule.h"

#include <vector>

#include "base/common/base.h"
#include "base/testing/gtest.h"
#include "base/common/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/string_tokenize.h"
#include "base/file/file_util.h"

TEST(UrlExtractRule, LoadUrlExtractRule) {
  std::string pattern_file("crawler2/general_crawler/tests/data/url_extract_rule.txt");
  std::vector<crawler3::util::RulePattern> pattern_rules;
  crawler3::util::LoadUrlExtractRule(pattern_file, &pattern_rules);
  CHECK_EQ(pattern_rules.size(), 1u);
  for (auto it = pattern_rules.begin(); it != pattern_rules.end(); ++it) {
    LOG(ERROR) << it->host_pattern << "\t" << it->path_pattern << "\t" << it->target_url_pattern;
  }
}
