#include "crawler/api/base.h"

#include "base/testing/gtest.h"
#include "base/common/logging.h"

TEST(CrawlerUtil, RawToClickUrl) {
  std::string url("http://www.baidu.com/");
  std::string normalized_url;
  EXPECT_TRUE(web::RawToClickUrl(url, NULL, &normalized_url));

  std::string url2(" www.baidu.com");
  std::string url3("HTtP://www.baidu.com/");
  std::string url4("http://www.baidu.com/#top0");
  std::string url5("http://%20%1Fwww.baidu.com/#uidsl2l");
  std::string url6("http://wWw.baidU.com:80/");
  std::string normalized_url2;
  std::string normalized_url3;
  std::string normalized_url4;
  std::string normalized_url5;
  std::string normalized_url6;
  // Must has schema
  EXPECT_FALSE(web::RawToClickUrl(url2, NULL, &normalized_url2));
  EXPECT_TRUE(web::RawToClickUrl(url3, NULL, &normalized_url3));
  EXPECT_TRUE(web::RawToClickUrl(url4, NULL, &normalized_url4));
  EXPECT_FALSE(web::RawToClickUrl(url5, NULL, &normalized_url5));
  EXPECT_TRUE(web::RawToClickUrl(url6, NULL, &normalized_url6));

  EXPECT_EQ(normalized_url, normalized_url3);
  EXPECT_EQ(normalized_url, normalized_url4);
  EXPECT_EQ(normalized_url, normalized_url6);
  std::cout << "Normalized url: " << normalized_url << std::endl;

  std::string url7("http://wWw.hao123.com/~smith");
  std::string url8("http://wWw.hao123.com/%7Esmith");
  std::string url9("http://wWw.hao123.com/%7esmith");
  std::string normalized_url7;
  std::string normalized_url8;
  std::string normalized_url9;
  EXPECT_TRUE(web::RawToClickUrl(url7, NULL, &normalized_url7));
  EXPECT_TRUE(web::RawToClickUrl(url8, NULL, &normalized_url8));
  EXPECT_TRUE(web::RawToClickUrl(url9, NULL, &normalized_url9));
  EXPECT_EQ(normalized_url7, normalized_url8);
  EXPECT_EQ(normalized_url7, normalized_url9);
  LOG(INFO) << "Normalized url: " << normalized_url7;
  std::string url90("http://wWw.中文.com/smith");
  std::string normalized_url90;
  EXPECT_TRUE(web::RawToClickUrl(url90, NULL, &normalized_url90));
  LOG(INFO) << "Before normalize: " << url90 << ", after normalize: " << normalized_url90;

  // 忽略掉 '#' 以及后面的所有参数.
  // NOTE:
  // google 比较特殊, 存在大量类似如下的 url:
  // http://www.google.com.hk/#hl=zh-CN&newwindow=1&safe=strict&site=&q=flower&oq=flower&aq=f&aqi=&aql=
  // 因此, 对此类 url 不进行该处理
  std::string google("http://www.google.com.hk/#hl=zh-CN&newwindow=1&safe=strict&site=&q=flower&oq="
                     "flower&aq=f&aqi=&aql=&gs_sm=3&gs_upl=6240l6240l0l6583l1l1l0l0l0l0l0l0ll0l0&fp"
                     "=6995e23aa793edfd");
  std::string url10("http://www.google.com.hk/#hl=zh-CN&newwindow=1&safe=strict&site=&q=flower&oq="
                    "flower&aq=f&aqi"
                    "=&aql=&gs_sm=3&gs_upl=6240l6240l0l6583l1l1l0l0l0l0l0l0ll0l0&fp=6995e23aa793edfd");
  EXPECT_TRUE(web::RawToClickUrl(google, NULL, &google));
  EXPECT_TRUE(web::RawToClickUrl(url10, NULL, &url10));
  EXPECT_EQ(url10, google);

  std::string url11("http://www.baidu.com/data/");
  std::string url12("http://www.baidu.com/data");
  EXPECT_TRUE(web::RawToClickUrl(url11, NULL, &url11));
  EXPECT_TRUE(web::RawToClickUrl(url12, NULL, &url12));
  LOG(INFO) << "Normalized: " << url11;
  LOG(INFO) << "Normalized: " << url12;
  std::string url13("http://www.bellydancesupply.netyyp/情人节/?鲜花情人节");
  EXPECT_TRUE(web::RawToClickUrl(url13, NULL, &url13));
  LOG(INFO) << "Normalized: " << url13;
}

TEST(CrawlerUtil, AssignShardId) {
  const int32 page_shard_num = 128;

  std::string url1("www.baidu.com");
  EXPECT_FALSE(web::has_scheme(url1));
  url1 = "http://" + url1;
  std::string normalized_url1;
  EXPECT_TRUE(web::RawToClickUrl(url1, NULL, &normalized_url1));
  std::string url2("http://www.baidu.com/");
  std::string normalized_url2;
  EXPECT_TRUE(web::RawToClickUrl(url2, NULL, &normalized_url2));
  int32 shard_id;
  int32 shard_id2;
  // 已经做过 归一化, |switch_normalize_url| 设置为 false 跳过归一化
  EXPECT_TRUE(crawler::AssignShardId(normalized_url1, page_shard_num, &shard_id, false));
  EXPECT_TRUE(crawler::AssignShardId(normalized_url2, page_shard_num, &shard_id2, false));
  EXPECT_EQ(shard_id, shard_id2);

  // 已经做过 归一化, 但是 |switch_normalize_url| 设置为 true, 程序内部再进行一次归一化
  EXPECT_TRUE(crawler::AssignShardId(normalized_url1, page_shard_num, &shard_id, true));
  EXPECT_TRUE(crawler::AssignShardId(normalized_url2, page_shard_num, &shard_id2, true));
  EXPECT_EQ(shard_id, shard_id2);

  // url 没有归一化处理, 但是 |switch_normalize_url| 设置为 false, 程序内部也不会进行归一化
  EXPECT_TRUE(crawler::AssignShardId(url1, page_shard_num, &shard_id, false));
  EXPECT_TRUE(crawler::AssignShardId(url2, page_shard_num, &shard_id2, false));
  EXPECT_NE(shard_id, shard_id2);

  // url 没有归一化处理, 但是 |switch_normalize_url| 设置为 true, 程序内部会进行归一化
  EXPECT_TRUE(crawler::AssignShardId(url1, page_shard_num, &shard_id, true));
  EXPECT_TRUE(crawler::AssignShardId(url2, page_shard_num, &shard_id2, true));
  EXPECT_EQ(shard_id, shard_id2);
  // FOR TMP DEBUG
  int reduce_id;
  EXPECT_TRUE(crawler::AssignShardId("http://www.baidu.com/", 128, &shard_id, true));
  EXPECT_TRUE(crawler::AssignReduceId("http://www.sina.com.cn/", 2000, &reduce_id, true));
  LOG(ERROR) << "sina shard id: " << shard_id << ", reduce id: " << reduce_id;
  EXPECT_TRUE(crawler::AssignShardId("http://v.360.cn/", 32, &shard_id, true));
  EXPECT_TRUE(crawler::AssignReduceId("http://v.360.cn/", 2000, &reduce_id, true));  // NOLINT
  LOG(ERROR) << "reduce shard id: " << shard_id << ", reduce id: " << reduce_id;
}

TEST(CrawlerUtil, AssignReduceId) {
  const int32 total_reduce_num = 1000;

  std::string url1("http://www.baidu.com");
  std::string normalized_url1;
  EXPECT_TRUE(web::RawToClickUrl(url1, NULL, &normalized_url1));
  std::string url2("http://www.baidu.com/");
  std::string normalized_url2;
  EXPECT_TRUE(web::RawToClickUrl(url2, NULL, &normalized_url2));
  int32 reduce_id;
  int32 reduce_id2;
  // 已经做过 归一化, |switch_normalize_url| 设置为 false 跳过归一化
  EXPECT_TRUE(crawler::AssignReduceId(normalized_url1, total_reduce_num, &reduce_id, false));
  EXPECT_TRUE(crawler::AssignReduceId(normalized_url2, total_reduce_num, &reduce_id2, false));
  EXPECT_EQ(reduce_id, reduce_id2);

  // 已经做过 归一化, 但是 |switch_normalize_url| 设置为 true, 程序内部再进行一次归一化
  EXPECT_TRUE(crawler::AssignReduceId(normalized_url1, total_reduce_num, &reduce_id, true));
  EXPECT_TRUE(crawler::AssignReduceId(normalized_url2, total_reduce_num, &reduce_id2, true));
  EXPECT_EQ(reduce_id, reduce_id2);

  // url 没有归一化处理, 但是 |switch_normalize_url| 设置为 false, 程序内部也不会进行归一化
  EXPECT_TRUE(crawler::AssignReduceId(url1, total_reduce_num, &reduce_id, false));
  EXPECT_TRUE(crawler::AssignReduceId(url2, total_reduce_num, &reduce_id2, false));
  EXPECT_NE(reduce_id, reduce_id2);

  // url 没有归一化处理, 但是 |switch_normalize_url| 设置为 true, 程序内部会进行归一化
  EXPECT_TRUE(crawler::AssignReduceId(url1, total_reduce_num, &reduce_id, true));
  EXPECT_TRUE(crawler::AssignReduceId(url2, total_reduce_num, &reduce_id2, true));
  EXPECT_EQ(reduce_id, reduce_id2);
}
/*
TEST(CrawlerUtil, ReverseUrl) {
  std::string url0("");
  std::string result0;
  // url1 gurl 失败
  EXPECT_FALSE(crawler::ReverseUrl(url0, &result0, false));
  EXPECT_FALSE(crawler::ReverseUrl(url0, &result0, true));

  std::string url1("www.baidu.com");
  std::string result1;
  // url1 gurl 失败, 因为缺少 scheme
  EXPECT_FALSE(crawler::ReverseUrl(url1, &result1, false));

  std::string url2("http://www.baidu.com/");
  std::string result2;
  std::string result2_1;
  EXPECT_TRUE(crawler::ReverseUrl(url2, &result2, true));
  EXPECT_EQ(result2, "http://com.baidu.www/");
  EXPECT_TRUE(crawler::ReverseUrl(result2, &result2_1, true));
  // After revert twice, it should equle to original
  EXPECT_EQ(url2, result2_1);

  std::string url3("http://user:pass@google.com:99/foo;bar?q=a#ref");
  std::string result3;
  std::string result3_1;
  EXPECT_TRUE(crawler::ReverseUrl(url3, &result3, true));
  EXPECT_EQ(result3, "http://user:pass@com.google:99/foo;bar?q=a");
  EXPECT_TRUE(crawler::ReverseUrl(result3, &result3_1, true));
  // After revert twice, it should equle to original
  EXPECT_EQ(result3_1, "http://user:pass@google.com:99/foo;bar?q=a");

  // 对于 host 为 IP 的 URL, 不进行反转
  std::string url5("http://180.86.52.140/difang.php?id=3145&eid=368&moid=8");
  std::string result5;
  EXPECT_TRUE(crawler::ReverseUrl(url5, &result5, false));
  EXPECT_EQ(url5, result5);
  EXPECT_TRUE(crawler::ReverseUrl(url5, &result5, true));
  EXPECT_EQ(url5, result5);

  std::string url6("http://180.86.52.com/difang.php?id=3145&eid=368&moid=8");
  std::string result6;
  EXPECT_TRUE(crawler::ReverseUrl(url6, &result6, true));
  EXPECT_EQ(result6, "http://com.52.86.180/difang.php?id=3145&eid=368&moid=8");
}

TEST(CrawlerUtil, ParseHost) {
  std::string domain0;
  EXPECT_FALSE(crawler::ParseHost("     ", NULL, &domain0, NULL));

  std::string host1("newS.sIna.com.cN");
  std::string tld1;
  std::string domain1;
  std::string subdomain1;
  EXPECT_TRUE(crawler::ParseHost(host1, &tld1, &domain1, &subdomain1));
  EXPECT_EQ(tld1, "com.cn");
  EXPECT_EQ(domain1, "sina.com.cn");
  EXPECT_EQ(subdomain1, "news");

  std::string host2("sina.com.cn");
  std::string tld2;
  std::string domain2;
  std::string subdomain2;
  EXPECT_TRUE(crawler::ParseHost(host2, &tld2, &domain2, &subdomain2));
  EXPECT_EQ(tld2, "com.cn");
  EXPECT_EQ(domain2, "sina.com.cn");
  EXPECT_EQ(subdomain2, "");

  std::string host3("www.sina.com.cn");
  std::string tld3;
  std::string domain3;
  std::string subdomain3;
  EXPECT_TRUE(crawler::ParseHost(host3, &tld3, &domain3, &subdomain3));
  EXPECT_EQ(tld3, "com.cn");
  EXPECT_EQ(domain3, "sina.com.cn");
  EXPECT_EQ(subdomain3, "www");

  std::string host4("www.sina.com");
  std::string tld4;
  std::string domain4;
  std::string subdomain4;
  EXPECT_TRUE(crawler::ParseHost(host4, &tld4, &domain4, &subdomain4));
  EXPECT_EQ(tld4, "com");
  EXPECT_EQ(domain4, "sina.com");
  EXPECT_EQ(subdomain4, "www");

  std::string host4_1("sina");
  std::string domain4_1;
  EXPECT_FALSE(crawler::ParseHost(host4_1, NULL, &domain4_1, NULL));

  std::string host5("192.168.11.55");
  std::string domain5;
  EXPECT_FALSE(crawler::ParseHost(host5, NULL, &domain5, NULL));

  std::string host6("192.168.11.55:8080");
  std::string domain6;
  EXPECT_FALSE(crawler::ParseHost(host6, NULL, &domain6, NULL));

  std::string host7("dict.cn..cn.com.cn");
  std::string domain7;
  EXPECT_FALSE(crawler::ParseHost(host7, NULL, &domain7, NULL));

  std::string host8(".dict.cn");
  std::string domain8;
  EXPECT_FALSE(crawler::ParseHost(host8, NULL, &domain8, NULL));

  std::string host9("dict.cn.");
  std::string domain9;
  EXPECT_FALSE(crawler::ParseHost(host9, NULL, &domain9, NULL));

  std::string host10("www.sohu.comm");
  std::string domain10;
  EXPECT_FALSE(crawler::ParseHost(host10, NULL, &domain10, NULL));
  // TODO(pengdan): support 中文 host
  std::string host11("http://www.网絡.hk");
  LOG(INFO) << host11;
  std::string domain11, click_url11;
  EXPECT_TRUE(web::RawToClickUrl(host11, NULL, &click_url11));
  LOG(INFO) << GURL(click_url11).host();
  // EXPECT_TRUE(crawler::ParseHost(GURL(click_url11).host(), NULL, &domain11, NULL));
  // EXPECT_EQ(domain11, "网絡.hk");
}
*/

