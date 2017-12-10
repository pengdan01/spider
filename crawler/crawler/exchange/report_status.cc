#include <vector>
#include <string>

#include "base/strings/string_split.h"
#include "base/strings/string_printf.h"
#include "base/time/time.h"
#include "base/strings/string_number_conversions.h"
#include "crawler/exchange/report_status.h"

namespace crawler {
bool LoadReportStatusFromString(ReportStatus* status, const std::string& str) {
  if (status == NULL) {
    return false;
  }
  // input format:
  // url \t ip \t start_time \t end_time \ success_or_not \t reason \t redir_times \t effective_url \t payload
  std::vector<std::string> fields;
  base::SplitString(str, "\t", &fields);
  if (kReportStatusNum != fields.size()) {
    return false;
  }
  int64 start_timepoint, end_timepoint;
  int redir_times;
  if (!base::StringToInt64(fields[2], &start_timepoint)) {
    return false;
  }
  if (!base::StringToInt64(fields[3], &end_timepoint)) {
    return false;
  }
  if (!base::StringToInt(fields[6], &redir_times)) {
    return false;
  }
  if (fields[4] != "T" && fields[4] != "F") {
    return false;
  }
  status->url = fields[0];
  status->ip = fields[1];
  status->start_time = start_timepoint;
  status->end_time = end_timepoint;
  status->success = (fields[4] == "T") ? true : false;
  status->reason = fields[5];
  status->redir_times = redir_times;
  status->effective_url = fields[7];
  status->pay_load = fields[8];

  return true;
}

std::string SerializeReportStatus(const ReportStatus &status) {
  char success_flag;
  if (status.success == true) {
    success_flag = 'T';
  } else {
    success_flag = 'F';
  }
  std::string begin_time_string, end_time_string;
  base::Time::FromDoubleT(status.start_time / 1000.0 / 1000).ToStringInMicroseconds(&begin_time_string);
  base::Time::FromDoubleT(status.end_time / 1000.0 / 1000).ToStringInMicroseconds(&end_time_string);

  std::string output = base::StringPrintf("%s\t%s\t%s\t%s\t%c\t%s\t%d\t%s\t%s", status.url.c_str(),
                                          status.ip.c_str(),
                                          begin_time_string.c_str(), end_time_string.c_str(), success_flag,
                                          status.reason.c_str(), status.redir_times,
                                          status.effective_url.c_str(), status.pay_load.c_str());
  return output;
}

}  // namespace crawle
