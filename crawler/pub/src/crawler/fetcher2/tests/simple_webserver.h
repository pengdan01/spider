#pragma once

#include <string>

#include "base/common/closure.h"
#include "base/thread/thread_pool.h"
#include "extend/web_server/web_server.h"
#include "extend/web_server/service_handler.h"
#include "extend/web_server/server_option.h"

class SimpleWebServer : public ::bamboo::ServiceHandler {
 public:
  SimpleWebServer() : pool_(1) {
  }

  virtual void Handle(bamboo::HTTPRequest* request,
                      bamboo::HTTPResponse* response,
                      Closure* done) {
    response->keep_alive = request->keep_alive();
    pool_.AddTask(NewCallback(this, &SimpleWebServer::Done, done));
  }

  void Done(Closure* done) {
    ScopedClosure finish(done);
  }

  void Start(const bamboo::ServerOption& option) {
    option_ = option;
    thread_.Start(NewCallback(this, &SimpleWebServer::ServProc));
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
    serv_.Init(option_);
    LOG_IF(FATAL, !serv_.Loop(this)) << "Failed to start serv.";
  }
 private:
  std::string confpath_;
  bamboo::ServerOption option_;
  bamboo::WebServer serv_;
  thread::Thread thread_;
  thread::ThreadPool pool_;
};
