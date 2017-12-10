#pragma once

#include <string>
#include <set>

#include "crawler/crawl/config_parse.h"
#include "spider/crawler/job_manager.h"
#include "spider/crawler/crawler.h"
#include "spider/crawler/fetch_result_handler.h"

namespace spider {
void ParseFetcherHandlerConfig(const extend::config::ConfigObject& config,
                               SubmitCrawlResultHandler::Options* optioin);
void ParseGeneralCrawlerConfig(const extend::config::ConfigObject& config,
                                GeneralCrawler::Options* crawler_option);

void ParseConfig(const std::string& config_path,
                 crawler2::crawl::LoadController::Options* controller_option,
                 crawler2::crawl::ProxyManager::Options* proxy_option,
                 crawler2::crawl::PageCrawler::Options* page_crawler_option,
                 JobManager::Options* job_mgr_option,
                 SubmitCrawlResultHandler::Options* handler_optioin,
                 GeneralCrawler::Options* crawler_option,
                 std::string* show_service_name);
}  // namespace spider 
