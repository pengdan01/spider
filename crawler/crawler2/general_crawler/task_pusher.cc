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

DEFINE_string(ip, "", "queue ip");
DEFINE_int32(port, 0, "queue port");
DEFINE_string(tube, "", "queue tube");
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

  LOG_IF(FATAL, FLAGS_url_type != 0 && FLAGS_url_type != 1 && FLAGS_url_type != 2)
      << "url type must be 0(kHTML), 1(kCss) or 2(kImage)";

  std::vector<std::string> lines;
  if (!base::file_util::ReadFileToLines(
          base::FilePath(FLAGS_datapath), &lines)) {
    LOG(ERROR) << "Failed to read file : " << FLAGS_datapath;
    return -1;
  }

  CrawlJobSubmitter::Option option;
  option.queue_ip = FLAGS_ip;
  option.queue_port = FLAGS_port;
  option.queue_tube = FLAGS_tube;
  CrawlJobSubmitter submitter(option);
  LOG_IF(FATAL, !submitter.Init()) << "Failed in submitter.Init()";
  int cnt = 1;
  for (auto iter = lines.begin(); iter!= lines.end(); ++iter) {
    crawler2::crawl_queue::JobInput input;
    input.set_submitted_timestamp(base::GetTimestamp());
    std::string url = *iter;
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
    if (cnt % 1000 == 0) {
      std::cout << "successfully submit " << cnt << " tasks." << std::endl;
    }

    cnt++;
  }

  return 0;
}
