#include <vector>
#include <algorithm>
#include <utility>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/mapreduce/mapreduce.h"
#include "web/url/url.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_codepage_conversions.h"
#include "crawler/proto/crawled_resource.pb.h"
#include "crawler/proto2/resource.pb.h"
#include "crawler/proto2/proto_transcode.h"
#include "crawler/proto2/gen_hbase_key.h"
#include "web/html/api/html_parser.h"

void Mapper() {
  auto task = base::mapreduce::GetMapTask();

  std::string key;
  std::string value;

  std::string output_key;
  std::string output_value;
  crawler2::Resource res;
  crawler::CrawledResource crawled_res;

  int64 bad_num = 0;
  int64 bad_raw_bytes = 0;

  while (task->NextKeyValue(&key, &value)) {
    // key:  16384-http://...
    if (key.size() ==  0) {
      LOG(WARNING) << "ignore invalid key: " << key;
      ++bad_num;
      bad_raw_bytes += value.size();
      continue;
    }

    res.Clear();
    CHECK(crawler2::HbaseDumpToResource(value.data(), value.size(), &res));

    // convert crawler2::Resource to crawler::CrawledResource
    crawled_res.Clear();
    crawler2::ResourceToCrawledResource(res, &crawled_res);
    output_value.clear();
    CHECK(crawled_res.SerializeToString(&output_value));

    // build old key
    const std::string &source_url = res.brief().source_url();

    CHECK(web::ReverseUrl(source_url, &output_key, false));

    task->Output(output_key, output_value);
  }
}

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "convert to old css format");
  CHECK(base::mapreduce::IsMapApp());
  Mapper();
}

