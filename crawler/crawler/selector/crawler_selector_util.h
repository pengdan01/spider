#pragma once
#include <string>
#include "web/url/url.h"
namespace crawler {

const int kMaxPathDepth = 10;
const int kMaxQueryLength = 1000;

// 对 |url| 使用过滤规则进行过滤, 目标: 过滤掉所有无效的 URL
// |resion| 若 |url| 被过滤, 且 |resion| 不为空, 存储过滤的原因
// |strict_rule| 表示是否使用严格的过滤规则
//
// Return:
// 当 |url| 满足某条过滤规则而被成功过滤时, 返回 true
// 当 不满足任何过滤规则时, 返回 false
// NOTE:
// 1. 严格的过滤规则 和 相对宽松的过滤规则相比:  相对宽松的过滤规则 没有过滤掉 一些重要搜索引擎的结果页
// 2. url 必须是 RawToClick 处理过的, 否则, 可能存在中文编码问题
bool WillFilterAccordingRules(const std::string &url, std::string *resion, bool strict_rule);

// 判断 |url| 是否是 VIP url
// |type|: url 类型 1 html, 2, css, 3, Image
// |from|: 数据来源
// |parent_url|: |url| is extracted from |parent_url|, only valid when type is not 1 (html)
//
// 当前认为以下 四类的 url 是 VIP url
// 1. Search Click Page
// 2. High Page-Rank page
// 3. Site Home Page: (不包含 QQ, Weibo 等泛域名)
// 4. Page showed in first-result page (Usually we configed those data in User Seed Conf file)
//
// Return:
// true for VIP url and false not
bool IsVIPUrl(const std::string &url, int type, char from, const std::string &parent_url, char parent_from);

// 对于一个 query, 大搜索一般会给出成千上万个 结果.
// 本函数判断一个给定的 搜索引擎链接是否是前 |N| 页
// |url|: 必须为搜索引擎生成的查询结果页链接, 否则返回 false
// |N|: 前 N 页, N start from 1

// 当前支持以下通用 搜索引擎
// 1. Google
// 2. Baidu
// 3. Bing
// 4. Sogou
// 5. Soso
//
// Return: 当时前 N 结果页时返回 true; 其他情况返回 false
bool IsGeneralSearchFirstNPage(const std::string &url, int N);

// 对于一个 query, 垂直搜索一般会给出成千上万个 结果.
// 本函数判断一个给定的 搜索引擎链接是否是前 |N| 页
// |url|: 必须为搜索引擎生成的查询结果页链接, 否则返回 false
// |N|: 前 N 页, N start from 1

// 当前支持以下通用垂直搜索
// 1. Baidu News
// 2. Baidu Zhidao
// 3. Baidu Baike
// 4. Baidu Wenku
//
// Return: 当时前 N 结果页时返回 true; 其他情况返回 false
bool IsVerticalSearchFirstNPage(const std::string &url, int N);

// 本函数在提取新的链接中用到, 用于判断一个链接是否是 BlackHole 链接
//
// |target_url|: 新提取的待判断的链接
// |parent_url|: |target_url| 从 |parent_url| 中提取出来的

//
// Return: ture or false
// NOTE:
// 定义 BlackHost 链接为: 从一个搜索结果页 A 中提取出来的一个搜索条件组合链接 B,
// B 和 A 的 host 相同
bool IsBlackHoleLink(GURL *target_url, GURL *parent_url);

// 本函数在提取新的 图片 链接中用到, 用于判断一个 图片 链接是否有价值抓取
//
// |target_url|: 待判断的 图片链接
//
// Return: ture or false
// NOTE:
// 1. 本函数不会判断 |target_url| 是否时 图片链接, 请使用方确保
// 2. 对于一些论坛, 贴吧, QQ 等 个人头像是没有抓取价值的
bool IsValuableImageLink(const std::string &target_url);
}
