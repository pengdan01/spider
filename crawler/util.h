#pragma once

//
// Created by yumeng on 2017/3/19.
//

#include <string>
#include <list>
#include <set>
#include <map>
#include <vector>
#include <sstream>

#include "base/random/pseudo_random.h"
#include "base/strings/string_split.h"
#include "crawler/proto2/resource.pb.h"
#include "crawler/realtime_crawler/crawl_queue.pb.h"

namespace spider {

// 将一个集合类型转成字符串
template<typename COLLECTION>
std::string Collection2String(COLLECTION col) {
  std::ostringstream oss;
  oss << "[";
  int pos = 0;
  for (auto iter = col.begin(); iter != col.end(); ++iter) {
    if (pos > 0) {
      oss << ", ";
    }
    pos++;
    oss << (*iter);
  }
  oss << "]";
  return oss.str();
}

// 将一个集合类型转成字符串
template<typename COLLECTION>
std::string Map2String(COLLECTION col) {
  std::ostringstream oss;
  oss << "[";
  int pos = 0;
  for (auto iter = col.begin(); iter != col.end(); ++iter) {
    if (pos > 0) {
      oss << ", ";
    }
    pos++;
    oss << iter->first << ":" << iter->second;
  }
  oss << "]";
  return oss.str();
}

// 从一组代理列表里，随机选一个代理出来，排除掉except_proxy
std::string SelectRandProxy(crawler2::crawl_queue::JobInput &job);

} // namespace spider
