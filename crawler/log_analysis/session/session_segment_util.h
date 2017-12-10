#pragma once
#include <vector>
#include <string>

namespace session_segment {

// 切分 pv log 的 session
// pv log 格式见 http://192.168.11.60:8081/pages/viewpage.action?pageId=1671229
// agentid timestamp url ref_url ref_query ref_search_engine ref_anchor url_attr entrance_id duration
// 前 4 个字段必须有
//
// 程序默认 pv log 的每个文件均按照 agentid 排序，相同 agentid 按照 timestamp
// 排序从小到大排序, 在此基础上进行 session 的切分
//
// 输入: pv log 的一行 (注意,如果是 streaming 的 mapred 输入, line = key + "\t" + value)
// 输出: 对应每条 log 的 session_id, id 号从 1 往上累加
void PvLogSessionID(const std::vector<std::string> &lines,
                    std::vector<int> *session_results);

// 同上, 输入为标识 session 的签名:
// 签名由 adgenid + session first pv timestamp + sessionid 计算得到
void PvLogSessionMD5(const std::vector<std::string> &lines,
                     std::vector<std::string> *session_results);

// 切分 search log 的 session
// search log 格式见 http://192.168.11.60:8081/pages/viewpage.action?pageId=1671227
// searchid agentid timestamp query se_name rank ref_url
//
// 程序默认 search log 的每个文件均按照 agentid 排序，相同 agentid 按照
// timestamp 排序从小到大排序, 相同 timestamp 按照 rank 从小到大排序
// 在此基础上进行 session 的切分
//
// 输入: search log 的一行 (注意,如果是 streaming 的 mapred 输入, line = key + "\t" + value)
// 输出: 对应每条 log 的 session_id, id 号从 1 往上累加
void SearchLogSessionID(const std::vector<std::string> &lines,
                        std::vector<int> *session_results);

// 同上, 输出为标识 session 的签名
// 签名由 adgenid + this session first pv timestamp + sessionid 计算得到
void SearchLogSessionMD5(const std::vector<std::string> &lines,
                         std::vector<std::string> *session_results);
}
