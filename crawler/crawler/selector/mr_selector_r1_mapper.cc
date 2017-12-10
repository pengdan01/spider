#include <iostream>
#include <string>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/mapreduce/mapreduce.h"
#include "base/strings/string_util.h"
#include "base/strings/string_split.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"
#include "web/url/url.h"
#include "web/url_norm/url_norm.h"
#include "feature/url/url_score.h"
#include "crawler/proto/crawled_resource.pb.h"
#include "crawler/selector/crawler_selector_util.h"
#include "crawler/api/base.h"

#include "log_analysis/common/log_common.h"

const uint32 kUserInputSeeds_RecordFieldNum = 1;
const uint32 kSearchlog_RecordFieldNum = 7;
const uint32 kNewLink_RecordFieldNum = 4;
const uint32 kLinkBase_RecordFieldNum = 15;
const uint32 kPVlog_RecordFieldNum = 10;
// TMP
const uint32 kNaviBoost = 1000;
const uint32 kUvRank = 2000;

// 360 domain
static const char * domains_360[] = {
  "1360.com", "3360.cn", "360.cn", "3600.com", "360cdn.cn", "360safe.com", "360so.com",
  "360ting.com", "4008158360.com", "51city.com", "51city.net", "6360.com", "aimi365.com", "appshare.cc",
  "ccecs.com", "cn-ns.net", "ermchina.cn", "fr-trading.com", "helpton.com", "kouxin.com",  "ludashi.com",
  "qhimg.com", "qihoo.com", "qihu.com", "qikoo.com", "smartermob.com", "theworld.cn", "woxihuan.com",
  "xihuan.me", "xkoo.com", "youhua.com", "yunpan.com", "yunpan.cn", "wanhutong.net", "wanhutong.com",
  "wanhutong.cn", "wanhutong.com.cn", "360zhushou.com", "2366.com", "china.cn"};

// 用户配置输入的 URL(kUserInputSeeds_RecordFieldNum) 的类型(1 html; 2 css; 3 image)
DEFINE_int32(seed_url_type, 1, "");

// Index model 相关参数
DEFINE_string(switch_use_index_model, "", "whether using index model or not");
DEFINE_string(index_model_name, "", "index model name");
DEFINE_bool(use_mmp_mode, false, "all the reducers in one node will load only one model");
// 是否使用过滤规则进行过滤, 默认使用
DEFINE_bool(switch_filter_rules, true, "switch of using crawler filter rules");

// 搜索结果的位次 百度搜索结果中，rank>10 的结果是广告
// 目前特意将该值配置的很大，即抓取 Search log 中的所有
// 出现的 urls(including ads)
const int32 kLastRankNotAds = 100;
// resource_type 类型
// 1 HTML
// 2 CSS
// 3 Image
int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "crawler_selector_1");
  CHECK(base::mapreduce::IsMapApp());
  base::mapreduce::MapTask *task = base::mapreduce::GetMapTask();
  CHECK_NOTNULL(task);
  // 说明: 在 mapper 中本来是不需要使用到 index model 的 ( 只有在 reduce 中采用到 )
  // 但是, 由于 mapper 阶段运行时间较长. 经常出现程序运行到 reducer 阶段时因为 Index
  // Model 加载失败 导致整个 hadoop 任务失败. 为了让可能出现的错误尽早暴露, 在 map 阶
  // 段加载 Index Model, 验证 Model 能否正常加载.
  {
    if (FLAGS_switch_use_index_model == "y" || FLAGS_switch_use_index_model == "Y") {
      CHECK(!FLAGS_index_model_name.empty()) << "index_model_name is empty";
      urlscore::UrlScorer computer;
      bool status = computer.Init(FLAGS_index_model_name, FLAGS_use_mmp_mode);
      if (!status) {
        LOG(ERROR) << "Error, failed to Init Object ScoreComputer";
        return -1;
      }
    }
  }
  bool tmp_flag = false;
  const std::string &filepath = task->GetInputSplit().hdfs_path;
  CHECK(!filepath.empty());
  LOG(INFO) << "Input file path: " << filepath;
  // 临时输入: url \t refer
  if (base::MatchPattern(filepath, "*/for_pengdan_pv/*")) {
    tmp_flag = true;
  }
  // 数据来之 Nave boost 数据
  bool is_navi_boost = false;
  bool is_navi_click_rank = false;
  if (base::MatchPattern(filepath, "*/navi_boost/*")) {
    is_navi_boost = true;
    if (base::MatchPattern(filepath, "*/navi_boost/click_rank/*")) {
      is_navi_click_rank = true;
    }
  }
  // 数据来之 VIP link base
  bool is_linkbase_vip = false;
  if (base::MatchPattern(filepath, "*/linkbase_vip/*")) {
    is_linkbase_vip = true;
  }
  bool is_uv_rank = false;
  if (base::MatchPattern(filepath, "*/data/uv_rank/*")) {
    is_uv_rank = true;
  }


  web::UrlNorm url_norm;

  std::string key;
  std::string value;
  std::string url;
  std::string refer_url;
  std::string click_url;
  int resource_type = 1;
  int64 filter_cnt = 0;
  int64 total_url_cnt = 0;
  std::vector<std::string> tokens;
  std::string flag;
  double score = 0.0;
  std::string reason;
  // 用来标识该 url 的数据来源, 下游模块会对不同的数据来源进行区分出来
  // 'P' : 表示来自 PV log
  // 'S' : 表示来自 Search Log
  // 'N' : 表示来自 网页提取出来的新的 link
  // 'E' : 表示来自 用户通过 Seeds 配置文件
  // 'L' : 表示来自 Linkbase
  // 'K' : 表示不知道 该 url 的来源
  char from = 'K';
  char refer_from = 'K';
  while (task->NextKeyValue(&key, &value)) {
    if (key.empty()) continue;
    ++total_url_cnt;
    // 解析并提取 url 记录的各个字段
    // 使用 SplitString 对数据进行原始切分, 不做任何额外出来
    tokens.clear();
    size_t num = 0;
    if (!value.empty()) {
      base::SplitString(value, "\t", &tokens);
      num = tokens.size();
    }
    num++;  // 整条记录的 field# = 1(key) + value#
    if (is_navi_boost) num = kNaviBoost;  // 来之 navi boost 数据
    if (is_uv_rank) num = kUvRank;
    switch (num) {
      // Format: query \t url \t score      // top10
      // Format: query \t url \t score      // daily
      // Format: url \t click_rank          // click rank
      case kNaviBoost: {
        std::string score_str;
        if (is_navi_click_rank == false) {
          url = tokens[0];
          score_str = tokens[1];
        } else {
          url = key;
          score_str = tokens[0];
        }
        flag = "BA";
        from = 'A';   // nAvi boost
        refer_url = "";
        refer_from = 'K';

        if (!base::StringToDouble(score_str, &score)) {
          LOG(ERROR) << "Convert string to double fail, str: " << tokens[1];
          continue;
        }

        break;
      }
      case kUvRank: {
        url = key;
        flag = "BA";
        from = 'A';
        resource_type = 1;
        refer_url = "";
        refer_from = 'K';
        break;
      }
      // 由 crawler 抓取的 new links, 记录格式如下：
      // newlink \t 1 \t fromlink \t refer_from
      case kNewLink_RecordFieldNum: {
        url = key;
        // 前提假设： Resource_type 取值为 0 ～ 9 即：不能超过一个字符长度
        if (tokens[0].size() != 1) {
          LOG(ERROR) << "Resource_type should be [0,9] and 1 byte, but is: " << tokens[0];
          continue;
        }
        resource_type = tokens[0][0] - '0';
        // TMP
        if (resource_type == 1) continue;
        if (resource_type == crawler::kImage && !crawler::IsValuableImageLink(url)) {
          LOG(ERROR) << "image is not IsValuableImageLink, url: " << url;
          continue;
        }
        refer_url = tokens[1];
        flag = "BB";
        from = 'N';
        refer_from = tokens[2][0];
        break;
      }
      // linkbase 库中 link 格式如下：
      // source_url \t effective_url \t link_type \t timestamp \t score \t response_code \t file_time \t
      // total_time \t redirect_cnt \t download_speed \t content_len_download \t content_type \t
      // html_header_lineescaped \t simhash \t update_fail_cnt
      case kLinkBase_RecordFieldNum: {
        click_url = key;
        if (tokens[1].size() != 1) {
          LOG(ERROR) << "Resource_type should be [0,9] and 1 byte, but is: " << tokens[1];
          continue;
        }
        resource_type = tokens[1][0] - '0';
        refer_url = "";
        flag = "AA";
        if (is_linkbase_vip) {
          from = 'V';  // VIP linkbase
        } else {
          from = 'L';
        }
        refer_from = 'K';
        break;
      }
      // 由用户通过配置文件输入的 links, 记录格式如下 即，一行就是一条 link
      // www.hao123.com
      case kUserInputSeeds_RecordFieldNum + 1: {
        // 由用户输入, 只有一列, 但是最后有一个 tab 键
        LOG_IF(FATAL, !tmp_flag && !tokens[0].empty()) << "Invalid Seed Record Format: " << tokens[0];
        if (tmp_flag) {  // XXX(pengdan): 临时数据处理
          url = key;
          // Filter ??
          std::string::size_type pos = url.find("??");
          if (pos != std::string::npos) {
            LOG(ERROR) << "Ignored, as ?? in url: " << url;
            continue;
          }
          // 对 PV log 的处理规则:
          GURL gurl(url);
          std::string host = gurl.host();
          // 对于 host 起始含有 % 的 特殊处理
          bool truncated = false;
          while (true) {
            if (!(host[0] == '%' && host.size() >= 3)) break;
            host = host.substr(3);
            truncated = true;
          }
          if (truncated == true) url = "http://" + host + gurl.path();
          // 1. user.qzone.qq.com 数据太多, 这里先过滤掉该数据
          if ((base::MatchPattern(host, "*user.qzone.qq.com*") ||
               base::MatchPattern(host, "*pengyou.com*") ||
               base::MatchPattern(host, "*weixin.qq.com*")) && gurl.path() != "/") {
            LOG(ERROR) << "Ignored need login url: " << url;
            continue;
          }
          // 5. flash 游戏的 过滤掉
          if (base::MatchPattern(gurl.path(), "/qzone/*")) continue;

          resource_type = 1;
          refer_url = tokens[0];
          flag = "BB";
          from = 'P';
          if (log_analysis::IsGeneralSearchClick(url, refer_url, NULL, NULL) ||
              (!log_analysis::IsGeneralSearch(url, NULL, NULL) && !log_analysis::IsVerticalSearch(url, NULL, NULL) && log_analysis::IsVerticalSearch(refer_url, NULL, NULL) && !log_analysis::IsAds(url, NULL))) {  // NOLINT
            flag = "BA";
            from = 'S';
          }
          std::string target_url;
          if (log_analysis::ParseGoogleTargetUrl(url, &target_url)) {
            LOG(INFO) << "parse google url: " << url  << ", target url: " << target_url;
            url = target_url;
          }
          refer_from = 'K';
          break;
        }
      }
      case kUserInputSeeds_RecordFieldNum: {
        url = key;
        resource_type = FLAGS_seed_url_type;  // Default set resource_type to HTML
        refer_url = "";
        flag = "BA";
        from = 'E';
        refer_from = 'K';
        break;
      }
      // Search log 数据，记录格式如下： sid\tmid\ttimestamp\tquery\tse_name\trank\tres_url
      // eg: www.google.com.hk\twww.360.cn\t8
      // 具体各个字段含义，请参考：http://192.168.11.60:8081/pages/viewpage.action?pageId=1671227
      case kSearchlog_RecordFieldNum: {
        int rank;
        if (!base::StringToInt(tokens[4], &rank)) {
          LOG(ERROR) << "Failed to convert string to int, string is: " << tokens[4];
          continue;
        }
        // We are only interested in none-ads-urls
        if (rank > kLastRankNotAds) continue;
        url = tokens[5];
        resource_type = 1;  // Default set resource_type to HTML
        refer_url = "";
        // Tricky !!! 由于对于 Search log 中的 URL 将直接进行抓取，不用计算 score ，这里用
        // “BA” 加以区分其他路数据
        flag = "BA";
        from = 'S';
        refer_from = 'K';
        break;
      }
      case 3: {  // XXX(pengdan): 高点击 URL, Record: query \t url \t score
        url = tokens[0];
        resource_type = 1;  // Default set resource_type to HTML
        refer_url = "";
        flag = "BA";
        from = 'S';
        refer_from = 'K';
        break;
      }
      // PV log 数据，记录格式如下：
      // agentid \t timestamp \t url \t refer_url \t refer_query \t refer_search_engine \t refer_anchor
      // \t url_attr \t entrance_id \t duration
      // 具体各个字段含义，请参考：http://192.168.11.60:8081/pages/viewpage.action?pageId=1671227
      case kPVlog_RecordFieldNum: {
        resource_type = 1;  // Default set resource_type to HTML
        url = tokens[1];
        std::string::size_type pos = url.find("??");
        if (pos != std::string::npos) {
          DLOG(ERROR) << "Ignored, as ?? in url: " << url;
          continue;
        }
        refer_url = tokens[2];
        std::string refer_query = tokens[3];
        std::string refer_search_engine = tokens[4];
        base::TrimWhitespaces(&refer_query);
        base::TrimWhitespaces(&refer_search_engine);
        // 对 PV log 的处理规则:
        GURL gurl(url);
        std::string host = gurl.host();
        // 对于 host 起始含有 % 的 特殊处理
        bool truncated = false;
        while (true) {
          if (!(host[0] == '%' && host.size() >= 3)) break;
          host = host.substr(3);
          truncated = true;
        }
        if (truncated == true) url = "http://" + host + gurl.path();
        // 1. user.qzone.qq.com 数据太多, 这里先过滤掉该数据
        if ((base::MatchPattern(host, "*user.qzone.qq.com*") ||
            base::MatchPattern(host, "*weibo.com*") ||
            base::MatchPattern(host, "*hero.qzoneapp.com*") ||
            base::MatchPattern(host, "*weixin.qq.com*")) && gurl.path() != "/")  {
          LOG_EVERY_N(WARNING, 200) << "ignored url: " << url << ", reason: not valuable url";
          continue;
        }
        // 4. Host 为 IP 的过滤掉
        if (gurl.HostIsIPAddress()) continue;
        // 5. flash 游戏的 过滤掉
        if (base::MatchPattern(gurl.path(), "/qzone/*")) {
          LOG_EVERY_N(WARNING, 200) << "ignored url: " << url << ", reason: not valuable url, /qzone/*";
          continue;
        }
        // Tricky !!! 由于对于 PV log 中的 URL 将直接进行抓取，不用计算 score ，这里用
        // “BA” 加以区分其他路数据
        flag = "BA";
        // Search Click
        refer_from = 'K';
        if (!refer_query.empty() || !refer_search_engine.empty()) {
          from = 'S';
        } else {
          from = 'P';
        }
        break;
      }
      default:
        LOG(ERROR) << "Format error(field# " << num << "), input record, ignored: " << key + "\t" + value;
        break;
    }
    // 只对待抓取的 url 以及用户配置输入的 url 进行 RawToClick 处理，因为 linkbase 中的 url 在入库前已经处理过
    if (flag[0] == 'B') {
      if (from == 'P' || from == 'S') {  // Urls from Search Log | PV log
        //  Search | PV Log 存在大量形式如 www.tzyy120.com/QQ:765293969 或 www.ym618.com/TEL:400-6166-918
        //  的无效数据，需要对其进行处理，去除后面的无效信息 “QQ:765293969” or “TEL:400-6166-918”
        std::string::size_type pos1, pos2, pos3, pos4;
        pos1 = url.find("TEL:");
        pos2 = url.find("tel:");
        pos3 = url.find("QQ:");
        pos4 = url.find("qq:");
        if (pos1 != std::string::npos) {
          url = url.substr(0, pos1);
        } else if (pos2 != std::string::npos) {
          url = url.substr(0, pos2);
        } else if (pos3 != std::string::npos) {
          url = url.substr(0, pos3);
        } else if (pos4 != std::string::npos) {
          url = url.substr(0, pos4);
        }
      }
      if (!web::has_scheme(url)) {
        url = "http://" + url;
      }
      if (!web::RawToClickUrl(url, NULL, &click_url)) {
        LOG_EVERY_N(WARNING, 200) << "ignored url: " << url << ", reason: Fail in RawToClick()"
                                  << ", ignored. count: " << google::COUNTER;
        continue;
      }
      // XXX(pengdan):暂停对 360 站点的抓取
      GURL gurl(url);
      bool is_360 = false;
      const std::string &host = gurl.host();
      for (int i = 0; i < (int)arraysize(domains_360); ++i) {
        if (base::EndsWith(host, domains_360[i], false)) {
          LOG(ERROR) << "Ignore 360 urls: " << url;
          is_360 = true;
          break;
        }
      }
      if (is_360 == true)  {
        // continue;
      }
      // XXX(pengdan): tmp end

      // 应用爬虫 URL 过滤规则,确定本 URL 是否需要忽略
      // 设置 false, 使用较为宽松的过滤规则: 让 重要搜索引擎结果页通过
      if (FLAGS_switch_filter_rules == true) {
        if (crawler::WillFilterAccordingRules(click_url, &reason, true)) {  // XXX(pengdan): 改成 不抓去新链接
          ++filter_cnt;
          LOG_EVERY_N(WARNING, 200) << "ignored url: " << click_url << ", reason: "
                                    << reason  << ", ignored. count: " << google::COUNTER;
          continue;
        }
        // 只取前 3 页的搜索结果页
        if (log_analysis::IsGeneralSearch(click_url, NULL, NULL) && !crawler::IsGeneralSearchFirstNPage(click_url, 1)) {  // NOLINT
          LOG(ERROR)  << "ingore url, should be in first page: " << click_url;
          continue;
        }
        if (log_analysis::IsVerticalSearch(click_url, NULL, NULL) && !crawler::IsVerticalSearchFirstNPage(click_url, 1)) {  // NOLINT
          LOG(ERROR)  << "ingore url, should be in first page: " << click_url;
          continue;
        }
      }
    }
    // 归一化
    if (!url_norm.NormalizeClickUrl(click_url, &click_url)) {
      LOG(ERROR) << "Fail in NormalizeClickUrl(), url: " << click_url;
      continue;
    }
    value = base::StringPrintf("%s%s\t%d\t%c\t%c\t%f", flag.c_str(), refer_url.c_str(), resource_type, from,
                               refer_from, score);
    task->Output(click_url, value);
  }

  LOG(INFO) <<"Total url#: " << total_url_cnt << ", apply rules and filter url#: " << filter_cnt;
  return 0;
}
