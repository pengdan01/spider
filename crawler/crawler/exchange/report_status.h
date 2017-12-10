#pragma once

#include <string>
#include "base/common/basic_types.h"

namespace crawler {
struct ReportStatus {
  std::string url;
  std::string ip;
  int64 start_time;
  int64 end_time;
  bool success;
  std::string reason;
  int32 redir_times;
  std::string effective_url;
  std::string pay_load;

  ReportStatus()
      : start_time(0)
      , end_time(0)
      , success(false)
      , redir_times(0) {
  }
};

const unsigned int kReportStatusNum = 9;

bool LoadReportStatusFromString(ReportStatus* stauts, const std::string& str);
std::string SerializeReportStatus(const ReportStatus& status);
}  // namespace crawler
