// 简单用于测试的 fetcher
#pragma once

#include <string>
#include <deque>

#include "crawler/fetcher2/fetcher.h"
#include "crawler/fetcher2/load_controller.h"

class SimpleFetcher {
 public:
  explicit SimpleFetcher(const crawler2::LoadController::Options& controller_options);
  SimpleFetcher();
  ~SimpleFetcher();
  void Init(const char* ip_control);
  void Fetch(std::deque<crawler2::UrlToFetch>* urls_to_fetch, crawler2::Fetcher::FetchedUrlCallback*);
 private:
  crawler2::LoadController::Options controller_options_;
  crawler2::LoadController load_controller_;
  crawler2::Fetcher::Options fetcher_options_;
  crawler2::Fetcher* fetcher_;
};
