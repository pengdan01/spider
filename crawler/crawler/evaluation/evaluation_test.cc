#include <string>

#include "base/testing/gtest.h"
#include "base/common/base.h"
#include "crawler/evaluation/evaluation.h"


TEST(CrawlerEvaluation, simple) {
  ReportStatus status[] = {
    {"http://www.sina.com/a", 0, 0, false, "", 0, "", ""},
    {"http://www.sina.com/a", 0, 0, false, "", 0, "", ""},
    {"http://www.sina.com/a", 0, 0, false, "", 0, "", ""},
    {"http://www.sina.com/a", 0, 0, true, "", 0, "", ""},
    {"http://www.sina.com/b", 0, 0, true, "", 0, "", ""},
  };

  
}
