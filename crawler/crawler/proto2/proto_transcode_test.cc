#include "crawler/proto2/proto_transcode.h"

#include <arpa/inet.h>
#include <string>
#include <vector>

#include "crawler/proto/crawled_resource.pb.h"
#include "crawler/proto2/resource.pb.h"
#include "base/testing/gtest.h"
#include "base/file/file_util.h"
#include "base/strings/string_split.h"
#include "web/url_norm/url_norm.h"

class CrawlerProtoTransferTest : public ::testing::Test {
 public:
  void SetUp() {
    Clear();
    SetupUrl("http://www.m100.cc/Shop/ShowTrademark.asp?"
             "ChannelID=1000&TrademarkName=%BD%F0%C5%F4",
             &crawl_resource_);
  }
  void SetupUrl(const std::string& url, crawler::CrawledResource* cr) {
    std::string click_url, norm_url;
    CHECK(web::RawToClickUrl(url, "UTF-8",  &click_url));
    CHECK(url_normalizer_.NormalizeClickUrl(click_url, &norm_url));
    cr->set_url(norm_url);
    cr->set_source_url(url);
    cr->set_effective_url(url);
  }

  void Clear() {
    crawl_resource1.Clear();
    crawl_resource2.Clear();
    hbase_resource1.Clear();
    hbase_resource2.Clear();
    pb_crawl_resource1.clear();
    pb_crawl_resource2.clear();
    pb_hbase_resource1.clear();
    pb_hbase_resource2.clear();
  }

  void TransferResource() {
    crawler2::CrawledResourceToResource(crawl_resource1, &hbase_resource1);
    crawler2::ResourceToCrawledResource(hbase_resource1, &crawl_resource2);
    crawler2::CrawledResourceToResource(crawl_resource2, &hbase_resource2);
    // NOTE(duyanlong): 下面的字段在新的 crawler resource 中没有,
    //                  所以 手动添加 方便 测试
    if (crawl_resource1.has_simhash()) {
      crawl_resource2.set_simhash(crawl_resource1.simhash());
    }
    if (crawl_resource1.crawler_info().has_namelookup_time()) {
      crawl_resource2.mutable_crawler_info()->set_namelookup_time(
          crawl_resource1.crawler_info().namelookup_time());
    }
    // ===========================
    CHECK(crawl_resource1.SerializeToString(&pb_crawl_resource1));
    CHECK(crawl_resource2.SerializeToString(&pb_crawl_resource2));
    CHECK(hbase_resource1.SerializeToString(&pb_hbase_resource1));
    CHECK(hbase_resource2.SerializeToString(&pb_hbase_resource2));
  }

 public:
  crawler::CrawledResource crawl_resource1;
  crawler::CrawledResource crawl_resource2;
  crawler2::Resource hbase_resource1;
  crawler2::Resource hbase_resource2;
  std::string pb_crawl_resource1;
  std::string pb_crawl_resource2;
  std::string pb_hbase_resource1;
  std::string pb_hbase_resource2;
 protected:
  crawler::CrawledResource crawl_resource_;
 private:
  web::UrlNorm url_normalizer_;
};

TEST_F(CrawlerProtoTransferTest, UrlNorm) {
  EXPECT_DEATH(SetupUrl("asfdasfaf", &crawl_resource1), "web::RawToClickUrl");
}

TEST_F(CrawlerProtoTransferTest, TransferAndRevertCompare) {
  // 1. Init
  SetupUrl(crawl_resource_.source_url(), &crawl_resource1);
  crawl_resource1.set_resource_type(crawler::ResourceType::kHTML);
  crawl_resource1.mutable_crawler_info()->set_link_score(0.1);
  crawl_resource1.mutable_crawler_info()->set_response_code(100);
  crawl_resource1.mutable_crawler_info()->set_file_time(32423424);
  crawl_resource1.mutable_crawler_info()->set_crawler_timestamp(234243);
  crawl_resource1.mutable_crawler_info()->set_crawler_duration(9.234);
  crawl_resource1.mutable_crawler_info()->set_namelookup_time(92.238);
  crawl_resource1.mutable_crawler_info()->set_connect_time(234.1321545);
  crawl_resource1.mutable_crawler_info()->set_appconnect_time(32.123);
  crawl_resource1.mutable_crawler_info()->set_pretransfer_time(2342.4553);
  crawl_resource1.mutable_crawler_info()->set_starttransfer_time(23.96);
  crawl_resource1.mutable_crawler_info()->set_redirect_time(2342.8);
  crawl_resource1.mutable_crawler_info()->set_redirect_count(3);
  crawl_resource1.mutable_crawler_info()->set_donwload_speed(2342.2);
  crawl_resource1.mutable_crawler_info()->set_content_length_donwload(997.12);
  crawl_resource1.mutable_crawler_info()->set_header_size(3240);
  crawl_resource1.mutable_crawler_info()->set_content_type("what");
  crawl_resource1.mutable_crawler_info()->set_update_fail_cnt(324);
  crawl_resource1.mutable_crawler_info()->set_from("U");
  crawl_resource1.set_http_header_raw("adfafas");
  crawl_resource1.set_simhash(23472394293874lu);
  crawl_resource1.set_content_raw("adfafas");
  crawl_resource1.set_content_utf8("safasdfdfalkj;ajfgewitpoucj");
  crawl_resource1.set_original_encoding("ascii");
  crawl_resource1.set_is_truncated(true);
  crawl_resource1.add_css_urls("asfadfasfa");
  crawl_resource1.add_css_urls("asfadfssdsfafdsasfa");
  crawl_resource1.add_image_urls("ioojomumom");
  crawl_resource1.add_image_urls("iuoiumnjk");
  // 2. Transfer
  TransferResource();
  // 3. CHECK equal
  EXPECT_EQ(pb_crawl_resource1, pb_crawl_resource2)
      << crawl_resource1.Utf8DebugString()
      << "\n================\n"
      << crawl_resource2.Utf8DebugString();
  EXPECT_EQ(pb_hbase_resource1, pb_hbase_resource2)
      << hbase_resource1.Utf8DebugString()
      << "\n================\n"
      << hbase_resource2.Utf8DebugString();
}

TEST_F(CrawlerProtoTransferTest, UrlEmpty) {
  Clear();
  EXPECT_DEATH(crawler2::CrawledResourceToResource(crawl_resource1, &hbase_resource1), "IsInitialized");
  // XXX(duyanlong): 都是 optional 成员, 所以 IsInitialize 为 true
  // EXPECT_DEATH(ResourceToCrawledResource(hbase_resource2, &crawl_resource2), "");
}

TEST_F(CrawlerProtoTransferTest, UrlCompare) {
  // 1. Init
  SetupUrl(crawl_resource_.url(), &crawl_resource1);
  crawl_resource1.set_resource_type(crawler::ResourceType::kHTML);
  // 2. Transfer
  TransferResource();
  // 3. Check
  EXPECT_EQ(pb_crawl_resource1, pb_crawl_resource2);
  EXPECT_EQ(pb_hbase_resource1, pb_hbase_resource2);

  EXPECT_EQ(crawl_resource1.has_url(), crawl_resource2.has_url());
  EXPECT_EQ(hbase_resource1.brief().has_url(), hbase_resource2.brief().has_url());
  EXPECT_EQ(hbase_resource1.brief().has_url(), crawl_resource2.has_url());

  EXPECT_EQ(crawl_resource1.has_source_url(), crawl_resource2.has_source_url());
  EXPECT_EQ(hbase_resource1.brief().has_source_url(), hbase_resource2.brief().has_source_url());
  EXPECT_EQ(hbase_resource1.brief().has_source_url(), crawl_resource2.has_source_url());

  EXPECT_EQ(crawl_resource1.has_effective_url(), crawl_resource2.has_effective_url());
  EXPECT_EQ(hbase_resource1.brief().has_effective_url(), hbase_resource2.brief().has_effective_url());
  EXPECT_EQ(hbase_resource1.brief().has_effective_url(), crawl_resource2.has_effective_url());

  EXPECT_EQ(crawl_resource1.url(), crawl_resource2.url());
  EXPECT_EQ(hbase_resource1.brief().url(), hbase_resource2.brief().url());
  EXPECT_EQ(hbase_resource1.brief().url(), crawl_resource2.url());

  EXPECT_EQ(crawl_resource1.source_url(), crawl_resource2.source_url());
  EXPECT_EQ(hbase_resource1.brief().source_url(), hbase_resource2.brief().source_url());
  EXPECT_EQ(hbase_resource1.brief().source_url(), crawl_resource2.source_url());

  EXPECT_EQ(crawl_resource1.effective_url(), crawl_resource2.effective_url());
  EXPECT_EQ(hbase_resource1.brief().effective_url(), hbase_resource2.brief().effective_url());
  EXPECT_EQ(hbase_resource1.brief().effective_url(), crawl_resource2.effective_url());
}

TEST(HbaseDumpToResource, ShorterString) {
  crawler2::Resource resource;
  std::string test_string(19u, '\0');
  EXPECT_FALSE(crawler2::HbaseDumpToResource(test_string.c_str(), test_string.length(), &resource));
}

TEST(HbaseDumpToResource, EmptyResource) {
  std::string test_string(20u, '\0');
  crawler2::Resource resource;
  EXPECT_TRUE(crawler2::HbaseDumpToResource(test_string.c_str(), test_string.length(), &resource));
}

TEST(HbaseDumpToResource, Common) {
  crawler2::Resource resource;
  resource.mutable_brief()->set_url("dsfa");
  resource.mutable_brief()->set_effective_url("dsfa");
  resource.mutable_brief()->set_source_url("dsfa");
  resource.mutable_brief()->set_resource_type(crawler2::kAudio);
  resource.mutable_content()->set_raw_content("dsaffd");
  resource.mutable_parsed_data()->add_css_urls("slfdakadsfahf");
  resource.mutable_parsed_data()->add_css_urls("sadsfahf");

  std::string hbase_str;
  crawler2::ResourceToHbaseDump(resource, &hbase_str);
  crawler2::Resource resource2;
  EXPECT_TRUE(crawler2::HbaseDumpToResource(hbase_str.c_str(), hbase_str.length(), &resource2));

  std::string str;
  crawler2::ResourceToHbaseDump(resource2, &str);
  CHECK_EQ(str, hbase_str);

  std::string pb1, pb2;
  ASSERT_TRUE(resource.SerializeToString(&pb1));
  ASSERT_TRUE(resource2.SerializeToString(&pb2));
  EXPECT_EQ(pb1, pb2);
}

TEST(HbaseDumpToResource, SizeError) {
  crawler2::Resource resource;
  resource.mutable_brief()->set_url("dsfa");
  resource.mutable_brief()->set_effective_url("dsfa");
  resource.mutable_brief()->set_source_url("dsfa");
  resource.mutable_brief()->set_resource_type(crawler2::kAudio);
  resource.mutable_content()->set_raw_content("dsaffd");
  resource.mutable_parsed_data()->add_css_urls("slfdakadsfahf");
  resource.mutable_parsed_data()->add_css_urls("sadsfahf");

  int bytes[5];
  std::string resource_str[5];
  ASSERT_TRUE(resource.brief().SerializeToString(&resource_str[0]));
  ASSERT_TRUE(resource.content().SerializeToString(&resource_str[1]));
  ASSERT_TRUE(resource.parsed_data().SerializeToString(&resource_str[2]));
  ASSERT_TRUE(resource.graph().SerializeToString(&resource_str[3]));
  ASSERT_TRUE(resource.alt().SerializeToString(&resource_str[4]));

  for (int case_idx = 0; case_idx < 5; ++case_idx) {
    --bytes[case_idx];
    std::string hbase_str;
    for (int i = 0; i < 5; ++i) {
      hbase_str.append(reinterpret_cast<char*>(&bytes[i]), 4u);
      hbase_str.append(resource_str[i]);
    }
    crawler2::Resource resource2;
    EXPECT_FALSE(crawler2::HbaseDumpToResource(hbase_str.c_str(), hbase_str.length(), &resource2));
    ++bytes[case_idx];
  }

  for (int case_idx = 0; case_idx < 5; ++case_idx) {
    bytes[case_idx]++;
    std::string hbase_str;
    for (int i = 0; i < 5; ++i) {
      hbase_str.append(reinterpret_cast<char*>(&bytes[i]), 4u);
      hbase_str.append(resource_str[i]);
    }
    crawler2::Resource resource2;
    EXPECT_FALSE(crawler2::HbaseDumpToResource(hbase_str.c_str(), hbase_str.length(), &resource2));
    bytes[case_idx]--;
  }
  std::string hbase_str;
  for (int i = 0; i < 5; ++i) {
    hbase_str.append(reinterpret_cast<char*>(&bytes[i]), 4u);
    hbase_str.append(resource_str[i]);
  }
  hbase_str.append("add");
  crawler2::Resource resource2;
  EXPECT_FALSE(crawler2::HbaseDumpToResource(hbase_str.c_str(), hbase_str.length(), &resource2));
}

TEST(HbaseDumpToResource, PbError) {
  crawler2::Resource resource;
  resource.mutable_brief()->set_url("dsfa");
  resource.mutable_brief()->set_effective_url("dsfa");
  resource.mutable_brief()->set_source_url("dsfa");
  resource.mutable_brief()->set_resource_type(crawler2::kAudio);
  resource.mutable_content()->set_raw_content("dsaffd");
  resource.mutable_parsed_data()->add_css_urls("slfdakadsfahf");
  resource.mutable_parsed_data()->add_css_urls("sadsfahf");

  int bytes[5];
  std::string resource_str[5];
  ASSERT_TRUE(resource.brief().SerializeToString(&resource_str[0]));
  ASSERT_TRUE(resource.content().SerializeToString(&resource_str[1]));
  ASSERT_TRUE(resource.parsed_data().SerializeToString(&resource_str[2]));
  ASSERT_TRUE(resource.graph().SerializeToString(&resource_str[3]));
  ASSERT_TRUE(resource.alt().SerializeToString(&resource_str[4]));

  {
    std::string hbase_str;
    for (int i = 0; i < 5; ++i) {
      hbase_str.append(reinterpret_cast<char*>(&bytes[(i + 1) % 5]), 4u);
      hbase_str.append(resource_str[(i + 1) % 5]);
    }
    crawler2::Resource resource2;
    EXPECT_FALSE(crawler2::HbaseDumpToResource(hbase_str.c_str(), hbase_str.length(), &resource2));
  }
  {
    std::string hbase_str;
    for (int i = 0; i < 5; ++i) {
      resource_str[i] = "1234567890";
      bytes[i] = (int)resource_str[i].size();
      hbase_str.append(reinterpret_cast<char*>(&bytes[i]), 4u);
      hbase_str.append(resource_str[i]);
    }
    crawler2::Resource resource2;
    EXPECT_FALSE(crawler2::HbaseDumpToResource(hbase_str.c_str(), hbase_str.length(), &resource2));
  }
}

//  老数据 兼容 测试
TEST(HbaseDumpToResource, Compatable) {
  std::string hbase;
  CHECK(base::file_util::ReadFileToString("crawler/proto2/tests/hbase_oldformat.dat", &hbase));

  // 老版本 第一个 int 是 brief 的 size
  const int* p = reinterpret_cast<const int*>(hbase.data());
  ASSERT_GT(*p, 0) << *p;

  // 新版本 第一个 int 是版本号, 一定小于 0
  crawler2::Resource resource;
  ASSERT_TRUE(crawler2::HbaseDumpToResource(hbase.data(), hbase.size(), &resource));
  std::string hbase_dump;
  crawler2::ResourceToHbaseDump(resource, &hbase_dump);
  ASSERT_FALSE(hbase_dump.empty());
  if (hbase_dump == hbase) {
    LOG(ERROR) << "new hbase dump string same as old one??";
  } else {
    const int* p = reinterpret_cast<const int*>(hbase_dump.data());
    ASSERT_EQ(*p, -3) << *p;
    ++p;
    int hint_size = *reinterpret_cast<const int *>(p);
    ++p;
    std::string hint_string(reinterpret_cast<const char*>(p), hint_size);
    LOG(ERROR) << "Hint_string is [" << hint_string << "]";
    std::vector<std::string> fields;
    base::SplitString(hint_string, "\t", &fields);
    ASSERT_EQ(fields.size(), 8u);
  }

  crawler2::Resource resource2;
  ASSERT_TRUE(crawler2::HbaseDumpToResource(hbase_dump.data(), hbase_dump.size(), &resource2));
  // 两个版本的 dump 的 resourc 相同
  ASSERT_TRUE(resource.SerializeToString(&hbase));
  ASSERT_TRUE(resource2.SerializeToString(&hbase_dump));
  EXPECT_EQ(hbase, hbase_dump);
}
// TODO(duyanlong): more ut
