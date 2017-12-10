#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/string_split.h"
#include "base/strings/string_tokenize.h"
#include "base/mapreduce/mapreduce.h"
#include "crawler/api/base.h"
#include "web/url/gurl.h"

DEFINE_string(domain_filepath, "./domain_set.dat", "The filename of domain set");

bool LoadFile(const std::string &filename, std::set<std::string> *domains);

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "");
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  CHECK_NOTNULL(task);

  CHECK(!FLAGS_domain_filepath.empty());

  std::set<std::string> domain_set;
  std::set<std::string>::const_iterator it;
  // 加载文件倒内存
  LOG_IF(FATAL, !LoadFile(FLAGS_domain_filepath, &domain_set)) << "Failed in load domain file, file: "
                                                               << FLAGS_domain_filepath;
  std::string key;
  std::string value;
  std::vector<std::string> tokens;
  char type;

  // 输入数据: linkbase record
  while (task->NextKeyValue(&key, &value)) {
    if (key.empty() || value.empty()) continue;
    tokens.clear();
    base::SplitString(value, "\t", &tokens);
    type = tokens[1][0];
    if (type != '1') continue;

    const std::string &url = tokens[0];
    GURL gurl(url);
    const std::string &host = gurl.host();
    it = domain_set.find(host);
    if (it != domain_set.end()) {
      task->Output(url, "");
      continue;
    }
    std::string domain;
    if (!crawler::ParseHost(host, NULL, &domain, NULL)) {
      LOG(ERROR) << "ParseHost fail, host: " << host;
      continue;
    }
    it = domain_set.find(domain);
    if (it != domain_set.end()) {
      task->Output(url, "");
    }
  }
  domain_set.clear();
  return 0;
}

bool LoadFile(const std::string &filename, std::set<std::string> *domains) {
  CHECK_NOTNULL(domains);
  if (filename.empty()) {
    LOG(ERROR) << "Filename is empty";
    return false;
  }
  std::ifstream fin(filename);
  if (!fin.good()) {
    LOG(ERROR) << "Failed in open file: " << filename;
    return false;
  }
  std::string line;
  while (getline(fin, line)) {
    base::TrimWhitespaces(&line);
    if (line.empty()) continue;
    domains->insert(line);
  }
  if (!fin.eof()) {
    LOG(ERROR) << "File is not normally end(fin.eof()==false)";
    return false;
  }
  return true;
}
