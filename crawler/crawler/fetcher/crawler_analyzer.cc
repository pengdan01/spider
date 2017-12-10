#include "crawler/fetcher/crawler_analyzer.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_util.h"

DEFINE_string(crawler_vip_host, "crawler/fetcher/VIPhost.dat", "VIP host file");

namespace crawler {

static const std::set<std::string> *CreateVIPHostSet() {
  CHECK(!FLAGS_crawler_vip_host.empty());
  std::set<std::string> *vip_set = new std::set<std::string>();
  std::ifstream fin(FLAGS_crawler_vip_host);
  LOG_IF(FATAL, !fin.good()) << "Failed in open vip host file: " << FLAGS_crawler_vip_host
                             << ", using gflag --crawler_vip_host to specified it.";
  std::string line;
  while (getline(fin, line)) {
    base::TrimWhitespaces(&line);
    if (line.empty()) continue;
    vip_set->insert(line);
  }
  CHECK(fin.eof());
  LOG(INFO) << "VIP host loaded#: " << vip_set->size() << " records";
  return vip_set;
}

static const std::set<std::string> *GetVIPSet() {
  // XXX(suhua): gcc will guarantee it's thread safe
  // this is standard single-ton pattern.
  static const std::set<std::string> *vip = CreateVIPHostSet();
  return vip;
}

bool IsVIPHost(const std::string &host) {
  const std::set<std::string> *vip_dict = GetVIPSet();
  CHECK_NOTNULL(vip_dict);
  auto it = vip_dict->find(host);
  return it != vip_dict->end();
}
}  // end crawler
