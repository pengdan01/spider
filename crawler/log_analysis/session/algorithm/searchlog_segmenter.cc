#include "log_analysis/session/algorithm/searchlog_segmenter.h"
#include <string>
#include <unordered_map>
#include <iostream>
#include "base/common/base.h"
#include "base/common/logging.h"
#include "nlp/common/term_container.h"
#include "nlp/segment/segmenter.h"
#include "base/strings/string_split.h"
#include "base/strings/string_printf.h"
#include "base/hash_function/md5.h"

namespace session_segment {
const int kTimePos = 2;  // time_stamp 在 log 的字段
const int kQueryPos = 3;   // query 在 log 中的字段

const float SearchLogSegmenter::kSimilarThr = 0.2;

SearchLogSegmenter::SearchLogSegmenter() {
  segmenter_ = new nlp::segment::Segmenter();
  reset();
}

SearchLogSegmenter::~SearchLogSegmenter() {
  if (segmenter_) {
    delete segmenter_;
  }
}

void SearchLogSegmenter::extract_records(const std::vector<std::string> &lines) {
  base::Time last_log_time;
  record_format_ record;

  std::vector<std::string> line_vec;
  nlp::term::TermContainer term_container;
  agentid_.clear();
  for (int idx = 0; idx < static_cast<int>(lines.size()); idx++) {
    line_vec.clear();
    const std::string &line = lines[idx];
    base::SplitString(line, "\t", &line_vec);
    if (agentid_ == "") {
      agentid_ = line_vec[1];
    } else {
      CHECK_STREQ(agentid_.c_str(), line_vec[1].c_str());
    }
    record.query_segs.clear();
    record.query = line_vec[kQueryPos];
    record.time_interval = -1;
    if (segmenter_->SegmentT(record.query, &term_container)) {
      for (int jdx = 0; jdx < term_container.basic_term_num(); jdx++) {
        record.query_segs.push_back(term_container.basic_term_slice(record.query, jdx));
      }
    } else {
      LOG(ERROR) << base::StringPrintf("%s segment error", record.query.c_str());
    }
    // 计算与上条 pv 的时间间隔, 如果是首条, 则设置为 0
    record.time_stamp = line_vec[kTimePos];
    const std::string &time_stamp = record.time_stamp;
    base::Time log_time;
    if (time_stamp.length() != 14) {
      LOG(ERROR) << base::StringPrintf("[%s] Timestamp length error. [%d]",
                                       time_stamp.c_str(), static_cast<int>(time_stamp.length()));
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
  }
}

bool SearchLogSegmenter::is_similar_record(const record_format_ &record,
                                          const record_format_ &last_record) {
  int related_term_num = 0;
  for (int idx = 0; idx < static_cast<int>(record.query_segs.size()); idx++) {
    for (int jdx = 0; jdx < static_cast<int>(last_record.query_segs.size()); jdx++) {
      if (record.query_segs[idx].as_string() == last_record.query_segs[jdx].as_string()) {
        related_term_num++;
        break;
      }
    }
  }
  int length = static_cast<int>(record.query_segs.size() + last_record.query_segs.size());
  if (length == 0) {
    return true;
  }
  float sim = related_term_num * 2.0 / length;
  if (sim >= kSimilarThr) {
    return true;
  }
  return false;
}

bool SearchLogSegmenter::merge_sessions(int last_session_start,
                                        int this_session_start,
                                        int this_session_end) {
  CHECK_GE(this_session_end, this_session_start);
  if (this_session_start <= last_session_start) {
    // 第一个 session
    return false;
  }
  for (int idx = this_session_start; idx <= this_session_end; idx++) {
    for (int jdx = last_session_start; jdx < this_session_start; jdx++) {
      if (records_[idx].query == records_[jdx].query) {
        return true;
      }
    }
  }
  return false;
}

void SearchLogSegmenter::SegmentAlg() {
  int session_id = 1;
  int last_session_start = 0;
  int this_session_start = 0;

  for (int idx = 0; idx < static_cast<int>(records_.size()); idx++) {
    record_format_ &record = records_[idx];
    if (0 == idx) {
      record.session_id = session_id;
      continue;
    }
    const record_format_ &last_record = records_[idx-1];
    if (record.query == last_record.query) {
      record.session_id = session_id;
      continue;
    }
    if (is_similar_record(record, last_record) &&
        record.time_interval < kSimRecordTimeThr) {
      record.session_id = session_id;
      continue;
    }
    if (record.time_interval < kDiffRecordTimeThr) {
      record.session_id = session_id;
      continue;
    }
    if (merge_sessions(last_session_start, this_session_start, idx-1)) {
      for (int jdx = this_session_start; jdx < idx; jdx++) {
        records_[jdx].session_id = session_id - 1;
      }
      record.session_id = session_id;
    } else {
      record.session_id = (++session_id);
      last_session_start = this_session_start;
    }
    this_session_start = idx;
  }
  int this_session_end = static_cast<int>(records_.size()) - 1;
  if (merge_sessions(last_session_start, this_session_start, this_session_end)) {
    for (int jdx = this_session_start; jdx <= this_session_end; jdx++) {
      records_[jdx].session_id = session_id - 1;
    }
  }
}

void SearchLogSegmenter::SegmentSessionID(const std::vector<std::string> &lines,
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

void SearchLogSegmenter::SegmentSessionMD5(const std::vector<std::string> &lines,
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

