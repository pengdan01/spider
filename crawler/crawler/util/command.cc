#include "crawler/util/command.h"

#include "base/common/logging.h"

namespace crawler {

bool execute_cmd_with_retry(const std::string &cmd_str, int try_times) {
  LOG_IF(FATAL, cmd_str.empty() || try_times <= 0) << "Invalid function parameters.";
  int status;
  for (int i = 0; i < try_times; ++i) {
    status = system(cmd_str.c_str());
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
      LOG(WARNING) << "Hadoop cmd: " << cmd_str << " execute fail and will retry after 1 second...";
      sleep(1);
    } else {
      return true;
    }
  }
  return false;
}

bool HadoopPutOneFileWithRetry(const std::string &local_file, const std::string &hdfs_file, int try_times) {
  LOG_IF(FATAL, local_file.empty() || hdfs_file.empty() || try_times <= 0)
      << "Invalid function parameters: string is empty or try_times <= 0";
  int i = 0;
  int status;
  std::string put_cmd = "${HADOOP_HOME}/bin/hadoop fs -put " + local_file + " " + hdfs_file;
  std::string rm_cmd = "${HADOOP_HOME}/bin/hadoop fs -rm " + hdfs_file;
  while (i < try_times) {
    status = system(put_cmd.c_str());
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) break;
    system(rm_cmd.c_str());
    sleep(1);
    ++i;
  }
  return (i < try_times) ? true : false;
}

bool HadoopGetOneFileWithRetry(const std::string &local_file, const std::string &hdfs_file, int try_times) {
  LOG_IF(FATAL, local_file.empty() || hdfs_file.empty() || try_times <= 0)
      << "Invalid function parameters: string is empty or try_times <= 0";
  int i = 0;
  int status;
  std::string get_cmd = "${HADOOP_HOME}/bin/hadoop fs -get " + hdfs_file + " " + local_file;
  std::string rm_cmd = "rm -f " + local_file;
  while (i < try_times) {
    status = system(get_cmd.c_str());
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) break;
    system(rm_cmd.c_str());
    sleep(1);
    ++i;
  }
  return (i < try_times) ? true : false;
}

bool HadoopRenameWithRetry(const std::string &old_file, const std::string &new_file, int try_times) {
  LOG_IF(FATAL, old_file.empty() || new_file.empty() || try_times <= 0)
      << "Invalid function parameters: string is empty or try_times <= 0";
  int i = 0;
  int status;
  std::string rename_cmd = "${HADOOP_HOME}/bin/hadoop fs -mv " + old_file + " " + new_file;
  std::string rm_cmd = "${HADOOP_HOME}/bin/hadoop fs -rm " + new_file;
  while (i < try_times) {
    status = system(rename_cmd.c_str());
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) break;
    system(rm_cmd.c_str());
    sleep(1);
    ++i;
  }
  return (i < try_times) ? true : false;
}
}  // namespace crawler
