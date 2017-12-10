#pragma once

#include <fstream>
#include <unordered_set>
#include <deque>
#include <vector>
#include <string>

#include "crawler/fetcher2/fetcher.h"

namespace crawler2 {

// 读取输出抓取任务数据
void LoadUrlsToFetch(const std::string &file_name, std::deque<crawler2::UrlToFetch> *urls_to_fetch);

// 读取 https url 列表
void LoadHttpsFiles(const std::string &file_name, std::unordered_set<std::string> *https_urls);

// 读取抓取状态
//
// 除了 proto buffer 外, crawler2::UrlFetched 的所有字段均被设置.
void LoadUrlFetchedStatus(const std::string &file_name, std::vector<crawler2::UrlFetched> *urls_fetched);

// 将结构体 url_fetched 转换成单行字符串
void UrlFetchedToString(const crawler2::UrlFetched &url_fetched, std::string *str);

// 将字符串转换成结构体 url_fetched
void StringToUrlFetched(const std::string& str, crawler2::UrlFetched* url_fetched);

bool ReadKeyValue(std::istream *is, std::string *key, std::string *value);

}  // namespace crawler2

