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
  UTWebServer(const std::unordered_map<std::string, std::string>& url_data)
      : pool_(1)
      , url_data_(url_data)
      , serv_(NULL) {
  }

  virtual void OnRequest(bamboo::HTTPRequest *request, bamboo::HTTPResponse *response, Closure *done) {
    ScopedClosure finish(done);
    response->http_status_code = bamboo::HTTPStatusCode::kOK;
    response->keep_alive = request->keep_alive();
    std::string key(request->query().data(),
                    request->query().length());
    ::base::TrimWhitespaces(&key);
    auto iter = url_data_.find(key);
    if (iter != url_data_.end()) {
      const std::string& html_data = iter->second;
      response->data = html_data;
    } else {
      LOG(ERROR) << "not find key: " << key;
      response->data.append("");
    }
  }

  void Start(const bamboo::WebServer::Options& options) {
    CHECK(NULL == serv_);
    serv_ = new ::bamboo::WebServer(options, this);
    thread_.Start(NewCallback(this, &UTWebServer::ServProc));
    while (true) {
      if (serv_->IsRunning()) {
        break;
      }
    }
  }

  void Stop() {
    serv_->Stop();
    thread_.Join();
    pool_.JoinAll();

    if (serv_) {
      delete serv_;
    }
  }

  void ServProc() {
    LOG_IF(FATAL, !serv_->Init()) << "Failed to start serv.";
    serv_->Loop();
  }
 private:
  thread::Thread thread_;
  thread::ThreadPool pool_;
  const std::unordered_map<std::string, std::string>& url_data_;
  bamboo::WebServer* serv_;
};
