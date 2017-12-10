#include <iostream>
#include <string>
#include <algorithm>
#include <vector>

#include "base/common/base.h"
#include "base/file/file_util.h"
#include "base/common/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/string_tokenize.h"
#include "base/strings/string_printf.h"
#include "base/mapreduce/mapreduce.h"
#include "crawler/proto/crawled_resource.pb.h"
#include "crawler/offline_analyze/offline_analyze_util.h"

// 可以使用这个工具在本地运行，生成图片文件，每个文件只含有
// http body

struct Image {
  std::string value;
  std::string image_type;
};

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "offline analyze");
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  CHECK_NOTNULL(task);

  std::string key, value;
  crawler::CrawledResource pb;
  std::vector<Image> buffers;
  std::string image_type;
  std::vector<std::string> tokens;
  int start_id = 0;
  // 网页数据输入的 key 为 source_url, value 为一个 proto buffer
  while (task->NextKeyValue(&key, &value)) {
    if (key.empty() || value.empty()) continue;
    if (!pb.ParseFromString(value)) {
      LOG(ERROR) << "pb.ParseFromString(value) fail";
      continue;
    }
    const std::string &page_data = pb.content_raw();
    const std::string &head_data = pb.http_header_raw();
    if (page_data.empty() || head_data.empty()) {
      continue;
    } else {
      image_type = pb.crawler_info().content_type();
      if (2 != base::Tokenize(image_type, "/", &tokens)) {
        std::cerr << "image_type: " << image_type << ", field# !=2, igored.\n";
        continue;
      }
      if (base::StartsWith(tokens[1], "html", false)) {
        std::cerr << "abnormal type: " << tokens[1] << std::endl;
        continue;
      }
      image_type = tokens[1];
      base::Tokenize(image_type, ";", &tokens);
      Image img;
      img.value = page_data;
      img.image_type = tokens[0];
      buffers.push_back(img);
      if (buffers.size() > 50000) break;
    }
  }
  random_shuffle(buffers.begin(), buffers.end());
  int cnt = 10000;
  if ((int)buffers.size() < cnt) cnt = (int)buffers.size();
  for (int i = 0; i < cnt; ++i) {
    std::string path = base::StringPrintf("data/image_%d.%s", start_id++, buffers[i].image_type.c_str());
    base::FilePath filepath(path);
    LOG_IF(FATAL, -1 == base::file_util::WriteFile(filepath, &buffers[i].value[0], buffers[i].value.size()))
        << "Write fail";
  }
  std::cout << "File Counter: " << start_id << std::endl;
  return 0;
}
