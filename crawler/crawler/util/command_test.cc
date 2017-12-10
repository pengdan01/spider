#include "crawler/util/command.h"

#include "base/testing/gtest.h"

// TEST(Command, ExecuteHadoopCmdWithRetry) {
//   std::string cmd_str1 = "${HADOOP_HOME}/bin/hadoop fs -put ~/.bashrc /tmp/";
//   EXPECT_TRUE(crawler::execute_cmd_with_retry(cmd_str1, 3));
//   std::string cmd_str2 = "${HADOOP_HOME}/bin/hadoop fs -rm /tmp/.bashrc";
//   EXPECT_TRUE(crawler::execute_cmd_with_retry(cmd_str2, 3));
//   std::string str3 = "${HADOOP_HOME}/bin/hadoop fs -ls /user/pengdan/crawler_online/stat/2012* 1> /tmp/a";
//   EXPECT_TRUE(crawler::execute_cmd_with_retry(str3, 3));
// }
//
// TEST(Command, HadoopPutOneFileWithRetry) {
//   std::string local_file("~/.bashrc");
//   std::string hdfs_file("/tmp/pengdan/bashrc");
//   EXPECT_TRUE(crawler::HadoopPutOneFileWithRetry(local_file, hdfs_file, 3));
// }
//
// TEST(Command, HadoopGetOneFileWithRetry) {
//   std::string local_file("/tmp/shard_number_info");
//   std::string hdfs_file("/app/crawler/shard_number_info");
//   EXPECT_TRUE(crawler::HadoopGetOneFileWithRetry(local_file, hdfs_file, 3));
// }
//
// TEST(Command, HadoopRenameWithRetry) {
//   std::string local_file("~/.bashrc");
//   std::string hdfs_file("/tmp/pengdan/bashrc");
//   std::string new_file("/tmp/pengdan/bashrc_new");
//   EXPECT_TRUE(crawler::HadoopPutOneFileWithRetry(local_file, hdfs_file, 3));
//   EXPECT_TRUE(crawler::HadoopRenameWithRetry(hdfs_file, new_file, 3));
//   EXPECT_TRUE(crawler::HadoopRenameWithRetry(new_file, hdfs_file, 3));
// }
