#pragma once

#include <string>

#include "crawler/proto2/resource.pb.h"

namespace crawler2 {

struct UrlToFetch {
  std::string url;
  std::string ip;
  int resource_type;  // 定义见 crawler/proto2/resource.proto
  double importance;
  std::string referer;
  bool use_proxy;  // 是否需要使用代理
  int tried_times;  // 已经尝试抓过几次
};

struct UrlFetched {
  UrlToFetch url_to_fetch;

  // 抓取的起止时间
  int64 start_time;
  int64 end_time;

  // 抓取是否成功 (跟 web server 完成 HTTP 会话就算成功)
  bool success;

  // 抓取成功时 (success == true), ret_code 是 http response code
  // 抓取失败时 (success == false), ret_code 是 内部错误编码 (curl 返回)
  // 抓取任务被丢弃时 (success == false), ret_code 是 -1
  //
  // NOTE:
  // 下列情况 url 会被丢弃, ret_code 为 -1:
  // 同一 IP 的 URL 连续失败超限， 该 IP 上所有其他 URL 被丢弃
  int ret_code;
  // 抓取成功时, 值是 http response code (200, 404, 500, 503, etc. 但不会是
  // 3XX, 而是输出 3XX 跳转后的 response code.)
  // 抓取失败时, 值是 fetcher 输出的错误消息
  std::string reason;

  // 如果 url_to_fetch 要求使用代理, 实际使用了哪个代理
  std::string proxy_server;

  // ====================================================
  // 下面的字段, 抓取失败 时不设置, 值为构造时的默认非法值
  // ====================================================
  int redir_times;
  std::string effective_url;

  // 网络传输的下载字节数
  int64 download_bytes;
  // 下载的文件大小 (如果传输过程有压缩, 是解压后文件大小)
  int64 file_bytes;
  // 是否因超过文件大小限制被截断
  bool truncated;

  // 解析出来的 proto buffer
  Resource res;

  UrlFetched() {
    start_time = -1;
    end_time = -1;
    success = false;
    ret_code = 0;
    redir_times = -1;
    download_bytes = -1;
    file_bytes = -1;
    truncated = false;
  }
};

}  // namespace crawler2

