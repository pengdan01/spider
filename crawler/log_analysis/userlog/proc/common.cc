#include "log_analysis/userlog/proc/common.h"

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <map>
#include <set>

#include "nlp/common/nlp_util.h"
#include "log_analysis/common/log_common.h"
#include "log_analysis/common/error_code.h"
#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/encoding/base64.h"
#include "base/encoding/line_escape.h"
#include "base/strings/string_split.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/timestamp.h"
#include "base/mapreduce/mapreduce.h"
#include "base/random/pseudo_random.h"
#include "base/hash_function/city.h"
#include "base/hash_function/md5.h"

bool decode_time(const std::string& orig_str, std::string* time_stamp) {
  std::string tmp_str;
  if (!base::Base64Decode(orig_str, &tmp_str)) {
    return false;
  }
  CHECK_EQ(tmp_str.size(), 4u);
  int32 time_number;
  // std::reverse(tmp_str.begin(), tmp_str.end());
  memcpy(&time_number, &(tmp_str[0]), tmp_str.size());
  if (!log_analysis::ConvertTimeFromSecToFormat(base::IntToString(time_number),
                                                "%Y%m%d%H%M%S", &tmp_str)) {
    return false;
  }
  *time_stamp = tmp_str;
  return true;
}

bool decode_search_type(const std::string& orig_str, std::string* attr) {
  std::string tmp_str;
  if (!base::Base64Decode(orig_str, &tmp_str)) {
    return false;
  }
  if (tmp_str.size() != 4u) {
    LOG(ERROR) << "decode search type error" << orig_str;
    return false;
  }
  int32 number;
  // std::reverse(tmp_str.begin(), tmp_str.end());
  memcpy(&number, &(tmp_str[0]), tmp_str.size());
  *attr = base::IntToString(number);
  return true;
}

bool decode_attr(const std::string& orig_str, std::string* attr) {
  std::string tmp_str;
  if (!base::Base64Decode(orig_str, &tmp_str)) {
    return false;
  }
  CHECK_EQ(tmp_str.size(), 2u);
  int16 number;
  // std::reverse(tmp_str.begin(), tmp_str.end());
  memcpy(&number, &(tmp_str[0]), tmp_str.size());
  *attr = base::IntToString(number);
  return true;
}

