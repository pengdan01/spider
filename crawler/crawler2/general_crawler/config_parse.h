#pragma once

#include <string>

#include "crawler/crawl/config_parse.h"
#include "crawler2/general_crawler/job_manager.h"
#include "crawler2/general_crawler/crawler.h"
#include "crawler2/general_crawler/extractor.h"
#include "crawler2/general_crawler/fetch_result_handler.h"

namespace crawler3 {
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
                 GeneralCrawler::Options* crawler_option);
}  // namespace crawler3
