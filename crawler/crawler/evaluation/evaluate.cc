#include <string>
#include "base/common/base.h"
#include "crawler/evaluation/evaluation.h"

DEFINE_string(topn_url, "", "");
DEFINE_string(topn_sites, "", "");
DEFINE_string(statistic, "", "");
DEFINE_string(website_report, "", "");
DEFINE_string(url_report, "", "");

int main(int argc, char *argv[]) {
  ::base::InitApp(&argc, &argv, "");
  crawler::Evaluation evaluation;
  evaluation.Init(FLAGS_topn_url, FLAGS_topn_sites);
  evaluation.Evaluate(FLAGS_url_report);
  if (!FLAGS_url_report.empty()) {
    evaluation.SaveUrlReport(FLAGS_url_report);
  }

  if (!FLAGS_website_report.empty())  {
    evaluation.SaveWebsiteReport(FLAGS_website_report);
  }
  return 0;
}
