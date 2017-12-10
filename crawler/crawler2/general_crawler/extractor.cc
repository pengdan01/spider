#include "crawler2/general_crawler/extractor.h"

#include <map>
#include <set>

#include "base/common/logging.h"
#include "base/common/scoped_ptr.h"
#include "base/encoding/line_escape.h"
#include "base/encoding/base64.h"
#include "base/strings/string_printf.h"
#include "base/time/timestamp.h"
#include "base/hash_function/url.h"
#include "rpc/redis/redis_control/redis_control.h"
#include "rpc/job_queue/job_queue.h"
#include "rpc/http_counter_export/export.h"
#include "web/url_category/url_categorizer.h"
#include "web/url/url.h"
#include "third_party/jsoncpp/include/json/json.h"
#include "extend/regexp/re3/re3.h"

#include "crawler/realtime_crawler/crawl_queue.pb.h"
#include "crawler2/general_crawler/util/url_util.h"
#include "web/html/api/html_parser.h"
#include "web/html/api/html_node.h"
#include "feature/web/page_types/hub_recog.h"

namespace crawler3 {

DEFINE_int64_counter(extractor, res_fetched, 0, "the number of res fetched from job queue");
DEFINE_int64_counter(extractor, res_without_content, 0, "不包含内容的resource 数量");
DEFINE_int64_counter(extractor, res_queue_len, 0, "本地队列的长度");
DEFINE_int64_counter(extractor, new_url_queue_len, 0, "the queue length of new url");
DEFINE_int64_counter(extractor, new_url_submit, 0, "the number of submitted new url");

DEFINE_bool(is_submit_newlink, true, "是否提交从本页面提取出的新链接给 url 处理队列");
DEFINE_int32(max_url_depth, 3, "限制 url 的最大深度, 对深度超过 |max_url_depth| 的 新 url, 会忽略之");
DEFINE_string(url_extract_pattern_file, "", "only urls matching the pattern in file will be extracted, "
              "file record format: 1. host pattern; 2. path pattern; 3. target url pattern. For Field 1 & 2"
              ", base::MatchPattern() is used, for  Field 3, extend::Re3::re3::FullMatch() is used.");

DEFINE_string(recg_data_path, "data/recg",
              "存放图片识别时需要的训练测试数据, 其下面包含了两个目录: training and testing");

DEFINE_bool(res_save_in_local, true, "是否本地存放下载的网页");
DEFINE_bool(submit_to_output_queue, false, "是否将网页下发给下游队列");
DEFINE_bool(build_next_n_url, true, "是否构造翻页url, 默认构造淘宝, 京东, 苏宁等 hub list 翻页 url");
DEFINE_int32(build_max_n_url, 10, "最多构造 |build_max_n_url| url");
DEFINE_bool(filter_using_url_categorizer, false ,
            "是否使用 url categorizer 对 effective url 进行过滤, 默认 false, 不过滤");
DEFINE_bool(filter_using_bad_titles, true,
            "是否使用 bad titles set 对 网页 title 进行过滤, 默认 true, 过滤");
// 将网页 res 保存成文本格式还是二进制格式
DEFINE_bool(res_save_in_bin_format, true, "网页 res 保存成二进制还是文本格式. 默认保存成 binary 格式, "
            "本地脚本负责将其转换成 sfile 并上传到 hadoop; 如果保存为文本格式: 则本地脚本直接将其上传 hadoop"
            "即可(st), 不用转 sfile. 文本格式: "
            "key: Base64Encode(res.url()), value: Base64Encode(res.SerializeToString())");


GeneralExtractor::GeneralExtractor(const Options& options, redis::RedisController& redis_controler)
    : options_(options)
      , stopped_(false)
      // res 文件数据量大, 每 300 秒 写一次; 使用 binary 写入
      , resource_saver_(options.resource_prefix, options.res_saver_timespan, FLAGS_res_save_in_bin_format)
      , title_saver_(options.title_prefix, options.saver_timespan)
      , taobao_saver_("output/taobao_item", options.res_saver_timespan, true)
      , total_no_res_(0)
      , redis_controler_(redis_controler) {
        CHECK_GT(options.saver_timespan, 0);
        CHECK(!options.resource_prefix.empty());
        CHECK(!options.title_prefix.empty());
        CHECK(!options.job_queue_ip.empty());
        CHECK_NE(options.job_queue_port, 0);
        if (FLAGS_is_submit_newlink) {
          CHECK(!options.url_queue_ip.empty());
          CHECK_GT(options.url_queue_port, 0);
        }
        if (FLAGS_submit_to_output_queue) {
          CHECK(!options.res_output_queue_ip.empty());
          CHECK_GT(options.res_output_queue_port, 0);
        }
        CHECK(!options.error_titles.empty());
        // 加载 bad title 到 集合 bad_titles_
        std::ifstream myfin(options.error_titles);
        CHECK(myfin.good());
        std::string line;
        while (getline(myfin, line)) {
          if (line.empty()) {
            continue;
          }
          bad_titles_.insert(line);
        }
        CHECK(myfin.eof());
      }

GeneralExtractor::~GeneralExtractor() {
  // Stop();
}

bool GeneralExtractor::AddResourceExtraInfo(crawler2::Resource *res, std::string *desc) {
  CHECK_NOTNULL(desc);

  // uv rank, query, anchor
  if (!res->has_brief()) {
    *desc = "res has no brief";
    return false;
  }
  if (!res->brief().has_source_url()) {
    *desc = "res has no source url";
    return false;
  }
  if (!res->brief().has_crawl_info()) {
    *desc = "res has no crawl info";
    return false;
  }
  if (!res->brief().crawl_info().has_response_code()) {
    *desc = "res has no response_code";
    return false;
  }
  if (res->brief().crawl_info().response_code() != 200) {
    *desc = "response_code != 200, is:" + base::StringPrintf("%ld",
                                                             res->brief().crawl_info().response_code());
    return false;
  }

  std::string click_url, normed_source_url;
  if (!::web::RawToClickUrl(res->brief().source_url(), NULL, &click_url)
      || !url_norm_.NormalizeClickUrl(click_url, &normed_source_url)) {
    *desc = "Failed to RawToClick or NormClickUrl, url: " + res->brief().source_url();
    return false;
  }

  uint64 url_sign = base::CalcUrlSign(normed_source_url.data(), normed_source_url.size());
  std::vector<uint64> url_signs;
  url_signs.push_back(url_sign);
  std::map<uint64, std::map<std::string, std::string>> redis_results;
  // BatchInquire() 返回 true, 但是不一定找到元素
  if (!redis_controler_.BatchInquire(url_signs, &redis_results)) {
    *desc = "url not found in redis: " + res->brief().source_url() + ", sign: "
        + base::StringPrintf("%lu", url_sign);
    return false;
  }

  auto r_it = redis_results.find(url_sign);
  if (r_it == redis_results.end()) {
    *desc = "url not found in redis: " + res->brief().source_url() + base::StringPrintf("%lu", url_sign);
    return false;
  }

  VLOG(3) << "url found in redis: " << res->brief().source_url() << ", sign: " << url_sign;

  const std::map<std::string, std::string>& field_map = r_it->second;

  // uv rank
  auto f_it = field_map.find("1");
  if (f_it != field_map.end()) {
    crawler2::Graph graph;
    CHECK(graph.ParseFromString(f_it->second));
    CHECK(graph.has_uv_rank());
    VLOG(3) << "uv rank: " << graph.uv_rank().Utf8DebugString();
    res->mutable_graph()->mutable_uv_rank()->Swap(graph.mutable_uv_rank());
  }

  // click rank
  f_it = field_map.find("2");
  if (f_it != field_map.end()) {
    crawler2::Graph graph;
    CHECK(graph.ParseFromString(f_it->second));
    CHECK(graph.has_click_rank());
    VLOG(3) << "click rank: " << graph.click_rank().Utf8DebugString();
    res->mutable_graph()->mutable_click_rank()->Swap(graph.mutable_click_rank());
  }

  // query
  f_it = field_map.find("3");
  if (f_it != field_map.end()) {
    crawler2::Graph graph;
    CHECK(graph.ParseFromString(f_it->second));
    CHECK_NE(graph.query_size(), 0);
    for (int query_idx = 0; query_idx < graph.query_size(); ++query_idx) {
      VLOG(3) << "query[" << query_idx << "]:"
              << graph.query(query_idx).Utf8DebugString();
      graph.mutable_query(query_idx)->Swap(res->mutable_graph()->add_query());
    }
  }

  // anchor
  f_it = field_map.find("4");
  if (f_it != field_map.end()) {
    crawler2::Graph graph;
    CHECK(graph.ParseFromString(f_it->second));
    CHECK_NE(graph.anchor_size(), 0);
    for (int anchor_idx = 0; anchor_idx < graph.anchor_size(); ++anchor_idx) {
      VLOG(3) << "anchor[" << anchor_idx << "]:"
              << graph.anchor(anchor_idx).Utf8DebugString();
      graph.mutable_anchor(anchor_idx)->Swap(res->mutable_graph()->add_anchor());
    }
  }
  *desc = "done, url: " + res->brief().source_url();
  return true;
}

void GeneralExtractor::AddNewUrlToLocalQueue(const crawler2::crawl_queue::JobOutput *output) {
  CHECK_NOTNULL(output);
  int32 depth = output->job().detail().depth();
  if (depth > FLAGS_max_url_depth) {
    LOG(ERROR) << "depth[" << depth << "] exceed max depth[" << FLAGS_max_url_depth
               << "] and NOT extract url from this url: " << output->res().brief().source_url();
    return;
  }

  const std::string &referer = output->res().brief().source_url();
  extend::re3::Re3 *re = NULL;
  if (!extract_patterns_.empty()) {
    GURL parent(referer);
    const std::string &host = parent.host();
    const std::string &path= parent.path();
    for (auto it = extract_patterns_.begin(); it != extract_patterns_.end(); ++it) {
      if (base::MatchPattern(host, it->host_pattern) &&
          base::MatchPattern(path, it->path_pattern)) {
        const std::string &pattern = it->target_url_pattern;
        re = new extend::re3::Re3(pattern);
        break;
      }
    }
  }
  std::set<std::string> tmp_set;
  for (auto it = output->res().parsed_data().anchor_urls().begin();
       it != output->res().parsed_data().anchor_urls().end();
       ++it) {
    // RawToClick
    std::string clicked_url;
    if (!web::RawToClickUrl(*it, NULL, &clicked_url)) {
      LOG(ERROR) << "web::RawToClickUrl() fail, url: " << *it;
      continue;
    }
    // Filter NOT wanted url according rules
    // std::string reason;
    // if (WillFilterAccordingRules(clicked_url, &reason)) {
    //   LOG(ERROR) << reason;
    //   continue;
    // }
    // apply rule: must match pattern
    GURL gurl(clicked_url);
    const std::string &spec_url = gurl.spec();
    if (re != NULL) {
      if (!extend::re3::Re3::FullMatch(spec_url, *re)) {
        LOG(ERROR) << "NOT match, url: " << spec_url << ", pattern: " << re->pattern();
        continue;
      } else {
        LOG(ERROR) << "match, url: " << spec_url << ", pattern: " << re->pattern();
      }
    }
    // add to local tmp set
    // 对于 tmall 商品页, 由于是在 list 页面 (html) 中提取, 这里需要将其转化成 Mobile 版本
    // 淘宝商品页在 解析 Json 格式 list 页面时已经转换过了
    if (gurl.host() == "detail.tmall.com") {
      std::string m_url;
      if (ConvertTaobaoUrlFromPCToMobile(spec_url, &m_url)) {
        tmp_set.insert(m_url);
      }
    } else {
      tmp_set.insert(spec_url);
    }
  }
  if (tmp_set.size() > 0) {
    for (auto it = tmp_set.begin(); it != tmp_set.end(); ++it) {
      const std::string &url = *it;
      crawler3::url::NewUrl *newurl = new crawler3::url::NewUrl();
      newurl->set_clicked_url(url);
      newurl->set_depth(depth + 1);
      newurl->set_referer(referer);
      newurl->set_resource_type(crawler2::kHTML);
      newurls_.Put(newurl);
    }
  }

  COUNTERS_extractor__new_url_queue_len.Reset(newurls_.Size());
  if (re != NULL) {
    delete re;
  }
}

void GeneralExtractor::SaveTaobaoItem(const std::vector< ::crawler3::url::TaobaoProductInfo> &items) {
  int64 ts = base::GetTimestamp();
  for (auto it = items.begin(); it != items.end(); ++it) {
    const std::string &link = (*it).link();
    std::string output;
    LOG_IF(FATAL, !(*it).SerializeToString(&output)) <<"SerializeToString() fail, url:" << link;
    taobao_mutex_.Lock();
    taobao_saver_.AppendKeyValue(link, output, ts);
    taobao_mutex_.Unlock();
  }
}

void GeneralExtractor::AddNewUrls(const std::vector<std::string> &urls) {
  std::unordered_set<std::string> tmp_set;
  for (int i = 0; i < (int)urls.size(); ++i) {
    tmp_set.insert(urls[i]);
  }
  if (tmp_set.size() > 0) {
    for (auto it = tmp_set.begin(); it != tmp_set.end(); ++it) {
      const std::string &url = *it;
      crawler3::url::NewUrl *newurl = new crawler3::url::NewUrl();
      newurl->set_clicked_url(url);
      newurl->set_depth(1);
      newurl->set_referer("");
      newurl->set_resource_type(crawler2::kHTML);
      newurls_.Put(newurl);
    }
  }
  COUNTERS_extractor__new_url_queue_len.Reset(newurls_.Size());
}

bool GeneralExtractor::RecgImagePrice(const std::string &url, const std::string &image_data,
                                      std::string *result) {
  CHECK_NOTNULL(result);
  GURL gurl(url);
  const std::string &path = gurl.path();
  std::string::size_type pos = path.rfind(".");
  if (pos == std::string::npos) {
    LOG(ERROR) << "not find dot in url path, url: " << url;
    return false;
  }
  std::string image_type = path.substr(pos+1);
  return (recg_.PriceVMemImage(image_data, image_type, result));
}

bool GeneralExtractor::ExtractSuNingPrice(const std::string &json_data,
                                          std::string *result) {
  CHECK_NOTNULL(result);
  const std::string &data = json_data;

  Json::Reader reader;
  Json::Value root;
  if (data.empty() || !reader.parse(data, root, false)) {
    LOG(ERROR) << "suning price parse parse fail, json data: " << data;
    return false;
  }
  const std::string &promotionPrice = root["promotionPrice"].asString();
  const std::string &netPrice = root["netPrice"].asString();
  if (!promotionPrice.empty()) {
    result->assign(promotionPrice);
  } else {
    result->assign(netPrice);
  }
  return true;
}

void GeneralExtractor::SubmitResProc() {
  thread::SetCurrentThreadName("Extractor[SubmitResProc]");
  job_queue::Client *client = NULL;
  if (FLAGS_submit_to_output_queue) {
    client = new job_queue::Client(options_.res_output_queue_ip, options_.res_output_queue_port);
    CHECK(client->Connect()) << "Failed to connect to job_queue["
                             << options_.res_output_queue_ip << ":"
                             << options_.res_output_queue_port << "]";
    if (!options_.res_output_queue_tube.empty()) {
      CHECK(client->Use(options_.res_output_queue_tube))
          << "Failed to call Use[" << options_.res_output_queue_tube <<"]";
    }
    LOG(ERROR) << "Success to connect res output queue["
               << options_.res_output_queue_ip << ":" << options_.res_output_queue_port
               << ":" << options_.res_output_queue_tube << "]";
  }

  int64 total_res = 0;
  while (true) {
    if (resources2_.Closed()) {
      break;
    }
    crawler2::crawl_queue::JobOutput* output = resources2_.Take();
    if (output == NULL) {
      LOG(ERROR) << "blocking queue: resources2_ closed.";
      break;
    }
    scoped_ptr<crawler2::crawl_queue::JobOutput> auto_output(output);

    const std::string &s_url = output->res().brief().source_url();
    // res 保存在本地文件
    if (FLAGS_res_save_in_local) {
      SaveResource(output->res());
      LOG(INFO) << "saved in local, url: " << s_url;
    }
    // 将网页提交给输出队列
    if (client != NULL) {
      std::string result;
      if (!output->res().SerializeToString(&result)) {
        LOG(ERROR) << "SerializeToString() fail, url: " << s_url;
        continue;
      }
      int ret = client->PutEx(result.data(), result.size(), 5, 0, 10);
      if (ret <= 0) {
        if (!client->Ping() && !client->Reconnect()) {
          LOG(ERROR) << "Failed to connect to jobqueue ["
                     << options_.res_output_queue_ip << ":"
                     << options_.res_output_queue_port <<"]";
          delete client;
          usleep(100 * 1000);
          client = new job_queue::Client(options_.res_output_queue_ip, options_.res_output_queue_port);
          LOG_IF(ERROR, !client->Connect()) << "Failed to connect to url_queue";
        }
        crawler2::crawl_queue::JobOutput *again = new crawler2::crawl_queue::JobOutput(*output);
        resources2_.Put(again);
        // TODO(pengdan): 需要限制以下 resources2 的 length
        LOG(ERROR) << "res will Put to output queue again, url:" << s_url;
        continue;
      }
      LOG(INFO) << "submit to output queue[" << options_.res_output_queue_ip << ":"
                << options_.res_output_queue_port << "], url: " << s_url;
    }
    total_res++;

    LOG_EVERY_N(INFO, 500) << "[summary] res[" << total_res
                           << "] has been received.";
  }
}

void GeneralExtractor::ExtractorProc() {
  thread::SetCurrentThreadName("Extractor[ExtractorProc]");

  crawler2::crawl_queue::JobOutput* output = NULL;
  while ((output = resources_.Take()) != NULL) {
    scoped_ptr<crawler2::crawl_queue::JobOutput> auto_output(output);
    const std::string &s_url = output->res().brief().source_url();
    crawler2::ResourceType resource_type = output->res().brief().resource_type();
    // 如果是图片, 直接扔到 提交队列, 不做后续的任何判断处理
    if (resource_type == crawler2::kImage) {
      crawler2::crawl_queue::JobOutput *new_output = new crawler2::crawl_queue::JobOutput(*output);
      resources2_.Put(new_output);
      continue;
    }
    GURL gurl(s_url);
    const std::string &host = gurl.host();
    const std::string &path = gurl.path();
    const std::string &query = gurl.query();
    // 对一些特殊的 url 进行处理, 例如: 商品 list 页面
    // 这些页面时不需要存储, 但是需要从中提取信息, 如 link
    // 需求: list.taobao.com: 需要存 JSON 格式数据
    {
      // taobao item list
      if (host == "list.taobao.com") {
        // page is JSON  file
        const std::string &page_utf8 = output->res().content().utf8_content();
        if (page_utf8.empty()) {
          LOG(ERROR) << "page utf8 is empty, url: " << s_url;
          continue;
        } else {
          std::vector< ::crawler3::url::TaobaoProductInfo> items;
          int page_num;
          if (ParseJsonFormatPage(page_utf8, &items, &page_num)) {
            // 这个数据目前没有用上,暂且不存到本地文件
            // SaveTaobaoItem(items);
            // Add new urls to queue;
            std::vector<std::string> urls;
            if (FLAGS_build_next_n_url &&
                page_num > 1 &&
                query.find("&s=") == std::string::npos) {
              int build_num = (page_num < FLAGS_build_max_n_url) ? page_num : FLAGS_build_max_n_url;
              BuildNextNUrl(s_url, build_num-1, &urls);
            }
            for (auto it = items.begin(); it != items.end(); ++it) {
              std::string url;
              // 将 taobao 商品 url 从 PC 版转换成 Mobile 版
              if (crawler3::ConvertTaobaoUrlFromPCToMobile(it->link(), &url)) {
                urls.push_back(url);
              } else {
                LOG(ERROR) << "crawler3::ConvertTaobaoUrlFromPCToMobile() fail, url: " << it->link();
              }
            }
            AddNewUrls(urls);
          } else {
            LOG(ERROR) << "parse json fail, url: " << s_url;
            continue;
          }
        }
        // 注释 continue; 因为, 需要存 list Json 格式数据
        // continue;
      }
      // 天猫 tmall list
      if (host == "list.tmall.com") {
        // page is html
        const std::string &page_utf8 = output->res().content().utf8_content();
        if (page_utf8.empty()) {
          LOG(ERROR) << "page utf8 is empty, url: " << s_url;
          continue;
        } else {
          int page_num;
          if (GetPageNumFromTmallListPage(page_utf8, &page_num)) {
            std::vector<std::string> urls;
            if (FLAGS_build_next_n_url &&
                page_num > 1 &&
                (query.find("&s=") == std::string::npos || query.find("&s=0") != std::string::npos)) {
              int build_num = (page_num < FLAGS_build_max_n_url) ? page_num : FLAGS_build_max_n_url;
              BuildNextNUrl(s_url, build_num-1, &urls, 60);
            }
            AddNewUrls(urls);
          } else {
            LOG(ERROR) << "GetPageNumFromTmallListPage() fail, url: " << s_url;
          }
        }
      }
      // 需要进行后续处理, 如 提取链接等,所以不能 continue
      // 360buy item list
      if (base::MatchPattern(s_url, "http://www.360buy.com/products/*.html") &&
          path.find("-0-0-0-0-0-0-") == std::string::npos) {  // list 首页, 没有构造过翻页 url
        const std::string &page_utf8 = output->res().content().utf8_content();
        if (page_utf8.empty()) {
          LOG(ERROR) << "page utf8 is empty, url: " << s_url;
          continue;
        }
        // 获取该搜索 list 页面的 page num
        int page_num;
        if (!GetPageNumFromJingDongListPage(page_utf8, &page_num)) {
          LOG(ERROR) << "get page num fail, url: " << s_url;
        } else {
          if (page_num < 2) {
            LOG(ERROR) << "page < 2, NO need to build next N pages, page num: " << page_num;
          } else {
            std::vector<std::string> urls;
            if (FLAGS_build_next_n_url &&
                GetJingDongNextUrl(s_url, page_num-1, &urls)) {
              AddNewUrls(urls);
            }
          }
        }
      }
      // 需要进行后续处理, 如 提取链接等,所以不能 continue
      // suning item list
      if (base::MatchPattern(s_url, "http://search.suning.com/emall/*.do*") &&
          query.find("&il=0&si=5&st=14&iy=-1") == std::string::npos) {  // list 首页, 没有构造过翻页 url
        const std::string &page_utf8 = output->res().content().utf8_content();
        if (page_utf8.empty()) {
          LOG(ERROR) << "page utf8 is empty, url: " << s_url;
          continue;
        }
        // 获取该搜索 list 页面的 page num
        int page_num;
        if (!GetPageNumFromSuNingListPage(page_utf8, &page_num)) {
          LOG(ERROR) << "get page num fail, url: " << s_url;
        } else {
          if (page_num < 2) {
            LOG(ERROR) << "page < 2, NO need to build next N pages, page num: " << page_num;
          } else {
            std::vector<std::string> urls;
            if (FLAGS_build_next_n_url &&
                GetSuNingNextUrl(s_url, page_num-1, &urls)) {
              AddNewUrls(urls);
            }
          }
        }
      }
      // 需要进行后续处理, 如 提取链接等,所以不能 continue
    }

    // 对于 effective url 用 UrlCategorizer 进行过滤
    if (FLAGS_filter_using_url_categorizer) {
      const std::string &e_url = output->res().brief().effective_url();
      int url_id;
      std::string tmp;
      if (!url_categoryizer_.Category(e_url, &url_id, &tmp)) {
        LOG(ERROR) << "UrlCategorizer::Category fail, e_url: " << e_url << ", source url: "
                   <<  output->res().brief().source_url();
        continue;
      }
      // ListPage is need
      if (url_id < web::util::kSplitLineForCrawler &&
          url_id != web::util::kListPage &&
          url_id != web::util::kSpecialFile &&
          url_id != web::util::kSkippable) {
        LOG(ERROR) << "url droped, e_url: " << e_url << ", typeid: " << url_id << ", source url: "
                   << output->res().brief().source_url();
        continue;
      }
    }
    // 处理 title 并过滤
    {
      if (output->res().has_content() && output->res().content().has_html_title()) {
        const std::string &title = output->res().content().html_title();
        if (!title.empty()) {
          std::string raw_title;
          if (base::LineUnescape(title, &raw_title)) {
            if (FLAGS_filter_using_bad_titles && bad_titles_.find(raw_title) != bad_titles_.end())  {
              LOG(ERROR) << "url droped, url: " << output->res().brief().source_url()
                         << ", title: " << raw_title;
              continue;
            } else {
              title_mutex_.Lock();
              title_saver_.AppendLine(raw_title, base::GetTimestamp());
              title_mutex_.Unlock();
            }
          }
        }
      }
    }
    // add info: uv_rank click_rank query anchor ...
    // std::string result_desc;
    // LOG_IF(WARNING, !AddResourceExtraInfo(*output->mutable_res(), &result_desc))
    //     << "AddResourceExtraInfo() result: " << result_desc;

    // 处理价格数据
    {
      // 对于京东, 对其价格图片进行识别
      if (base::MatchPattern(s_url, "http://www.360buy.com/product/*.html")) {
        if (output->res().has_alt() && output->res().alt().has_price_image()) {
          const crawler2::ImageFile &image= output->res().alt().price_image();
          if (image.has_url() && !image.url().empty() &&
              image.has_raw_content() && !image.raw_content().empty()) {
            const std::string &image_url = image.url();
            const std::string &image_data = image.raw_content();
            std::string price;
            if (RecgImagePrice(image_url, image_data, &price)) {
              output->mutable_res()->mutable_alt()->mutable_price_image()->set_price(price);
            } else {
              LOG(ERROR) << "recg image price fail, image url: " << image_url;
            }
          }
        }
        // 处理 苏宁 价格 json 格式
      } else if (base::MatchPattern(s_url, "http://www.suning.com/emall/prd_*.html")) {
        if (output->res().has_alt() && output->res().alt().has_price_image()) {
          const crawler2::ImageFile &image= output->res().alt().price_image();
          const std::string &url = image.url();
          const std::string &data = image.raw_content();
          std::string price;
          if (ExtractSuNingPrice(data, &price)) {
            output->mutable_res()->mutable_alt()->mutable_price_image()->set_price(price);
          } else {
            LOG(ERROR) << "ExtractSuNingPrice() fail, price url: " << url;
          }
        }
      }
    }

    // 将 res 数据保存起来,
    // 京东 list 页面不需要保存 res
    // 苏宁 list 页面不需要保存
    // 淘宝/天猫 list 页面保存: 淘宝的为 JSON 格式, 天猫的为网页
    if (!base::MatchPattern(s_url, "http://www.360buy.com/products/*.html") &&
        !base::MatchPattern(s_url, "http://search.suning.com/emall/*")) {
      crawler2::crawl_queue::JobOutput *new_output = new crawler2::crawl_queue::JobOutput(*output);
      resources2_.Put(new_output);
    }

    // 对于 taobao, 提交链接已经做了特别处理 ( 解析 Json file), 后续的过程不需要
    // 抓取的商品 item 页面, 也不需要从中提取链接
    if (base::MatchPattern(s_url, "http://www.360buy.com/product/*.html") ||
        host == "a.m.taobao.com" || host == "a.m.tmall.com" ||
        host == "item.taobao.com" || host == "detail.tmall.com" ||
        host == "www.suning.com" || host == "list.taobao.com") {
      continue;
    }

    // No need to submit newlink
    if (!FLAGS_is_submit_newlink) {
      continue;
    }

    // 提交从本页面提取出的新 anchor url 到本地 queue
    if (output->res().has_parsed_data() &&
        output->res().parsed_data().anchor_urls_size() > 0) {
      // if (!output->res().content().has_utf8_content() ||
      //     output->res().content().utf8_content().empty()) {
      //   continue;
      // }
      // const std::string &utf8_html = output->res().content().utf8_content();
      // const std::string &e_url = output->res().brief().effective_url();
      // // Page type recognization
      // web::HTMLDoc doc(utf8_html.c_str(), e_url.c_str());
      // web::HTMLParser parser;
      // parser.Init();
      // parser.Parse(&doc, web::HTMLParser::kParseAll);
      // if (feature::IsHubPage(&doc)) {
      //   LOG(INFO) << "Is hub page, url: " << e_url;
      // } else {
      //   LOG(INFO) << "Is NOT hub page, still extract url, url: " << e_url;
      // }
      AddNewUrlToLocalQueue(output);
    }
  }
}

void GeneralExtractor::FetchJobProc() {
  thread::SetCurrentThreadName("GeneralExtractor[FetchJobProc]");

  // 负责从 job_queue 当中获取数据并检查数据的合法性
  job_queue::Client client(options_.job_queue_ip, options_.job_queue_port);
  CHECK(client.Connect()) << "Failed to connect to job_queue["
                          << options_.job_queue_ip << ":"
                          << options_.job_queue_port << "]";
  if (!options_.job_queue_tube.empty()) {
    CHECK(client.Watch(options_.job_queue_tube))
        << "Failed to call Watch[" << options_.job_queue_tube <<"]";
  }

  LOG(ERROR) << "Success to connect server["
             << options_.job_queue_ip << ":" << options_.job_queue_port
             << ":" << options_.job_queue_tube << "]";

  int64 task_fetched = 0;
  while (true) {
    if (resources_.Closed()) {
      break;
    }

    if (resources_.Size() > 1000) {
      LOG_EVERY_N(INFO, 1000)
          << " Too many task in queue [" <<  resources_.Size()
          << "] sleep for sometime";
      usleep(100 * 1000);
    }


    if (!client.Ping() && !client.Reconnect()) {
      LOG(ERROR) << "Failed to connect to jobqueue ["
                 << options_.job_queue_ip << ":"
                 << options_.job_queue_port <<"]";
      usleep(100 * 0000);
      continue;
    }

    job_queue::Job job;
    if (!client.ReserveWithTimeout(&job, 1)) {
      LOG_EVERY_N(INFO, 1000) << "[summary] Get Job timeout.";
      continue;
    }

    COUNTERS_extractor__res_fetched.Increase(1);

    if (!client.Delete(job.id)) {
      LOG(ERROR) << "Failed to delete job:" << job.id;
    }

    task_fetched++;
    LOG_EVERY_N(INFO, 1000) << "[summary] tasks[" << task_fetched
                            << "] have been fetched..";

    crawler2::crawl_queue::JobOutput* output = new crawler2::crawl_queue::JobOutput;
    if (!output->ParseFromString(job.body)) {
      LOG(ERROR) << "Failed to parse JobOutput from string."
                 << " job.body.length: " << job.body.length()
                 << " job.body: " << job.body.substr(0, 100);
      delete output;
      continue;
    }
    if (!output->has_res()) {
      total_no_res_++;
      COUNTERS_extractor__res_without_content.Increase(1);
      LOG(ERROR) << "Has no res, url: " << output->job().detail().url();
      delete output;
      continue;
    }

    resources_.Put(output);
    COUNTERS_extractor__res_queue_len.Reset(resources_.Size());
  }
}

int GeneralExtractor::ResourceQueueSize() {
  return resources_.Size();
}

void GeneralExtractor::SaveResource(const crawler2::Resource &res) {
  if (!res.has_brief()) {
    LOG(ERROR) << "Has no brief";
    return;
  }
  if (!res.brief().has_crawl_info()) {
    LOG(ERROR) << "Has no crawl_info";
    return;
  }
  if (!res.brief().crawl_info().has_response_code()) {
    LOG(ERROR) << "Has no response_code";
    return;
  }
  if (res.brief().crawl_info().response_code() != 200) {
    LOG(ERROR) << "Ignored url: " << res.brief().source_url() << ", ret code is: "
               << res.brief().crawl_info().response_code();
    return;
  }

  std::string key, value;
  if (FLAGS_res_save_in_bin_format) {  // 二进制格式存储
    crawler2::GenNewHbaseKey(res.brief().url(), &key);
    ResourceToHbaseDump(res, &value);
    // 加锁
    res_mutex_.Lock();
    resource_saver_.AppendKeyValue(key, value, base::GetTimestamp());
    res_mutex_.Unlock();
  } else {  // 文本格式储存
    CHECK(base::Base64Encode(res.brief().url(), &key));
    std::string output;
    CHECK(res.SerializeToString(&output));
    CHECK(base::Base64Encode(output, &value));
    // 加锁
    res_mutex_.Lock();
    resource_saver_.Append(key +"\t" + value + "\n", base::GetTimestamp());
    res_mutex_.Unlock();
  }
}

void GeneralExtractor::SubmitNewUrlProc() {
  thread::SetCurrentThreadName("GeneralExtractor[SubmitNewUrlProc]");

  job_queue::Client *client = new job_queue::Client(options_.url_queue_ip, options_.url_queue_port);
  CHECK(client->Connect()) << "Failed to connect to job_queue["
                          << options_.url_queue_ip << ":"
                          << options_.url_queue_port << "]";
  if (!options_.url_queue_tube.empty()) {
    CHECK(client->Use(options_.url_queue_tube))
        << "Failed to call Use[" << options_.url_queue_tube <<"]";
  }
  LOG(ERROR) << "Success to connect url server["
             << options_.url_queue_ip << ":" << options_.url_queue_port
             << ":" << options_.url_queue_tube << "]";
  // 负责将 newurl 提交到 url 处理队列
  int64 total_urls = 0;
  crawler3::url::NewUrl *newurl = NULL;
  while ((newurl = newurls_.Take()) != NULL) {
    scoped_ptr<crawler3::url::NewUrl> auto_ptr(newurl);
    std::string output;
    if (!newurl->SerializeToString(&output)) {
      LOG(ERROR) << "Failed in SerializedToString(&output)";
      continue;
    }
    int ret = client->PutEx(output.data(), output.size(), 5, 0, 10);
    if (ret < 0) {
      if (!client->Ping() && !client->Reconnect()) {
        LOG(ERROR) << "Failed to connect to jobqueue ["
                   << options_.url_queue_ip << ":"
                   << options_.url_queue_port <<"]";
        delete client;
        usleep(100 * 1000);
        client = new job_queue::Client(options_.url_queue_ip, options_.url_queue_port);
        LOG_IF(ERROR, !client->Connect()) << "Failed to connect to url_queue";
      }
      continue;
    }
    LOG(ERROR) << "Added to url job queue, url: " << newurl->clicked_url();

    total_urls++;
    COUNTERS_extractor__new_url_submit.Increase(1);

    LOG_EVERY_N(INFO, 1000) << "[summary] urls[" << total_urls
                            << "] has been submitted.";
  }
}

void GeneralExtractor::Start() {
  LOG_IF(FATAL, !recg_.Init(FLAGS_recg_data_path + "/training", FLAGS_recg_data_path + "/testing"))
      << "recg_.Init() fail";
  if (!FLAGS_url_extract_pattern_file.empty()) {
    LOG(ERROR) << "start loading url extract rule...";
    util::LoadUrlExtractRule(FLAGS_url_extract_pattern_file, &extract_patterns_);
  }
  if (FLAGS_is_submit_newlink) {
    submit_newurl_thread_.Start(NewCallback(this, &GeneralExtractor::SubmitNewUrlProc));
  }
  fetch_job_thread_.Start(NewCallback(this, &GeneralExtractor::FetchJobProc));

    submit_res_thread_.Start(NewCallback(this, &GeneralExtractor::SubmitResProc));

  for (int i = 0; i < options_.processor_thread_num; ++i) {
    ::thread::Thread *thread = new ::thread::Thread;
    thread->Start(::NewCallback(this, &GeneralExtractor::ExtractorProc));
    processor_threads_.push_back(thread);
  }
  LOG(ERROR) << "GeneralExtractor Started.";
}

void GeneralExtractor::Stop() {
  resources_.Close();
  fetch_job_thread_.Join();

  resources2_.Close();
  submit_res_thread_.Join();

  if (FLAGS_is_submit_newlink) {
    newurls_.Close();
    submit_newurl_thread_.Join();
  }

  for (auto iter = processor_threads_.begin();
       iter != processor_threads_.end(); ++iter) {
    (*iter)->Join();
    delete (*iter);
  }
  stopped_ = true;
}

void GeneralExtractor::Loop() {
  while (!stopped_) {
    sleep(1);
  }
}

void ParseGeneralExtractorConfig(const extend::config::ConfigObject& config,
                                  GeneralExtractor::Options* options) {
  options->resource_prefix = config["resource_prefix"].value.string;
  options->title_prefix = config["title_prefix"].value.string;
  options->saver_timespan = config["saver_timespan"].value.integer * 1000 * 1000l;
  options->res_saver_timespan = config["res_saver_timespan"].value.integer * 1000 * 1000l;
  options->processor_thread_num = config["processor_thread_num"].value.integer;
  options->job_queue_ip = config["job_queue_ip"].value.string;
  options->job_queue_port = config["job_queue_port"].value.integer;
  options->job_queue_tube = config["job_queue_tube"].value.string;
  options->error_titles = config["error_titles"].value.string;

  if (FLAGS_is_submit_newlink) {
    options->url_queue_ip = config["url_queue_ip"].value.string;
    options->url_queue_port = config["url_queue_port"].value.integer;
    options->url_queue_tube = config["url_queue_tube"].value.string;
  }
  if (FLAGS_submit_to_output_queue) {
    options->res_output_queue_ip = config["res_output_queue_ip"].value.string;
    options->res_output_queue_port = config["res_output_queue_port"].value.integer;
    options->res_output_queue_tube = config["res_output_queue_tube"].value.string;
  }
}

}  // namespace feautre
