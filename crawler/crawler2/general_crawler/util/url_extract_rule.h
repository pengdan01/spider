#pragma once

#include <vector>
#include <string>

namespace crawler3 {
namespace util {

struct RulePattern {
  std::string host_pattern;
  std::string path_pattern;
  std::string target_url_pattern;
  RulePattern() {}
  RulePattern(const std::string &host_p,
              const std::string &path_p,
              const std::string &target_url_p):
      host_pattern(host_p),
      path_pattern(path_p),
      target_url_pattern(target_url_p) {
      }
};

void LoadUrlExtractRule(const std::string &file, std::vector<RulePattern> *rules);

}  // end namespace util
}  // end namespace crawler3
