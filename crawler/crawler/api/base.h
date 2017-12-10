#pragma once
#include <string>

#include "base/common/basic_types.h"
#include "web/url/url.h"

namespace crawler {
/*
// 对 |url| 进行语法格式上的检查以及归一化
// 归一化成功 返回 true
// 归一化失败 返回 false, 失败原因用 LOG(WARNING) 输出
//
// NOTE:
// 目前已经使用规则如下:
// 1. 忽略掉 url 开头或结尾多余的空白字符
// 2. url 最大长度检查, length(url) <= crawler::kMaxUrlSize(当前最大长度限制为 2KB)
// 3. 忽略掉 '#' 以及后面的所有参数
// 4. 若 url 没有 schema, 使用 http 补全该 url schema
// 5. 使用 GURL 该 url 进行语法格式等检查
//
bool NormalizeUrl(const std::string &url, std::string *normlized_url) WARN_UNUSED_RESULT;
*/
// 给定一个 click url, 计算该 url 将被分配到的 shard_id, shard_id 属于区间 [0, shard_num)
//
// |url|: 必须是 RawToClickUrl() 处理过的 click url, 否则分配到的 shard_id 可能是无效的!!!
// |shard_num|: shard 总数, page/css/image 的 shard 总数, 必须从 hadoop 文件
//              /app/crawler/shard_number_info 中读取
// |shard_id|: 分配到的 shard id
// |switch_rawtoclick_url|: 若 |switch_rawtoclick_url| 为 true, 则程序内部会首先调用 RawToClickUrl()
//                         对 |url| 进行处理, 然后分配 shard_id; 若 |switch_rawtoclick_url| 为
//                         false, 则调用方保证已经对 |url| 进行了 RawToClickUrl() 处理, 直接分配 shard_id
//
// 当 |url| RawToClickUrl() 处理失败时, 返回 false
// 当 RawToClickUrl() 处理成功或 |switch_rawtoclick_url| 为 false(跳过 RawToClickUrl()) 时, 返回 true
// 并设置 |shard_id|
//
bool AssignShardId(const std::string &url, int32 shard_num, int32 *shard_id, bool switch_rawtoclick_url)
    WARN_UNUSED_RESULT;

// 给定一个 click url, 计算该 click url 将被分配到的 reduce_id, reduce_id 属于区间 [0, reduce_num)
//
// |url|: 必须是 RawToClickUrl() 处理过的 url, 否则分配到的 reduce_id 可能是无效的!!!
// |reduce_num|: reduce 总数 |reduce_id|: 分配到的 reduce_id
// |switch_rawtoclick_url|: 若 |switch_rawtoclick_url| 为 true, 则程序内部会首先调用 RawToClickUrl() 对
//                         |url| 进行处理, 然后分配 reduce_id; 若 |switch_rawtoclick_url| 为 false,
//                         则调用方保证已经对 |url| 进行了 RawToClickUrl() 处理, 直接分配 reduce_id
//
// 当 |url| RawToClickUrl() 处理失败时, 返回 false
// 当 RawToClickUrl() 处理成功或 |switch_rawtoclick_url| 为 false(跳过 RawToClickUrl()) 时, 返回 true
// 并设置 |reduce_id|
//
bool AssignReduceId(const std::string &url, int32 reduce_num, int32 *reduce_id, bool switch_rawtoclick_url)
    WARN_UNUSED_RESULT;
/*
// 给定一个 url 或 rurl, 对其 host 部分进行反转操作, 例如:
// http://news.sina.com.cn/c/2012-03-27/022424177684.shtml 经过本函数处理后, 将输出
// http://cn.com.sina.news/c/2012-03-27/022424177684.shtml
// 对结果再调用一次本函数处理后, 将变成原样
//
// |input|: 待反转处理的 url 或 rurl
// |result|: 存放得到的 rulr 或 url
// |switch_rawtoclick_url|: 若 |switch_rawtoclick_url| 为 true, 则程序内部会首先调用 RawToClickUrl() 对
//                         |url| 进行处理; 若 |switch_rawtoclick_url| 为 false, 则调用方保证已经对 |url|
//                         进行了 RawToClickUrl() 处理
//
//
// 处理成功则返回 true, 且将结果写入 |result|
// 处理失败 (当 url RawToClick() 处理返回 false
//
// NOTE:
// 1. 关于 rurl 的详细定义, 参见术语表 //build_toold/doc/glossary.txt
// 2. 对于 IP, 不进行反转.
//
bool ReverseUrl(const std::string &input, std::string *result, bool switch_rawtoclick_url) WARN_UNUSED_RESULT;

// 给定一个 host, 解析该 host 的 tld, domain, subdomain
//
// |host|: 待解析的 host
// |tld|:  若解析成功, 存放解析出来的 tld
// |domain|: 若解析成功, 存放解析出来的 domain
// |subdomain|: 若解析成功, 存放解析出来的 subdomain
//
// 处理成功返回 true, 且将结果分别写入 |tld|, |domain|, |subdomain|;
// 如果 |host| 不合法 或者 |host| 为 IP 等, 返回 false
//
// NOTE: 1. 关于 host, tld, domain, subdomain 的详细定义, 参见术语表 //build_toold/doc/glossary.txt
//       2. 通过 --crawler_tld_file 指定 tld.dat 的本地路径, ParseHost() 需要使用到该词表文件;
//          --crawler_tld_file 默认取值 crawler/api/tld.dat
//       3. 如果调用模块在 hadoop 上运行, 则在提交 hadoop 任务参数中, 需要额外添加如下参数:
//          --mr_cache_archives /app/crawler/app_data/crawler.tar.gz#crawler(若 --crawler_tld_file 使用默认值)
//
bool ParseHost(const std::string &host, std::string *tld, std::string *domain,
               std::string *subdomain) WARN_UNUSED_RESULT;
*/
}  // namespace crawler
