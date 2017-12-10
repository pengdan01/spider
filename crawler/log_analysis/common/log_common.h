#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include "base/common/basic_types.h"
#include "base/common/logging.h"

namespace log_analysis {

// 通用搜索引擎的名称常量
const char kBaidu[]         = "Baidu";
const char kGoogle[]        = "Google";
const char kBing[]          = "Bing";
const char kSogou[]         = "Sogou";
const char kSoso[]          = "Soso";
const char kYahoo[]         = "Yahoo";
const char kYoudao[]        = "Youdao";

// 垂直搜索引擎的名称常量
const char kBaiduNews[]     = "BaiduNews";
const char kBaiduMp3[]      = "BaiduMp3";
const char kBaiduImage[]    = "BaiduImage";
const char kBaiduVideo[]    = "BaiduVideo";
const char kBaiduZhidao[]   = "BaiduZhidao";
const char kBaiduBaike[]    = "BaiduBaike";
const char kBaiduWenku[]    = "BaiduWenku";
const char kBaiduDict[]     = "BaiduDict";
const char kSosoBlog[]      = "SosoBlog";
const char kYoudaoGame[]    = "YoudaoGame";
const char kTaobao[]        = "Taobao";
const char kTaobaoTuan[]    = "TaobaoTuan";
const char kTaobaoPrice[]   = "TaobaoPrice";

// 给定一个 url, 判断是否是 通用搜索引擎 的 url.
//
// 如果是通用搜索, 且传人的 |query|, |search_engine| 参数不为 NULL,
// 将解析出的 url 中的 query, 以及搜索引擎的名称返回
// 名称 的取值在本文件头部常量定义区域
// NOTE: |query|, |search_engine| 可以为 NULL
bool IsGeneralSearch(const std::string& url,
                     std::string* query,
                     std::string* search_engine);

// 给定一个 url, 判断是否是 垂直搜索引擎 的 url.
//
// 如果是垂直搜索, 且传人的 |query|, |vertical_search| 参数不为 NULL,
// 将解析出的 url 中的 query, 以及搜索引擎的名称返回
// 名称 的取值在本文件头部常量定义区域
// NOTE: |query|, |vertical_search| 可以为 NULL
bool IsVerticalSearch(const std::string& url,
                     std::string* query,
                     std::string* vertical_search);

// 给定一个 url, 判断是否是 站内搜索 的 url.
//
// 如果是站内搜索, 且传人的 |query|, |site_name| 参数不为 NULL,
// 将解析出的 url 中的 query, 以及网站的名称返回
// 由于名称 的取值不设置常量
// NOTE: |query|, |site_name| 可以为 NULL
bool IsSiteInternalSearch(const std::string& url,
                          std::string* query,
                          std::string* site_name);

// 判断一个 url 是否是百度或谷歌的广告连接
// 如果是广告, 返回 true
//
// |src| 参数若不为 NULL,
// 则当 url 是广告连接时, 将跳转后的广告地址返回;
// 若不是广告连接, |src| 将为空串
// NOTE: |src| 可以为 NULL
bool IsAds(const std::string& orig_url, std::string* src);

// 给出 target 和 referer url, 判断是否是 通用搜索 的 非广告 点击
// 如果是点击, 且传人的 |query|, |search_engine| 参数不为 NULL,
// 则将 referer url 的 query 和 搜索引擎名称 返回
//
// NOTE: 当 referer url 是 通用搜索 引擎时, target url 是以下情况不认为是正常的搜索结果点击
// 1. 仍然是一个通用搜索结果页面 (用户点击了 相关搜索)
// 2. 是一个垂直搜索页面 (Google/Baidu 的特殊展现, 会将自家垂直频道的搜索链接嵌入到自然结果中)
//
// TODO(zhengying):
// 这个 api 当前的实现, 无法区分 在搜索结果页面直接点击频道页 的情况, 需要改进
bool IsGeneralSearchClick(const std::string& target_url, const std::string& referer_url,
                          std::string* query, std::string* search_engine);

// 解析 Google 结果页 url. Google 的很多搜索结果是以跳转链接呈现的.
// 本函数就是帮您从纷繁的 Google 跳转链接中解析出真正的目标 url
//
// 返回值: 如果 |orig_url| 是一个 Google 的跳转链接, 并且我们正确解析出了目标 url, 返回 true.
//         否则返回 false
//
// 参数:
// |orig_url|: 待判断的 url. 可以是任意的 url
// |target_url|: 目标 url. 如果函数返回 true, 那么 |target_url| 就是该目标 url.
//               否则 |target_url| 为空
// NOTE: |target_url| 可以为 NULL
bool ParseGoogleTargetUrl(const std::string& orig_url, std::string* target_url);

bool ParseBaiduTargetUrl(const std::string& orig_url, std::string* target_url);

inline std::string BaiduTargetUrl(const std::string& orig_url) {
  std::string target_url;
  if (!ParseBaiduTargetUrl(orig_url, &target_url)) {
    target_url = orig_url;
  }
  return target_url;
}

inline std::string GoogleTargetUrl(const std::string& orig_url) {
  std::string target_url;
  if (!ParseGoogleTargetUrl(orig_url, &target_url)) {
    target_url = orig_url;
  }
  return target_url;
}

//////////////////////////////////////////////
// NOTE: 以下函数在 log_analysis 模块内部使用.  不建议其他模块使用
//////////////////////////////////////////////

// 对 |url| 进行归一化处理, 并去除 |url| 的 query 部分
//
// 成功返回 true, |normed_url| 是归一化之后的结果
// 失败返回 false, |normed_url| == "", 只有当 url 非法时才会返回 false
bool abandon_url_query(const std::string& url, std::string* normed_url);

// 对 base64 编码的字符串进行解码, 并转换为 UTF8 编码的字符串
bool Base64ToUTF8(const std::string& text_base64, std::string* utf8_text) WARN_UNUSED_RESULT;

// 对 base64 编码的 url 进行解码, 并转换为 click url
// 处理成功返回 true; 解码失败、url 转换失败、不合法 url、 异常 url 等情况 返回 false
bool Base64ToClickUrl(const std::string& text_base64, std::string* click_url) WARN_UNUSED_RESULT;

// 时间转换函数. 对 //base/time/time.h 的封装
// |time_in_sec|: 从 1970 年 1 月 1 日 0 时 0 分 0 秒开始计算的秒数
// |format|: 表示时间格式的字符串, 如 "%Y%m%d%H%M%S"
bool ConvertTimeFromSecToFormat(const std::string& time_in_sec, const char* format,
                                std::string* formatted_time) WARN_UNUSED_RESULT;
bool ConvertTimeFromFormatToSec(const std::string& formatted_time, const char* format,
                                int64* time_in_sec) WARN_UNUSED_RESULT;

// 可以选择几列进行 Join
// NOTE: fields 在 0-9 之间，如果传入 "10" 会认为是要 1,0 两个字段
std::string JoinFields(const std::vector<std::string>& flds, const std::string& delim, const char* fields);
}
