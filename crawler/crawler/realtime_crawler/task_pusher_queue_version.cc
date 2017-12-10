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
#include "crawler/realtime_crawler/job_submit.h"

// 监控的队列和端口
DEFINE_string(in_ip, "", "");
DEFINE_int32(in_port, 11110, "");
// 提交给的队列和端口
DEFINE_string(ip, "", "");
DEFINE_int32(port, 20080, "");
DEFINE_string(datapath, "", "");
DEFINE_string(target_ip, "", "");
DEFINE_int32(target_port, 5000, "");
DEFINE_string(target_tube, "", "");
DEFINE_int32(target_priority, 5, "");

using crawler2::crawl_queue::CrawlJobSubmitter;
using crawler2::crawl_queue::JobDetail;

int main(int argc, char **argv) {
  base::InitApp(&argc, &argv, "task_submit");
  CHECK(!FLAGS_in_ip.empty() && FLAGS_in_port > 0);
  job_queue::Client *client = new job_queue::Client(FLAGS_in_ip, FLAGS_in_port);
  CHECK_NOTNULL(client);
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

    const std::string &body = job.body;

    crawler2::crawl_queue::JobInput input;
    input.set_submitted_timestamp(base::GetTimestamp());
    input.mutable_detail()->set_url(body);
    if (!FLAGS_target_ip.empty()) {
      input.mutable_target_queue()->set_ip(FLAGS_target_ip);
      input.mutable_target_queue()->set_port(FLAGS_target_port);
      input.mutable_target_queue()->set_tube_name(FLAGS_target_tube);
      input.mutable_target_queue()->set_priority(FLAGS_target_priority);
    }
    submitter.AddJob(input);
    LOG(ERROR) << "successfully submit url: " << body << std::endl;
    cnt++;
  }
  return 0;
}
