//
// Created by yumeng on 2017/6/30.
//

#include <iostream>

#include "serving_base/utility/signal.h"
#include "spider/crawler/zookeeper/zkclient.h"

using namespace spider;
using namespace std;

class CbCb : public spider::NodeDataWatcher {
  void node_data_cb(const std::string &path, const std::string &data) {
    cout << "Found " << path << ", data: " << data << endl;
  }
};

int main(int argc, char **argv) {
  base::InitApp(&argc, &argv, "zk_test_bin");

  ZKClient zkClient("100.81.1.68:2181,100.81.2.180:2181,100.81.4.141:2181");

  bool ret = zkClient.Init();
  cout << "init zk client ret=" << ret << endl;

  ret = zkClient.CreatePersistentNodeIfPresent("/yumeng_test", "lalala");
  cout << "create /yumeng_test ret=" << ret << endl;

  std::string data;
  ret = zkClient.GetNodeData("/yumeng_test", &data);
  cout << "get /yumeng_test ret=" << ret << ", data=" << data << endl;

  data.clear();
  CbCb nodeDataWatcher;
  ret = zkClient.GetAndWatchNodeData("/yumeng_test", &nodeDataWatcher, &data);
  cout << "getAndWatch /yumeng_test ret=" << ret << ", data=" << data << endl;

  while (serving_base::SignalCatcher::GetSignal() < 0) {
    base::SleepForSeconds(1);
  }
}