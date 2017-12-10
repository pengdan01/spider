#pragma once

#include <string>
#include <unordered_set>
#include <vector>
#include "base/time/time.h"
#include "base/common/slice.h"

namespace nlp {
namespace segment {
class Segmenter;
}
}

namespace session_segment {

class SearchLogSegmenter {
  // 记录 search_log session 切割时需要的数据及中间结果
  struct record_format_ {
    record_format_() : time_interval(-1), session_id(0) {
    }
    std::string time_stamp;
    int time_interval;                       // 与上条记录的时间间隔, 单位为秒
    std::string query;                       // search query
    std::vector<base::Slice> query_segs;     // query segment
    int session_id;                          // 计算得到的 sessionID
  };

 public:
  SearchLogSegmenter();
  ~SearchLogSegmenter();

  // log 格式: \t 分割, 前 4 个字段必须拥有
  // searchid agentid timestamp query se_name rank ref_url
  // logs 严格按照 timestamp 从小到大排序
  // 输出: 对应每条 log 的 session_id, id 号从 1 往上累加
  void SegmentSessionID(const std::vector<std::string> &lines,
                        std::vector<int> *session_results);

  // log 格式: \t 分割, 前 4 个字段必须拥有
  // searchid agentid timestamp query se_name rank ref_url
  // logs 严格按照 timestamp 从小到大排序
  // 输出: 对应每条 log 的 session 的 md5
  void SegmentSessionMD5(const std::vector<std::string> &lines,
                         std::vector<std::string> *session_results);

 private:
  void SegmentAlg();

  void reset() {
    records_.clear();
    agentid_.clear();
  }

  bool is_similar_record(const record_format_ &record,
                         const record_format_ &last_record);

  // 抽取 session 切割所需要的字段, 存入|records_|中, 并计算该用户可能的默认 url
  void extract_records(const std::vector<std::string> &lines);

  bool merge_sessions(int last_session_start,
                      int this_session_start,
                      int this_session_end);

 private:
  // 判断为相似 record 时, 切割 session 的时间阈值
  static const int kSimRecordTimeThr = 30*60;

  // 判断为不相似 record 时, 切割 session 的时间阈值
  static const int kDiffRecordTimeThr = 10*60;

  // 判断两条 record 是否类似时，使用到的阈值
  static const float kSimilarThr;

  // 保存用户的所有 pv 数据, 严格按时间顺序
  std::vector<record_format_> records_;  //

  //
  std::string agentid_;

  nlp::segment::Segmenter *segmenter_;
};
}

