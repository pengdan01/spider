#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <string>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/common/gflags.h"
#include "base/time/timestamp.h"
#include "base/strings/string_util.h"
#include "base/strings/string_tokenize.h"
#include "base/thread/thread_pool.h"
#include "crawler/fetcher/fetcher_thread.h"
#include "crawler/util/command.h"

DEFINE_string(hdfs_output_dir, "", "the output directory.");
DEFINE_string(tmp_file, "", "the local temp task files");

int main(int argc, char *argv[]) {
  base::InitApp(&argc, &argv, "Fetcher Worker");

  LOG_IF(FATAL, FLAGS_tmp_file.empty()) << "the local tmp task file var is empty";
  LOG_IF(FATAL, FLAGS_hdfs_output_dir.empty()) << "the local tmp task file var is empty";
  crawler::ResourceSaver saver;

  std::vector<std::string> tokens;
  int num = base::Tokenize(FLAGS_tmp_file, "/", &tokens);
  std::string filename = tokens[num-1];

  std::string cmd;
  cmd = "bash build_tools/mapreduce/sfile_writer.sh " + FLAGS_hdfs_output_dir +  "/" +  filename
      + " < " + FLAGS_tmp_file;
  LOG(INFO) << "executing cmd: " << cmd;
  if (!crawler::execute_cmd_with_retry(cmd, 5)) {
      LOG(ERROR) << "cmd: " << cmd << " fail.";
      return -1;
  }
  return 0;
}
