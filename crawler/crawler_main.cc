#include <fstream>
#include <iostream>

#include "base/thread/thread.h"
#include "base/time/timestamp.h"
#include "base/time/time.h"
#include "base/process/process_util.h"
#include "rpc/http_counter_export/export.h"

#include "crawler/crawl/rpc_service_controller.h"
#include "spider/crawler/crawler.h"
#include "spider/crawler/config_parse.h"
#include "spider/crawler/job_manager.h"
#include "spider/crawler/fetch_result_handler.h"
#include "spider/crawler/job_monitor.h"
#include "spider/crawler/amonitor_reporter.h"

#include "serving_base/utility/signal.h"

DEFINE_string(config_path, "", "");
DEFINE_string(service_name, "", "标识服务名，用来grep进程使用");

using crawler2::Resource;
using crawler2::crawl::CrawlTask;
using crawler2::crawl::CrawlRoutineStatus;
using crawler2::crawl::LoadController;
using crawler2::crawl::ProxyManager;
using crawler2::crawl::PageCrawler;
using crawler2::crawl::RpcServiceController;
using crawler2::crawl_queue::JobInput;

using spider::GeneralCrawler;
using spider::JobManager;
using spider::CrawlTaskHandler;
using spider::JobMonitor;
using spider::SubmitCrawlResultHandler;
using spider::ParseConfig;

DEFINE_int64_counter(general_crawler, memory, 0, "memory");
DEFINE_uint64(max_running_time_in_ms, 0, "main will exit after max_running_time_in_ms ms");

int main(int argc, char* argv[]) {
  base::InitApp(&argc, &argv, "reco spider");
  // 先关闭这个http counter监听，总是端口被占用而core掉
  // LOG(INFO) << "Start HTTPCounter on port: " << rpc_util::FLAGS_http_port_for_counters;
  // rpc_util::HttpCounterExport();

  LoadController::Options controller_option;
  ProxyManager::Options proxy_option;
  PageCrawler::Options page_crawler_option;
  JobManager::Options job_mgr_option;
  GeneralCrawler::Options crawler_option;
  SubmitCrawlResultHandler::Options handler_option;
  std::string show_service_name;

  LOG(INFO) << "Parsing Config...";
  ParseConfig(FLAGS_config_path, &controller_option, &proxy_option,
              &page_crawler_option, &job_mgr_option, &handler_option,
              &crawler_option, &show_service_name);

  spider::AmonitorReporter::GetInstance().Init(show_service_name);

  LOG(INFO) << "DataHandler starting up...";
  // handler 将失败任务写回 job manager 监听 job queue
  SubmitCrawlResultHandler handler(handler_option);
  handler.Init();

  LOG(INFO) << "JobManager starting up...";
  JobManager job_mgr(job_mgr_option);
  job_mgr.Init();

  LOG(INFO) << "GeneralCrawler staring up...";
  GeneralCrawler crawler(crawler_option, proxy_option, controller_option,
                         page_crawler_option);
  crawler.Start(&handler);

  LOG(INFO) << "JobMonitor starting up...";
  JobMonitor monitor(&job_mgr, &crawler);
  monitor.Start();

  base::ProcessMetrics* process_metrics =
       base::ProcessMetrics::CreateProcessMetrics(base::GetCurrentProcessHandle());
  LOG(INFO) << "crawler started...";
  // 初始化一下信号捕捉器，收到kill信号，要正确地退出，不要丢数据
  serving_base::SignalCatcher::Initialize();

  // 设置一个程序最大运行时间，到时间自动退出， 如果没设置，则永远运行
  if (FLAGS_max_running_time_in_ms == 0l) {
    while (serving_base::SignalCatcher::GetSignal() < 0) {
      base::SleepForSeconds(1);
      COUNTERS_general_crawler__memory.Reset((int64_t) process_metrics->GetWorkingSetSize());
    }
  } else {
    uint64 start_time = (uint64) base::GetTimestamp();
    while (base::GetTimestamp() - start_time < FLAGS_max_running_time_in_ms * 1000
        && serving_base::SignalCatcher::GetSignal() < 0) {
      base::SleepForSeconds(1);
      COUNTERS_general_crawler__memory.Reset((int64_t) process_metrics->GetWorkingSetSize());
    }
  }
  if (serving_base::SignalCatcher::GetSignal() >= 0) {
    LOG(INFO) << "Receive signal " << serving_base::SignalCatcher::GetSignal() << ", just safe to exit";
  }
  // 清理操作，优雅退出
  job_mgr.StopReadDataFromMQ();
  monitor.Stop();
  crawler.Stop();
  handler.Stop();
  job_mgr.StopWriteBackFailedJobToMQ();
  delete process_metrics;
  spider::AmonitorReporter::GetInstance().Stop();
  LOG(INFO) << "All counters: " << spider::AmonitorReporter::GetInstance().GetLastCounterValue();
  return 0;
}
