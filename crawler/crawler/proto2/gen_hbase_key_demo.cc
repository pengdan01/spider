#include <iostream>
#include <string>

#include "base/common/logging.h"
#include "base/common/gflags.h"
#include "base/common/basic_types.h"
#include "base/testing/gtest.h"
#include "crawler/proto2/gen_hbase_key.h"

int main(int argc, char** argv) {
  std::string url;
  while (std::getline(std::cin, url)) {
    std::string new_hbase_key;
    crawler2::GenNewHbaseKey(url, &new_hbase_key);
    std::cout << new_hbase_key << std::endl;
  }
  return 0;
}
