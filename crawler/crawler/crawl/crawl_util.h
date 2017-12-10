#pragma once
#include <string>

namespace crawler2 {
namespace crawl {
// AJAX url is a URL containing a #! hash fragment
bool IsAjaxUrl(const std::string &url);
// 转换 AJAX url 到 可抓取 格式
void TransformAjaxUrl(const std::string &url, std::string *new_url);
}
}
