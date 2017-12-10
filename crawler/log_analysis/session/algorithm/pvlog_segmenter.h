#pragma once

#include <string>
#include <unordered_set>
#include <vector>
#include "base/time/time.h"
#include "base/hash_function/city.h"

namespace session_segment {

class PvLogSegmenter {
  enum SegmentType {
    kRefLongTime = -1,  // 有 ref_url, 与下一条记录相距较长时间
    kNoRefShortTime,    // 没有 ref_url, 与下一条记录相距较短时间
    kNoRefLongTime,     // 没有 ref_url, 与下一条记录相距较长时间
    kOther              // 其他
  };

  // 记录 pv_log session 切割时需要的数据及中间结果
  struct record_format_ {
    record_format_() : time_interval(-1), session_id(0) {
    }
    std::string time_stamp;
    int time_interval;         // 与上条记录的时间间隔, 单位为秒
    // std::string url;        // URL
    uint64 url_sign;           // 保存 URL 签名
    std::string ref_url;    // REF_URL
    uint64 ref_url_sign;       // 保存 REF_URL 签名
    std::string host;          // URL 对应的 HOST
    // std::unordered_set <uint64> host_seg_signs;
    std::string ref_host;      // REF_URL 对应的 HOST
    // std::unordered_set <uint64> ref_host_seg_signs;
    int session_id;            // 计算得到的 sessionID
  };

 public:
  PvLogSegmenter();
  ~PvLogSegmenter();

  // log 格式: \t 分割, 前 4 个字段必须拥有
  // agentid timestamp url ref_url ref_query ref_search_engine ref_anchor url_attr entrance_id duration
  // logs 严格按照 timestamp 从小到大排序
  // 输出: 对应每条 log 的 session_id, id 号从 1 往上累加
  void SegmentSessionID(const std::vector<std::string> &lines,
                        std::vector<int> *session_results);

  // 同上, 但输出为对应每条 log 的 session 的签名
  void SegmentSessionMD5(const std::vector<std::string> &lines,
                         std::vector<std::string> *session_results);

 private:
  void SegmentAlg();

  void reset() {
    records_.clear();
    const std::string def_url = "";
    home_url_sign_ = base::CityHash64(def_url.c_str(), def_url.size());
    agentid_.clear();
  }

  // 抽取 session 切割所需要的字段, 存入|records_|中, 并计算该用户可能的默认 url
  void extract_records(const std::vector<std::string> &lines);

  // 判断前后两个 session 是否应该合并.
  // 传入这两个 session 切割的依据, 不同的切割类型判断方式有所不同
  bool merge_sessions(int last_session_start,
                     int this_session_start,
                     int this_session_end,
                     SegmentType type);

  // 判断两个 host 是否相似, 如 www.baidu.com 和 baidu.com 应该是同一个 host
  bool is_similar_host(const std::string &host_1,
                       const std::string &host_2);

  bool is_empty_url(const std::string &url) {
    return (url == "NULL" || url == "");
  }

 private:
  static const int kNonRefTimeThr = 5*60;  // 没有 ref_url 时, 可能切割 session 的时间阈值

  static const int kRefTimeThr = 30*60;    // 拥有 ref_url 时, 可能切割 session 的时间阈值

  // 之前是拥有 ref_url, 但间隔时间较长而暂时切割. 判断是否合并使用的阈值
  static const float kRefLongSimThr;

  // 之前没有 ref_url, 同时间隔时间较长而暂时切割. 判断是否合并使用的阈值
  static const float kNoRefLongSimThr;

  // 之前没有 ref_url, 同时间隔时间较短而暂时切割. 判断是否合并使用的阈值
  static const float kNoRefShortSimThr;

  // 判断是否是 home_url 使用的阈值
  static const int kHomeUrlCountThr = 3;

  // 判断是否是 home_url 使用的阈值
  static const float kHomeUrlFreThr;

  // 保存 home_url 的签名
  uint64 home_url_sign_;
  //
  std::string agentid_;
  // std::string home_host;
  // std::unordered_set<uint64> home_host_seg_signs_;

  // std::unordered_map<std::string, uint64> hash_dict_;

  // 保存用户的所有 pv 数据, 严格按时间顺序
  std::vector<record_format_> records_;  //
};
}

