#pragma once

#include <string>
#include "crawler/proto/crawled_resource.pb.h"

namespace crawler {
// 替换 url 的 domain
bool FastReplaceAlias(std::string* url);
std::string ReplaceAlias(const std::string& url);

enum {
  kNotWorthless = 0,   // 有价值页面
  kAds,            // 广告页面
  kActionPage,     // 用户行为页
  kSearchResult,   // 搜索结果
  kListPage,       // 列表页
  kClientPage,     // 客户端软件访问的页面
  kToolsPage,      // 工具页面
  kPrivatePage,    // 隐私页面
  kOthersPage,        // 位置原因
};

// 如果 URL 是对搜索引擎有价值的页面，返回 kNotWorthless
// 否则返回原因
int IsWorthlessPage4SE(const std::string& url);


// 资源类型
// 资源类型定义与 crawler/proto/ 当中
int URLResourceType(const std::string& url);

// 是否是搜索结果页
// 搜索结果页包含站内搜索结果， 垂直搜索和通用搜索三种
// 如果是其中任何一种都会返回 true
// 此处依赖于 log_analysis 当中对搜索结果的判断
bool IsSearchResult(const std::string& url);


// 整理URL
// 如果返回 false， URL 可以直接丢弃
// 否则应该使用 new_url 进行抓取
bool TidyURL(const std::string& url, std::string* new_url);
}  // namespace crawler
