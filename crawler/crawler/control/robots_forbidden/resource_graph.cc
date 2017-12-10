#include <stdlib.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/common/gflags.h"
#include "base/strings/string_split.h"
#include "base/strings/string_number_conversions.h"
#include "base/hash_function/url.h"

#include "web/url_norm/url_norm.h"
#include "rpc/redis/redis_control/redis_control.h"
#include "crawler/proto2/proto_transcode.h"
#include "crawler/proto2/gen_hbase_key.h"
#include "crawler/proto2/resource.pb.h"
#include "feature/redis_dict/redis_dict_const.h"

DEFINE_string(output, "", "output file");
DEFINE_int32(anchor_TH, 10, "anchor 阈值, 至少一个 anchor 的 freq 超过 anchor_TH 时才会保留 resource 信息");

int main(int argc, char** argv) {
  base::InitApp(&argc, &argv, "查询 redis, 获取 resource graph 信息");

  redis::RedisController redis_controler;
  int success, total;  // 成功连接上的 redis server 数量 vs 所有 server 数量
  const int rw_timeout_in_ms = 10000;
  redis_controler.Init(rw_timeout_in_ms, &total, &success);
  CHECK_EQ(total, success) << "some redis server cannot be reached.";

  // url norm
  web::UrlNorm url_norm;

  // output
  FILE* fp = fopen(FLAGS_output.c_str(), "wb");
  CHECK(fp != NULL) << FLAGS_output;

  std::string line;
  std::vector<std::string> flds;
  std::map<uint64, std::map<std::string, std::string>> redis_results;

  int64 num = 0;
  // url level ...
  while (std::getline(std::cin, line)) {
    if (line.empty()) {
      continue;
    }
    flds.clear();
    base::SplitString(line, "\t", &flds);
    if (flds.size() < 2) {
      LOG(ERROR) << "line fields < 2, " << line;
      continue;
    }
    const std::string url = flds[0];
    int32 level;
    if (!base::StringToInt(flds[1], &level)) {
      LOG(ERROR) << "field 1 Not a int32, " << line;
      continue;
    }
    std::string click_url;
    std::string normed_source_url;
    if (!web::RawToClickUrl(url, NULL, &click_url)) {
      LOG(ERROR) << "Failed to RawToClick trans, url: " << url;
      continue;
    }
    if (!url_norm.NormalizeClickUrl(click_url, &normed_source_url)) {
      LOG(ERROR) << "Failed to Normalize click url: " << click_url;
      continue;
    }

    crawler2::Resource resource;
    // content
    resource.mutable_content()->set_utf8_content("<html></html>");

    // brief
    resource.mutable_brief()->set_original_encoding("UTF-8");
    resource.mutable_brief()->set_source_url(click_url);
    resource.mutable_brief()->set_effective_url(click_url);
    resource.mutable_brief()->set_url(normed_source_url);
    resource.mutable_brief()->set_resource_type(crawler2::kHTML);
    resource.mutable_brief()->mutable_crawl_reason()->set_robots_level(crawler2::kRobotsDisallow);
    resource.mutable_brief()->mutable_crawl_info()->set_response_code(200);  // 伪造爬虫抓取成功
    if (level == 1) {
      // both indexed by baidu and google
      resource.mutable_brief()->mutable_crawl_reason()->set_is_indexed_baidu(true);
      resource.mutable_brief()->mutable_crawl_reason()->set_is_indexed_google(true);
    } else if (level == 2) {
      // indexed by baidu
      resource.mutable_brief()->mutable_crawl_reason()->set_is_indexed_baidu(true);
    } else if (level == 3) {
      // indexed by google
      resource.mutable_brief()->mutable_crawl_reason()->set_is_indexed_google(true);
    } else if (level == 4) {
      // from uv, not indexed neither by baidu nor by google
    } else {
      LOG(ERROR) << "Invalid level: " << level;
      continue;
    }

    // graph
    uint64 url_sign = base::CalcUrlSign(normed_source_url.data(), normed_source_url.size());
    std::vector<uint64> url_signs;
    url_signs.push_back(url_sign);
    redis_results.clear();
    int64 max_anchor_freq = 0;
    if (!redis_controler.BatchInquire(url_signs, &redis_results) || redis_results.empty()) {
    } else {
      auto r_it = redis_results.find(url_sign);
      if (r_it == redis_results.end()) {
      } else {
        const std::map<std::string, std::string>& field_map = r_it->second;
        auto f_it = field_map.find(redis_util::kUVRankField);
        if (f_it != field_map.end()) {
          // uv rank
          crawler2::Graph graph;
          CHECK(graph.ParseFromString(f_it->second));
          CHECK(graph.has_uv_rank());
          VLOG(3) << "uv rank: " << graph.uv_rank().Utf8DebugString();
          resource.mutable_graph()->mutable_uv_rank()->Swap(graph.mutable_uv_rank());
        }
        f_it = field_map.find(redis_util::kShowRankField);
        if (f_it != field_map.end()) {
          // show rank
          crawler2::Graph graph;
          CHECK(graph.ParseFromString(f_it->second));
          CHECK(graph.has_show_rank());
          VLOG(3) << "show rank: " << graph.show_rank().Utf8DebugString();
          resource.mutable_graph()->mutable_show_rank()->Swap(graph.mutable_show_rank());
        }
        f_it = field_map.find(redis_util::kClickRankField);
        if (f_it != field_map.end()) {
          // click rank
          crawler2::Graph graph;
          CHECK(graph.ParseFromString(f_it->second));
          CHECK(graph.has_click_rank());
          VLOG(3) << "click rank: " << graph.click_rank().Utf8DebugString();
          resource.mutable_graph()->mutable_click_rank()->Swap(graph.mutable_click_rank());
        }
        f_it = field_map.find(redis_util::kUrlQueriesField);
        if (f_it != field_map.end()) {
          // query
          crawler2::Graph graph;
          CHECK(graph.ParseFromString(f_it->second));
          CHECK(graph.query_size() != 0u);  // NOLINT
          for (int query_idx = 0; query_idx < graph.query_size(); ++query_idx) {
            VLOG(3) << "query[" << query_idx << "]:"
                    << graph.query(query_idx).Utf8DebugString();
            graph.mutable_query(query_idx)->Swap(resource.mutable_graph()->add_query());
          }
        }
        f_it = field_map.find(redis_util::kUrlAnchorsField);
        if (f_it != field_map.end()) {
          // anchor
          crawler2::Graph graph;
          CHECK(graph.ParseFromString(f_it->second));
          CHECK(graph.anchor_size() != 0u);  // NOLINT
          for (int anchor_idx = 0; anchor_idx < graph.anchor_size(); ++anchor_idx) {
            VLOG(3) << "anchor[" << anchor_idx << "]:"
                    << graph.anchor(anchor_idx).Utf8DebugString();
            if (graph.anchor(anchor_idx).has_freq() &&
                graph.anchor(anchor_idx).freq() > max_anchor_freq) {
              max_anchor_freq = graph.anchor(anchor_idx).freq();
            }
            graph.mutable_anchor(anchor_idx)->Swap(resource.mutable_graph()->add_anchor());
          }
        }
      }  // end 填充 resource 的 graph
    }  // end 查询 redis

    // 没有 anchor 或者 阈值不达标的 resource 不输出
    if (!resource.has_graph() ||
        resource.graph().anchor_size() <= 0 ||
        max_anchor_freq < FLAGS_anchor_TH) {
      continue;
    }

    // 输出
    CHECK(resource.IsInitialized()) << line;
    CHECK(resource.has_brief()) << line;
    CHECK(resource.brief().has_url()) << line;

    std::string key, value;
    crawler2::GenNewHbaseKey(resource.brief().url(), &key);
    crawler2::ResourceToHbaseDump(resource, &value);
    CHECK_LT(key.size(), 2UL*1024*1024*1024) << line;
    CHECK_LT(value.size(), 2UL*1024*1024*1024) << line;
    VLOG(10) << resource.Utf8DebugString();

    uint32 key_len = key.size();
    PLOG_IF(FATAL, 4 != fwrite(&key_len, 1, sizeof(key_len), fp)) << "Write Key size(4 bytes) fail";
    PLOG_IF(FATAL, key_len != fwrite(key.data(), 1, key_len, fp)) << "Write Key fail";
    uint32 value_len = value.size();
    PLOG_IF(FATAL, 4 != fwrite(&value_len, 1, sizeof(value_len), fp)) << "Write Value size(4 bytes) fail";
    PLOG_IF(FATAL, value_len != fwrite(value.data(), 1, value_len, fp)) << "Write Value fail";

    if (++num % 10000 == 0) {
      LOG(INFO) << "proc num: " << num;
    }

  }  // end 从标准输入读取每一行

  fclose(fp);

  return 0;
}  // end main(...


