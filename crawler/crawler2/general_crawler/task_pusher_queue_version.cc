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
#include "base/common/base.h"
#include "base/common/closure.h"
#include "rpc/job_queue/job_queue.h"

#include "crawler/realtime_crawler/crawl_queue.pb.h"
#include "crawler2/general_crawler/job_submit.h"
#include "crawler2/general_crawler/util/url_util.h"

// 监控的队列和端口
DEFINE_string(in_ip, "", "");
DEFINE_int32(in_port, 0, "");
// 提交给的队列和端口
DEFINE_string(ip, "", "");
DEFINE_int32(port, 0, "");
DEFINE_string(datapath, "", "");
DEFINE_string(target_ip, "", "");
DEFINE_int32(target_port, 0, "");
DEFINE_string(target_tube, "", "");
DEFINE_int32(target_priority, 5, "");
DEFINE_int32(url_type, 0, "0: kHTML; 1: kCSS, 2: kImage");

using crawler3::CrawlJobSubmitter;
using crawler2::crawl_queue::JobDetail;

int main(int argc, char **argv) {
  base::InitApp(&argc, &argv, "task_submit");
  CHECK(!FLAGS_in_ip.empty() && FLAGS_in_port > 0);
  LOG_IF(FATAL, FLAGS_url_type != 0 && FLAGS_url_type != 1 && FLAGS_url_type != 2)
      << "url type must be 0(kHTML), 1(kCss) or 2(kImage)";
  job_queue::Client *client = new job_queue::Client(FLAGS_in_ip, FLAGS_in_port);
  LOG_IF(FATAL, !client->Connect()) << "job queue client connect fail";
  LOG_IF(FATAL, !client->Ping()) << "job queue client ping fail";

  CrawlJobSubmitter::Option option;
  option.queue_ip = FLAGS_ip;
  option.queue_port = FLAGS_port;
  CrawlJobSubmitter submitter(option);
  LOG_IF(FATAL, !submitter.Init()) << "Failed in submitter.Init()";
  int cnt = 1;
  while (true) {
    job_queue::Job job;
    if (!client->ReserveWithTimeout(&job, 20)) {
      LOG_EVERY_N(ERROR, 1000) << "reserve time out";
      if (!client->IsConnected()) {
        LOG_IF(ERROR, !client->Connect()) << "IsConnect is false and Connect fail";
      } else {
        LOG_IF(ERROR, !client->Reconnect()) << "IsConnect is true and Reconnect fail";
      }
      continue;
    }
    if (!client->Delete(job.id)) {
      LOG(ERROR) << "failed in delete job from queue, job id: " << job.id;
    }

    crawler2::crawl_queue::JobInput input;
    input.set_submitted_timestamp(base::GetTimestamp());
    std::string url(job.body);
    // 对于 非 taobao url , 不会有影响
    crawler3::RewriteTaobaoItemList(&url);
    input.mutable_detail()->set_url(url);
    switch (FLAGS_url_type) {
      case 0:
        input.mutable_detail()->set_resource_type(crawler2::kHTML);
        break;
      case 1:
        input.mutable_detail()->set_resource_type(crawler2::kCSS);
        break;
      case 2:
        input.mutable_detail()->set_resource_type(crawler2::kImage);
        break;
    }
    input.mutable_target_queue()->set_ip(FLAGS_target_ip);
    input.mutable_target_queue()->set_port(FLAGS_target_port);
    input.mutable_target_queue()->set_tube_name(FLAGS_target_tube);
    input.mutable_target_queue()->set_priority(FLAGS_target_priority);

    submitter.AddJob(input);
    LOG(ERROR) << "successfully submit url(if taobao list, it is rewrited): " << url << std::endl;
    cnt++;
  }
  return 0;
}
