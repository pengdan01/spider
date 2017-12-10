#include "crawler/crawl/rpc_service_controller.h"

#include "base/common/closure.h"
#include "rpc/stumy/rpc_channel.h"

namespace crawler2 {
namespace crawl {
RpcServiceController::~RpcServiceController() {
  if (!stopped_) {
    Stop();
  }
}

void RpcServiceController::Start() {
  CHECK(!listening_);
  service_thread_.Start(NewCallback(this, &RpcServiceController::ServiceProc));

  while (!IsServiceStarted()) {
    usleep(50);
  }

  CHECK(listening_);
}

void RpcServiceController::Stop() {
  CHECK(!stopped_);
  CHECK(listening_);
  server_.Stop();
  service_thread_.Join();
  stopped_ = true;
}

bool RpcServiceController::IsServiceStarted() {
  // 通过 rpc 链接判断是否满足要求
  if (!listening_) {
    return false;
  } else {
    // 如果能够成功链接上，证明服务已经启动
    stumy::RpcChannel channel("127.0.0.1", port_);
    return channel.Connect();
  }
}

void RpcServiceController::ServiceProc() {
  CHECK(!listening_);
  CHECK(server_.ListenOnPort(port_));
  CHECK(server_.ExportService(service_));
  listening_ = true;
  server_.Loop();
}
}  // namespace crawl
}  // namespace crawler2
