#include <vector>

#include "base/strings/string_split.h"
#include "spider/crawler/config_parse.h"

#include "extend/config/config.h"
#include "crawler/crawl/config_parse.h"

namespace spider {

void ParseFetcherHandlerConfig(const extend::config::ConfigObject &config,
                               const extend::config::ConfigObject &kafka_config,
                               SubmitCrawlResultHandler::Options *option) {
  option->saver_timespan = config["saver_timespan"].integer() * 1000 * 1000l;
  option->status_prefix = config["status_prefix"].str();
  option->kafka_brokers = kafka_config["kafka_brokers"].str();
  option->kafka_job_request_topic = kafka_config["kafka_job_request_topic"].str();
  option->kafka_job_result_topic = kafka_config["kafka_job_result_topic"].str();
}

void ParseGeneralCrawlerConfig(const extend::config::ConfigObject &config,
                               GeneralCrawler::Options *crawler_option) {
  crawler_option->max_queue_length = config["max_queue_length"].integer();
  crawler_option->max_duration_inqueue = config["max_duration_inqueue"].integer();
  crawler_option->host_use_proxy_file = config["host_use_proxy_file"].str();
  crawler_option->percentage_use_proxy = config["percentage_use_proxy"].integer();
}

void ParseJobMgrOption(const extend::config::ConfigObject &config,
                       const extend::config::ConfigObject &kafka_config,
                       JobManager::Options *job_mgr_option) {
  job_mgr_option->kafka_brokers = kafka_config["kafka_brokers"].str();
  job_mgr_option->kafka_job_request_topic = kafka_config["kafka_job_request_topic"].str();
  job_mgr_option->kafka_consumer_group_name = kafka_config["kafka_consumer_group_name"].str();
  job_mgr_option->max_retry_times = config["max_retry_times"].integer();
  job_mgr_option->holdon_when_jobfull = config["holdon_when_jobfull"].integer();

  if (kafka_config.GetSubObject("kafka_assignor") != NULL) {
    job_mgr_option->kafka_assignor = kafka_config.GetSubObject("kafka_assignor")->str();
  }
  if (kafka_config.GetSubObject("kafka_seek_end") != NULL) {
    std::string kafka_seek_end = kafka_config.GetSubObject("kafka_seek_end")->str();
    job_mgr_option->kafka_seek_end = (kafka_seek_end == "true");
  }

  if (config.GetSubObject("dynamic_conf_addresses") != NULL) {
    std::string prefix = config.GetSubObject("dynamic_conf_addresses")->str();
    size_t pos = prefix.find('/');
    if (pos == std::string::npos) {
      LOG(FATAL) << "dynamic_conf_addresses must contains a prefix dir path, eg: 11.11.11.11/prefix_dir";
    }
    if (!base::EndsWith(prefix, "/", false)) {
      prefix = prefix + "/";
    }
    job_mgr_option->dynamic_conf_addresses = prefix + job_mgr_option->kafka_consumer_group_name;
  }

  if (config.GetSubObject("black_sites") != NULL) {
    std::vector<std::string> black_sites;
    base::SplitString(config["black_sites"].str(), ",", &black_sites);
    for (auto it = black_sites.begin(); it != black_sites.end(); ++it) {
      std::string site = *it;
      base::TrimWhitespaces(&site);
      if (!site.empty()) {
        job_mgr_option->black_sites.push_back(site);
      }
    }
  }
  if (config.GetSubObject("max_dedup_cache_size") != NULL) {
    job_mgr_option->max_dedup_cache_size = config.GetSubObject("max_dedup_cache_size")->integer();
  }
  if (config.GetSubObject("min_dedup_interval_ms") != NULL) {
    job_mgr_option->min_dedup_interval_ms = config.GetSubObject("min_dedup_interval_ms")->integer();
  }
}

void ParseConfig(const std::string &config_path,
                 crawler2::crawl::LoadController::Options *controller_option,
                 crawler2::crawl::ProxyManager::Options *proxy_option,
                 crawler2::crawl::PageCrawler::Options *page_crawler_option,
                 JobManager::Options *job_mgr_option,
                 SubmitCrawlResultHandler::Options *handler_option,
                 GeneralCrawler::Options *crawler_option,
                 std::string *show_service_name) {
  extend::config::Config config;
  CHECK_EQ(0, config.ParseFile(config_path))
    << "Failed to parse config file: "
    << config_path << " because: " << config.error_desc();
  crawler2::crawl::ParseLoadControllerConfig(config["crawler"]["load_controller"],
                                             controller_option);
  crawler2::crawl::ParseProxyManagerConfig(config["crawler"]["proxy"],
                                           proxy_option);
  crawler2::crawl::ParsePageCrawlerConfig(config["crawler"]["page_crawler"],
                                          page_crawler_option);
  ParseGeneralCrawlerConfig(config["crawler"], crawler_option);
  ParseJobMgrOption(config["crawler"]["job_manager"], config["crawler"]["kafka_config"], job_mgr_option);
  ParseFetcherHandlerConfig(config["crawler"]["handler"], config["crawler"]["kafka_config"], handler_option);
  *show_service_name = config["crawler"]["show_service_name"].str();

  // 保持这个最大重试次数一致
  handler_option->max_retry_times = job_mgr_option->max_retry_times;
}

}  // namespace crawler2
