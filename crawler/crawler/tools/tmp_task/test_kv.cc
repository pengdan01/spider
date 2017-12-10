#include <iostream>
#include <fstream>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_printf.h"
#include "crawler/proto/crawled_resource.pb.h" 

DEFINE_string(kv_format_file, "", "key_value file");

int main(int argc, char *argv[]) {
  char keybuf[10240];
  char valuebuf[10240000];
  base::InitApp(&argc, &argv, "");
  FILE *fp = fopen(FLAGS_kv_format_file.c_str(), "rb");
  LOG_IF(FATAL, fp == NULL) << "Failed to open file: " << FLAGS_kv_format_file;
  crawler::CrawledResource pb;
  int key_len, value_len;
  while (!feof(fp)) {
    // read key len
    int n = fread(&key_len, 1, sizeof(int), fp);
    if (n == 0) break;
    LOG_IF(FATAL, n != sizeof(int)) << "key Should read 4 bytes, but only read: " << n << " bytes";
    // read key data
    n = fread(keybuf, 1, key_len, fp);
    LOG_IF(FATAL, n != key_len) << "key Should read " << key_len << " , but only read: " << n << " bytes";
    keybuf[key_len] = 0;
    // read value len
    n = fread(&value_len, 1, sizeof(int), fp);
    LOG_IF(FATAL, n != sizeof(int)) << "value Should read 4 bytes, but only read: " << n << " bytes";
    // read value data
    n = fread(valuebuf, 1, value_len, fp);
    LOG_IF(FATAL, n != value_len) << "value Should read " << value_len << " bytes, but only read: " << n << " bytes";
    valuebuf[value_len] = 0;
    LOG(ERROR) << "key: " << keybuf;
    if (!pb.ParseFromArray(valuebuf, value_len)) {
      LOG(FATAL) << "parse pb fail, value_len: " <<  value_len; 
    }
  }
  fclose(fp);
  return 0;
}
