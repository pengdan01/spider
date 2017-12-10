
#pragma once

#include "base/common/logging.h"
#include "base/thread/thread.h"
#include "rpc/stumy/stumy_server.h"
#include "rpc/stumy/service.h"

namespace crawler2 {
namespace crawl {

// 此类实现了一个多线程 rpc 控制的实现
// 在主线程中调用 RpcServiceController 的 Start
// 服务将启动但不会被 Block 住， 除非用户显示调用 Loop()
class RpcServiceController {
 public:
  RpcServiceController(stumy::Service* service, int port)
      : service_(service)
      , port_(port)
      , stopped_(false)
      , listening_(false) {
    CHECK_GT(port_, 0);
    CHECK_NOTNULL(service_);
  }

  ~RpcServiceController();

  void Start();
  void Stop();
  bool IsServiceStarted();
 private:
  void ServiceProc();

  ::thread::Thread service_thread_;
  ::stumy::Service* service_;
  ::stumy::StumyServer server_;
  int port_;
  std::atomic<bool> stopped_;
  std::atomic<bool> listening_;

  friend class ::thread::Thread;
  DISALLOW_COPY_AND_ASSIGN(RpcServiceController);
};
}  // namespace crawl
}  // namespace crawler2
