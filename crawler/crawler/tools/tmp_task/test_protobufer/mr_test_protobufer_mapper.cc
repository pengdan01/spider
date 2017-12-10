#include <iostream>
#include <string>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_printf.h"
#include "base/mapreduce/mapreduce.h"
#include "web/url/url.h"
#include "crawler/proto2/resource.pb.h"
#include "crawler/proto2/proto_transcode.h"

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "parse html from HFfile");
  CHECK(base::mapreduce::IsMapApp());
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  CHECK_NOTNULL(task);

  std::string key;
  std::string value;
  crawler2::Resource pb;

  // 网页数据输入的 key 为 source_url, value 为一个 hbase dump 格式
  while (task->NextKeyValue(&key, &value)) {
    if (key.empty() || value.empty()) continue;
    pb.Clear();
    LOG_IF(FATAL, !crawler2::HbaseDumpToResource(value.data(), value.size(), &pb))
        << "crawler2::HbaseDumpToResource() fail, value: " << value;
    // css data
    // const std::string &url = pb.brief().source_url();
    // if (pb.content().has_raw_content()) {
    //   task->Output(url, base::StringPrintf("%d", (int)pb.content().raw_content().size())); 
    // } else {
    //   LOG(ERROR) << "has no raw content, rt code: " << pb.brief().crawl_info().response_code()
    //              << ", url: " << pb.brief().source_url();
    // }
    // Brief  referer
    CHECK(pb.has_brief());
    CHECK(pb.brief().has_referer());
    task->Output(pb.brief().source_url(), pb.brief().referer()); 
  }
  return 0;
}
