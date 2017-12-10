#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <iostream>

#include "base/process/process_util.h"
#include "base/time/timestamp.h"
#include "base/time/time.h"
#include "crawler2/general_crawler/extractor.h"
#include "rpc/redis/redis_control/redis_control.h"
#include "rpc/http_counter_export/export.h"


DEFINE_int32(refresh_count, 300, "seconds to refresh counter.");
DEFINE_string(config_path, "", "");

using crawler3::GeneralExtractor;

DEFINE_int64_counter(general_extractor, memory, 0, "memory");

int main(int argc, char* argv[]) {
  base::InitApp(&argc, &argv, "general extracotr");
  LOG(INFO) << "Start HTTPCounter on port: "
            << rpc_util::FLAGS_http_port_for_counters;
  rpc_util::HttpCounterExport();

  extend::config::Config config;
  CHECK_EQ(0, config.ParseFile(FLAGS_config_path)) << config.error_desc();

  GeneralExtractor::Options extractor_options;
  ParseGeneralExtractorConfig(config["general_extractor"]["extractor"],
                               &extractor_options);

  // --ds_redis_ips_file 指定 server lists
  ::redis::RedisController redis_controler;
  int success, total;
  redis_controler.Init(5000, &total, &success);
  // CHECK_EQ(success, total);

  GeneralExtractor  extractor(extractor_options, redis_controler);

  extractor.Start();

  base::ProcessMetrics* process_metrics =
      base::ProcessMetrics::CreateProcessMetrics(base::GetCurrentProcessHandle());
  while (true) {
    COUNTERS_general_extractor__memory.Reset(process_metrics->GetWorkingSetSize());
    sleep(1);
  }

  return 0;
}
