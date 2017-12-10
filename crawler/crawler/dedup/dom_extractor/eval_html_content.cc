#include <iostream>
#include <string>
#include <stack>
#include <sstream>
#include <vector>
#include <fstream>

#include "base/common/base.h"
#include "base/file/file_util.h"
#include "base/file/file_path.h"
#include "base/encoding/line_escape.h"
#include "base/strings/string_util.h"
#include "base/strings/string_split.h"
#include "base/testing/gtest.h"
#include "feature/web/extractor/evaluation/main_content_evaluation.h"
#include "crawler/dedup/dom_extractor/content_collector.h"

DEFINE_string(data_path, "", "");

int main(int argc, char* argv[]) {
  ::base::InitApp(&argc, &argv, "");
  std::vector<std::string> lines;
  if (!base::file_util::ReadFileToLines(
          base::FilePath(FLAGS_data_path), &lines)) {
    LOG(ERROR) << "Failed to read file : " << FLAGS_data_path;
    return -1;
  }

  dom_extractor::ContentCollector content_collector;
  int cnt = 0;
  float average_precision = 0.0f;
  float average_recall = 0.0f;
  std::cout << "precision\trecall\turl" << std::endl;
  for (auto iter = lines.begin(); iter!= lines.end(); ++iter) {
    std::vector<std::string> items;
    ::base::SplitString(*iter, "\t", &items);
    if (items.size() != 2) {
      LOG(ERROR) << "Wrong format line[" << cnt
                 << "], whose size is: " << items.size()
                 << "\t" << *iter;
      continue;
    }

    std::string data;
    if (!base::LineUnescape(items[1], &data)) {
      LOG(ERROR) << "Failed to LineUnescape on url:"
                 << items[0] << std::endl;
      continue;
    }
    feature::LabeledContent content;
    if (!content.ParseFromString(data)) {
      LOG(ERROR) << "Failed to ParseFromString on url: " << items[0];
      continue;
    }

    feature::EvaluationValue eva;
    std::string title;
    std::string content_computed;

    content_collector.ExtractMainContent(content.full_html(), content.url(), &title, &content_computed);
    std::string labeled_data = content.labeled_content();
    std::string all = content.full_html();
    if (!feature::EvaluateMainContent(&labeled_data, &content_computed,
                                      &all, &eva)) {
      LOG(ERROR) << "Failed to Evaluation on url: " << items[0];
      continue;
    }

    /*
    std::cout << content.full_html() << std::endl;
    std::ofstream bad_fout("zhidao6.html");
    bad_fout << content.full_html() << std::endl;
    */

    LOG(INFO) << content.url();
    // LOG(INFO) << "html";
    // LOG(INFO) << content.full_html();
    LOG(INFO) << "labeled_data";
    LOG(INFO) << labeled_data;
    LOG(INFO) << "computed data";
    LOG(INFO) << content_computed;

    std::cout << precision(eva) << "\t" << recall(eva) << "\t"
              << items[0] << std::endl;
    average_precision += precision(eva);
    average_recall += recall(eva);
    cnt++;
  }

  if (cnt) {
    std::cout << average_precision / cnt << "\t"
              << average_recall / cnt << "\t"
              << cnt << std::endl;
  }

  return 0;
}
