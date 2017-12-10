#include <vector>

#include "base/testing/gtest.h"
#include "crawler/realtime_crawler/realtime_crawler.h"
#include "crawler/realtime_crawler/config_parse.h"
#include "crawler/realtime_crawler/job_manager.h"
#include "crawler/realtime_crawler/job_submit.h"
#include "crawler/realtime_crawler/tests/crawl_data_handler.h"
#include "crawler/realtime_crawler/crawl_queue.pb.h"
#include "crawler/crawl/rpc_service_controller.h"


using crawler2::crawl::CrawlRoutineStatus;
using crawler2::crawl_queue::SubmitCrawlResultHandler;
using crawler2::crawl_queue::RealtimeCrawler;
using crawler2::crawl_queue::CrawlTaskHandler;
using crawler2::crawl_queue::JobManager;
using crawler2::crawl_queue::JobInput;

TEST(JobManager, Simple) {
  using crawler2::crawl_queue::CrawlJobSubmitter;
  using crawler2::crawl_queue::JobInput;
  using crawler2::crawl_queue::JobDetail;

  // 提交几个任务
  CrawlJobSubmitter::Option submitter_option;
  submitter_option.queue_ip = "127.0.0.1";
  submitter_option.queue_port = 40000;
  CrawlJobSubmitter submitter(submitter_option);
  submitter.Init();
  JobDetail detail;
  detail.set_url("http://www.sina.com/");
  EXPECT_TRUE(submitter.AddJob(detail));
  EXPECT_TRUE(submitter.AddJob(detail));
  EXPECT_TRUE(submitter.AddJob(detail));

  JobManager::Options option;
  option.queue_ip = submitter_option.queue_ip;
  option.queue_port = submitter_option.queue_port;

  JobManager manager(option);
  manager.Init();

  JobManager::JobQueue que;
  while (que.size() < 3u) {
    manager.TakeAll(&que);
  }

  while (!que.empty()) {
    RealtimeCrawler::JobData* item = que.front();
    /*
    manager.DeleteJob(item->jobid);
    */
    item->done->Run();
    que.pop_front();
  }

  // 尝试获取任务
  manager.Stop();
  LOG(ERROR) << "test JobManager:Simple finished";
}

TEST(JobManager, JobInTube) {
  using crawler2::crawl_queue::CrawlJobSubmitter;
  using crawler2::crawl_queue::JobInput;
  using crawler2::crawl_queue::JobDetail;

  // 提交几个任务
  CrawlJobSubmitter::Option submitter_option;
  submitter_option.queue_ip = "127.0.0.1";
  submitter_option.queue_port = 40000;
  submitter_option.queue_tube = "tube";
  CrawlJobSubmitter submitter(submitter_option);
  submitter.Init();
  JobDetail detail;
  detail.set_url("http://www.sina.com/");
  EXPECT_TRUE(submitter.AddJob(detail));
  EXPECT_TRUE(submitter.AddJob(detail));
  EXPECT_TRUE(submitter.AddJob(detail));

  JobManager::Options option;
  option.queue_ip = submitter_option.queue_ip;
  option.queue_port = submitter_option.queue_port;
  option.queue_tube = submitter_option.queue_tube;

  JobManager manager(option);
  manager.Init();

  JobManager::JobQueue que;
  while (que.size() < 3u) {
    manager.TakeAll(&que);
  }

  while (!que.empty()) {
    que.front()->done->Run();
    que.pop_front();
  }

  // 尝试获取任务
  manager.Stop();
}

