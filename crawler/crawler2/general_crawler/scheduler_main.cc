#include <sys/types.h>
#include <unistd.h>
#include <iostream>

#include "base/time/timestamp.h"
#include "base/file/file_util.h"
#include "base/strings/string_split.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "base/process/process_util.h"
#include "rpc/redis/redis_control/redis_control.h"
#include "rpc/http_counter_export/export.h"
#include "crawler2/general_crawler/config_parse.h"

#include "crawler2/general_crawler/scheduler.h"

using crawler3::Scheduler;

// 读入/输出的 job queue 连接参数
DEFINE_string(in_queue_ip, "127.0.0.1", "待处理 url 队列 ip");
DEFINE_int32(in_queue_port, -1, "待处理 url 队列 port");
DEFINE_string(in_queue_tube, "", "待处理 url 队列管道 tube");
DEFINE_string(out_queue_file, "data/schedule_output_queue.txt", "存放调度后的 url 将要被输出到的队列集合."
              "文件格式: queue_ip:queue_port:queue_tube or queue_ip:queue_port."
              "以 '#' 开头的行会被忽略");

DEFINE_int64_counter(scheduler, memory, 0, "memory");

void ParseOutQueue(const std::string &, std::vector<Scheduler::Options>*);

int main(int argc, char* argv[]) {
  base::InitApp(&argc, &argv, "scheduler");
  LOG(INFO) << "Start HTTPCounter on port: " << rpc_util::FLAGS_http_port_for_counters;
  rpc_util::HttpCounterExport();
  // 0. 参数有效性检查
  CHECK(FLAGS_in_queue_port > 0 && !FLAGS_in_queue_ip.empty());
  CHECK(!FLAGS_out_queue_file.empty() && base::file_util::PathExists(FLAGS_out_queue_file));

  // 1. 初始化
  redis::RedisController redis_controler;  // --ds_redis_ips_file 指定 server lists
  int success, total;
  redis_controler.Init(5000, &total, &success);
  CHECK_EQ(success, total);

  Scheduler::Options options;
  options.queue_ip = FLAGS_in_queue_ip;
  options.queue_port = FLAGS_in_queue_port;
  options.queue_tube = FLAGS_in_queue_tube;

  std::vector<Scheduler::Options> out_queue_options;
  ParseOutQueue(FLAGS_out_queue_file, &out_queue_options); 

  // 2. 生成 调度对象, 并启动;
  Scheduler scheduler(options, out_queue_options, redis_controler);
  CHECK(scheduler.Init());

  // 3. 运行调度
  scheduler.Start();

  base::ProcessMetrics* process_metrics =
      base::ProcessMetrics::CreateProcessMetrics(base::GetCurrentProcessHandle());
  while (true) {
    COUNTERS_scheduler__memory.Reset(process_metrics->GetWorkingSetSize());
    sleep(1);
  }

  return 0;
}

void ParseOutQueue(const std::string &out_queue_file, std::vector<Scheduler::Options>* out_queue_options) {
  std::vector<std::string> lines, tokens;
  CHECK(base::file_util::ReadFileToLines(out_queue_file, &lines));
  for (int i = 0; i < (int)lines.size(); ++i) {
    std::string line = lines[i];
    base::TrimWhitespaces(&line);
    if (line.empty() || line[0] == '#') {
      continue;
    }
    tokens.clear(); 
    base::SplitString(line, ":", &tokens);
    int field_num = tokens.size();
    CHECK(field_num == 2 || field_num == 3);
    Scheduler::Options option; 
    option.queue_ip = tokens[0];
    option.queue_port = base::ParseIntOrDie(tokens[1]);
    if (field_num == 3) {
      option.queue_tube = tokens[2];
    }
    out_queue_options->push_back(option);
  }

  CHECK_GT(out_queue_options->size(), 0U);
}
