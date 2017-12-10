#include "crawler/util/auto_split_file.h"

#include <inttypes.h>
#include <time.h>

#include <string>

#include "base/strings/string_printf.h"

using std::string;
namespace crawler {
AutoSplitFile::AutoSplitFile(const string &file_prefix, int64 total_file_size)
  : file_prefix_(file_prefix), total_file_size_(total_file_size), current_file_size_(0) {
}

AutoSplitFile::~AutoSplitFile() {
}

static string GetFileName(const string &prefix) {
  time_t now = time(NULL);
  struct tm t;
  localtime_r(&now, &t);
  int year = t.tm_year + 1900;
  int month = t.tm_mon + 1;
  int day = t.tm_mday;
  int hour = t.tm_hour;
  int minite = t.tm_min;
  int second = t.tm_sec;

  int64_t pid = getpid();

  return StringPrintf("%s.%04d%02d%02d%02d%02d%02d.%ld",
                      prefix.c_str(),
                      year, month, day, hour, minite, second,
                      pid);
}

bool AutoSplitFile::Init() {
  if (total_file_size_ <= 0 || file_prefix_.empty() == true) {
    return false;
  }
  last_file_.open(GetFileName(file_prefix_).c_str());
  return last_file_;
}

void AutoSplitFile::Deinit() {
  last_file_.close();
}

bool AutoSplitFile::WriteString(const string &data) {
  return Write(data.c_str(), data.size());
}

bool AutoSplitFile::Write(const char *data, int64 size) {
  if (!last_file_.is_open()) return false;

  if (current_file_size_ >= total_file_size_) {
    last_file_.close();
    last_file_.open(GetFileName(file_prefix_).c_str());
    current_file_size_= 0;
    if (!last_file_) return false;
  }

  last_file_.write(data, size);
  current_file_size_ += size;

  return last_file_;
}
}  // end namespace
