#pragma once

#include <fstream>
#include <string>

#include "base/common/basic_types.h"

namespace crawler {
class AutoSplitFile {
 public:

  AutoSplitFile(const std::string &file_prefix, int64 file_size);
  ~AutoSplitFile();

  bool Init();
  void Deinit();

  bool WriteString(const std::string &data);
  bool Write(const char *data, int64 size);

 private:
  const std::string file_prefix_;
  int64 total_file_size_;
  int64 current_file_size_;
  std::ofstream last_file_;
};
}  // end namespace

