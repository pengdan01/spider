#pragma once
#include <vector>
#include <string>
// mid：机器mid值
// product：产品标识
// combo：产品模块标识
// version：产品版本
// server_timestamp:服务器收到数据的UTC时间戳，从1970年1月1日0时0分0秒开始计算的秒数
// proto_version:协议版本号，目前只有v2
// url相关数据的kv对列表：key_id1|base64(v1);key_id2|base64(v2);key_id3|base64(v3) ...
// refer_url相关数据的kv对列表：key_id1|base64(v1);key_id2|base64(v2);key_id3|base64(v3) ... 。注：如果上面url的kv列表中已经包含了refer_url的相关信息，该列就是空串

// 04d5e2e88b74f3bdb5d72014c2d8b643        wd      urlproc 2.8.0.1040      1335282659      2       1|qcyWTw==;0|ac35bc1de674bfd79
// 83691dcdc02ce0f;6|AQA=;4|aHR0cDovL3VzZXIucXpvbmUucXEuY29tLzY4MDQzMD9BRFVJTj0zOTg2MDk2OTgmQURTRVNTSU9OPTEzMzUyMTkwOTYmQURUQUc9Q
// 0xJRU5ULlFRLjQ1MDFfRnJpZW5kVGlwLjAmcHRsYW5nPTIwNTIjIWFwcD00;7|6IW+6K6v576O5aWz4piG5qWa5YS/4piGIC0tIOiFvui

enum {
  kIDCurrentURLMD5            = 0,  // We use 16 bytes hex MD5 string
  kIDWebPageStartTime         = 1,  // measured in the number of seconds since the Unix Epoch (January 1 1970 00:00:00 GMT)
  kIDWebPageDuratrion         = 2,  // measured in seconds
  kIDWebPageOpenedType        = 3,  // enum URLType
  kIDCurrentURL               = 4,
  kIDCurrentURLCharset        = 5,
  kIDCurrentURLAttr           = 6,  // enum URLAttribute, the attribute of current url
  kIDCurrentURLTitle          = 7,
  kIDCurrentURLTitleCharset   = 8,
  kIDQ3WScore                 = 9,
  kIDReferURLMD5              = 100,
  kIDReferURL                 = 101,
  kIDReferURLCharset          = 102,
  kIDAnchorMD5                = 200,
  kIDAnchor                   = 201,
  kIDAnchorCharset            = 202,
  kIDSearchKeywordMD5              = 300,  //The md5 of the search keyword
  kIDSearchKeyword                 = 301,  //The keyword
  kIDSearchEngineType              = 302,  //using enum SearchEngineType, uint8
  kIDSearchKeywordCharset          = 303,  //
  kIDSearchResultWebPageURLMD5     = 304,
  kIDSearchResultWebPageURL        = 305,
  kIDSearchResultWebPageURLCharset = 306,
  kIDSearchResultURLMD5            = 307,
  kIDSearchResultURL               = 308,
};

bool decode_time(const std::string& orig_str, std::string* time_stamp);

bool decode_attr(const std::string& orig_str, std::string* attr);

bool decode_search_type(const std::string& orig_str, std::string* search_type);

