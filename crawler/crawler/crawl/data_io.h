#pragma once

#include <fstream>
#include <unordered_set>
#include <unordered_map>
#include <deque>
#include <vector>
#include <string>

namespace crawler2 {
namespace crawl {

// 读取 https url 列表
void LoadHttpsFiles(const std::string &file_name, std::unordered_set<std::string> *https_urls);

// 读取 client redirect urls 文件
void LoadClientRedirectFile(const std::string& file_name,
                            std::unordered_map<std::string, std::string>* client_redirect_urls);
}  // namespace crawl
}  // namespace crawler2

