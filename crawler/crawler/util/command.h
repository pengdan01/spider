#pragma once

#include <string>

namespace crawler {

// |cmd_str| 是一个 shell 命令，本函数最多尝试执行该命令 |try_times| 次
// 若命令执行成功, 返回 ture
// 若命令执行失败, 返回 false
bool execute_cmd_with_retry(const std::string &cmd_str, int try_times);

bool HadoopPutOneFileWithRetry(const std::string &local_file, const std::string &hdfs_file, int try_times);

bool HadoopGetOneFileWithRetry(const std::string &local_file, const std::string &hdfs_file, int try_times);

bool HadoopRenameWithRetry(const std::string &old_file, const std::string &new_file, int try_times);

}  // end namespace
