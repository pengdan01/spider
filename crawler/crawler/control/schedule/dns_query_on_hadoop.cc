// 对待抓取的 url host 去重
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <set>
#include <vector>
#include <string>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/mapreduce/mapreduce.h"
#include "base/encoding/line_escape.h"
#include "base/hash_function/city.h"
#include "base/strings/string_split.h"
#include "base/strings/string_number_conversions.h"
#include "web/url/gurl.h"

#include "crawler/util/dns_resolve.h"

static const char KDNSServer[] = "180.153.240.21,180.153.240.22,180.153.240.23,180.153.240.24,180.153.240.25";

void Mapper() {
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  std::string key;
  std::string value;
  std::set<std::string> url_hosts;
  while (task->NextKeyValue(&key, &value)) {
    const std::string &url=key;
    GURL gurl(url);
    url_hosts.insert(gurl.host());
  }
  auto it = url_hosts.begin();
  for ( ; it != url_hosts.end(); ++it) {
    task->Output(*it, "");
  }
}

void Reducer() {
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  std::string key;
  std::string value;
  std::set<std::string> url_hosts;
  while (task->NextKey(&key)) {
    task->Output(key, "");
  }
}

int main(int argc, char *argv[]) {
  base::mapreduce::InitMapReduce(&argc, &argv, "dns query");
  if (base::mapreduce::IsMapApp()) {
    Mapper();
  } else if (base::mapreduce::IsReduceApp()) {
    Reducer();
  } else {
    CHECK(false);
  }
}
