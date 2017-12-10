#pragma once

#include <vector>
#include <string>

#include "base/common/basic_types.h"

namespace crawler {
// 存储每一个 IP 边界节点
// REVIEW(suhua):  类名用大写
struct EdgeNode {
  uint32 ip;  // 32 bit ip
  // flag 有两种取值:  'b' or 'e'
  // 'b': IP 边界开始； 'e': IP 边界末尾（含边界）
  char flag;

  bool operator<(const EdgeNode &node2) const {
    return ip < node2.ip;
  }
};
// 从文件 |filename| 提取 ip  边界片段，保存在 vector |ip_scopes| 中
// 文件的格式如下 ：
// 103.5.36.0 - 103.5.39.255 : 103.5.36/22 : cn : 103.5.36 - 103.5.39 : APNIC
// 当文件名为空指针或者文件的格式不匹配时，函数会异常退出
bool BuildIPScopeFromFile(const char*filename, std::vector<EdgeNode> *ip_scopes);

// 转换形如 59.69.23.11 形式的 ip 到 uint32
bool ConvertIp2Int(const std::string &ip_str, uint32 &ip);

// 判断 uint32 ip 是否在给定 ip 断内
// 若在，返回 true ，反之，返回 false
bool InCurrentIPScope(const std::vector<EdgeNode> *ip_scopes, uint32 ip);

// 判断形如 59.69.23.11 形式 ip 是否在给定 ip 断内
// 若在，返回 true ，反之，返回 false
bool InCurrentIPScope(const std::vector<EdgeNode> *ip_scopes, const std::string &ip_str);

// 从 url 中抽取域名，例如： http://www.sohu.com/news/weibo.html 的域名为：www.sohu.com
// bool GetDomainFromUrl(const std::string &src_url, std::string &domain);

// 计算域名 |domain| 对应的 ip 列表
bool GetIpFromDomain(const std::string &domain, std::vector<std::string> *ips);

// 判断域名 |domain| 是否是国内的域名。一个域名通常由多个 ip ，称一个域名为国内域名，当且仅当
// 该域名的所有 ip 均为国内 ip
// |ip_scopes| 为国内 ip 段，必须事先存在。
bool IsDomainInCountry(const std::string &domain, const std::vector<EdgeNode> *ip_scopes);
}  // end namespace
