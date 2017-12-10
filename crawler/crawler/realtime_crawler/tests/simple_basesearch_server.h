#pragma once

#include <cstdatomic>

#include "serving/serving_proto/serving.pb.h"
#include "rpc/stumy/stumy_server.h"
#include "base/thread/sync.h"
#include "base/common/closure.h"

class CDocNumBaseSearchImpl : public serving::BaseSearch {
 public:
  CDocNumBaseSearchImpl()
      : cdocs_count_(0) {
  }

  virtual void ModifyRealTimeIndex(stumy::RpcController* controller,
                                   const serving::ModifyRealTimeIndexRequest* request,
                                   serving::ModifyRealTimeIndexResponse* response,
                                   Closure* done) {
    ScopedClosure auto_closure(done);
    ::thread::AutoLock lock(&mutex_);
    response->set_success(true);
    cdocs_count_ += request->cdocs_size();
  }

  int cdocs_count() const { return cdocs_count_;}
 private:
  std::atomic<int> cdocs_count_;
  ::thread::Mutex mutex_;
};

