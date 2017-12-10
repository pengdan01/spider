#include "crawler/updater/updater_util.h"

#include <set>
#include <vector>
#include <fstream>

#include "base/common/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/string_tokenize.h"
#include "base/strings/string_number_conversions.h"
#include "web/url/gurl.h"
#include "base/hash_function/fast_hash.h"

#include "crawler/proto/crawled_resource.pb.h"

namespace crawler {
bool ExtractParameterValueFromHeader(const std::string &line_escape_header, const std::string &parameter,
                                     std::string *value) {
  CHECK_NOTNULL(value);
  if (line_escape_header.empty() || parameter.empty()) {
    DLOG(WARNING) << "header or parameter is empty string, header: " << line_escape_header
                 << ", parameter: " << parameter;
    return false;
  }
  // Case Sensitive
  // 由于 |line_escape_header| 可能叠加了多个 header, 从最后的一个 header 提取该参数
  // NOTE: 当 URL 重定向时, |line_escape_header| 包含了所有 Follow 的 header 信息
  std::string::size_type pos = 0;
  pos = line_escape_header.rfind(parameter);
  if (pos == std::string::npos) {
    DLOG(WARNING) << "Not find '" << parameter << "' in header:  " << line_escape_header;
    return false;
  }
  std::string::size_type pos2 = pos + parameter.size();
  if (!(line_escape_header[pos2] == ':' && line_escape_header[pos2+1] == ' ')) {
    DLOG(WARNING) << "Format error, according to header format, :[space] should follow " << parameter;
    return false;
  }
  pos2 += 2;
  pos = pos2 + 1;
  while (pos < line_escape_header.size()) {
    if (line_escape_header[pos] == '\r' || line_escape_header[pos] == '\\') break;
    ++pos;
  }
  if (pos == line_escape_header.size()) {
    DLOG(WARNING) << "Format error, according to header format, \\r\\n shoudld be found.";
    return false;
  }
  *value = line_escape_header.substr(pos2, pos-pos2);
  return true;
}

}  // namespace crawler

