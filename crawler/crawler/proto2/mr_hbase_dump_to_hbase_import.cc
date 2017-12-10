#include <vector>
#include <algorithm>
#include <utility>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/mapreduce/mapreduce.h"
#include "base/zip/snappy.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"
#include "crawler/proto2/resource.pb.h"
#include "crawler/proto2/proto_transcode.h"
#include "crawler/proto2/gen_hbase_key.h"

void Mapper() {
  auto task = base::mapreduce::GetMapTask();

  std::string key;
  std::string value;

  std::string output_tmp;
  std::string output_key;
  std::string output_value;

  crawler2::Resource resource;

  while (task->NextKeyValue(&key, &value)) {
    resource.Clear();
    crawler2::HbaseDumpToResource(value.data(), value.size(), &resource);

    if (!crawler2::IsNewHbaseKey(key)) {
      output_key.clear();
      if (!crawler2::UpdateHbaseKey(key, &output_key)) {
        output_key = key;
      }
    } else {
      output_key = key;
    }

    if (resource.has_brief()) {
      output_tmp.clear();
      output_value.clear();
      resource.brief().SerializeToString(&output_tmp);
      crawler2::GenHbaseValue("brief", output_tmp, &output_value);
      task->Output(output_key, output_value);
    }
    if (resource.has_content()) {
      output_tmp.clear();
      output_value.clear();

      if (resource.content().has_raw_content()) {
        // TODO(suhua): 临时添加的代码, 爬虫 和 网页库 全部升级后, 删除之
        CHECK(!resource.content().has_snappy_raw_content());
        // 生成压缩过的 snappy_raw_content();
        base::snappy::Compress(resource.content().raw_content().data(),
                               resource.content().raw_content().size(),
                               resource.mutable_content()->mutable_snappy_raw_content());
        // 清除 proto 中的 raw_content();
        resource.mutable_content()->clear_raw_content();
      }

      resource.content().SerializeToString(&output_tmp);
      crawler2::GenHbaseValue("content", output_tmp, &output_value);
      task->Output(output_key, output_value);
    }
    if (resource.has_parsed_data()) {
      output_tmp.clear();
      output_value.clear();
      resource.parsed_data().SerializeToString(&output_tmp);
      crawler2::GenHbaseValue("parsed_data", output_tmp, &output_value);
      task->Output(output_key, output_value);
    }
    if (resource.has_graph()) {
      output_tmp.clear();
      output_value.clear();
      resource.graph().SerializeToString(&output_tmp);
      crawler2::GenHbaseValue("graph", output_tmp, &output_value);
      task->Output(output_key, output_value);
    }
    if (resource.has_alt()) {
      output_tmp.clear();
      output_value.clear();
      resource.alt().SerializeToString(&output_tmp);
      crawler2::GenHbaseValue("alt", output_tmp, &output_value);
      task->Output(output_key, output_value);
    }
  }
}

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "encoding");
  CHECK(base::mapreduce::IsMapApp());
  Mapper();
}

