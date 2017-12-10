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

  std::string output_key;
  std::string output_tmp1;
  std::string output_tmp2;
  std::string output_tmp3;
  std::string output_tmp4;
  std::string output_tmp5;
  std::string output_value1;
  std::string output_value2;
  std::string output_value3;
  std::string output_value4;
  std::string output_value5;

  crawler2::Resource resource;
  web::UrlNorm url_norm;

  while (task->NextKeyValue(&key, &value)) {
    resource.Clear();
    CHECK(resource.ParseFromString(value));
    CHECK(resource.brief().has_effective_url());

    std::string effective_click_url;
    if (!web::RawToClickUrl(resource.brief().effective_url(), NULL, &effective_click_url)) {
      task->ReportAbnormalData("RawToClickUrlFailed", key, resource.brief().effective_url());
      continue;
    }

    std::string url;
    CHECK(url_norm.NormalizeClickUrl(effective_click_url, &url))
        << "effective_click_url: " << resource.brief().effective_url();
    resource.mutable_brief()->set_url(url);

    crawler2::GenNewHbaseKey(url, &output_key);

    output_tmp1.clear();
    output_tmp2.clear();
    output_tmp3.clear();
    output_tmp4.clear();
    output_tmp5.clear();

    output_value1.clear();
    output_value2.clear();
    output_value3.clear();
    output_value4.clear();
    output_value5.clear();

    if (resource.content().has_utf8_content()) {
      // NOTE: 注意, 目前入库时需要清理掉 utf8 content
      resource.mutable_content()->clear_utf8_content();
    }

    resource.brief().SerializeToString(&output_tmp1);
    resource.content().SerializeToString(&output_tmp2);
    resource.parsed_data().SerializeToString(&output_tmp3);
    resource.graph().SerializeToString(&output_tmp4);
    resource.alt().SerializeToString(&output_tmp5);

    crawler2::GenHbaseValue("brief", output_tmp1, &output_value1);
    crawler2::GenHbaseValue("content", output_tmp2, &output_value2);
    crawler2::GenHbaseValue("parsed_data", output_tmp3, &output_value3);
    crawler2::GenHbaseValue("graph", output_tmp4, &output_value4);
    crawler2::GenHbaseValue("alt", output_tmp5, &output_value5);

    task->Output(output_key, output_value1);
    task->Output(output_key, output_value2);
    task->Output(output_key, output_value3);
    task->Output(output_key, output_value4);
    task->Output(output_key, output_value5);
  }
}

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "encoding");
  CHECK(base::mapreduce::IsMapApp());
  Mapper();
}

