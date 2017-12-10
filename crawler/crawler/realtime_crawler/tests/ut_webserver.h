#pragma once

#include <string>
#include <unordered_map>

#include "base/common/closure.h"
#include "base/thread/thread_pool.h"
#include "base/strings/string_util.h"
#include "extend/web_server/web_server.h"
#include "extend/web_server/service_handler.h"

class UTWebServer : public ::bamboo::ServiceHandler {
 public:
  UTWebServer(const std::unordered_map<std::string, std::string>& url_data,
              const bamboo::WebServer::Options &options)
      : pool_(1)
      , serv_(options, this)
      , url_data_(url_data) {
  }

  virtual void OnRequest(bamboo::HTTPSession* session, Closure* done) {
    ScopedClosure finish(done);
    bamboo::HTTPRequest &request = session->request;
    bamboo::HTTPResponse &response = session->response;
    response.keep_alive = request.keep_alive();
    std::string key(request.query().data(), request.query().length());
    ::base::TrimWhitespaces(&key);
    auto iter = url_data_.find(key);
    if (iter != url_data_.end()) {
      const std::string& html_data = iter->second;
      response.data = html_data;
    } else {
      response.data.append("");
    }
  }

  void Start() {
    LOG_IF(FATAL, !serv_.Init()) << "Failed to start serv.";
    thread_.Start(NewCallback(this, &UTWebServer::ServProc));
    while (true) {
      if (serv_.IsRunning()) {
        break;
      }
    }
  }

  void Stop() {
    serv_.Stop();
    thread_.Join();
    pool_.JoinAll();
  }

  void ServProc() {
    serv_.Loop();
  }
 private:
  std::string confpath_;
  thread::ThreadPool pool_;
  bamboo::WebServer serv_;
  thread::Thread thread_;
  const std::unordered_map<std::string, std::string>& url_data_;
};
