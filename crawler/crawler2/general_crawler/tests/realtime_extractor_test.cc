#include <vector>

#include "base/testing/gtest.h"
#include "base/thread/thread.h"
#include "crawler/realtime_crawler/config_parse.h"

TEST(RealtimeExtractorText, StartStop) {
  using crawler2::crawl_queue::IndexRequestSubmitter;
  using crawler2::crawl_queue::RealtimeExtractor;
  // 启动队列
  const char* config_path = "crawler/realtime_crawler/tests/realtime_extractor.cfg";
  extend::config::Config config;
  ASSERT_EQ(0, config.ParseFile(config_path)) << config.error_desc();

  IndexRequestSubmitter::Options submitter_options;
  RealtimeExtractor::Options extractor_options;
  ParseIndexSubmitterConfig(config["realtime_extractor"]["index_submitter"], &submitter_options);
  ParseRealtimeExtractorConfig(config["realtime_extractor"]["extractor"], &extractor_options);

  IndexRequestSubmitter submitter(submitter_options);
  RealtimeExtractor extractor(extractor_options, &submitter);
  submitter.Start();
  extractor.Start();
  extractor.Loop();
}
