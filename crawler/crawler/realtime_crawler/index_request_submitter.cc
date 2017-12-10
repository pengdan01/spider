#include "crawler/realtime_crawler/index_request_submitter.h"

#include "base/thread/thread.h"
#include "base/thread/thread_util.h"
#include "base/common/logging.h"
#include "base/common/closure.h"
#include "base/common/scoped_ptr.h"
#include "rpc/stumy/rpc_channel.h"
#include "rpc/http_counter_export/export.h"

namespace crawler2 {
namespace crawl_queue {

DEFINE_int64_counter(index_submitter, submitted_packets, 0, "提交给实时索引的条目数");
DEFINE_int64_counter(index_submitter, submitted_success_packets, 0, "提交给实时索引的条目数");
DEFINE_int64_counter(index_submitter, connect_failed,  0, "连接失败的次数");
DEFINE_int64_counter(index_submitter, dropped,  0, "连接失败的次数");
DEFINE_int64_counter(index_submitter, resubmit_packets,  0, "cdocs numbers re-submit to queue");

void IndexRequestSubmitter::Start() {
  thread_.Start(::NewCallback(this, &IndexRequestSubmitter::SubmitProc));
}

void IndexRequestSubmitter::Stop() {
  cdocs_.Close();
  thread_.Join();
}

void IndexRequestSubmitter::SubmitProc() {
  thread::SetCurrentThreadName("IndexRequestSubmitter[SubmitProc]");

  serving::ModifyRealTimeIndexRequest* index_request
      = new serving::ModifyRealTimeIndexRequest;
  std::string* cdoc = NULL;
  int total_cdocs = 0;
  int64 last_submit_timestamp = kInt64Max;
  while ((cdoc = cdocs_.Take()) != NULL) {
    scoped_ptr<std::string> auto_cdoc(cdoc);
    index_request->add_cdocs(*cdoc);
    total_cdocs++;

    //  累积太多 cdoc, 需要丢弃, 保证实时性
    if (cdocs_.Size() > options_.max_cdcos_to_push) {
      std::deque<std::string*> cdocs;
      cdocs_.TryMultiTake(&cdocs);
      COUNTERS_index_submitter__dropped.Increase(cdocs.size());
      feature::HTMLInfo info;
      while (!cdocs.empty()) {
        std::string *cdoc = cdocs.front();
        // XXX(pengdan): 将丢弃的 cdoc 的 url 打印到 INFO
        info.Clear();
        if (info.ParseFromString(*cdoc)) {
          LOG(INFO) << info.source_url() << " is droped, too many cdocs";  
        } else {
          LOG(INFO) << "Doc(PF) exceed " << options_.max_cdcos_to_push << " and droped";  
        }
        delete cdoc;
        cdocs.pop_front();
      }
    }

    // 检查是否达到上限
    if (index_request->cdocs_size() % options_.check_frequency != 0) {
      continue;
    }
    // 当满足如下三个条件之一时, 推送 cdoc 给 下游:
    // 1. 当请求的 byte size >= |max_cdocs_buffer|;
    // 2. 当请求的 codoc 数 >= |max_cdocs_num|
    // 3. 当距离上一次推送提交时间间隔 >= |max_submit_interval * 1000 * 1000| 
    if (index_request->ByteSize() < options_.max_cdocs_buffer
        && index_request->cdocs_size() < options_.max_cdocs_num
        && (base::GetTimestamp() - last_submit_timestamp) < options_.max_submit_interval * 1000 * 1000) {
      continue;
    }

    int retry_times = 0;
 retry:
    index_server_id_ = (index_server_id_ + 1) % options_.servers.size();
    const IndexerInfo& serv = options_.servers[index_server_id_];
    // 尝试链接
    stumy::RpcChannel* channel = new stumy::RpcChannel(serv.ip, serv.port);
    if (!channel->Connect()) {
      LOG(ERROR) << "Failed to connect[" << serv.ip << ":" << serv.port <<"]";
      COUNTERS_index_submitter__connect_failed.Increase(1);
      retry_times++;
      delete channel;

      // 如果尝试次数过多，则不再重试
      if (retry_times > options_.connect_retry_times) {
        continue;
      }
      sleep(1);
      goto retry;
    }

    index_request->set_command(serving::kAddCDocs);
    serving::BaseSearch::Stub* stub = new serving::BaseSearch::Stub(channel);
    stumy::RpcClientController* rpc = new stumy::RpcClientController;
    // 设置 rpc 的超时
    rpc->SetDeadline(120);  // 120s

    serving::ModifyRealTimeIndexResponse* index_response
        = new serving::ModifyRealTimeIndexResponse;
    LOG(INFO) << "submit request to leaf_server(" << serv.ip << ":"<< serv.port << ") with "
              << index_request->cdocs_size() << " docs with bytes["
              << index_request->ByteSize() << "] to " << "[" << serv.ip << ":"
              << serv.port <<"]" << " total cdocs: " << total_cdocs;

    ClosureDoneParam param(rpc, channel, stub, index_request, index_response, serv.ip);
    Closure* done = NewCallback(this, &IndexRequestSubmitter::SubmitDone, param);

    COUNTERS_index_submitter__submitted_packets.Increase(index_request->cdocs_size());
    stub->ModifyRealTimeIndex(rpc, index_request, index_response, done);
    // 记录推送 cdoc 的时间点
    last_submit_timestamp = base::GetTimestamp();

    index_request = new serving::ModifyRealTimeIndexRequest;
  }
}

void IndexRequestSubmitter::SubmitDone(ClosureDoneParam param) {
  if (param.rpc->status() == stumy::RpcController::kOk && param.response->success()) {
    LOG(INFO) << "submit to leaf_server(" << param.server_ip << ") successfully";
    COUNTERS_index_submitter__submitted_success_packets.Increase(param.request->cdocs_size());
  } else {
    LOG(INFO)<< "submit to leaf_server(" << param.server_ip << ") failed(will retry), because: "
             << param.response->reason() << ", rpc status: " << param.rpc->error_text();
    // 重新灌回
    for (int i = 0; i < (int)param.request->cdocs_size(); ++i) {
      const std::string &cdoc = param.request->cdocs(i);
      AddCDoc(cdoc);
    }
    COUNTERS_index_submitter__resubmit_packets.Increase(param.request->cdocs_size());
    // 对于重试的 cdoc, 不重复计算
    COUNTERS_index_submitter__submitted_packets.Decrease(param.request->cdocs_size());
  }
  // Close the channel
  param.channel->Close();

  delete param.rpc;
  delete param.request;
  delete param.response;
  delete param.stub;
  delete param.channel;
}

void IndexRequestSubmitter::AddCDoc(const std::string& str_cdoc) {
  std::string* cdoc = new std::string(str_cdoc);
  cdocs_.Put(cdoc);
}


void ParseIndexSubmitterConfig(const extend::config::ConfigObject& config,
                               IndexRequestSubmitter::Options* options) {
  options->check_frequency = config["check_frequency"].value.integer;
  options->max_cdocs_buffer = config["max_cdoc_buffer"].value.integer;
  options->max_cdocs_num = config["max_cdocs_num"].value.integer;
  options->max_submit_interval = config["max_submit_interval"].value.integer;
  options->max_cdcos_to_push = config["max_cdcos_to_push"].value.integer;

  const extend::config::ConfigObject& obj = config["leaf_server_list"];
  for (size_t i = 0; i < obj.value.array->size(); ++i) {
    const extend::config::ConfigObject* item = obj.value.array->at(i);
    IndexRequestSubmitter::IndexerInfo info;
    info.ip = (*item)["ip"].value.string;
    info.port = (*item)["port"].value.integer;
    options->servers.push_back(info);
  }
}

void ParseIndexSubmitterConfig2(const extend::config::ConfigObject& config,
                                IndexRequestSubmitter::Options* options) {
  options->check_frequency = config["check_frequency"].value.integer;
  options->max_cdocs_buffer = config["max_cdoc_buffer"].value.integer;
  options->max_cdocs_num = config["max_cdocs_num"].value.integer;
  options->max_submit_interval = config["max_submit_interval"].value.integer;
  options->max_cdcos_to_push = config["max_cdcos_to_push"].value.integer;

  const extend::config::ConfigObject& obj = config["leaf_server_list2"];
  for (size_t i = 0; i < obj.value.array->size(); ++i) {
    const extend::config::ConfigObject* item = obj.value.array->at(i);
    IndexRequestSubmitter::IndexerInfo info;
    info.ip = (*item)["ip"].value.string;
    info.port = (*item)["port"].value.integer;
    options->servers.push_back(info);
  }
}

}  // namespace crawl_queue
}  // namespace cralwer2
