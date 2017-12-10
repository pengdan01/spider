#include <fstream>
#include <iostream>

#include "base/thread/thread.h"
#include "base/time/timestamp.h"
#include "base/time/time.h"
#include "base/process/process_util.h"
#include "crawler/realtime_crawler/realtime_crawler.h"
#include "crawler/realtime_crawler/config_parse.h"
#include "crawler/realtime_crawler/job_manager.h"
#include "crawler/realtime_crawler/fetch_result_handler.h"
#include "crawler/realtime_crawler/job_monitor.h"
#include "crawler/crawl/rpc_service_controller.h"
#include "rpc/http_counter_export/export.h"

DEFINE_string(config_path, "", "");

using crawler2::Resource;
using crawler2::crawl::CrawlTask;
using crawler2::crawl::CrawlRoutineStatus;
using crawler2::crawl::LoadController;
using crawler2::crawl::ProxyManager;
using crawler2::crawl::PageCrawler;
using crawler2::crawl_queue::RealtimeCrawler;
using crawler2::crawl::RpcServiceController;
using crawler2::crawl_queue::JobManager;
using crawler2::crawl_queue::CrawlTaskHandler;
using crawler2::crawl_queue::JobMonitor;
using crawler2::crawl_queue::SubmitCrawlResultHandler;
using crawler2::crawl_queue::JobInput;

DEFINE_int64_counter(realtime_crawler, memory, 0, "memory");

int main(int argc, char* argv[]) {
  ::base::InitApp(&argc, &argv, "rt_fetcher");
  LOG(INFO) << "Start HTTPCounter on port: "
            << rpc_util::FLAGS_http_port_for_counters;
  rpc_util::HttpCounterExport();
  
  LoadController::Options controller_option;
  ProxyManager::Options proxy_option;
  PageCrawler::Options page_crawler_option;
  JobManager::Options job_mgr_option;
  RealtimeCrawler::Options crawler_option;
  SubmitCrawlResultHandler::Options handler_option;

  LOG(INFO) << "Parsing Config...";
  ParseConfig(FLAGS_config_path, &controller_option, &proxy_option,
              &page_crawler_option, &job_mgr_option, &handler_option,
              &crawler_option);

  //LOG(INFO) << "Load all dict...";
  //CHECK(crawl_queue::DictManager::LoadDict()) << "Load dict error";

  LOG(INFO) << "DataHandler starting up...";
  SubmitCrawlResultHandler handler(handler_option);
  handler.Init();

  LOG(INFO) << "JobManager starting up...";
  JobManager job_mgr(job_mgr_option);
  job_mgr.Init();

  LOG(INFO) << "RealtimeCrawler staring up...";
  RealtimeCrawler crawler(crawler_option, proxy_option, controller_option,
                          page_crawler_option);
  crawler.Start(&handler);

  LOG(INFO) << "JobMonitor starting up...";
  JobMonitor monitor(&job_mgr, &crawler);
  monitor.Start();

  base::ProcessMetrics* process_metrics =
       base::ProcessMetrics::CreateProcessMetrics(base::GetCurrentProcessHandle());
  LOG(INFO) << "crawler started...";
  while (true) {
    sleep(1);
    COUNTERS_realtime_crawler__memory.Reset(process_metrics->GetWorkingSetSize());
  }

  return 0;
}
