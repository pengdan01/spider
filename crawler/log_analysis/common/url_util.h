#pragma once

#include <string>

namespace log_analysis {

namespace util {

enum UrlValue {
  // type worthless for se
  kAds,            // 广告页面
  kSearchResult,     // 搜索页, 包括大搜索, 垂直, 站内 等
  kListPage,       // 列表页
  kToolsPage,
  kPrivatePage,
  kUnknown,

  // NOTE: 不要随便调整 worthless 的次序
  // normal 或者 worthy 总是需要比 worthless 高
  // worthy 总是要比 normal 高

  // normal for se
  kNormal,

  // worthy for se
  kSearchClick,  // 搜索结果, 需要给出 dest 和 ref 才能判定
  kQA,           // 问答
  kWiki,         // wiki, 百科等
  kBlog,         // 博客, 微博等
  kForum,        // 论坛, 贴吧等
};

// 使用了线程安全的静态对象 Re3
bool WorthlessUrlForSE(const std::string& url);

}  // namespace
}  // namespace
