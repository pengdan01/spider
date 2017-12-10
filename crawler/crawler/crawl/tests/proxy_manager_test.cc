#include <vector>

#include "base/testing/gtest.h"
#include "base/thread/thread.h"
#include "base/common/scoped_ptr.h"
#include "web/html/api/parser.h"

#include "crawler/crawl/page_crawler.h"
#include "crawler/crawl/proxy_manager.h"
#include "crawler/crawl/tests/ut_webserver.h"

using crawler2::crawl::ProxyManager;

TEST(ProxyManager, SimpleOneProxy) {
  ProxyManager::Options options;
  options.max_successive_failures = 5;
  options.holdon_duration_after_failures = 1000;
  options.proxy.push_back("127.0.0.1:12333");
  ProxyManager manager(options);
  manager.Init();
  for (int i = 0; i < 5; ++i) {
    std::string proxy = manager.SelectBestProxy(0);
    ASSERT_TRUE(!proxy.empty());
    manager.ReportStatus(proxy, false, 2);
  }

  EXPECT_TRUE(manager.SelectBestProxy(2).empty());
  EXPECT_TRUE(manager.SelectBestProxy(1001).empty());

  // 如果超过 1000 之后，那么应该可以获得
  std::string proxy = manager.SelectBestProxy(1002);
  EXPECT_TRUE(!proxy.empty());
  manager.ReportStatus(proxy, false, 1);
  proxy = manager.SelectBestProxy(1003);
  EXPECT_TRUE(!proxy.empty());
  manager.ReportStatus(proxy, false, 1003);
  EXPECT_TRUE(manager.SelectBestProxy(2000).empty());
  EXPECT_TRUE(!manager.SelectBestProxy(2004).empty());
}
