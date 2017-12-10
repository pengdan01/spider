
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "log_analysis/session/session_segment_util.h"
#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/common/gflags.h"
#include "base/strings/string_split.h"

// DEFINE_string(src_file, "test_data/case.10000", "log data");
// DEFINE_string(dst_file, "test_data/result.10000", "result data");

void ParsePvLogFile(const std::string &src_file_name,
                    const std::string &dst_file_name) {
  std::ifstream src_file(src_file_name.c_str());
  if (!src_file) {
    LOG(FATAL) << "Can't open src file: " << src_file_name << std::endl;
  }
  std::ofstream dst_file(dst_file_name.c_str());
  if (!dst_file) {
    LOG(FATAL) << "Can't open result file: " << dst_file_name << std::endl;
  }

  std::string line, agentid, last_agentid;
  std::vector<std::string> logs, line_vec;
  std::vector<int> session_results;
  std::vector<std::string> md5_results;
  while (std::getline(src_file, line)) {
    line_vec.clear();
    base::SplitString(line, "\t", &line_vec);
    agentid = line_vec[0];
    if (agentid != last_agentid) {
      if (last_agentid != "") {
        session_segment::PvLogSessionID(logs, &session_results);
        session_segment::PvLogSessionMD5(logs, &md5_results);
        CHECK_EQ(logs.size(), session_results.size());
        for (size_t idx = 0; idx < logs.size(); idx++) {
          dst_file << logs[idx] << "\t" << session_results[idx] << "\t" << md5_results[idx] << std::endl;
        }
      }
      last_agentid = agentid;
      logs.clear();
      session_results.clear();
    }
    logs.push_back(line);
  }
  if (last_agentid != "") {
    session_segment::PvLogSessionID(logs, &session_results);
    session_segment::PvLogSessionMD5(logs, &md5_results);
    CHECK_EQ(logs.size(), session_results.size());
    for (size_t idx = 0; idx < logs.size(); idx++) {
      dst_file << logs[idx] << "\t" << session_results[idx] << "\t" << md5_results[idx] << std::endl;
    }
  }
}

void ParseSearchLogFile(const std::string &src_file_name,
                        const std::string &dst_file_name) {
  std::ifstream src_file(src_file_name.c_str());
  if (!src_file) {
    LOG(FATAL) << "Can't open src file: " << src_file_name << std::endl;
  }
  std::ofstream dst_file(dst_file_name.c_str());
  if (!dst_file) {
    LOG(FATAL) << "Can't open result file: " << dst_file_name << std::endl;
  }

  std::string line, agentid, last_agentid;
  std::vector<std::string> logs, line_vec;
  // std::vector<int> session_results;
  std::vector<std::string> session_results;
  while (std::getline(src_file, line)) {
    line_vec.clear();
    base::SplitString(line, "\t", &line_vec);
    agentid = line_vec[1];
    if (agentid != last_agentid) {
      if (last_agentid != "") {
        session_segment::SearchLogSessionMD5(logs, &session_results);
        CHECK_EQ(logs.size(), session_results.size());
        for (size_t idx = 0; idx < logs.size(); idx++) {
          dst_file << logs[idx] << "\t" << session_results[idx] << std::endl;
        }
      }
      last_agentid = agentid;
      logs.clear();
      session_results.clear();
    }
    logs.push_back(line);
  }
  if (last_agentid != "") {
    session_segment::SearchLogSessionMD5(logs, &session_results);
    CHECK_EQ(logs.size(), session_results.size());
    for (size_t idx = 0; idx < logs.size(); idx++) {
      dst_file << logs[idx] << "\t" << session_results[idx] << std::endl;
    }
  }
}

int main(int argc, char **argv) {
  base::InitApp(&argc, &argv, "Prepare data.");
  if (argc < 3) {
    LOG(ERROR) << "Please input src_file and res_file name and log type!";
    return 0;
  }
  if (std::string(argv[3]) == "search_log") {
    ParseSearchLogFile(argv[1], argv[2]);
  } else if (std::string(argv[3]) == "pv_log") {
    ParsePvLogFile(argv[1], argv[2]);
  }

  // SessionSegmentF(FLAGS_src_file, FLAGS_dst_file);
  return 0;
}

