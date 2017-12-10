#pragma once

#include <string>

#include "extend/config/config.h"
#include "crawler/crawl/page_crawler.h"
#include "crawler/crawl/load_controller.h"
#include "crawler/crawl/proxy_manager.h"


namespace crawler2 {
namespace crawl {

void ParseLoadControllerConfig(const extend::config::ConfigObject& config,
                               LoadController::Options* option);

void ParsePageCrawlerConfig(const extend::config::ConfigObject& config,
                            PageCrawler::Options* option);

void ParseProxyManagerConfig(const extend::config::ConfigObject& config,
                            ProxyManager::Options* option);
}  // namespace crawl
}  // namespace crawler2
