#include <vector>

#include "base/testing/gtest.h"
#include "extend/config/config.h"
#include "crawler/crawl/config_parse.h"
#include "crawler/crawl/load_controller.h"
#include "crawler/crawl/page_crawler.h"
#include "crawler/crawl/proxy_manager.h"

using crawler2::crawl::LoadController;
using crawler2::crawl::PageCrawler;
using crawler2::crawl::ProxyManager;

TEST(Config, ProxyManager1) {
  extend::config::Config config;
  const char* config_path = "crawler/crawl/tests/test.cfg";
  ASSERT_EQ(0, config.ParseFile(config_path))
      << "Failed to parse config file: "
      << config_path << " because: " <<  config.error_desc();
  ProxyManager::Options option;
  crawler2::crawl::ParseProxyManagerConfig(
      config["crawl"]["proxy1"], &option);

  const char* proxy_list[] = {"192.168.11.40", "192.168.11.41",
                              "192.168.11.42"};
  EXPECT_EQ(option.proxy.size(), ARRAYSIZE_UNSAFE(proxy_list));
  for (size_t i = 0; i < option.proxy.size(); ++i) {
    EXPECT_EQ(proxy_list[i], option.proxy[i]);
  }
  EXPECT_EQ(option.max_successive_failures, 15);
  EXPECT_EQ(option.holdon_duration_after_failures, 0);
}

TEST(Config, ProxyManager2) {
  extend::config::Config config;
  const char* config_path = "crawler/crawl/tests/test.cfg";
  ASSERT_EQ(0, config.ParseFile(config_path))
      << "Failed to parse config file: "
      << config_path << " because: " <<  config.error_desc();
  ProxyManager::Options option;
  crawler2::crawl::ParseProxyManagerConfig(
      config["crawl"]["proxy2"], &option);

  const char* proxy_list[] = {"192.168.11.40", "192.168.11.42"};
  EXPECT_EQ(option.proxy.size(), ARRAYSIZE_UNSAFE(proxy_list));
  for (size_t i = 0; i < option.proxy.size(); ++i) {
    EXPECT_EQ(proxy_list[i], option.proxy[i]);
  }

  EXPECT_EQ(option.max_successive_failures, 3);
  EXPECT_EQ(option.holdon_duration_after_failures, 10000000l);
}

TEST(Config, PageCrawler) {
  extend::config::Config config;
  const char* config_path = "crawler/crawl/tests/test.cfg";
  ASSERT_EQ(0, config.ParseFile(config_path))
      << "Failed to parse config file: "
      << config_path << " because: " <<  config.error_desc();
  PageCrawler::Options option;
  crawler2::crawl::ParsePageCrawlerConfig(
      config["crawl"]["page_crawler"], &option);

  EXPECT_EQ(option.connect_timeout_in_seconds, 10);
  EXPECT_EQ(option.transfer_timeout_in_seconds, 60);
  EXPECT_EQ(option.low_speed_limit_in_bytes_per_second, 500);
  EXPECT_EQ(option.low_speed_duration_in_seconds, 20);
  EXPECT_EQ(option.user_agent, "360Spider");
  EXPECT_EQ(option.user_agent_p, "Mozilla/5.0");
  EXPECT_EQ(option.max_redir_times, 32);
  EXPECT_EQ(option.enable_curl_debug, false);
}


TEST(Config, LoadController) {
  extend::config::Config config;
  const char* config_path = "crawler/crawl/tests/test.cfg";
  ASSERT_EQ(0, config.ParseFile(config_path))
      << "Failed to parse config file: "
      << config_path << " because: " <<  config.error_desc();
  LoadController::Options option;
  crawler2::crawl::ParseLoadControllerConfig(
      config["crawl"]["page_crawler"]["load_controller"],
      &option);
  EXPECT_EQ(option.default_max_qps, 3);
  EXPECT_EQ(option.default_max_connections, 6);
  EXPECT_EQ(option.max_connections_in_all, 1000);
  EXPECT_EQ(option.max_holdon_duration_after_failed, 10000);
  EXPECT_EQ(option.min_holdon_duration_after_failed, 5000);
  EXPECT_EQ(option.failed_times_threadhold, 10);
  EXPECT_EQ(option.max_failed_times, 100);
  EXPECT_EQ(option.check_frequency, 15);
}
