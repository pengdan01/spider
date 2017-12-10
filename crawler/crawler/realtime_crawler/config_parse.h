#pragma once

#include <string>

#include "crawler/crawl/config_parse.h"
#include "crawler/realtime_crawler/job_manager.h"
#include "crawler/realtime_crawler/realtime_crawler.h"
#include "crawler/realtime_crawler/realtime_extractor.h"
#include "crawler/realtime_crawler/fetch_result_handler.h"

namespace crawler2 {
namespace crawl_queue {
void ParseFetcherHandlerConfig(const extend::config::ConfigObject& config,
                               SubmitCrawlResultHandler::Options* optioin);
void ParseRealtimeCrawlerConfig(const extend::config::ConfigObject& config,
                                RealtimeCrawler::Options* crawler_option);

void ParseConfig(const std::string& config_path,
                 crawl::LoadController::Options* controller_option,
                 crawl::ProxyManager::Options* proxy_option,
                 crawl::PageCrawler::Options* page_crawler_option,
                 JobManager::Options* job_mgr_option,
                 SubmitCrawlResultHandler::Options* handler_optioin,
                 RealtimeCrawler::Options* crawler_option);
}  // namespace crawl_queue
}  // namespace crawler2
