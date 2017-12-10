#include "crawler/util/auto_split_file.h"

#include "base/testing/gtest.h"

TEST(AutoSplitFile, AutoSplitFileTest) {
  const std::string kFilePrefix("/tmp/part");
  const int64 kFileSize = 1024;

  crawler::AutoSplitFile auto_split_file(kFilePrefix, kFileSize);
  EXPECT_TRUE(auto_split_file.Init());

  const std::string kContent("This is just a simple gtest used to test the function of class AutoSplitFile. "
                             "The filesize will be about to kFileSize, but you should not make any assumption"
                             " that the file size will exeactly be kFileSize.");
  const int kTimes = 9000;

  for (int i =0; i < kTimes; ++i) {
    EXPECT_TRUE(auto_split_file.WriteString(kContent));
  }
  auto_split_file.Deinit();
}
