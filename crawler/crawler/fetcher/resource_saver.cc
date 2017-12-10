#include "crawler/fetcher/resource_saver.h"

#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <map>
#include <set>

#include "base/time/timestamp.h"
#include "base/encoding/line_escape.h"
#include "base/strings/string_util.h"
#include "base/file/file_util.h"
#include "base/common/slice.h"
#include "base/thread/thread_pool.h"
#include "base/common/closure.h"
#include "base/strings/utf_codepage_conversions.h"
#include "web/url/url.h"
#include "crawler/util/command.h"
#include "crawler/api/base.h"
#include "base/strings/string_printf.h"
#include "web/html/utils/url_extractor.h"
#include "crawler/selector/crawler_selector_util.h"
#include "crawler/fetcher/crawler_analyzer.h"
#include "crawler/exchange/report_status.h"
#include "log_analysis/common/log_common.h"

namespace crawler {

bool ResourceSaver::Init() {
  pid_t pid = getpid();
  int64 thread_id = pthread_self();
  int64 timestamp = base::GetTimestamp();

  CHECK(!saver_local_working_dir_.empty());
  base::FilePath file_path1(saver_local_working_dir_);
  LOG_IF(FATAL, !base::file_util::CreateDirectory(file_path1)) << "Create local workding dir fail";

  // Init the file size counters
  page_file_size_counters_ = std::make_pair(0, 0);
  css_file_size_counters_ = std::make_pair(0, 0);
  image_file_size_counters_ = std::make_pair(0, 0);
  newlink_file_size_counters_ = 0;
  status_file_size_counters_ = 0;

  // Init local file names and file pointers
  // page
  std::ostringstream filename, filename2;
  filename << saver_local_working_dir_ << "/page_" << pid << "_" << thread_id << "_" << timestamp;
  filename2 << saver_local_working_dir_ << "/linkpage_" << pid << "_" << thread_id << "_" << timestamp;
  page_local_filenames_ = std::make_pair(filename.str(), filename2.str());

  FILE *fp_page = fopen(page_local_filenames_.first.c_str(), "wb");
  PLOG_IF(FATAL, !fp_page)  << "Fail to open local page file: " << page_local_filenames_.first;
  FILE *fp_link = fopen(page_local_filenames_.second.c_str(), "w");
  PLOG_IF(FATAL, !fp_link) << "Fail to open local link file: " << page_local_filenames_.second;
  fp_pages_ = std::make_pair(fp_page, fp_link);

  // css
  filename.str("");  // clear the stream
  filename << saver_local_working_dir_ << "/css_"  << pid << "_" << thread_id << "_" << timestamp;
  filename2.str("");
  filename2 << saver_local_working_dir_ << "/linkcss_" << pid << "_" << thread_id << "_" << timestamp;
  css_local_filenames_ = std::make_pair(filename.str(), filename2.str());

  FILE *fp_css = fopen(css_local_filenames_.first.c_str(), "wb");
  PLOG_IF(FATAL, !fp_css) << "Fail to open local css file: " << css_local_filenames_.first;
  fp_link = fopen(css_local_filenames_.second.c_str(), "w");
  PLOG_IF(FATAL, !fp_link)  << "Fail to open local link file: " << css_local_filenames_.second;
  fp_csses_ = std::make_pair(fp_css, fp_link);
  // image
  filename.str("");  // clear the stream
  filename << saver_local_working_dir_ << "/image_" << pid << "_" << thread_id << "_" << timestamp;
  filename2.str("");
  filename2 << saver_local_working_dir_ << "/linkimage_" << pid << "_" << thread_id << "_" << timestamp;
  image_local_filenames_ = std::make_pair(filename.str(), filename2.str());

  FILE *fp_image = fopen(image_local_filenames_.first.c_str(), "wb");
  PLOG_IF(FATAL, !fp_image) << "Fail to open local image file: " << image_local_filenames_.first;
  fp_link = fopen(image_local_filenames_.second.c_str(), "w");
  PLOG_IF(FATAL, !fp_link)  << "Fail to open local link file: " << image_local_filenames_.second;
  fp_images_ = std::make_pair(fp_image, fp_link);

  filename.str("");  // clear the stream
  filename << saver_local_working_dir_ << "/newlink_" << pid << "_" << thread_id << "_" << timestamp;
  newlink_local_filename_ = filename.str();
  fp_newlink_ = fopen(newlink_local_filename_.c_str(), "w");
  PLOG_IF(FATAL, !fp_newlink_)  << "Fail to open local newlink file: " << newlink_local_filename_;

  filename.str("");  // clear the stream
  filename << saver_local_working_dir_ << "/status_" << pid << "_" << thread_id << "_" << timestamp;
  status_local_filename_ = filename.str();
  fp_status_ = fopen(status_local_filename_.c_str(), "w");
  PLOG_IF(FATAL, !fp_status_)  << "Fail to open local status file: " << status_local_filename_;

  return true;
}

bool ResourceSaver::StoreStatus(const ReportStatus& status) {
  std::string out = SerializeReportStatus(status);
  std::string record = out + "\n";
  int32 size = (int32)record.size();
  int32 n = fwrite(record.c_str(), 1, size, fp_status_);
  PLOG_IF(FATAL, size !=  n) << "Failed write status, expect write bytes: "
                             << size << ", acutal write bytes: " << n;
  status_file_size_counters_ += size;

  if (status_file_size_counters_ > text_file_max_size_) {
    fclose(fp_status_);
    std::ostringstream hdfs_filename;
    hdfs_filename << "part_" << getpid() << "_" << pthread_self()  << "_"
                  << base::GetTimestamp();
    std::string tmp_hdfs_filename = "_tmp_" + hdfs_filename.str();
    if (!HadoopPutOneFileWithRetry(status_local_filename_, status_hdfs_dir_ + "/" + tmp_hdfs_filename, 3)) {
      LOG(ERROR) << "Error, Failed to put " << status_local_filename_ << " to hadoop";
      status_local_filename_ = status_local_filename_ + "_2";
      PLOG_IF(FATAL, NULL == (fp_status_ = fopen(status_local_filename_.c_str(), "w")))
          << "Open local file fai: " << status_local_filename_;
      status_file_size_counters_ = 0;
      return false;
    }
    LOG_IF(ERROR, !HadoopRenameWithRetry(status_hdfs_dir_ + "/" + tmp_hdfs_filename,
                                         status_hdfs_dir_ + "/" + hdfs_filename.str(), 3))
        << "Error, Failed to rename hdfs file: " << status_hdfs_dir_ + "/" + tmp_hdfs_filename;
    PLOG_IF(FATAL, NULL == (fp_status_ = fopen(status_local_filename_.c_str(), "w")))
        << "Open local file fail: " << status_local_filename_;
    status_file_size_counters_ = 0;
  }
  return true;
}

// Store Link in text format
bool ResourceSaver::StoreLink(const CrawledResource &info, int64 *file_size, FILE *fp) {
  const CrawlerInfo &crawler_info = info.crawler_info();
  std::ostringstream record_stream;
  record_stream << info.source_url() << "\t";
  record_stream << info.effective_url() << "\t";
  record_stream << info.resource_type() << "\t";
  record_stream << crawler_info.crawler_timestamp() << "\t";
  record_stream << crawler_info.link_score() << "\t";
  record_stream << crawler_info.response_code() << "\t";
  record_stream << crawler_info.file_time() << "\t";
  record_stream << crawler_info.crawler_duration()<< "\t";
  record_stream << crawler_info.redirect_count() << "\t";
  // 本次 session 平均传输速度，单位 bytes/sec
  record_stream << crawler_info.donwload_speed() << "\t";
  // the value read from the Content-Length: field. Since 7.19.4, this returns -1 if the size isn't known
  record_stream << crawler_info.content_length_donwload() << "\t";
  if (crawler_info.has_content_type() && !crawler_info.content_type().empty()) {
    record_stream << crawler_info.content_type() << "\t";
  } else {
    record_stream << "NULL" << "\t";
  }
  record_stream << base::LineEscapeString(info.http_header_raw()) << "\t";
  // simhash
  record_stream << info.simhash() << "\t";
  // Attention: the last field should be end with \n
  record_stream << crawler_info.update_fail_cnt() << "\n";

  uint32 write_size = record_stream.str().size();
  PLOG_IF(FATAL, write_size != fwrite(&record_stream.str()[0], 1, write_size, fp))
      << "Write string to local link file fail.";
  *file_size += write_size;

  return true;
}

// Extract New Links and store
bool ResourceSaver::ExtractAndStoreNewLink(const CrawledResource &info, bool flag, bool flag2) {
  const std::string &source_url = info.source_url();
  const std::string &effective_url = info.effective_url();
  std::string page_body;
  std::string code_page;
  if (info.has_content_utf8() && !info.content_utf8().empty()) {
    page_body = info.content_utf8();
    code_page= info.original_encoding();
  } else {
    // When HTMLToUTF8() fail, page_body will be set to an empty string, So the
    // return value is not checked here
    const char *tmp_code_page = base::HTMLToUTF8(info.content_raw(), info.http_header_raw(), &page_body);
    if (tmp_code_page == NULL || page_body.empty()) {
      LOG(WARNING) << "Convert to UTF8 fail, ignore newlinks in page: " << source_url;
      return true;
    }
    code_page = tmp_code_page;
  }
  std::vector<std::string> link_html;
  std::vector<std::string> link_css;
  std::vector<web::ImageInfo> images;
  std::vector<std::pair<std::string, std::string> > anchors;
  char from = 'K';
  if (info.crawler_info().has_from() && !info.crawler_info().from().empty()) {
    const std::string &from_str = info.crawler_info().from();
    from = from_str[0];
  }
  // We do't need scripts at present, So  a NULL pointer is set to the forth parameter of ExtractURLs()
  // XXX(pengdan): Must use effective url (NOT souce url), (relative url)
  /*
  web::utils::ExtractURLs(page_body.c_str(), effective_url.c_str(), &link_css, NULL, &images,
                          &anchors);
                          */
  std::string reason;
  GURL gurl(effective_url);
  // const std::string &path = gurl.path();
  const std::string &host = gurl.host();
  const std::string &query = gurl.query();
  // 提取新 link 的条件:
  // 1. Host must be an VIP host
  // 2. 不会从普通网页中提取链接, 只会站点首页或目录首页中提取新的链接
  // if (IsVIPHost(host) && !base::MatchPattern(path, "*/*.*")) {
  // if (path == "/" && IsVIPHost(host)) {
  bool is_search = false;
  // if ((log_analysis::IsGeneralSearch(source_url, NULL, NULL) && crawler::IsGeneralSearchFirstNPage(source_url, 1)) ||  // NOLINT
  //    (log_analysis::IsVerticalSearch(source_url, NULL, NULL) && crawler::IsVerticalSearchFirstNPage(source_url, 1))) {  // NOLINT
  //  is_search = true;
  // }
  /*
  if ((path == "/" && IsVIPHost(host) && from != 'F') ||   // VIP 站点首页且不是外国 IP
      (path == "/bbs/" && host == "www.tianya.cn") ||  // 天涯论坛
      is_search == true) {  //  搜索结果页面
  */
  if (false) {  // Navi bost 存在大量的由高质量链接, 这里不提去链接
    link_html.clear();
    std::vector<std::pair<std::string, std::string> >::const_iterator it = anchors.begin();
    for (; it != anchors.end(); ++it) {
      std::string url = it->first;
      const std::string &anchor_text = it->second;
      if (url.empty() || anchor_text.empty()) continue;
      // 使用较为宽松的过滤条件: 让重要搜索引擎结果页不被滤掉
      if (!web::UTF8RawToClickUrl(url, code_page.c_str(), &url)) continue;
      if (crawler::WillFilterAccordingRules(url, &reason, false)) continue;
      // 如果是搜索结果页, 忽略掉从本结果页中提取倒的其他搜索组合链接:
      if (is_search && (log_analysis::IsVerticalSearch(url, NULL, NULL) ||
                        log_analysis::IsGeneralSearch(url, NULL, NULL))) continue;
     // 忽略掉 BlackHoleLink, 比如:
     // http://edu.360.cn/edu/?channel=zd&city=%B9%FE%B6%FB%B1%F5&famous=%B2%BB%CF%DE&grade=%b8%df%d2%bb&prices=500-999&nav_red=&cp=&page=&order= 提取同 host 的其他链接  // NOLINT
     GURL target(url);
     if (crawler::IsBlackHoleLink(&target, &gurl)) continue;
      link_html.push_back(url);
    }
    // Write Html url
    if ((int)link_html.size() >= kMinOutlink) {
      sort(link_html.begin(), link_html.end());
      link_html.erase(unique(link_html.begin(), link_html.end()), link_html.end());
    }
    // 外链数 限制: >= |kMinOutlink|, 取值: 5
    if ((int)link_html.size() >= kMinOutlink) {
      for (size_t i = 0; i < link_html.size(); ++i) {
        std::string record = base::StringPrintf("%s\t%d\t%s\t%c\n", link_html[i].c_str(), crawler::kHTML,
                                                effective_url.c_str(), from);
        int32 size = (int32)record.size();
        int32 n = fwrite(&record[0], 1, size, fp_newlink_);
        PLOG_IF(FATAL, size !=  n) << "Failed write newlink, expect write bytes: "
                                   << size << ", acutal write bytes: " << n;
        newlink_file_size_counters_ += size;
      }
    }
  }
  // Write Css url
  if (flag == true) {  // 提取 image/css 链接
    if (flag2 == true) {
      from = 'V';  // XXX(pengdan): 对于 css/image, 为了区分他们是否是 VIP  url, 对 from 字段进行标记
    }
    if (link_css.size() > 0) {
      std::vector<std::string> tmp;
      std::string url;
      for (size_t i = 0; i < link_css.size(); ++i) {
        url = link_css[i];
        if (!web::UTF8RawToClickUrl(url, code_page.data(), &url)) {
          LOG(ERROR) << "web::UTF8RawToClickUrl() fail, url: " << url << ", code page: " << code_page;
          continue;
        }
        tmp.push_back(url);
      }
      link_css = tmp;
      if (link_css.size() > 1) {
        sort(link_css.begin(), link_css.end());
        link_css.erase(unique(link_css.begin(), link_css.end()), link_css.end());
      }
    }
    for (size_t i = 0; i < link_css.size(); ++i) {
      std::string record = base::StringPrintf("%s\t%d\t%s\t%c\n", link_css[i].c_str(), crawler::kCSS,
                                              effective_url.c_str(), from);
      int32 size = (int32)record.size();
      int32 n = fwrite(&record[0], 1, size, fp_newlink_);
      PLOG_IF(FATAL, size !=  n) << "Failed write newlink, expect write bytes: "
                                 << size << ", acutal write bytes: " << n;
      newlink_file_size_counters_ += size;
    }
    // Write Image url
    if (IsVIPHost(host) && query.empty() && flag2) {
      std::vector<std::string> links;
      if (images.size() > 0) {
        std::string url;
        for (size_t i = 0; i < images.size(); ++i) {
          const web::ImageInfo &i_info = images[i];
          // 已经有了高度 宽度信息的图片, 不再抓取
          if (i_info.width_specified || i_info.height_specified) continue;
          url = i_info.url;
          if (!web::UTF8RawToClickUrl(url, code_page.data(), &url)) {
            LOG(ERROR) << "web::UTF8RawToClickUrl() fail, url: " << url << ", code page: " << code_page;
            continue;
          }
          // 对于 贴吧, 论坛等个人头像 logo, 没有价值, 不应抓取
          // if (!crawler::IsValuableImageLink(url)) continue;

          // if (true == crawler::WillFilterAccordingRules(url, &reason, true)) {
          //   LOG(INFO) << reason;
          //   continue;
          // }
          links.push_back(url);
        }
        if (links.size() > 1) {
          sort(links.begin(), links.end());
          links.erase(unique(links.begin(), links.end()), links.end());
        }
      }
      for (size_t i = 0; i < links.size(); ++i) {
        std::string record = base::StringPrintf("%s\t%d\t%s\t%c\n", links[i].c_str(), crawler::kImage,
                                                effective_url.c_str(), from);
        int32 size = (int32)record.size();
        int32 n = fwrite(&record[0], 1, size, fp_newlink_);
        PLOG_IF(FATAL, size !=  n) << "Failed write newlink, expect write bytes: "
                                   << size << ", acutal write bytes: " << n;
        newlink_file_size_counters_ += size;
      }
    }
  }
  if (newlink_file_size_counters_ > text_file_max_size_) {
    fclose(fp_newlink_);
    std::ostringstream hdfs_filename;
    hdfs_filename << "part_" << getpid() << "_" << pthread_self()  << "_"
                  << base::GetTimestamp();
    std::string tmp_hdfs_filename = "_tmp_" + hdfs_filename.str();
    if (!HadoopPutOneFileWithRetry(newlink_local_filename_, newlink_hdfs_dir_ + "/" + tmp_hdfs_filename, 3)) {
      LOG(ERROR) << "Error, Failed to put " << newlink_local_filename_ << " to hadoop";
      newlink_local_filename_ = newlink_local_filename_ + "_2";
      PLOG_IF(FATAL, NULL == (fp_newlink_ = fopen(newlink_local_filename_.c_str(), "w")))
          << "Open local file fai: " << newlink_local_filename_;
      newlink_file_size_counters_ = 0;
      return false;
    }
    LOG_IF(ERROR, !HadoopRenameWithRetry(newlink_hdfs_dir_ + "/" + tmp_hdfs_filename,
                                         newlink_hdfs_dir_ + "/" + hdfs_filename.str(), 3))
        << "Error, Failed to rename hdfs file: " << newlink_hdfs_dir_ + "/" + tmp_hdfs_filename;
    PLOG_IF(FATAL, NULL == (fp_newlink_ = fopen(newlink_local_filename_.c_str(), "w")))
        << "Open local file fail: " << newlink_local_filename_;
    newlink_file_size_counters_ = 0;
  }
  return true;
}

/*
// Web Page, Css, Image will store in hfile format, and the key needs to be sorted
bool ResourceSaver::SortFile(const std::string& sfile, int64 file_size) {
  struct stat file_stat;
  if (stat(sfile.c_str(), &file_stat) < 0) {
    LOG(ERROR) << "Failed to stat file:" << sfile;
    return false;
  }
  if (file_stat.st_size != file_size) {
    LOG(ERROR) << "The file maybe not complete, as file_stat != file_size";
    return false;
  }
  FILE *fp1 = fopen(sfile.c_str(), "rb");
  if (fp1 == NULL) {
    LOG(ERROR) << "Failed to open local file: " << sfile;
    return false;
  }
  char *file_buffer = new char[file_size];
  CHECK_NOTNULL(file_buffer);
  int64 count = 0;
  // read |file_size| bytes to file buffer, the user must make sure that file_size
  // is equal to the total size of file |sfile|
  do {
    count += fread(file_buffer + count, 1, file_size - count, fp1);
  } while (count < file_size && !feof(fp1));
  PLOG_IF(FATAL, count != file_size) << "We should read " << file_size << "bytes, but only read "
                                    << count << " bytes";
  fclose(fp1);
  // now we parse the buffer
  base::Slice key, value;
  uint32 key_len, value_len;
  char *ptr = file_buffer;
  const char *buffer_end = file_buffer + file_size;
  std::map<base::Slice, base::Slice> buffer_map;
  while (ptr != buffer_end) {
    key_len = *(reinterpret_cast<uint32*>(ptr));
    ptr += sizeof(uint32);
    if (ptr > buffer_end) {  // when the file is destoried, this may happen
      delete [] file_buffer;
      return false;
    }
    key.set(ptr, key_len);
    ptr += key_len;
    if (ptr > buffer_end) {  // when the file is destoried, this may happen
      delete []file_buffer;
      return false;
    }
    value_len = *(reinterpret_cast<uint32*>(ptr));
    ptr += sizeof(uint32);
    if (ptr > buffer_end) {  // when the file is destoried, this may happen
      delete []file_buffer;
      return false;
    }
    value.set(ptr, value_len);
    buffer_map.insert(std::make_pair(key, value));
    ptr += value_len;
    if (ptr > buffer_end) {  // when the file is destoried, this may happen
      delete []file_buffer;
      return false;
    }
  }
  // now write back
  FILE *fp2 = fopen(sfile.c_str(), "wb");
  PLOG_IF(FATAL, !fp2) << "Failed to open local file for write back: " << sfile;
  std::map<base::Slice, base::Slice>::const_iterator it = buffer_map.begin();
  while (it != buffer_map.end()) {
    uint32 key_len = it->first.size();
    const char *key_data = it->first.data();
    PLOG_IF(FATAL, 4 != fwrite(&key_len, 1, sizeof(uint32), fp2)) << "Write 4 bytes to file fail";
    PLOG_IF(FATAL, key_len != fwrite(key_data, 1, key_len, fp2)) << "Write " << key_len
                                                                << " bytes to file fail";
    uint32 value_len = it->second.size();
    const char *value_data  = it->second.data();
    PLOG_IF(FATAL, 4 != fwrite(&value_len, 1, sizeof(uint32), fp2)) << "Write 4 bytes to file fail";
    PLOG_IF(FATAL, value_len != fwrite(value_data, 1, value_len, fp2)) << "Write " << value_len
                                                                      << " bytes to file fail";
    ++it;
  }
  fclose(fp2);
  delete []file_buffer;
  return true;
}
*/
/*
// 此函数对文件的 size 没有进行严格的检查, 可以处理文件受损的情况
// (一般在本地执行失败或者导致本地的文件不完整时使用)
bool ResourceSaver::SortFile2(const std::string& sfile) {
  struct stat file_stat;
  if (stat(sfile.c_str(), &file_stat) < 0) {
    LOG(ERROR) << "Failed to stat file:" << sfile;
    return false;
  }
  int64 size = file_stat.st_size;
  FILE *fp1 = fopen(sfile.c_str(), "rb");
  if (fp1 == NULL) {
    LOG(ERROR) << "Failed to open local file: " << sfile;
    return false;
  }
  char *file_buffer = new char[size];
  CHECK_NOTNULL(file_buffer);
  int64 count = 0;
  // read |file_size| bytes to file buffer, the user must make sure that file_size
  // is equal to the total size of file |sfile|
  do {
    count += fread(file_buffer + count, 1, size - count, fp1);
  } while (count < size && !feof(fp1));
  PLOG_IF(FATAL, count != size) << "We should read " << size << "bytes, but only read "
                                << count << " bytes";
  fclose(fp1);
  // now we parse the buffer
  base::Slice key, value;
  uint32 key_len, value_len;
  char *ptr = file_buffer;
  const char *buffer_end = file_buffer + size;
  std::map<base::Slice, base::Slice> buffer_map;
  while (ptr <  buffer_end) {
    key_len = *(reinterpret_cast<uint32*>(ptr));
    ptr += sizeof(uint32);
    if (ptr + key_len >= buffer_end) {  // when the file is destoried, this may happen
      break;
    }
    key.set(ptr, key_len);
    ptr += key_len;

    value_len = *(reinterpret_cast<uint32*>(ptr));
    ptr += sizeof(uint32);
    if (ptr + value_len > buffer_end) {  // when the file is destoried, this may happen
      break;
    }
    value.set(ptr, value_len);
    buffer_map.insert(std::make_pair(key, value));
    ptr += value_len;
  }
  // now write back
  FILE *fp2 = fopen(sfile.c_str(), "wb");
  PLOG_IF(FATAL, !fp2) << "Failed to open local file for write back: " << sfile;
  std::map<base::Slice, base::Slice>::const_iterator it = buffer_map.begin();
  while (it != buffer_map.end()) {
    uint32 key_len = it->first.size();
    const char *key_data = it->first.data();
    PLOG_IF(FATAL, 4 != fwrite(&key_len, 1, sizeof(uint32), fp2)) << "Write 4 bytes to file fail";
    PLOG_IF(FATAL, key_len != fwrite(key_data, 1, key_len, fp2)) << "Write " << key_len
                                                                << " bytes to file fail";
    uint32 value_len = it->second.size();
    const char *value_data  = it->second.data();
    PLOG_IF(FATAL, 4 != fwrite(&value_len, 1, sizeof(uint32), fp2)) << "Write 4 bytes to file fail";
    PLOG_IF(FATAL, value_len != fwrite(value_data, 1, value_len, fp2)) << "Write " << value_len
                                                                      << " bytes to file fail";
    ++it;
  }
  fclose(fp2);
  delete []file_buffer;
  return true;
}
*/
bool ResourceSaver::StoreToLocal(FILE *fp, int64 *file_size_ptr,
                                 const std::string &key, const std::string &value) {
  uint32 key_len, value_len;

  key_len = key.size();
  PLOG_IF(FATAL, 4 != fwrite(&key_len, 1, sizeof(key_len), fp)) << "Write key size(4 bytes) to file fail";
  PLOG_IF(FATAL, key_len != fwrite(&key[0], 1, key_len, fp)) << "Write key data to file fail";

  value_len = value.size();
  PLOG_IF(FATAL, 4 != fwrite(&value_len, 1, sizeof(value_len), fp)) << "Write value size to file fail";
  PLOG_IF(FATAL, value_len != fwrite(&value[0], 1, value_len, fp)) <<"Write value to file fail.";

  *file_size_ptr += (4 + key_len + 4 + value_len);

  return true;
}

// Upload page/css/image and links from local to hadoop
bool ResourceSaver::UploadToHadoop(const std::pair<int64, int64> &file_size_pair,
                                   std::pair<std::string, std::string> *local_filename_pair,
                                   const std::string &hdfs_dir, const std::string &link_hdfs_dir) {
  // We only need to sort the file(page/css/image) which will be stored in HFILE format
  /*
  if (!SortFile(local_filename_pair->first, file_size_pair.first)) {
    LOG(ERROR) << "SortFile fail, file:" << local_filename_pair->first;
    local_filename_pair->first = local_filename_pair->first + "_2";
    local_filename_pair->second = local_filename_pair->second + "_2";
    return false;
  }
  */
  std::ostringstream hdfs_filename;
  hdfs_filename << "part_" << getpid() << "_" << pthread_self()  << "_" << base::GetTimestamp();
  std::string tmp_hdfs_filename = "_tmp_" + hdfs_filename.str();
  std::string cmd;
  cmd = "bash build_tools/mapreduce/sfile_writer.sh -output " + hdfs_dir + "/"
      + tmp_hdfs_filename + " < " + local_filename_pair->first;
  LOG(INFO) << "Info, Executeing cmd: " << cmd << " .....";
  if (!execute_cmd_with_retry(cmd, 5)) {
    LOG(ERROR) << "Error, Failed to Execute cmd: " << cmd;
    local_filename_pair->first = local_filename_pair->first + "_2";
    local_filename_pair->second = local_filename_pair->second + "_2";
    return false;
  }
  LOG(INFO) << "Info, Finished cmd: " << cmd;
  cmd = "hadoop fs -mv " +  hdfs_dir + "/" + tmp_hdfs_filename + " " + hdfs_dir + "/" + hdfs_filename.str();
  if (!HadoopRenameWithRetry(hdfs_dir + "/" + tmp_hdfs_filename, hdfs_dir + "/" + hdfs_filename.str(), 3)) {
    LOG(ERROR) << "Error, Failed to Execute cmd: " << cmd;
    local_filename_pair->first = local_filename_pair->first + "_2";
    local_filename_pair->second = local_filename_pair->second + "_2";
    return false;
  }

  // Now put local link file to hadoop
  if (!HadoopPutOneFileWithRetry(local_filename_pair->second, link_hdfs_dir + "/" + tmp_hdfs_filename, 3)) {
    LOG(ERROR) << "Error, Failed to put to hadoop: " << local_filename_pair->second;
    local_filename_pair->first = local_filename_pair->first + "_2";
    local_filename_pair->second = local_filename_pair->second + "_2";
    return false;
  }
  cmd = "hadoop fs -mv " + link_hdfs_dir + "/" + tmp_hdfs_filename + " " +
      link_hdfs_dir + "/" + hdfs_filename.str();
  if (!HadoopRenameWithRetry(link_hdfs_dir + "/" + tmp_hdfs_filename,
                             link_hdfs_dir + "/" + hdfs_filename.str(), 3)) {
    LOG(ERROR) << "Error, Failed to rename tmp hdfs file: " << link_hdfs_dir + "/" + tmp_hdfs_filename;
    local_filename_pair->first = local_filename_pair->first + "_2";
    local_filename_pair->second = local_filename_pair->second + "_2";
    return false;
  }
  return true;
}

// webpage/css/image 采用二进制存储到本地，然后上传到 hadoop
// 为了支持 hadoop 上 key-value 格式的存储以及快速查找的需求，采用 HFILE 格式存储网页
// 由于 HFILE 要求数据按照 key 有序；隐藏需要先对 本地缓存文件先排序，然后使用工具脚本
//  build_tools/mapreduce/hfile_writer.sh 将文件转成 HFILE 格式并上传到 HADOOP
//
// 每次写入记录格式： key_lenkey_datavalue_lenvalue_data
bool ResourceSaver::StoreProtoBuffer(const CrawledResource &info, char store_type) {
  std::string info_serialized_str;
  bool result = info.SerializeToString(&info_serialized_str);
  if (result != true) {
    LOG(ERROR) << "Failed to SerializeToString() for info, and igore url: " << info.source_url();
    return false;
  }
  const std::string &key = info.source_url();
  /*
  std::string reverse_key;
  // 对 key(url) 进行反转, 这样便于 DEBUG 和 快速提取某一站点的所有 url
  if (!web::ReverseUrl(key, &reverse_key, false)) {
    LOG(ERROR) << "Failed in ReverseUrl(), url: " << key;
    return false;
  }
  */
  const std::string &value = info_serialized_str;

  std::pair<int64, int64> *file_size_pair = NULL;
  std::pair<FILE *, FILE *> *fp_pair = NULL;
  std::pair<std::string, std::string> *local_filename_pair = NULL;
  std::string *hdfs_dir_ptr = NULL;
  switch (store_type) {
    case 'P' : {
      file_size_pair = &page_file_size_counters_;
      fp_pair = &fp_pages_;
      local_filename_pair = &page_local_filenames_;
      hdfs_dir_ptr = &page_hdfs_dir_;
      break;
    }
    case 'C' : {
      file_size_pair = &css_file_size_counters_;
      fp_pair = &fp_csses_;
      local_filename_pair = &css_local_filenames_;
      hdfs_dir_ptr = &css_hdfs_dir_;
      break;
    }
    case 'I' : {
      file_size_pair = &image_file_size_counters_;
      fp_pair = &fp_images_;
      local_filename_pair = &image_local_filenames_;
      hdfs_dir_ptr = &image_hdfs_dir_;
      break;
    }
  }
  // store page/css/image
  if (!StoreToLocal(fp_pair->first, &file_size_pair->first, key, value)) {
    LOG(ERROR) << "Failed in StoreToLocal(), store_type: " << store_type;
    return false;
  }
  // Store related link
  if (!StoreLink(info, &file_size_pair->second, fp_pair->second)) {
    LOG(ERROR) << "Failed in StoreLink(), store_type: " << store_type;
    return false;
  }
  // 检查文件是否大于本地文件 max file size ?
  if (file_size_pair->first > binary_file_max_size_ || file_size_pair->second > text_file_max_size_) {
    fclose(fp_pair->first);
    fclose(fp_pair->second);
    // Upload file to hadoop
    LOG_IF(ERROR, !UploadToHadoop(*file_size_pair, local_filename_pair, *hdfs_dir_ptr, link_hdfs_dir_))
        << "Failed in upload file to hadoop, we assigned a new local filename";
    PLOG_IF(FATAL, (fp_pair->first = fopen(local_filename_pair->first.c_str(), "wb")) == NULL)
        << "Reopen local page/css/image file fail: " << local_filename_pair->first;
    PLOG_IF(FATAL, (fp_pair->second = fopen(local_filename_pair->second.c_str(), "w")) == NULL)
        << "Reopen local link of page/css/image file fail: " << local_filename_pair->second;
    file_size_pair->first = 0;
    file_size_pair->second = 0;
  }
  return true;
}

void ResourceSaver::HandleLocalFile(UploadTask task) {
  int type = task.type;
  bool status;
  if (type == 0) {  // handle text
    std::string cmd;
    std::string tmp_hdfs_name = "_tmp_" + task.hdfs_name;
    cmd = "hadoop fs -put " + task.local_filename + " " + task.hdfs_dir + "/" + tmp_hdfs_name;
    LOG(INFO) << "Info, Executeing cmd: " << cmd << " .....";
    status = HadoopPutOneFileWithRetry(task.local_filename, task.hdfs_dir + "/" + tmp_hdfs_name, 3);
    if (status == false) {
      LOG(ERROR) << "Error, Failed to Execute cmd: " << cmd;
    } else {
      status = HadoopRenameWithRetry(task.hdfs_dir + "/" + tmp_hdfs_name,
                                     task.hdfs_dir + "/" + task.hdfs_name, 3);
      if (status == false) {
        LOG(ERROR) << "Error, Failed to Execute cmd: " << cmd;
      }
    }
    base::FilePath file_path(task.local_filename);
    LOG_IF(WARNING, !base::file_util::Delete(file_path, true)) << "Delete fail: " << task.local_filename;
  } else if (type == 1) {  // handle hfile
    std::pair<std::string, std::string> tmp_pair(task.local_filename1, task.local_filename2);
    std::pair<int64, int64> tmp_pair_size(task.file_size1, task.file_size2);
    status = UploadToHadoop(tmp_pair_size, &tmp_pair, task.hdfs_dir, task.link_hdfs_dir);
    if (status == false) {
      LOG(ERROR) << "Failed in UploadToHadoop() for web page, the local buffer files will be ignored: ";
    }
    base::FilePath file_path(task.local_filename1);
    LOG_IF(WARNING, !base::file_util::Delete(file_path, true)) << "Delete fail: " << file_path.ToString();
    base::FilePath file_path2(task.local_filename2);
    LOG_IF(WARNING, !base::file_util::Delete(file_path2, true)) << "Delete fail: " << file_path2.ToString();
  } else {
    LOG(ERROR) << "Invalid task type: " << type;
  }
}

// Now Resource Release
// Using a thread pool to upload local file to hadoop
void ResourceSaver::ResourceRelease() {
  // Close fds
  fclose(fp_newlink_);
  fclose(fp_status_);

  fclose(fp_pages_.first);
  fclose(fp_pages_.second);

  fclose(fp_csses_.first);
  fclose(fp_csses_.second);

  fclose(fp_images_.first);
  fclose(fp_images_.second);

  thread::ThreadPool thread_pools(5);
  // Upload local temporary file to hadoop
  // newlink
  if (newlink_file_size_counters_ > 0) {
    std::ostringstream hdfs_filename;
    hdfs_filename << "part_" << getpid() << "_" << pthread_self()  << "_"
                  << base::GetTimestamp();
    UploadTask task(0, newlink_local_filename_, newlink_hdfs_dir_, hdfs_filename.str());
    thread_pools.AddTask(NewCallback(this, &ResourceSaver::HandleLocalFile, task));
  } else {
    base::FilePath file_path(newlink_local_filename_);
    LOG_IF(WARNING, !base::file_util::Delete(file_path, true)) << "Delete fail: " << file_path.ToString();
  }
  // status
  if (status_file_size_counters_ > 0) {
    std::ostringstream hdfs_filename;
    hdfs_filename << "part_" << getpid() << "_" << pthread_self()  << "_"
                  << base::GetTimestamp();
    UploadTask task(0, status_local_filename_, status_hdfs_dir_, hdfs_filename.str());
    thread_pools.AddTask(NewCallback(this, &ResourceSaver::HandleLocalFile, task));
  } else {
    base::FilePath file_path(status_local_filename_);
    LOG_IF(WARNING, !base::file_util::Delete(file_path, true)) << "Delete fail: " << file_path.ToString();
  }
  // web page
  // 只需要检查本地缓存的网页文件大小是否非零即可，因为相应的链接文件和其是同步的
  if (page_file_size_counters_.first != 0) {
    UploadTask task(1, page_file_size_counters_.first, page_file_size_counters_.second,
                    page_local_filenames_.first, page_local_filenames_.second, page_hdfs_dir_,
                    link_hdfs_dir_);
    thread_pools.AddTask(NewCallback(this, &ResourceSaver::HandleLocalFile, task));
  } else {
    base::FilePath file_path(page_local_filenames_.first);
    LOG_IF(WARNING, !base::file_util::Delete(file_path, true)) << "Delete fail: " << file_path.ToString();
    base::FilePath file_path2(page_local_filenames_.second);
    LOG_IF(WARNING, !base::file_util::Delete(file_path2, true)) << "Delete fail: " << file_path2.ToString();
  }
  // css
  if (css_file_size_counters_.first != 0) {
    UploadTask task(1, css_file_size_counters_.first, css_file_size_counters_.second,
                    css_local_filenames_.first, css_local_filenames_.second, css_hdfs_dir_,
                    link_hdfs_dir_);
    thread_pools.AddTask(NewCallback(this, &ResourceSaver::HandleLocalFile, task));
  } else {
    base::FilePath file_path(css_local_filenames_.first);
    LOG_IF(WARNING, !base::file_util::Delete(file_path, true)) << "Delete fail: " << file_path.ToString();
    base::FilePath file_path2(css_local_filenames_.second);
    LOG_IF(WARNING, !base::file_util::Delete(file_path2, true)) << "Delete fail: " << file_path2.ToString();
  }
  // image
  if (image_file_size_counters_.first != 0) {
    UploadTask task(1, image_file_size_counters_.first, image_file_size_counters_.second,
                    image_local_filenames_.first, image_local_filenames_.second, image_hdfs_dir_,
                    link_hdfs_dir_);
    thread_pools.AddTask(NewCallback(this, &ResourceSaver::HandleLocalFile, task));
  } else {
    base::FilePath file_path(image_local_filenames_.first);
    LOG_IF(WARNING, !base::file_util::Delete(file_path, true)) << "Delete fail: " << file_path.ToString();
    base::FilePath file_path2(image_local_filenames_.second);
    LOG_IF(WARNING, !base::file_util::Delete(file_path2, true)) << "Delete fail: " << file_path2.ToString();
  }

  thread_pools.JoinAll();
}
}  // end crawler
