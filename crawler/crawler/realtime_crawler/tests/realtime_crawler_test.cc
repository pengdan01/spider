#include <vector>

#include "base/testing/gtest.h"
#include "base/thread/thread.h"
#include "base/common/scoped_ptr.h"
#include "base/common/closure.h"
#include "crawler/realtime_crawler/realtime_crawler.h"
#include "crawler/realtime_crawler/config_parse.h"
#include "crawler/realtime_crawler/tests/ut_webserver.h"
#include "crawler/realtime_crawler/fetch_result_handler.h"
#include "crawler/realtime_crawler/tests/simple_basesearch_server.h"
#include "crawler/realtime_crawler/tests/crawl_data_handler.h"
#include "crawler/realtime_crawler/job_submit.h"
#include "crawler/realtime_crawler/job_monitor.h"
#include "crawler/crawl/rpc_service_controller.h"

using crawler2::Resource;
using crawler2::crawl::LoadController;
using crawler2::crawl::ProxyManager;
using crawler2::crawl::PageCrawler;
using crawler2::crawl::RpcServiceController;
using crawler2::crawl::CrawlTask;
using crawler2::crawl::CrawlRoutineStatus;
using crawler2::crawl_queue::CrawlJobSubmitter;
using crawler2::crawl_queue::CrawlTaskHandler;
using crawler2::crawl_queue::RealtimeCrawler;
using crawler2::crawl_queue::SubmitCrawlResultHandler;
using crawler2::crawl_queue::JobManager;
using crawler2::crawl_queue::JobInput;
using crawler2::crawl_queue::JobDetail;
using crawler2::crawl_queue::JobMonitor;

TEST(RealtimeCrawlerTest, StartStop) {
  const char* config_path="crawler/realtime_crawler/tests/realtime_crawler.cfg";
  LoadController::Options controller_option;
  ProxyManager::Options proxy_option;
  PageCrawler::Options page_crawler_option;
  JobManager::Options job_mgr_option;
  SubmitCrawlResultHandler::Options handler_option;
  RealtimeCrawler::Options crawler_option;
  CounterCrawlTaskHandler handler;
  ParseConfig(config_path, &controller_option,
              &proxy_option, &page_crawler_option,
              &job_mgr_option, &handler_option, &crawler_option);
  RealtimeCrawler crawler(crawler_option, proxy_option,
                          controller_option, page_crawler_option);
  crawler.Start(&handler);
  crawler.Stop();
}

TEST(RealtimeCrawlerTest, FetchSingle) {
  const char* html_path = "/index.html";
  const char* html_data = "<html><title>h</title></html>";
  std::unordered_map<std::string, std::string> url_data;
  url_data[html_path] = html_data;

  bamboo::WebServer::Options options;
  options.port = 31118;
  UTWebServer web_server(url_data, options);
  web_server.Start();
  CounterCrawlTaskHandler handler;

  // 发送一个请求, 从本地 webserver 的到请求， 检查 mock_index_server
  // 是否能够接受到请求
  const char* config_path="crawler/realtime_crawler/tests/realtime_crawler.cfg";
  LoadController::Options controller_option;
  ProxyManager::Options proxy_option;
  PageCrawler::Options page_crawler_option;
  JobManager::Options job_mgr_option;
  SubmitCrawlResultHandler::Options handler_option;
  RealtimeCrawler::Options crawler_option;
  ParseConfig(config_path, &controller_option, &proxy_option,
              &page_crawler_option, &job_mgr_option, &handler_option,
              &crawler_option);

  RealtimeCrawler crawler(crawler_option,
                          proxy_option,
                          controller_option,
                          page_crawler_option);

  crawler.Start(&handler);

  // 添加一些任务
  RealtimeCrawler::JobData data;
  data.timestamp = ::base::GetTimestamp();
  data.job.mutable_detail()->set_url("http://127.0.0.1:31118/index.html");
  crawler.AddTask(&data);
  crawler.AddTask(&data);
  crawler.AddTask(&data);

  while (handler.count < 3) {
    usleep(50);
  }

  // index_service 接收到的数据数量不为 0
  crawler.Stop();
  web_server.Stop();
}

TEST(RealtimeCrawlerTest, CrawlWithJobMonitor) {
  const char* html_path = "/index.html";
  const char* html_data = "<html><title>h</title></html>";
  std::unordered_map<std::string, std::string> url_data;
  url_data[html_path] = html_data;

  bamboo::WebServer::Options options;
  options.port = 31118;
  UTWebServer web_server(url_data, options);
  web_server.Start();
  CounterCrawlTaskHandler handler;

  // 发送一个请求, 从本地 webserver 的到请求， 检查 mock_index_server
  // 是否能够接受到请求
  const char* config_path="crawler/realtime_crawler/tests/realtime_crawler.cfg";
  LoadController::Options controller_option;
  ProxyManager::Options proxy_option;
  PageCrawler::Options page_crawler_option;
  JobManager::Options job_mgr_option;
  SubmitCrawlResultHandler::Options handler_option;
  RealtimeCrawler::Options crawler_option;
  ParseConfig(config_path, &controller_option, &proxy_option,
              &page_crawler_option, &job_mgr_option, &handler_option,
              &crawler_option);

  // 提交任务
  CrawlJobSubmitter::Option submitter_option;
  submitter_option.queue_ip = job_mgr_option.queue_ip;
  submitter_option.queue_port = job_mgr_option.queue_port;
  submitter_option.queue_tube = job_mgr_option.queue_tube;
  CrawlJobSubmitter submitter(submitter_option);
  submitter.Init();
  JobDetail detail;
  detail.set_url("http://127.0.0.1:31118/index.html");
  EXPECT_TRUE(submitter.AddJob(detail));
  EXPECT_TRUE(submitter.AddJob(detail));
  EXPECT_TRUE(submitter.AddJob(detail));

  // 启动任务监控
  JobManager job_mgr(job_mgr_option);
  job_mgr.Init();

  // 启动爬虫
  RealtimeCrawler crawler(crawler_option,
                          proxy_option,
                          controller_option,
                          page_crawler_option);

  crawler.Start(&handler);

  // 将两者联合起来
  JobMonitor monitor(&job_mgr, &crawler);
  monitor.Start();

  while (handler.count < 3) {
    usleep(50);
  }

  // index_service 接收到的数据数量不为 0
  monitor.Stop();

  // 为了能够时期顺利退出
  EXPECT_TRUE(submitter.AddJob(detail));
  EXPECT_TRUE(submitter.AddJob(detail));
  EXPECT_TRUE(submitter.AddJob(detail));
  crawler.Stop();
  web_server.Stop();
}


