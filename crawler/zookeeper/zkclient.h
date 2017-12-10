/*
 * yumeng 06/30/2017
 */

#ifndef ZK_ZKCLIENT_H_
#define ZK_ZKCLIENT_H_

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <map>
#include <set>
#include <vector>

#include "base/thread/sync.h"
#include "base/common/sleep.h"
#include "base/common/gflags.h"
#include "base/common/logging.h"

#include "third_party/zookeeper/include/zookeeper.h"

namespace spider {

class NodeDataWatcher {
 public:

  /**
   * 节点数据变化的callback接收方法，不一定真的会变，但是zk如果监控到事件，会回调一下
   *
   * @param path
   * @param data
   */
  virtual void node_data_cb(const std::string &path, const std::string &data) = 0;

  virtual ~NodeDataWatcher() {}
};

/**
 * zk 事件定义
 */
struct Event {
  // zk 路径
  std::string path;
  // 事件类型，具体见 zk 定义
  int type;
};

/**
 *	对c语言版的zookeeper做一个简单的封装，简单一点
 *
 */
class ZKClient {
 public:
  ZKClient(const std::string &host, int timeout = 5000);

  ~ZKClient();

  /**
   * 在使用此类前，必须先调用此方法初始化一下，会返回是否初始化成功
   *
   * @return
   */
  bool Init();

  /**
   * 处理zookeeper的各种事件信息
   *
   * @param type
   * @param state
   * @param path
   */
  void ProcessEvent(zhandle_t *zzh, int type, int state, const char *path);

  /**
   * 同步获取节点数据
   *
   * @return
   */
  bool GetNodeData(const std::string &path, std::string *node_data);

  /**
   * 同步获取节点的子列表
   *
   * @param path
   * @param value
   * @return
   */
  bool GetChildren(const std::string &path, std::vector<std::string> *value);

  /**
   * 时刻监控着一个节点的数据变化
   *
   * @param path
   * @param watcher
   */
  bool GetAndWatchNodeData(const std::string &path, NodeDataWatcher *watcher, std::string *node_data);

  /**
   * 创建一个持久的节点, 如果节点已存在，也返回true
   *
   * @param path
   * @param data
   * @return
   */
  bool CreatePersistentNodeIfPresent(const std::string &path, const std::string &data);

 private:
  void ReAddWatcher();

 private:

  std::string host_;
  int timeout_;

  zhandle_t *zhandle_;
  FILE* log_fp_;

  volatile bool connected_;
  // mutex for some lock
  thread::Mutex mutex_;

  // 所有监听数据变更的监听器列表
  std::map<std::string, std::vector<NodeDataWatcher*> > watchers_;
};
}

#endif /* ZK_ZKCLIENT_H_ */
