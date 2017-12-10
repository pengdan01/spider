#include <vector>
#include <algorithm>
#include <utility>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/mapreduce/mapreduce.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"
#include "web/url_norm/url_norm.h"
#include "web/url/url.h"
#include "crawler/proto2/resource.pb.h"
#include "crawler/proto2/proto_transcode.h"
#include "crawler/proto2/gen_hbase_key.h"


void Mapper() {
  auto task = base::mapreduce::GetMapTask();

  std::string key;
  std::string value;

  std::string click_url;
  std::string normed_url;
  std::string hbase_key;

  web::UrlNorm url_norm;
  while (task->NextKeyValue(&key, &value)) {
    click_url.clear();
    normed_url.clear();
    hbase_key.clear();

    // XXX(suhua): 注意, hbase key 实际上需要的是 effective url; 但是这里 mr
    // 输入数据的 key, 有可能是 source url (比如是从日志中得到, 还没有抓取过,
    // 自然没有 source url), 因此会有一定的偏差.
    const std::string &url = key;
    if (!web::RawToClickUrl(url, NULL, &click_url)) {
      task->ReportAbnormalData("RawToClickUrlFailed", key, value);
      continue;
    }

    CHECK(url_norm.NormalizeClickUrl(click_url, &normed_url))
        << "key: " << key
        << ", click_url: " << click_url;

    hbase_key.clear();
    crawler2::GenNewHbaseKey(normed_url, &hbase_key);

    task->Output(hbase_key, value);
  }
}

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv,
                                 "consider key as url and convert it to hbase key, "
                                 "and then output hbase key and value.");
  CHECK(base::mapreduce::IsMapApp());
  Mapper();
}

