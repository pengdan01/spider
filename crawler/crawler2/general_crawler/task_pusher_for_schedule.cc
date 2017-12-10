#include <cstdatomic>

#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>

#include "base/file/file_util.h"
#include "base/file/file_path.h"
#include "base/file/file_stream.h"
#include "base/time/timestamp.h"
#include "base/encoding/line_escape.h"
#include "base/strings/string_util.h"
#include "base/common/base.h"
#include "base/common/closure.h"
#include "rpc/job_queue/job_queue.h"

#include "crawler2/general_crawler/url.pb.h"
#include "crawler2/general_crawler/util/url_util.h"

DEFINE_string(seeds_url_file_path, "", "种子 url 文件路径");
DEFINE_string(queue_ip, "", "url 提交到 job queue 的 ip");
DEFINE_int32(queue_port, -1, "url 提交到 job queue 的 port");
DEFINE_string(queue_tube, "", "url 提交到 job queue 的 子管道");
DEFINE_int32(priority, 5, "url 提交到 job queue 的优先级");

DEFINE_int32(url_type, 0, "url是类型, 0:kHTML, 1:kCSS, 2:kImage");

using crawler3::url::NewUrl;

// 每行输入的 url 格式:
// 1. 每行只有一个 url
// 2. url + referer, 之间用 '\t' 分割
int main(int argc, char **argv) {
  base::InitApp(&argc, &argv, "task_submit");

  // 1. 检查各个参数
  CHECK(!FLAGS_seeds_url_file_path.empty()) << "seeds filename is null string ";
  CHECK(!FLAGS_queue_ip.empty()) << "job queue ip is null string ";
  CHECK_GT(FLAGS_queue_port, 0) << "job queue port is less than 0 ";

  // 2. 加载 seed url 文件
  std::vector<std::string> lines;
  if (!base::file_util::ReadFileToLines(base::FilePath(FLAGS_seeds_url_file_path), &lines)) {
    LOG(ERROR) << "Failed to read file : " << FLAGS_seeds_url_file_path;
    return -1;
  }
  // 3. 初始化 job queue client
  job_queue::Client *client = new job_queue::Client(FLAGS_queue_ip, FLAGS_queue_port);
  CHECK(client->Connect()) << "Failed to connect to job queue[" << FLAGS_queue_ip
                           << ":" << FLAGS_queue_port << "]";
  // 4. loop: 提交到指定 job queue
  int priority = FLAGS_priority;
  int cnt = 0;
  for (int i = 0; i < (int)lines.size(); ++i) {
    std::string line = lines[i];
    base::TrimWhitespaces(&line);
    if (line.empty()) {
      continue;
    }
    std::string url, referer;
    std::string::size_type pos = line.find("\t");
    if (pos != std::string::npos) {
      url = line.substr(0, pos);
      referer = line.substr(pos+1);
    } else {
      url = line;
    }

    // 对于 非 taobao url , 不会有影响
    crawler3::RewriteTaobaoItemList(&url);

    crawler3::url::NewUrl input;
    // XXX(pengdan): 假设 seed urls 都是 utf8 编码的
    input.set_clicked_url(url);
    input.set_depth(0);
    input.set_referer(referer);
    input.set_resource_type(FLAGS_url_type == 1 ? crawler2::kCSS :
                            (FLAGS_url_type == 2 ? crawler2::kImage : crawler2::kHTML));

    std::string output;
    if (!input.SerializeToString(&output)) {
      LOG(ERROR) << "Failed to SerializeToString(), url is: " << url;
      continue;
    }
    if (0 >= client->PutEx(output.data(), output.size(), priority, 0, 10)) {
      LOG(ERROR) << "Failed to PutEx(), url is: " << url;
      delete client;
      sleep(3);
      client = new job_queue::Client(FLAGS_queue_ip, FLAGS_queue_port);
      CHECK(client->Connect()) << "Failed to connect to job queue[" << FLAGS_queue_ip
                               << ":" << FLAGS_queue_port << "]";
      continue;
    }
    ++cnt;
    LOG_EVERY_N(ERROR, 500) << "successfully submit " << cnt << " tasks.";
  }
  LOG(ERROR) << "submit complete, succesfully_submitted_url/total_url: " << cnt << "/" << lines.size();

  return 0;
}
