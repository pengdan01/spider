#pragma once

#include <string>
#include <vector>

#include "base/common/base.h"
#include "base/common/basic_types.h"
#include "base/thread/thread.h"
#include "base/thread/blocking_queue.h"
#include "base/random/pseudo_random.h"
#include "base/time/timestamp.h"
#include "extend/config/config.h"
#include "serving/serving_proto/serving.pb.h"
#include "rpc/stumy/stumy_server.h"
#include "feature/web/page/html_features.pb.h"

namespace crawler2 {
namespace crawl_queue {

class IndexRequestSubmitter {
 public:
  struct IndexerInfo {
    std::string ip;
    int port;
  };

  struct Options {
    std::vector<IndexerInfo> servers;
    int check_frequency;
    int64 max_cdocs_buffer;
    int64 max_cdocs_num;
    int max_submit_interval;
    int64 max_cdcos_to_push;
    int64 connect_retry_times;
    Options()
        : check_frequency(10)
        , max_cdocs_buffer(1024 * 1024 * 20)
        , max_cdocs_num(4000)
        , max_submit_interval(300)
        , max_cdcos_to_push(200000)
        , connect_retry_times(5) {
    }
  };

  struct ClosureDoneParam {
    stumy::RpcClientController* rpc;
    stumy::RpcChannel* channel;
    serving::BaseSearch::Stub* stub;
    serving::ModifyRealTimeIndexRequest* request;
    serving::ModifyRealTimeIndexResponse* response;
    std::string server_ip;
    ClosureDoneParam(stumy::RpcClientController* t_rpc,
                     stumy::RpcChannel* t_channel,
                     serving::BaseSearch::Stub* t_stub, 
                     serving::ModifyRealTimeIndexRequest* t_request,
                     serving::ModifyRealTimeIndexResponse* t_response,
                     const std::string &t_server_ip){
      rpc = t_rpc;
      channel = t_channel;
      stub = t_stub;
      request = t_request;
      response = t_response;
      server_ip = t_server_ip;
    }
  };

  explicit IndexRequestSubmitter(const Options& options)
      : options_(options) {
    CHECK_GT(options.servers.size(), 0u);
    CHECK_GT(options.check_frequency, 0);
    base::PseudoRandom pr(base::GetTimestamp());
    index_server_id_ = pr.GetInt(0, options.servers.size()-1); 
  }

  void AddCDoc(const std::string& cdoc);
  void Start();
  void Stop();
 private:
  void SubmitProc();
  void SubmitDone(ClosureDoneParam param);

  ::thread::Thread thread_;
  ::thread::BlockingQueue<std::string*> cdocs_;
  Options options_;
  // XXX(pengdan): 存放当前正在使用的 index server id
  int index_server_id_;
};

void ParseIndexSubmitterConfig(const extend::config::ConfigObject& config,
                               IndexRequestSubmitter::Options* options);
void ParseIndexSubmitterConfig2(const extend::config::ConfigObject& config,
                                IndexRequestSubmitter::Options* options);
}  // namespace crawl_queue
}  // namespace cralwer2
