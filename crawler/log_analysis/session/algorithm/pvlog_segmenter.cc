#include "log_analysis/session/algorithm/pvlog_segmenter.h"
#include <string>
#include <unordered_map>
#include <iostream>
#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/mapreduce/mapreduce.h"
#include "base/strings/string_split.h"
#include "base/strings/string_printf.h"
#include "web/url/gurl.h"
#include "base/hash_function/md5.h"

namespace session_segment {
const int kTimePos = 1;  // time_stamp 在 log 的字段
const int kUrlPos = 2;   // url 在 log 中的字段
const int kRefPos = 3;   // ref_url 在 log 中的字段

const float PvLogSegmenter::kRefLongSimThr = 0.4;

const float PvLogSegmenter::kNoRefLongSimThr = 0.6;

const float PvLogSegmenter::kNoRefShortSimThr = 0.3;

const float PvLogSegmenter::kHomeUrlFreThr = 0.15;

PvLogSegmenter::PvLogSegmenter() {
  reset();
}

PvLogSegmenter::~PvLogSegmenter() {
}

void PvLogSegmenter::extract_records(const std::vector<std::string> &lines) {
  std::unordered_map<uint64, int> url_sign_count;
  base::Time last_log_time;
  record_format_ record;

  std::vector<std::string> line_vec;
  agentid_.clear();
  for (int idx = 0; idx < static_cast<int>(lines.size()); idx++) {
    line_vec.clear();
    const std::string &line = lines[idx];
    base::SplitString(line, "\t", &line_vec);
    if (agentid_ == "") {
      agentid_ = line_vec[0];
    } else {
      CHECK_STREQ(line_vec[0].c_str(), agentid_.c_str());
    }
    record.time_interval = -1;
    // record.url = line_vec[kUrlPos];
    // record.ref_url = line_vec[kRefPos];
    // 抽取 url, ref_url, 计算签名, 同时计算对应的 host 和 ref_host
    const std::string &url = line_vec[kUrlPos];
    const std::string &ref_url = line_vec[kRefPos];
    record.url_sign = base::CityHash64(url.c_str(), url.size());
    record.ref_url_sign = base::CityHash64(ref_url.c_str(), ref_url.size());
    GURL gurl(url);
    if (gurl.is_valid()) {
      record.host = gurl.host();
    } else {
      record.host = url;
    }
    if (is_empty_url(ref_url)) {
      record.ref_host = ref_url;
    } else {
      gurl = GURL(ref_url);
      if (gurl.is_valid()) {
        record.ref_host = gurl.host();
      } else {
        record.ref_host = ref_url;
      }
    }

    // 计算与上条 pv 的时间间隔, 如果是首条, 则设置为 0
    record.time_stamp = line_vec[kTimePos];
    const std::string &time_stamp = record.time_stamp;
    base::Time log_time;
    if (time_stamp.length() != 14) {
      LOG(ERROR) << base::StringPrintf("[%s] Timestamp length error", time_stamp.c_str());
    } else {
      std::string log_time_str = base::StringPrintf("%s-%s-%s %s:%s:%s",
                                                    time_stamp.substr(0, 4).c_str(),
                                                    time_stamp.substr(4, 2).c_str(),
                                                    time_stamp.substr(6, 2).c_str(),
                                                    time_stamp.substr(8, 2).c_str(),
                                                    time_stamp.substr(10, 2).c_str(),
                                                    time_stamp.substr(12, 2).c_str());

      if (!base::Time::FromStringInSeconds(log_time_str.c_str(), &log_time)) {
        LOG(ERROR) << base::StringPrintf("[%s] FromStringInSeconds error", time_stamp.c_str());
      } else if (last_log_time.is_null()) {
        record.time_interval = 0;
      } else {
        record.time_interval = static_cast<int>((log_time - last_log_time).InSeconds());
      }
      if (record.time_interval < 0) {
        record.time_interval = 0;
      } else {
        last_log_time = log_time;
      }
    }
    records_.push_back(record);
    // 加入 url_sign_count, 用于后续的 home_url 选取
    if (is_empty_url(ref_url)) {
      if (url_sign_count.find(record.url_sign) == url_sign_count.end()) {
        url_sign_count[record.url_sign] = 1;
      } else {
        url_sign_count[record.url_sign] += 1;
      }
    }
  }
  // 选取 home_url
  int max_count = 0;
  uint64 candicate_url_sign = home_url_sign_;
  auto iter = url_sign_count.begin();
  while (iter != url_sign_count.end()) {
    if (iter->second > max_count) {
      max_count = iter->second;
      candicate_url_sign = iter->first;
    }
    iter++;
  }
  if (max_count >= kHomeUrlCountThr ||
      max_count*1.0/records_.size() >= kHomeUrlFreThr) {
    home_url_sign_ = candicate_url_sign;
  }
}

bool PvLogSegmenter::merge_sessions(int last_session_start,
                                    int this_session_start,
                                    int this_session_end,
                                    SegmentType type) {
  CHECK_GE(this_session_end, this_session_start);
  if (this_session_start <= last_session_start) {
    // 第一个 session
    return false;
  }

  std::unordered_set<std::string> session_hosts;

  int session_length = this_session_end - this_session_start + 1;
  // 计算本 session 中与上个 session 较相关的 pv 数
  int related_record = 0;
  for (int idx = this_session_start; idx <= this_session_end; idx++) {
    const record_format_ &record = records_[idx];
    if (record.url_sign == home_url_sign_) {
      continue;
    }
    bool flag = false;
    for (int jdx = last_session_start; jdx < this_session_start; jdx++) {
      if (records_[jdx].url_sign != home_url_sign_ &&
          is_similar_host(record.host, records_[jdx].host)) {
        related_record += 1;
        flag = true;
        break;
      }
    }
    if (flag || is_empty_url(record.ref_host)) {
      continue;
    }
    bool ref_compare = true;
    for (int jdx = this_session_start; jdx < idx; jdx++) {
      if (record.ref_url_sign == records_[jdx].url_sign) {
        ref_compare = false;
      }
    }
    if (!ref_compare) {
      continue;
    }
    for (int jdx = last_session_start; jdx < this_session_start; jdx++) {
      if (records_[jdx].url_sign != home_url_sign_ &&
          record.ref_url_sign == records_[jdx].url_sign) {
        related_record += 1;
        break;
      }
    }
  }

  const int kConfidenceNum = 3;
  float threshold = 1.0;
  if (type == kRefLongTime) {
    threshold = kRefLongSimThr;
  } else if (type == kNoRefLongTime) {
    threshold = kNoRefLongSimThr;
  } else if (type == kNoRefShortTime) {
    threshold = kNoRefShortSimThr;
  }

  float sim = related_record * 1.0 / session_length;
  if (session_length >= kConfidenceNum && sim >= threshold) {
    return true;
  }
  related_record = 0;
  session_length = this_session_start - last_session_start;
  for (int idx = last_session_start; idx < this_session_start; idx++) {
    const record_format_ &record = records_[idx];
    for (int jdx = this_session_start; jdx <= this_session_end; jdx++) {
      if (record.url_sign == records_[jdx].url_sign) {
        related_record += 1;
        break;
      }
    }
  }
  sim = related_record * 1.0 / session_length;
  return (session_length >= kConfidenceNum && sim >= threshold);
}

bool PvLogSegmenter::is_similar_host(const std::string &host_1,
                                       const std::string &host_2) {
  std::vector<std::string> vec_1, vec_2;
  base::SplitString(host_1, ".", &vec_1);
  base::SplitString(host_2, ".", &vec_2);
  bool flag = true;
  for (int idx = 0; idx < static_cast<int>(vec_1.size()); idx++) {
    if (vec_1[idx] == "www" || vec_1[idx] == "com" || vec_1[idx] == "cn") {
      continue;
    }
    bool tmp_flag = false;
    for (int jdx = 0; jdx < static_cast<int>(vec_2.size()); jdx++) {
      if (vec_1[idx] == vec_2[jdx]) {
        tmp_flag = true;
        break;
      }
    }
    if (!tmp_flag) {
      flag = false;
      break;
    }
  }
  if (flag) {
    return true;
  }

  for (int idx = 0; idx < static_cast<int>(vec_2.size()); idx++) {
    if (vec_2[idx] == "www" || vec_2[idx] == "com" || vec_2[idx] == "cn") {
      continue;
    }
    bool tmp_flag = false;
    for (int jdx = 0; jdx < static_cast<int>(vec_1.size()); jdx++) {
      if (vec_2[idx] == vec_1[jdx]) {
        tmp_flag = true;
        break;
      }
    }
    if (!tmp_flag) {
      return false;
    }
  }
  return true;
}

void PvLogSegmenter::SegmentAlg() {
  int session_id = 1;
  int last_session_start = 0;
  int this_session_start = 0;
  SegmentType last_session_type = kOther;
  SegmentType this_session_type = kOther;

  for (int idx = 0; idx < static_cast<int>(records_.size()); idx++) {
    record_format_ &record = records_[idx];
    if (0 == idx) {
      record.session_id = session_id;
      continue;
    }
    record_format_ &last_record = records_[idx-1];
    if (last_record.url_sign == home_url_sign_) {
      record.session_id = session_id;
      continue;
    }
    if (!is_empty_url(record.ref_host)) {
      if (record.time_interval < kRefTimeThr) {
        record.session_id = session_id;
      } else {
        for (int jdx = idx - 1; jdx >= this_session_start; jdx--) {
          if (record.ref_url_sign == records_[jdx].url_sign) {
            record.session_id = session_id;
            break;
          }
        }
      }
      if (record.session_id == session_id) {
        continue;
      }
      this_session_type = kRefLongTime;
    } else {
      if (record.time_interval < kNonRefTimeThr) {
        record.session_id = session_id;
        continue;
      }
      if (record.time_interval < kRefTimeThr) {
        if (is_similar_host(record.host, last_record.host)) {
          record.session_id = session_id;
          continue;
        }
        this_session_type = kNoRefShortTime;
      } else {
        this_session_type = kNoRefLongTime;
      }
    }
    if (merge_sessions(last_session_start, this_session_start, idx-1, last_session_type)) {
      for (int jdx = this_session_start; jdx < idx; jdx++) {
        records_[jdx].session_id = session_id - 1;
      }
      record.session_id = session_id;
    } else {
      record.session_id = (++session_id);
      last_session_start = this_session_start;
    }
    this_session_start = idx;
    last_session_type = this_session_type;
  }

  int this_session_end = static_cast<int>(records_.size()) - 1;
  if (merge_sessions(last_session_start, this_session_start, this_session_end, last_session_type)) {
    for (int jdx = this_session_start; jdx <= this_session_end; jdx++) {
      records_[jdx].session_id = session_id - 1;
    }
  }
}

void PvLogSegmenter::SegmentSessionID(const std::vector<std::string> &lines,
                                      std::vector<int> *session_results) {
  reset();
  records_.reserve(lines.size());
  session_results->clear();
  if (lines.empty()) {
    return;
  }

  extract_records(lines);

  SegmentAlg();

  for (int idx = 0; idx < static_cast<int>(records_.size()); idx++) {
    session_results->push_back(records_[idx].session_id);
  }
}

void PvLogSegmenter::SegmentSessionMD5(const std::vector<std::string> &lines,
                                       std::vector<std::string> *session_results) {
  reset();
  records_.reserve(lines.size());
  session_results->clear();
  if (lines.empty()) {
    return;
  }

  extract_records(lines);

  SegmentAlg();

  std::string session_md5;
  int last_session_id = 0;
  for (int idx = 0; idx < static_cast<int>(records_.size()); idx++) {
    if (records_[idx].session_id != last_session_id) {
      std::string tmp_str = base::StringPrintf("%s\t%s\t%d", agentid_.c_str(),
                                               records_[idx].time_stamp.c_str(),
                                               records_[idx].session_id);
      session_md5 = MD5String(tmp_str);
      last_session_id = records_[idx].session_id;
    }
    session_results->push_back(session_md5);
    // session_results->push_back(records_[idx].session_id);
  }
}

}  // end namespace

