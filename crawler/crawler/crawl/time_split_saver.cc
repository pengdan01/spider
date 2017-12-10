#include <pthread.h>

#include "crawler/crawl/time_split_saver.h"
#include "base/time/timestamp.h"
#include "base/time/time.h"
#include "base/strings/string_split.h"
#include "base/strings/string_printf.h"

namespace crawler2 {
namespace crawl {

DEFINE_int32(min_file_size, 1024 * 1024 * 10, "dump文件的最小 size, 默认是 10MB");

TimeSplitSaver::TimeSplitSaver(const std::string& fileprefix,
                               int64 timespan_to_split)
    : fileprefix_(fileprefix)
    , kTimespanToSplit(timespan_to_split)
    , current_file_timestamp_(0)
    , current_format_filename_("")
    , current_file_size_(0)
    , open_in_binary_(false) {
}

TimeSplitSaver::TimeSplitSaver(const std::string& fileprefix,
                               int64 timespan_to_split,
                               bool open_in_binary)
    : fileprefix_(fileprefix)
    , kTimespanToSplit(timespan_to_split)
    , current_file_timestamp_(0)
    , current_format_filename_("")
    , current_file_size_(0)
    , open_in_binary_(open_in_binary) {
}

TimeSplitSaver::~TimeSplitSaver() {
  current_fstream_.flush();
  current_fstream_.close();
}

namespace {
std::string GetFormattingData(const std::string& prefix, int64 timestamp) {
  const int kHostNameLen = 1023;
  char hostname[kHostNameLen + 1] = {0};
  gethostname(hostname, kHostNameLen);

  base::Time t = base::Time::FromDoubleT(timestamp / 1000.0 / 1000.0);
  std::string formatted_time;
  CHECK(t.ToStringInFormat("%Y%m%d%H%M%S", &formatted_time));
  return base::StringPrintf("%s.%s.%s.%d.%lu", prefix.c_str(),
                              formatted_time.c_str(), hostname, getpid(), pthread_self());
}
}

std::string TimeSplitSaver::GetDataFilepath(int64 timestamp) {
  current_format_filename_ = GetFormattingData(fileprefix_, timestamp);
  return (current_format_filename_ + ".data");
}

std::string TimeSplitSaver::GetDoneFilepath() {
  return (current_format_filename_ + ".done");
}

bool TimeSplitSaver::OpenNewFile(int64 timestamp) {
  // 如果不是第一次打开文件，那么关闭之前文件并创建 done 文件
  if (current_file_timestamp_ != 0) {
    // 关闭文件
    if (current_fstream_.good()) {
      current_fstream_.close();
    }
    // 创建 done 文件
    std::fstream done_file;
    std::string done_file_path = GetDoneFilepath();
    done_file.open(done_file_path.c_str(), std::ios::out);
    CHECK(done_file.good()) << "failed operate on file: "
                            << done_file_path;
  }

  current_file_timestamp_ = timestamp;
  current_file_size_ = 0;

  std::string new_path = GetDataFilepath(current_file_timestamp_);
  if (!open_in_binary_) {
    current_fstream_.open(new_path.c_str(), std::ios::out);
  } else {
    current_fstream_.open(new_path.c_str(), std::ios::out|std::ios::binary);
  }
  CHECK(current_fstream_.good()) << "Failed to open file: " << new_path;
  return true;
}

bool TimeSplitSaver::AppendKeyValue(const std::string& key,
                                    const std::string& value,
                                    int64 timestamp) {
  CHECK(open_in_binary_);

  int key_len = key.size();
  int value_len = value.size();
  char *buf = new char[key_len + value_len + 8];

  char *ptr = buf;
  memmove(ptr, &key_len, sizeof(key_len));

  ptr += 4;
  memmove(ptr, key.data(), key_len);

  ptr += key_len;
  memmove(ptr, &value_len, sizeof(value_len));

  ptr += 4;
  memmove(ptr, value.data(), value_len);

  std::string out(buf, key_len + value_len + 8);
  delete [] buf;

  VLOG(5) << "url key: " << key << ", key size: " << key_len << ", value size: " << value_len
          << ", out string size: " << out.size();

  return Append(out, timestamp);
}

bool TimeSplitSaver::AppendLine(const std::string& str, int64 timestamp)  {
  return Append(str + "\n", timestamp);
}

bool TimeSplitSaver::Append(const std::string& str, int64 timestamp) {
  int64 curr_ts = ::base::GetTimestamp();
  // 如果写入时间超过了 |kTimespanToSplit|(seconds) 且 文件大小 >= |FLAGS_min_file_size|, 生成新文件
  if (((curr_ts - current_file_timestamp_) > kTimespanToSplit && current_file_size_ >= FLAGS_min_file_size)
      || current_file_timestamp_ == 0) {
    CHECK(OpenNewFile(timestamp));
  }

  CHECK(current_fstream_.good());
  current_fstream_.write(str.data(), str.size());
  current_file_size_ += str.size();

  current_fstream_.flush();
  return true;
}
}  // namespace fetcher
}  // namespace cralwer2
