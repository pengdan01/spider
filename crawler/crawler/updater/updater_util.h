#pragma once
#include <string>

#include "base/common/basic_types.h"

namespace crawler {

// 从 base::LineEscapeString() 处理过的 HTTP Header 中提取相关参数的值
// 提取成功 返回 true
// 提取失败 返回 false, 失败原因用 LOG(WARNING) 输出
//
// |line_escape_header|: base::LineEscapeString() 处理过的 HTTP Header
// |parameter|: HTTP Header 中的参数, 如: Last-Modified, Date..
// NOTE:
//
bool ExtractParameterValueFromHeader(const std::string &line_escape_header, const std::string &parameter,
                                     std::string *value) WARN_UNUSED_RESULT;
}  // namespace crawler
