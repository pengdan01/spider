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

// 监控的队列和端口
DEFINE_string(in_ip, "", "");
DEFINE_int32(in_port, 10000, "");

DEFINE_string(queue_ip, "", "url 提交到 job queue 的 ip");
DEFINE_int32(queue_port, -1, "url 提交到 job queue 的 port");
DEFINE_string(queue_tube, "", "url 提交到 job queue 的 子管道");
DEFINE_int32(priority, 5, "url 提交到 job queue 的优先级");

DEFINE_int32(url_type, 0, "url是类型, 0:kHTML, 1:kCSS, 2:kImage");

using crawler3::url::NewUrl;

int main(int argc, char **argv) {
  base::InitApp(&argc, &argv, "task_submit");

  // 1. 检查各个参数
  CHECK(!FLAGS_queue_ip.empty()) << "job queue ip is null string ";
  CHECK_GT(FLAGS_queue_port, 0) << "job queue port is less than 0 ";
  LOG_IF(FATAL, FLAGS_url_type != 0 && FLAGS_url_type != 1 && FLAGS_url_type != 2);

  // 2. 建立 job queue(读入) client
  job_queue::Client *in_client = new job_queue::Client(FLAGS_in_ip, FLAGS_in_port);
  CHECK(in_client->Connect()) << "job queue client connect fail";
  // 3. 建立 job queue(输出) client
  job_queue::Client *out_client = new job_queue::Client(FLAGS_queue_ip, FLAGS_queue_port);
  CHECK(out_client->Connect()) << "Failed to connect to job queue[" << FLAGS_queue_ip
                           << ":" << FLAGS_queue_port << "]";
  // 4. loop: 提交到指定 job queue
  int priority = FLAGS_priority;
  int cnt = 0;
  while (true) {
    job_queue::Job job;
    if (!in_client->ReserveWithTimeout(&job, 20)) {
      LOG_EVERY_N(ERROR, 1000) << "reserve time out";
      if (!in_client->IsConnected()) {
        LOG_IF(ERROR, !in_client->Connect()) << "IsConnect is false and Connect fail";
      } else {
        LOG_IF(ERROR, !in_client->Reconnect()) << "IsConnect is true and Reconnect fail";
      }
      usleep(30 * 1000);
      continue;
    }
    if (!in_client->Delete(job.id)) {
      LOG(ERROR) << "failed in delete job from queue, job id: " << job.id;
    }
    std::string line = job.body;
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
    if (0 >= out_client->PutEx(output.data(), output.size(), priority, 0, 10)) {
      LOG(ERROR) << "Failed to PutEx(), url is: " << url;
      delete out_client;
      sleep(3);
      out_client = new job_queue::Client(FLAGS_queue_ip, FLAGS_queue_port);
      CHECK(out_client->Connect()) << "Failed to connect to job queue[" << FLAGS_queue_ip
                               << ":" << FLAGS_queue_port << "]";
      continue;
    }
    ++cnt;
    LOG_EVERY_N(ERROR, 100) << "successfully submit " << cnt << " tasks.";
  }

  return 0;
}
