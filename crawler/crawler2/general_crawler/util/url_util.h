#pragma once
#include <string>
#include <vector>

#include "web/url/url.h"
#include "crawler2/general_crawler/url.pb.h"

namespace crawler3 {

const int kMaxPathDepth = 10;
const int kMaxQueryLength = 1000;
const int kMaxLinkSize= 1024;

// 对 |url| 使用过滤规则进行过滤, 目标: 过滤掉所有无效的 URL
// |reason| 若 |url| 被过滤, 且 |reason| 不为空, 存储过滤的原因
//
// Return:
// 当 |url| 满足某条过滤规则而被成功过滤时, 返回 true
// 当 不满足任何过滤规则时, 返回 false
// NOTE:
// 1. url 必须是 RawToClick 处理过的, 否则, 可能存在中文编码问题
bool WillFilterAccordingRules(const std::string &url, std::string *reason);

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


// 对 taobao 商品页 url 进行改写:
// FROM:
// http://item.taobao.com/item.htm?id=19566056574&ali_refid=a3_421052_1007:1103788978:7:94694e6acfd46a81f0a150eaaa84293d:9b845099f6694210a653e80cc5b57759&ali_trackid=1_9b845099f6694210a653e80cc5b57759  // NOLINT
// or http://detail.tmall.com/item.htm?spm=a2106.m874.1000384.d11.tsGaMA&id=16272290352
// TO:
// http://item.taobao.com/item.htm?id=19566056574
// http://detail.tmall.com/item.htm?id=16272290352
// Return: 成功返回 true; 失败返回 false
bool RewriteTaobaoItemDetail(std::string *url);

// 首先, 调用 RewriteTaobaoItemDetail 对 taobao 商品 url 进行重写,
// 中间输出的 url 格式类似:
// http://item.taobao.com/item.htm?id=19566056574 or
// http://detail.tmall.com/item.htm?id=16272290352
// 其次, 将其转换对应的 手机版本 url 格式
bool ConvertTaobaoUrlFromPCToMobile(const std::string &pc_url, std::string *m_url);

// 对 taobao 商品 list 页 url 进行改写:
// FROM:
// http://list.taobao.com/itemlist/peishi2011a.htm?spm=1.124404.152499.105&q=&ex_q=&isprepay=1&olu=yes&cat=50010404&sort=commend&msp=1&viewIndex=1&yp4p_page=0&commend=all&atype=b&style=grid&s=0&path=50010404&isnew=2&smc=1&smc=1&rr=1&user_type=0#!cat=50096151&isprepay=1&user_type=0&msp=1&as=0&viewIndex=1&yp   // NOLINT
// TO:
// http://list.taobao.com/itemlist/peishi2011a.htm?q=&ex_q=&isprepay=1&olu=yes&cat=50010404&sort=commend&msp=1&viewIndex=1&yp4p_page=0&commend=all&atype=b&style=grid&s=0&path=50010404&isnew=2&smc=1&smc=1&rr=1&user_type=0
// Return: 成功返回 true; 失败返回 false
bool RewriteTaobaoItemList(std::string *url);

// taobao 商品 list 页会有多个翻页, 本函数构造其他翻页对应的 url
// |url|: 展现在第一页对应的 url;
// |n|  : 构造首页后的多少个 url
// |next_n_url| : 构造的 url 保存在该数组中
//
// return: void
// NOTE(pengdan):
// 1. |url| 必须是经过 RewriteTaobaoItemList 处理过的
void BuildNextNUrl(const std::string &url, int n, std::vector<std::string> *next_n_url,
                   int each_page_item_num = 96);

// 由 手机版的 淘宝商品 url 构造其评论 url
// FROM:
// http://a.m.taobao.com/i18178811690.htm
// TO:
// http://a.m.taobao.com/ajax/rate_list.do?item_id=18178811690&p=1&ps=30
//
bool BuildTaobaoCommentLink(const std::string &url, std::string *comment_url, int latest_comment_num = 150);

// 由 手机版的 淘宝商品 url 构造其商品详情 url
// FROM:
// http://a.m.taobao.com/i18178811690.htm
// TO:
// http://a.m.taobao.com/ajax/desc_list.do?item_id=18178811690
//
bool BuildTaobaoProductDetailLink(const std::string &url, std::string *detail_url);
//
// 由 手机版的 淘宝商品 url 构造其商品参数 url
// FROM:
// http://a.m.taobao.com/i18178811690.htm
// TO:
// http://a.m.taobao.com/ajax/param.do?item_id=18178811690
//
bool BuildTaobaoProductParamLink(const std::string &url, std::string *detail_url);

// 从 taobao 返回的 list 页面(json 格式) 提取出 商品 item 信息
// page must be utf8 coding
//
bool ParseJsonFormatPage(const std::string &json_page_utf8,
                         std::vector< ::crawler3::url::TaobaoProductInfo> *items, int *page_num);

// 从天猫 list 页面中提取出搜索生成的总的结果页面数
bool GetPageNumFromTmallListPage(const std::string &page_utf8, int *page_num);

// 从 京东的 list 页面中提取出搜索生成的总的结果页面数
bool GetPageNumFromJingDongListPage(const std::string &page_utf8, int *page_num);
// NOTE(pengdan):
// 网页必须是 utf8 编码的

// 构造京东的 list 页面的 翻页 url
// |first_url|: format like: http://www.360buy.com/products/1315-1343-4001.html
bool GetJingDongNextUrl(const std::string &first_url, int next_n_page, std::vector<std::string> *urls);

// 由京东商品页面 url 构造该商品对应的价格图片 url
bool BuildJingDongPriceImageLink(const std::string &url, std::string *price_image_url);

// 由京东商品页面 url 构造该商品对应的评论数据(json) url
bool BuildJingDongCommentLink(const std::string &url, std::string *comment_url);


// 从 苏宁的 list 页面中提取出搜索生成的总的结果页面数
bool GetPageNumFromSuNingListPage(const std::string &page_utf8, int *page_num);
// NOTE(pengdan):
// 网页必须是 utf8 编码的

// 构造京东的 list 页面的 翻页 url
// |first_url|: format like: http://www.360buy.com/products/1315-1343-4001.html
bool GetSuNingNextUrl(const std::string &first_url, int next_n_page, std::vector<std::string> *urls);

// 由苏宁商品页面 url 构造该商品对应的价格 url
bool BuildSuNingPriceLink(const std::string &url, std::string *price_image_url);

// 由苏宁商品页面 url 构造该商品对应的评论数据 url
bool BuildSuNingCommentLink(const std::string &url, std::string *comment_url);

// 由于天猫最多显示 前 100 页, 为了获取尽可能多的商品页, 采用不同的排序方法:
// 默认排序, 按照销量排序, 按照价格升(降)序
void BuildTmallDifferentSortType(const std::string &url, std::vector<std::string> *urls);

// 由于淘宝最多显示 前 100 页, 为了获取尽可能多的商品页, 采用不同的排序方法:
// 默认 销量 人气 信用 最新 总价
// NOTE(pengdan): 需要现调用 crawler3::RewriteTaobaoItemList() 处理
void BuildTaobaoDifferentSortType(const std::string &url, std::vector<std::string> *urls);

// 对图片进行转换, 目的: 抓取原始 size 的图片
// FROM: http://q.i02.wimg.taobao.com/bao/uploaded/i1/846481524/T2dgK_XmxXXXXXXXXX_!!846481524.jpg_70x70.jpg
// TO: http://q.i02.wimg.taobao.com/bao/uploaded/i1/846481524/T2dgK_XmxXXXXXXXXX_!!846481524.jpg
void ConvertTaobaoImageUrl(const std::string &raw, std::string *new_url);
}
