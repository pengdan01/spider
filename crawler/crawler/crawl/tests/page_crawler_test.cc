#include <vector>

#include "base/testing/gtest.h"
#include "base/thread/thread.h"
#include "base/common/scoped_ptr.h"
#include "web/html/api/parser.h"

#include "crawler/crawl/page_crawler.h"
#include "crawler/crawl/tests/ut_webserver.h"

using crawler2::crawl::PageCrawler;
using crawler2::crawl::CrawlTask;
using crawler2::crawl::CrawlRoutineStatus;
using crawler2::Resource;

TEST(PageCrawler, StartStop) {
  PageCrawler::Options options;
  PageCrawler crawler(options);
  crawler.Init();
}

TEST(PageCrawler, CrawlOnePage) {
  const char* html_path = "/index.html";
  const char* html_data = "<html><title>h</title></html>";
  std::unordered_map<std::string, std::string> url_data;
  url_data[html_path] = html_data;

  // LOG(ERROR) << "key: " << url_data.front().first << ",value: " << url_data.front().second;
  bamboo::WebServer::Options option;
  option.port = 32782;
  UTWebServer web_server(url_data);
  web_server.Start(option);

  PageCrawler::Options options;
  PageCrawler crawler(options);
  crawler.Init();

  CrawlTask* task = new CrawlTask;
  scoped_ptr<CrawlTask> auto_task(task);
  task->url = "http://127.0.0.1:32782/index.html";
  task->referer = "http://127.0.0.1:32782/index.html";
  task->ip = "127.0.0.1";
  task->resource_type = crawler2::kHTML;
  task->https_supported = false;
  CrawlRoutineStatus status;
  Resource res;
  crawler.AddTask(task, &status, &res, NULL);
  crawler.WaitForCompleted();

  // 检查数据
  EXPECT_EQ(res.brief().url(), task->url);

  // 停止服务
  web_server.Stop();
}


// 抓取多个网页
void TaskDone(CrawlTask* task, CrawlRoutineStatus* status, Resource* res,
              std::vector<Resource*>* resources) {
  delete task;
  delete status;
  resources->push_back(res);
}

TEST(PageCrawler, CrawlMultiPage) {
  const uint32 request_count = 10u;
  const char* html_path = "/index.html";
  const char* html_data = "<html><title>h</title></html>";
  std::unordered_map<std::string, std::string> url_data;
  url_data[html_path] = html_data;

  bamboo::WebServer::Options option;
  option.port = 32782;
  UTWebServer web_server(url_data);
  web_server.Start(option);

  PageCrawler::Options options;
  PageCrawler crawler(options);
  crawler.Init();

  std::vector<Resource*> resources;
  for (uint32 i = 0; i < request_count; ++i) {
    CrawlTask* task = new CrawlTask;
    task->url = "http://127.0.0.1:32782/index.html";
    task->referer = "http://127.0.0.1:32782/index.html";
    task->ip = "127.0.0.1";
    task->resource_type = crawler2::kHTML;
    task->https_supported = false;
    CrawlRoutineStatus* status = new CrawlRoutineStatus;
    Resource* res = new Resource;
    Closure* done = NewCallback(TaskDone, task, status, res, &resources);
    crawler.AddTask(task, status, res, done);
  }

  crawler.WaitForCompleted();

  EXPECT_EQ(resources.size(), request_count);
  for (uint32 i = 0; i < request_count; ++i) {
    Resource* res = resources[i];

    // 检查数据
    EXPECT_EQ(res->brief().url(), "http://127.0.0.1:32782/index.html");
    EXPECT_EQ(res->brief().source_url(), "http://127.0.0.1:32782/index.html");
    EXPECT_EQ(res->brief().effective_url(), "http://127.0.0.1:32782/index.html");
    // EXPECT_EQ(res->content().raw_content(), html_data);
    EXPECT_GT(res->brief().crawl_info().crawl_time(), 0);
    EXPECT_GT(res->brief().crawl_info().crawl_timestamp(), 0);
    LOG(ERROR) << "response code: " << res->brief().crawl_info().response_code();
    delete res;
  }

  resources.clear();

  // 停止服务
  web_server.Stop();
}

// 同时抓取多个网页
void TaskDone(std::vector<CrawlTask*>* task,
              std::vector<CrawlRoutineStatus*>* status,
              std::vector<Resource*>* res) {
}

TEST(PageCrawler, CrawlMultiPageOneTime) {
  const uint32 request_count = 10u;
  const char* html_path = "/index.html";
  const char* html_data = "<html><title>h</title></html>";
  std::unordered_map<std::string, std::string> url_data;
  url_data[html_path] = html_data;

  bamboo::WebServer::Options option;
  option.port = 32782;
  UTWebServer web_server(url_data);
  web_server.Start(option);

  PageCrawler::Options options;
  PageCrawler crawler(options);
  crawler.Init();

  std::vector<Resource*> resources;
  std::vector<CrawlTask*> tasks;
  std::vector<CrawlRoutineStatus*> status;
  for (uint32 i = 0; i < request_count; ++i) {
    tasks.push_back(new CrawlTask());
    tasks[i]->url = "http://127.0.0.1:32782/index.html";
    tasks[i]->referer = "http://127.0.0.1:32782/index.html";
    tasks[i]->ip = "127.0.0.1";
    tasks[i]->resource_type = crawler2::kHTML;
    tasks[i]->https_supported = false;
    status.push_back(new CrawlRoutineStatus);
    resources.push_back(new Resource);
    Closure* done = NewCallback(TaskDone, &tasks, &status, &resources);
    crawler.AddMultiTask(&tasks, &status, &resources, done);
  }

  crawler.WaitForCompleted();

  EXPECT_EQ(resources.size(), request_count);
  for (uint32 i = 0; i < request_count; ++i) {
    Resource* res = resources[i];

    // 检查数据
    EXPECT_EQ(res->brief().url(), "http://127.0.0.1:32782/index.html");
    EXPECT_EQ(res->brief().source_url(), "http://127.0.0.1:32782/index.html");
    EXPECT_EQ(res->brief().effective_url(), "http://127.0.0.1:32782/index.html");
    // EXPECT_EQ(res->content().raw_content(), html_data);
    EXPECT_GT(res->brief().crawl_info().crawl_time(), 0);
    EXPECT_GT(res->brief().crawl_info().crawl_timestamp(), 0);
    EXPECT_EQ(status[i]->ret_code, 200);
    delete res;
    delete tasks[i];
    delete status[i];
  }

  resources.clear();

  // 停止服务
  web_server.Stop();
}
