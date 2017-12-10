#pragma once

#include <string>
#include <fstream>
#include <utility>

#include "base/common/basic_types.h"
#include "base/common/gflags.h"

namespace crawler2 {
namespace crawl {

// dump 文件的最小 size, 默认是 10MB
DECLARE_int32(min_file_size);

class TimeSplitSaver {
 public:
  // 默认使用文本的方式打开
  // |timespan_to_split|: 微秒
  TimeSplitSaver(const std::string& fileprefix, int64 timespan_to_split);
  TimeSplitSaver(const std::string& fileprefix, int64 timespan_to_split, bool open_in_binary);
  ~TimeSplitSaver();

  // 注意如果 str 不包含 \n, 那么函数不会换行
  bool Append(const std::string& str, int64 timestamp);
  // 此函数会在后面自动加 \n， 设计字符串拷贝，建议自己加好 \n
  // 并调用 Append 函数
  bool AppendLine(const std::string& str, int64 timesamp);
  // 将数据以 key_len(4byte)key(key.size()byte)value_len(4byte)value(value.size()) 的 二进制写入
  // XXX(pengdan): 以 二进制写入时, |open_in_binary| 必须设置为 true, 否则 程序崩溃
  bool AppendKeyValue(const std::string &key, const std::string &value, int64 timesamp);
 private:
  std::string GetDataFilepath(int64 timestamp);
  std::string GetDoneFilepath();
  bool OpenNewFile(int64 timestamp);

  // 文件的前缀
  std::string fileprefix_;

  // 分割文件的时间间隔
  const int64 kTimespanToSplit;

  // 打开当前文件的时间
  int64 current_file_timestamp_;
  // 当前的数据文件名字前缀(添加了时间搓, 进程, 线程等信息)
  std::string current_format_filename_;

  int64 current_file_size_;
  std::fstream current_fstream_;

  // 打开文件方式, 是否以 binary 方式打开文件
  bool open_in_binary_;

  DISALLOW_COPY_AND_ASSIGN(TimeSplitSaver);
};
}  // namespace realtime_fetcher
}  // namespace crawler2
