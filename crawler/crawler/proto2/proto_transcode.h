#pragma once

#include <string>

#include "crawler/proto/crawled_resource.pb.h"
#include "crawler/proto2/resource.pb.h"

namespace crawler2 {

// 要求 input.has_url() 为 true
void CrawledResourceToResource(const crawler::CrawledResource &input,
                               crawler2::Resource *result);

void ResourceToCrawledResource(const crawler2::Resource &input,
                               crawler::CrawledResource *result);

// 将从网页库导出的 hbase dump format (转储格式) 数据 (数据 |data|, 长度 |len|),
// 转换成 proto Resource (存放到 |result| 中)
//
// 返回是否转换成功
bool HbaseDumpToResource(const char *data, int len, crawler2::Resource *result);

// 将 proto Resource, 转换成 Hbase dump format (转储格式)
//
// NOTE:
// 1. 要求 res.IsInitialized() 为 true, 否则会崩溃
// 2. 注意, 数据用于导入网页库时,
//    如果 res.has_xxx() == false, 则网页库中的 xxx 字段会保留原始值
//    只有 res.has_xxx() == true, 才会覆盖网页库中对应字段.
//    推论: res.has_xxx() 且 res.xxx() 的内容为 空, 则会清空对应字段.
void ResourceToHbaseDump(const crawler2::Resource &res, std::string *str);

}  // namespace crawler2
