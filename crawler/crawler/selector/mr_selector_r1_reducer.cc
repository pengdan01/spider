#include <iostream>
#include <string>
#include <list>
#include <algorithm>

#include "base/common/base.h"
#include "base/strings/string_util.h"
#include "base/strings/string_tokenize.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"
#include "base/common/logging.h"
#include "base/mapreduce/mapreduce.h"
#include "feature/url/url_score.h"
#include "crawler/selector/crawler_selector_util.h"

DEFINE_string(switch_use_index_model, "", "whether using index model or not");
DEFINE_string(index_model_name, "", "index model name");
DEFINE_bool(use_mmp_mode, false, "all the reducer in the same machine will share one model");
DEFINE_double(model_score_threshold, 0.0, "a link with score below the threeshold will be ignored");
DEFINE_bool(not_crawle_already_in_linkbase, true, "a link will not be crawled if already in linkbase");
DEFINE_double(navi_boost_score_threshold, 0.0, "");

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "mr_selector_r1_reducer");
  CHECK(base::mapreduce::IsReduceApp());
  base::mapreduce::ReduceTask *task = base::mapreduce::GetReduceTask();
  CHECK_NOTNULL(task);

  std::string key;
  std::string value;
  char from;
  char refer_from;
  int resource_type = 0;
  int64 total_link_count = 0;
  int64 score_filter_count = 0;

  double score = 0;

  std::vector<std::string> tokens;
  std::vector<std::string> records;
  // Now creat a ScoreComputer object to calculate the score of each link need to crawle
  urlscore::UrlScorer computer;
  bool switch_use_index_model = false;
  if (FLAGS_switch_use_index_model == "y" || FLAGS_switch_use_index_model == "Y") {
    switch_use_index_model = true;
    CHECK(!FLAGS_index_model_name.empty()) << "index_model_name is empty";
    bool status = computer.Init(FLAGS_index_model_name, FLAGS_use_mmp_mode);
    if (!status) {
      LOG(ERROR) << "Error, failed to Init Object ScoreComputer";
      return -1;
    }
  }
  // mapper output format is: Url \t 'AA'('BB' or 'BA')ReferUrl \t ResourceType \t from \t refer_from \t score
  while (task->NextKey(&key)) {
    LOG_IF(FATAL, key.empty()) << "key is empty";
    records.clear();
    while (task->NextValue(&value)) {
      LOG_IF(FATAL, value.empty()) << "value is empty";
      records.push_back(value);
    }
    sort(records.begin(), records.end());
    const std::string &record = records[0];
    if (base::StartsWith(record, "AA", true)) {
      if (!FLAGS_not_crawle_already_in_linkbase) {
        for (int i = 1; i < (int)records.size(); ++i) {
          tokens.clear();
          int num = base::Tokenize(records[i], "\t", &tokens);
          LOG_IF(FATAL, num != 5) << "Invalid Record field# in Reducer: " << key + "\t" + records[i];
          resource_type = tokens[1][0] - '0';
          from = tokens[2][0];
          const std::string &refer_url = tokens[0].substr(2);
          refer_from = tokens[3][0];
          if (from != 'L' && from != 'V') {
            score = 1.0;
            if (from == 'A') {
              if (!base::StringToDouble(tokens[4], &score)) {
                LOG(ERROR) << "StringToDouble() fail, str: " << tokens[4];
                continue;
              }
            }
            value = base::StringPrintf("%s\t%f\t%c\t%s\t%c", tokens[1].c_str(), score, from ,
                                       refer_url.c_str(), refer_from);
            task->Output(key, value);
            break;
          }
        }
      } else {
        // 正常情况下, 1. 如果是一个普通的 url 且在 linkbase 中不存在;
        // 2. 或者是一个 vip url 且在 vip linkbase 中不存在, 则 进行抓取
        // 显然 条件 1 已经不满足, 该 URL 肯定在 linkbase 中存在, 只需要判断第 2 中可能
        for (int i = 0; i < (int)records.size(); ++i) {
          tokens.clear();
          int num = base::Tokenize(records[i], "\t", &tokens);
          LOG_IF(FATAL, num != 5) << "Invalid Record field# in Reducer: " << key + "\t" + records[i];
          resource_type = tokens[1][0] - '0';
          from = tokens[2][0];
          const std::string &refer_url = tokens[0].substr(2);
          refer_from = tokens[3][0];
          if (from == 'V') break;  // 已经在 vip 库中, 不需要抓取
          if (from != 'L' && from != 'V') {
            if (crawler::IsVIPUrl(key, resource_type, from, refer_url, refer_from)) {
              value = tokens[1] + "\t1.0\t" + from + "\t" + refer_url + "\t" + refer_from;
              task->Output(key, value);
              break;
            }
          }
        }
      }
    } else if (base::StartsWith(record, "BA", true)) {  // NO need to calculate score for those URLs
      tokens.clear();
      int num = base::Tokenize(record, "\t", &tokens);
      LOG_IF(FATAL, num != 5) << "Invalid Record field# in Reducer: " << key + "\t" + record;
      from = tokens[2][0];
      refer_from = tokens[3][0];
      value = tokens[1] + "\t1.0\t" + from + "\t" + tokens[0].substr(2) + "\t" + refer_from;
      task->Output(key, value);
    } else if (base::StartsWith(record, "BB", true)) {  // Need to calcuate score for those URSs
      tokens.clear();
      int num = base::Tokenize(record, "\t", &tokens);
      LOG_IF(FATAL, num != 5) << "Invalid Record field# in Reducer: " << key + "\t" + record;
      resource_type = tokens[1][0] - '0';
      from = tokens[2][0];
      const std::string &refer_url = tokens[0].substr(2);
      refer_from = tokens[3][0];
      // Is VIP url? if is , NO need to calculate score
      if (crawler::IsVIPUrl(key, resource_type, from, refer_url, refer_from)) {
        value = tokens[1] + "\t1.0\t" + from + "\t" + refer_url + "\t" + refer_from;
        task->Output(key, value);
        continue;
      }
      if (resource_type != 1) {  // Score is valid for kHTML only
        score = 1.0;
      } else {
        if (!switch_use_index_model) {
          score = 0.5;
        } else {
          score = computer.PredictScore(key);
        }
      }
      ++total_link_count;
      if (score >= FLAGS_model_score_threshold) {
        value = base::StringPrintf("%s\t%f\t%c\t%s\t%c", tokens[1].c_str(), score, from,
                                   tokens[0].substr(2).c_str(), refer_from);
        task->Output(key, value);
      } else {
        LOG(WARNING) << "ignore url: " << key << ", score: " << score;
        ++score_filter_count;
      }
    } else {
      LOG(FATAL) << "Invalid Record, should start with AA, BA or BB";
    }
  }
  LOG(INFO) << "total link(calculated score)#: " << total_link_count << ", output link#: "
            << (total_link_count-score_filter_count) << ", filter link(below threhold)#: "
            << score_filter_count;
  return 0;
}
