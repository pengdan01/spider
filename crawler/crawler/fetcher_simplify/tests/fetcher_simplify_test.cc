#include "crawler/fetcher_simplify/fetcher_simplify.h"

#include "base/testing/gtest.h"
#include "base/common/basic_types.h"
#include "base/common/sleep.h"
#include "base/common/base.h"
#include "base/time/time.h"
#include "base/file/file_util.h"

namespace crawler2 {
/*
TEST(FetcherSimplifyTest, UrlsFromFile) {
  const std::string test_url_file("crawler/fetcher_simplify/data/fetcher_simplify_test_url.txt"); 
  std::vector<std::string> urls;
  CHECK(base::file_util::ReadFileToLines(test_url_file, &urls));

  FetcherSimplify s_fetcher;
  BlockingQueue<UrlToFetch> *input_q = s_fetcher.GetRequestQueue();
  BlockingQueue<UrlFetched> *output_q = s_fetcher.GetResponseQueue();

  // Add urls to in queue
  for (int i = 0; i < (int)urls.size(); ++i) {
    UrlToFetch task;
    task.url = urls[i]; 
    task.resource_type = 1;
    task.use_proxy = false;
    input_q->Put(task);
    input_q->Put(task);
  }
  // XXX(pengdan): 测试时关闭掉 队列, 否则: s_fetcher.RunLoop() 一直执行,不会退出
  input_q->Close();

  // 进入 io 循环
  // 由于已经关闭了 队列, 所以抓取完后, 会退出
  s_fetcher.RunLoopBatch();

  UrlFetched out;
  int ret = 0;
  while (1==(ret = output_q->TryTake(&out))) {
    LOG(ERROR) << out.http_response_code;
    LOG(ERROR) << "url: " << out.url_to_fetch.url;
    LOG(ERROR) << "header size: " << out.header_raw.size();
    LOG(ERROR) << "page size: " << out.page_raw.size();
  }
}
*/

TEST(FetcherSimplifyTest, UrlsFromFileNotBatch) {
  const std::string test_url_file("crawler/fetcher_simplify/data/fetcher_simplify_test_url.txt"); 
  std::vector<std::string> urls;
  CHECK(base::file_util::ReadFileToLines(test_url_file, &urls));

  FetcherSimplify s_fetcher;
  BlockingQueue<UrlToFetch> *input_q = s_fetcher.GetRequestQueue();
  BlockingQueue<UrlFetched> *output_q = s_fetcher.GetResponseQueue();

  // Add urls to in queue
  for (int i = 0; i < (int)urls.size(); ++i) {
    UrlToFetch task;
    task.url = urls[i]; 
    task.resource_type = 1;
    task.use_proxy = false;
    input_q->Put(task);
  }
  // XXX(pengdan): 测试时关闭掉 队列, 否则: s_fetcher.RunLoop() 一直执行,不会退出
  input_q->Close();

  // 进入 io 循环
  // 由于已经关闭了 队列, 所以抓取完后, 会退出
  s_fetcher.RunLoop();

  UrlFetched out;
  int ret = 0;
  while (1==(ret = output_q->TryTake(&out))) {
    LOG(ERROR) << "response code: " << out.http_response_code << ", url: " << out.url_to_fetch.url;
    LOG(ERROR) << "header size: " << out.header_raw.size();
    LOG(ERROR) << "page size: " << out.page_raw.size();
    //LOG(ERROR) << "header size: " << out.header_raw;
    //LOG(ERROR) << "page size: " << out.page_raw;
  }
}

}
