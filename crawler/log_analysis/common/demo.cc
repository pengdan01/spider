#include <string>
#include <set>
#include <unordered_set>
#include <fstream>
#include <iostream>
#include "nlp/common/nlp_util.h"
#include "nlp/common/rune_type.h"
#include "nlp/segment/internal/segment_model.h"
#include "extend/static_dict/darts_clone/darts_clone.h"
#include "base/common/slice.h"
#include "base/common/logging.h"
#include "base/common/gflags.h"
#include "base/common/gflags_completions.h"
#include "base/common/base.h"
#include "base/mapreduce/mapreduce.h"
#include "base/strings/string_util.h"
#include "base/strings/string_split.h"
#include "base/file/file_util.h"
#include "base/file/file_stream.h"
#include "base/strings/utf_char_iterator.h"
#include "base/encoding/base64.h"
#include "log_analysis/common/log_common.h"

DEFINE_bool(base64, false, "");

int main(int argc, char *argv[]) {
  base::InitApp(&argc, &argv, "query count");
  std::string line;
  std::string query;
  std::string se;
  while (std::getline(std::cin, line)) {
    base::TrimTrailingWhitespaces(&line);
    if (log_analysis::IsGeneralSearch(line, &query, &se)) {
      std::cout << line << "\t" << query << "\t" << se << std::endl;
    } else {
      if (log_analysis::IsVerticalSearch(line, &query, &se)) {
        std::cout << line << "\t" << query << "\t" << se << std::endl;
      } else {
        std::cout << line << "\tfailed" << std::endl;
      }
    }
    if (FLAGS_base64) {
      std::string base64;
      if (!base::Base64Decode(line, &base64)) continue;
      line = base64;
      std::cout << line << std::endl;
      if (log_analysis::IsGeneralSearch(line, &query, &se)) {
        std::cout << line << "\t" << query << "\t" << se << std::endl;
      }
    }
  }
}
