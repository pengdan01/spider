#pragma once

#include <string>
#include <vector>
#include <utility>

#include "base/common/basic_types.h"
#include "base/common/logging.h"
#include "base/strings/string_tokenize.h"
#include "crawler/proto/crawled_resource.pb.h"
#include "crawler/exchange/report_status.h"

namespace crawler {

// 只有当某 URL 的 score 不小于该阈值时, 才会对该 URL 进行超时重试
const double kMinScoreForTimeoutRetry = 0.40;
// 只有当某 URL 目录深度不超过该阈值时, 才会提取新链接
const double kMaxPathDepthExtractNewLink = 2;
// 只有当某网页的 外链数 (去重后) 大于等于该阈值时, 才会采用该网页中提取的外链
const int kMinOutlink = 6;

struct UploadTask;
class ResourceSaver {
 public:
  ResourceSaver(const std::string &page_hdfs_dir, const std::string &link_hdfs_dir,
                const std::string &css_hdfs_dir, const std::string &image_hdfs_dir,
                const std::string &newlink_hdfs_dir, const std::string &status_hdfs_dir,
                const std::string &saver_workding_dir,
                int64 text_file_max_size, int64 binary_file_max_size):
      page_hdfs_dir_(page_hdfs_dir), link_hdfs_dir_(link_hdfs_dir),
      css_hdfs_dir_(css_hdfs_dir), image_hdfs_dir_(image_hdfs_dir), newlink_hdfs_dir_(newlink_hdfs_dir),
      status_hdfs_dir_(status_hdfs_dir),
      saver_local_working_dir_(saver_workding_dir), text_file_max_size_(text_file_max_size),
      binary_file_max_size_(binary_file_max_size) {}
  ResourceSaver() {}
  ~ResourceSaver() {}
  bool Init();
  void ResourceRelease();
  // |store_type| 为存储类型，目前支持三种存储类型：
  // ‘P’ 网页存储类型
  // ‘C’ Css 存储类型
  // 'I' Image 存储类型
  // 对于其他的 取值，函数直接返回 false
  bool StoreProtoBuffer(const CrawledResource &info, char store_type);
  bool ExtractAndStoreNewLink(const CrawledResource &info, bool flag1, bool flag2);
  bool StoreStatus(const ReportStatus& status);

 private:
  void HandleLocalFile(UploadTask task);
  bool StoreLink(const CrawledResource &info, int64 *file_size, FILE *fp);
  bool StoreToLocal(FILE *fp, int64 *file_size_ptr, const std::string &key, const std::string &value);
  bool UploadToHadoop(const std::pair<int64, int64> &file_size,
                      std::pair<std::string, std::string> *local_filename_pair,
                      const std::string &hdfs_dir, const std::string &link_hdfs_dir);
  std::string page_hdfs_dir_;
  std::string link_hdfs_dir_;
  std::string css_hdfs_dir_;
  std::string image_hdfs_dir_;
  std::string newlink_hdfs_dir_;
  // 该目录下 包含了所有 url 的抓取状态
  // (url, start_time, end_time, success_or_not, reason, redir_times, effective_url, pay_load)
  std::string status_hdfs_dir_;

  std::string saver_local_working_dir_;
  int64 text_file_max_size_;
  int64 binary_file_max_size_;
  // First element  : page/css/image local file name
  // Second element : local link file name of page/css/image
  std::pair<std::string, std::string> page_local_filenames_;
  std::pair<std::string, std::string> css_local_filenames_;
  std::pair<std::string, std::string> image_local_filenames_;

  std::string newlink_local_filename_;
  std::string status_local_filename_;

  // First element  : page/css/image local file pointer
  // Second element : local link file pointer of page/css/image
  std::pair<FILE*, FILE*> fp_pages_;
  std::pair<FILE*, FILE*> fp_csses_;
  std::pair<FILE*, FILE*> fp_images_;

  FILE *fp_newlink_;
  FILE *fp_status_;

  // First element  : page/css/image local file size counter
  // Second element : local link file size counter of page/css/image
  std::pair<int64, int64> page_file_size_counters_;
  std::pair<int64, int64> css_file_size_counters_;
  std::pair<int64, int64> image_file_size_counters_;

  int64 newlink_file_size_counters_;
  int64 status_file_size_counters_;
};

struct UploadTask {
  // 0: text
  // 1: hfile
  int type;
  std::string local_filename;
  std::string hdfs_dir;
  std::string hdfs_name;
  // 只有在 上传 hfile 时用到
  int64 file_size1;
  int64 file_size2;
  std::string local_filename1;
  std::string local_filename2;
  std::string link_hdfs_dir;
  UploadTask(int type, const std::string &filename, const std::string &hdfs_dir,
             const std::string &hdfs_name):
      type(type), local_filename(filename), hdfs_dir(hdfs_dir), hdfs_name(hdfs_name) {
      file_size1 = 0;
      file_size2 = 0;
    }
  UploadTask(int type, int64 file_size1, int64 file_size2, const std::string &filename1,
             const std::string &filename2, const std::string &hdfs_dir, const std::string &link_hdfs_dir):
      type(type), hdfs_dir(hdfs_dir), file_size1(file_size1), file_size2(file_size2),
    local_filename1(filename1), local_filename2(filename2), link_hdfs_dir(link_hdfs_dir) {}
};
}  // end crawler
