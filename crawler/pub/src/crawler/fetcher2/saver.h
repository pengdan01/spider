#pragma once

#include <string>

#include "crawler/proto2/resource.pb.h"

namespace crawler2 {

// 会自动切割文件的存储
//
// 会生成多个文件, 每个文件最大为 |file_max_size| bytes,
// 输出的第 i 个 (从 0 开始) 文件的文件名为 ("%s_%d", file_prefix, i)
class AutoSaver {
 public:
  AutoSaver(const std::string &file_prefix, int file_max_size_in_bytes);
  ~AutoSaver();

  // 追加输出一个 kv 记录
  //
  // 输出文为二进制格式, 依次为
  // 1. 4 字节: key.size()
  // 2. key.size() 字节: data of key
  // 3. 4 字节: value.size()
  // 4. value.size() 字节: data of value
  //
  // key/value 大小均不能超过 2GB
  bool Append(const std::string &key, const std::string &value);
 private:
  void CreateNewFile();

  std::string file_prefix_;
  int max_file_size_in_bytes_;

  // 当前是第几个文件
  int file_num_;
  // 当前输出的文件名
  std::string filename_;
  // 当前输出的文件指针
  FILE* fp_;
  // 已写入当前文件的字节数
  int file_bytes_;
};

}  // namespace crawler2

