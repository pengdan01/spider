#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <iostream>

#include "crawler/realtime_crawler/index_request_submitter.h"
#include "crawler/realtime_crawler/config_parse.h"
#include "base/process/process_util.h"
#include "base/time/timestamp.h"
#include "base/time/time.h"
#include "crawler/realtime_crawler/realtime_extractor.h"
#include "rpc/redis/redis_control/redis_control.h"
#include "rpc/http_counter_export/export.h"


DEFINE_int32(refresh_count, 300, "seconds to refresh counter.");
DEFINE_string(config_path, "", "");
DEFINE_bool(backup_mode, true,
            "用于作备份, 如果为 true, 则 cdoc 同时传送给 server list 和 server list2;"
            " 如果为 false, 则 cdoc 只传送给 server list");

using crawler2::crawl_queue::RealtimeExtractor;
using crawler2::crawl_queue::IndexRequestSubmitter;

DEFINE_int64_counter(realtime_extractor, memory, 0, "memory");

int main(int argc, char* argv[]) {
  ::base::InitApp(&argc, &argv, "rt_fetcher");
  LOG(INFO) << "Start HTTPCounter on port: "
            << rpc_util::FLAGS_http_port_for_counters;
  rpc_util::HttpCounterExport();
  
  extend::config::Config config;
  CHECK_EQ(0, config.ParseFile(FLAGS_config_path)) << config.error_desc();
  if (FLAGS_backup_mode == true) {
    LOG(ERROR)<< "cdoc transfre mode: BACKUP";
  } else {
    LOG(ERROR)<< "cdoc transfre mode: NO_BACKUP";
  }

  IndexRequestSubmitter::Options submitter_options;
  IndexRequestSubmitter::Options submitter_options2;
  ParseIndexSubmitterConfig(config["realtime_extractor"]["index_submitter"],
                            &submitter_options);
  ParseIndexSubmitterConfig2(config["realtime_extractor"]["index_submitter"],
                             &submitter_options2);

  RealtimeExtractor::Options extractor_options;
  ParseRealtimeExtractorConfig(config["realtime_extractor"]["extractor"],
                               &extractor_options);

  IndexRequestSubmitter submitter(submitter_options);
  IndexRequestSubmitter submitter2(submitter_options2);

  // --ds_redis_ips_file 指定 server lists
  ::redis::RedisController redis_controler;
  int success, total;
  redis_controler.Init(5000, &total, &success);
  // CHECK_EQ(success, total);

  RealtimeExtractor  extractor(extractor_options, &submitter, &submitter2, redis_controler,
                               FLAGS_backup_mode);
  if (FLAGS_backup_mode == true) {
    submitter.Start();
    submitter2.Start();
  } else {
    submitter.Start();
  }

  extractor.Start();

  base::ProcessMetrics* process_metrics =
      base::ProcessMetrics::CreateProcessMetrics(base::GetCurrentProcessHandle());
  while (true) {
    COUNTERS_realtime_extractor__memory.Reset(process_metrics->GetWorkingSetSize());
    sleep(1);
  }

  return 0;
}
