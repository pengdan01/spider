#include <iostream>
#include <string>
#include <unordered_map>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/encoding/line_escape.h"
#include "base/strings/string_printf.h"
#include "base/mapreduce/mapreduce.h"
#include "base/strings/utf_codepage_conversions.h"
#include "web/url/url.h"
#include "crawler/proto2/resource.pb.h"
#include "crawler/proto2/proto_transcode.h"
#include "crawler2/price_recg/price_recg.h"

DEFINE_string(path, "./data", "存放识别图片价格时需要的训练数据的路径, 该路径下包含了 training, testing 目录");

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "");
  CHECK(base::mapreduce::IsMapApp());
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  CHECK_NOTNULL(task);
  
  crawler2::AsciiRecognize recg;
  CHECK(recg.Init(FLAGS_path + "/training", FLAGS_path + "/testing"));

  std::string key;
  std::string value;
  // 网页数据输入的 sf 格式的 protocal buffer
  while (task->NextKeyValue(&key, &value)) {
    if (key.empty() || value.empty()) continue;
    crawler2::Resource pb;
    if (!crawler2::HbaseDumpToResource(value.data(), value.size(), &pb)) {
      LOG(FATAL) << "pb.ParseFromString() fail, url: " << key;
      continue;
    }
    // 检查 Alt 字段中是否存在 comment and price
    // if (!pb.has_alt()) {
    //   LOG(ERROR) << "has no alt, key: " << key;
    //   continue;
    // }
    // std::string comment;
    // if (!pb.alt().has_comment_data()) {
    //   LOG(ERROR) << "has no comment_data, key: " << key;
    // } else {
    //   std::string tmp;
    //   base::LineEscape(pb.alt().comment_data().raw_content(), &tmp);
    //   comment.append(pb.alt().comment_data().url() + "\t");
    //   comment.append(tmp);
    // }

    // std::string price_image;
    // if (!pb.alt().has_price_image()) {
    //   LOG(ERROR) << "has no comment_data, key: " << key;
    // } else {
    //   price_image = pb.alt().price_image().price();
    //   // const std::string &url = pb.alt().price_image().url();
    //   // const std::string &image_bin = pb.alt().price_image().raw_content();
    //   // price_image.append(url + ", price: ");

    //   // GURL gurl(url);
    //   // const std::string &path = gurl.path();
    //   // std::string::size_type pos = path.rfind(".");
    //   // if (pos == std::string::npos) {
    //   //   LOG(ERROR) << "not find dot in url, url: " << url;
    //   // } else {
    //   //   std::string image_type = path.substr(pos+1);
    //   //   std::string result;
    //   //   if (!recg.PriceVMemImage(image_bin, image_type, &result)) {
    //   //     LOG(ERROR) << "recg price fail, url: " << url;
    //   //   } else {
    //   //     LOG(ERROR) << "result: " << result;
    //   //     price_image.append(result);
    //   //   }
    //   // }
    // }
    // task->Output(key, "price_image: " + price_image + ", comment: " + comment);

    // 检查 图片字段:
     if (pb.content().image_files_size() > 0) {
      std::string content;
      for (auto it = pb.content().image_files().begin(); it != pb.content().image_files().end(); ++it) {
         content += (*it).url() + "|size:" + base::IntToString((*it).raw_content().size()) + ",";
      }
      task->Output(key, content); 
    }
  }
  return 0;
}
