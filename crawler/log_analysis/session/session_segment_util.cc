#include "log_analysis/session/session_segment_util.h"
#include "log_analysis/session/algorithm/pvlog_segmenter.h"
#include "log_analysis/session/algorithm/searchlog_segmenter.h"

namespace session_segment {

// PvLogSegmenter pv_seg;
// SearchLogSegmenter search_seg;

void PvLogSessionID(const std::vector<std::string> &lines,
                    std::vector<int> *session_results) {
  PvLogSegmenter pv_seg;
  pv_seg.SegmentSessionID(lines, session_results);
}

void SearchLogSessionID(const std::vector<std::string> &lines,
                        std::vector<int> *session_results) {
  SearchLogSegmenter search_seg;
  search_seg.SegmentSessionID(lines, session_results);
}

void SearchLogSessionMD5(const std::vector<std::string> &lines,
                         std::vector<std::string> *session_results) {
  SearchLogSegmenter search_seg;
  search_seg.SegmentSessionMD5(lines, session_results);
}

void PvLogSessionMD5(const std::vector<std::string> &lines,
                     std::vector<std::string> *session_results) {
  SearchLogSegmenter pv_seg;
  pv_seg.SegmentSessionMD5(lines, session_results);
}

}  // end namespace
