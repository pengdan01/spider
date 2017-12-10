#include "log_analysis/common/log_common.h"

#include <string>
#include <iostream>

#include "base/common/logging.h"
#include "base/common/basic_types.h"
#include "base/strings/string_util.h"
#include "base/encoding/base64.h"
#include "base/encoding/cescape.h"
#include "base/encoding/url_encode.h"
#include "base/strings/utf_codepage_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"
#include "web/url/gurl.h"
#include "web/url/url_util.h"
#include "base/time/time.h"

namespace log_analysis {
bool Base64ToUTF8(const std::string& text_base64, std::string* utf8_text) {
  utf8_text->clear();

  if (text_base64.empty()) return false;

  std::string temp_text;
  if (!base::Base64Decode(text_base64, &temp_text)) {
    DLOG(WARNING) << StringPrintf("failed to decode base64 [%s]", text_base64.c_str());
    return false;
  }

  if (NULL == base::HTMLToUTF8(temp_text, "", utf8_text)) {
    DLOG(WARNING) << StringPrintf("failed to convert to utf8, base64 [%s], decoded_base64 [%s]",
                               text_base64.c_str(), temp_text.c_str());
    return false;
  }
  return true;
}

bool ConvertTimeFromSecToFormat(const std::string& time_in_sec, const char* format,
                                std::string* formatted_time) {
  double sec;
  if (!base::StringToDouble(time_in_sec, &sec)) {
    DLOG(WARNING) << "failed to parse seconds: " << time_in_sec;
    return false;
  }
  base::Time parsed_time = base::Time::FromDoubleT(sec);

  formatted_time->clear();
  if (!parsed_time.ToStringInFormat(format, formatted_time)) {
    DLOG(WARNING) << "failed to convert time: " << time_in_sec << " to format: " << format;
    return false;
  }

  return true;
}

bool ConvertTimeFromFormatToSec(const std::string& formatted_time, const char* format,
                                int64* time_in_sec) {
  base::Time parsed_time;
  if (!base::Time::FromStringInFormat(formatted_time.c_str(), format, &parsed_time)) {
    DLOG(WARNING) << "failed to parse time format: " << formatted_time;
    return false;
  }
  *time_in_sec = parsed_time.ToDoubleT();
  return true;
}

std::string JoinFields(const std::vector<std::string>& flds, const std::string& delim, const char* fields) {
  // fileds is number sequence
  std::unordered_set<int> dedup;
  const char* p = fields;
  std::string str;
  for (; *p != '\0'; ++p) {
    int idx = *p - 48;
    CHECK(idx >= 0  && idx < (int)flds.size());
    CHECK(dedup.find(idx) == dedup.end());
    dedup.insert(idx);
    str += flds[idx];
    if (*(p+1) != '\0') {
      str += delim;
    }
  }
  return str;
}
}
