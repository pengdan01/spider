#include "crawler/realtime_crawler/config_parse.h"

#include "extend/config/config.h"
#include "crawler/crawl/config_parse.h"

namespace crawler2 {
namespace crawl_queue {
void ParseFetcherHandlerConfig(const extend::config::ConfigObject& config,
                               SubmitCrawlResultHandler::Options* option) {
  option->saver_timespan = config["saver_timespan"].integer() * 1000 * 1000l;
  option->status_prefix = config["status_prefix"].str();
  option->default_queue_port = config["default_queue_port"].integer();
  option->default_queue_tube = config["default_queue_tube"].str();
  option->default_queue_ip = config["default_queue_ip"].str();
}

void ParseRealtimeCrawlerConfig(const extend::config::ConfigObject& config,
                                RealtimeCrawler::Options* crawler_option) {
  crawler_option->max_queue_length = config["max_queue_length"].integer();
  crawler_option->max_duration_inqueue = config["max_duration_inqueue"].integer();
  crawler_option->host_use_proxy_file = config["host_use_proxy_file"].str();
  crawler_option->percentage_use_proxy = config["percentage_use_proxy"].integer();
}

void ParseJobMgrOption(const extend::config::ConfigObject& config,
                       JobManager::Options* job_mgr_option) {
  job_mgr_option->queue_ip = config["queue_ip"].str();
  job_mgr_option->queue_port = config["queue_port"].integer();
  job_mgr_option->queue_tube = config["queue_tube"].str();
  job_mgr_option->max_retry_times = config["max_retry_times"].integer();
  job_mgr_option->holdon_when_jobfull = config["holdon_when_jobfull"].integer();
}

void ParseConfig(const std::string& config_path,
                 crawl::LoadController::Options* controller_option,
                 crawl::ProxyManager::Options* proxy_option,
                 crawl::PageCrawler::Options* page_crawler_option,
                 JobManager::Options* job_mgr_option,
                 SubmitCrawlResultHandler::Options* handler_option,
                 RealtimeCrawler::Options* crawler_option) {
  extend::config::Config config;
  CHECK_EQ(0, config.ParseFile(config_path))
      << "Failed to parse config file: "
      << config_path << " because: " <<  config.error_desc();
  crawl::ParseLoadControllerConfig(config["realtime_crawler"]["load_controller"],
                            controller_option);
  crawl::ParseProxyManagerConfig(config["realtime_crawler"]["proxy"],
                          proxy_option);
  crawl::ParsePageCrawlerConfig(config["realtime_crawler"]["page_crawler"],
                         page_crawler_option);
  ParseRealtimeCrawlerConfig(config["realtime_crawler"], crawler_option);
  ParseJobMgrOption(config["realtime_crawler"]["job_manager"], job_mgr_option);
  ParseFetcherHandlerConfig(config["realtime_crawler"]["handler"], handler_option);
}

}  // namespace crawl_queue
}  // namespace crawler2
