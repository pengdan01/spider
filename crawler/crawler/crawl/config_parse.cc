#include "crawler/crawl/config_parse.h"

#include <iostream>
#include <string>

#include "crawler/crawl/page_crawler.h"
#include "crawler/crawl/load_controller.h"
#include "base/file/file_util.h"
#include "base/strings/string_util.h"
#include "base/strings/string_split.h"
#include "base/strings/string_number_conversions.h"

namespace crawler2 {
namespace crawl {
void ParsePageCrawlerConfig(const extend::config::ConfigObject& config,
                            PageCrawler::Options* fetcher_option) {
  CHECK(!config["connect_timeout_in_seconds"].IsNull())
      << "realtime_fetcher.fetcher.connect_timeout_in_seconds not exist";
  fetcher_option->connect_timeout_in_seconds = config["connect_timeout_in_seconds"].value.integer;

  CHECK(!config["transfer_timeout_in_seconds"].IsNull())
      << "realtime_fetcher.fetcher.transfer_timeout_in_seconds not exist";
  fetcher_option->transfer_timeout_in_seconds = config["transfer_timeout_in_seconds"].value.integer;

  CHECK(!config["low_speed_limit_in_bytes_per_second"].IsNull())
      << "realtime_fetcher.fetcher.low_speed_limit_in_bytes_per_second not exist";
  fetcher_option->low_speed_limit_in_bytes_per_second =
      config["low_speed_limit_in_bytes_per_second"].value.integer;

  CHECK(!config["low_speed_duration_in_seconds"].IsNull())
      << "realtime_fetcher.fetcher.low_speed_duration_in_seconds not exist";
  fetcher_option->low_speed_duration_in_seconds = config["low_speed_duration_in_seconds"].value.integer;

  CHECK(!config["user_agent"].IsNull()) << "realtime_fetcher.fetcher.user_agent not exist";
  fetcher_option->user_agent = config["user_agent"].value.string;

  CHECK(!config["user_agent_p"].IsNull()) << "realtime_fetcher.fetcher.user_agent_p not exist";
  fetcher_option->user_agent_p = config["user_agent_p"].value.string;
  LOG(ERROR) << "user_agent_p: " << fetcher_option->user_agent_p;

  CHECK(!config["max_redir_times"].IsNull()) << "realtime_fetcher.fetcher.max_redir_times not exist";
  fetcher_option->max_redir_times = config["max_redir_times"].value.integer;

  CHECK(!config["enable_curl_debug"].IsNull()) << "realtime_fetcher.fetcher.enable_curl_debug not exist";
  fetcher_option->enable_curl_debug = config["enable_curl_debug"].value.integer;

  CHECK(!config["client_redirect_urls_filepath"].IsNull())
      << "realtime_fetcher.fetcher.client_redirect_urls_filepath not exist";
  fetcher_option->client_redirect_urls_filepath= config["client_redirect_urls_filepath"].value.string;

  CHECK(!config["nobody"].IsNull()) << "realtime_fetcher.fetcher.nobody not exist";
  fetcher_option->nobody = config["nobody"].value.integer;
}

void ParseLoadControllerConfig(const extend::config::ConfigObject& config,
                               LoadController::Options* option) {
  // 解析 load_controller 的 option
  option->default_max_qps = config["default_max_qps"].value.integer;
  option->default_max_connections = config["default_max_connections"].value.integer;
  option->max_connections_in_all = config["max_connections_in_all"].value.integer;
  option->max_holdon_duration_after_failed = config["max_holdon_duration_after_failed"].value.integer;
  option->min_holdon_duration_after_failed = config["min_holdon_duration_after_failed"].value.integer;
  option->failed_times_threadhold = config["failed_times_threadhold"].value.integer;
  option->check_frequency = config["check_frequency"].value.integer;
  option->failed_times_threadhold = config["failed_times_threadhold"].value.integer;
  option->max_failed_times = config["max_failed_times"].value.integer;
  option->ip_load_file = config["ip_load_file"].value.string;
}

void ParseProxyManagerConfig(const extend::config::ConfigObject& config,
                             ProxyManager::Options* option) {
  const extend::config::ConfigObject& proxy = config["list"];
  for (size_t i = 0; i < proxy.value.array->size(); ++i) {
    const extend::config::ConfigObject* item = proxy.value.array->at(i);
    option->proxy.push_back(item->value.string);
  }
  option->max_successive_failures = config["max_successive_failures"].
      value.integer;
  int value = config["holdon_duration_after_failures"].value.integer;
  option->holdon_duration_after_failures = value * 1000 * 1000l;
}
}  // namespace realtime_fetcher
}  // namespace crawler2
