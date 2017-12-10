#pragma once

#include <string>

namespace crawler2 {

// 由 |url| 生成此 url 在 库中的存储 key,
// key 包括两个部分: shard id 和 revert url, 以 - 分隔
//
// NOTE: 请勿再使用 GenHbaseKey(), 用 GenNewHbaseKey() 来替代
void GenHbaseKey(const std::string &url, std::string *hbase_key);

// hbase key 升级了
void GenNewHbaseKey(const std::string &url, std::string *hbase_key);
bool IsNewHbaseKey(const std::string &new_hbase_key);
bool UpdateHbaseKey(const std::string &hbase_key, std::string *new_hbase_key);
// 同时拿到 shard id
void GenNewHbaseKeyAndShardID(const std::string &url, std::string *hbase_key, int *shard_id);

// 对 url 的 user/pass, host, port 部分，进行反转
//
// 对于合法的 url, 总是处理成功, 并返回 true
// 对于不合法的 url, 程序行为未定义, 会尽量检测并返回 false
//
// NOTE: 该函数只从字面上反转上述字段, 不会做任何的 url 归一化 操作
bool ReverseUrl(const std::string &url, std::string *reversed_url);

// DEPRECATED(suhua): 该函数不应再被调用
// 生成 |value| 在 库中的存储值
void GenHbaseValue(const char *family, const std::string &value, std::string *hbase_value);

}  // namespace crawler2

