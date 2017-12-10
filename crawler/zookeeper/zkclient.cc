/*
 * yumeng 2017/06/30
 */

#include <stdio.h>
#include <assert.h>

#include "spider/crawler/zookeeper/zkclient.h"

DEFINE_string(zk_log, "log/zk.log", " zk的日志所在位置");

void ZkDefaultWatcher(zhandle_t *zzh, int type, int state, const char *path, void *context) {
  // 出现过 context 为 NULL 的情况
  if (context == NULL) {
    LOG(ERROR) << "zk ctx is null";
    return;
  }

  spider::ZKClient *zk_client = (spider::ZKClient *) context;
  zk_client->ProcessEvent(zzh, type, state, path);
}

void spider::ZKClient::ProcessEvent(zhandle_t *zzh, int type, int state, const char *path) {
  if (path && strlen(path) > 0) {
    LOG(INFO) << "zk watcher type=" << type << " state = " << state << " for path: " << path;
  } else {
    LOG(INFO) << "zk watcher type=" << type << " state = " << state;
  }
  this->connected_ = state == ZOO_CONNECTED_STATE;

  if (type != ZOO_SESSION_EVENT) {
    Event evt;
    evt.path = path;
    evt.type = type;
    std::vector<NodeDataWatcher *> watchers;
    {
      thread::AutoLock lock(&mutex_);
      std::vector<NodeDataWatcher *> &list = watchers_[path];
      for (auto it = list.begin(); it != list.end(); ++it) {
        watchers.push_back(*it);
      }
    }
    if (watchers.empty()) {
      return;
    }
    std::string data;
    if (!GetAndWatchNodeData(path, NULL, &data)) {
      LOG(ERROR) << "can not fetch newest data for path: " << path;
      return;
    }
    for (auto it = watchers.begin(); it != watchers.end(); ++it) {
      (*it)->node_data_cb(path, data);
    }
  } else {
    if (state == ZOO_CONNECTED_STATE) {
      LOG(INFO) << "zookeeper connected: " << this->host_;
      this->zhandle_ = zzh;
    } else if (state == ZOO_AUTH_FAILED_STATE) {
      zookeeper_close(zzh);
      this->zhandle_ = NULL;
      LOG(FATAL) << "Authentication failure. shutdown...";
    } else if (state == ZOO_EXPIRED_SESSION_STATE) {
      LOG(INFO) << "Session expired. shutdown and re-init again.";
      zookeeper_close(zzh);
      this->zhandle_ = zookeeper_init(host_.c_str(), ZkDefaultWatcher, this->timeout_, NULL, this, 0);
      if (!zhandle_) {
        LOG(FATAL) << "Can't re-init zk: " << host_;
      }
      // 重新注册各个watcher, 并读取最新的值调用回调
      ReAddWatcher();
    } else {
      LOG(INFO) << "session state: " << state;
    }
  }
}

void spider::ZKClient::ReAddWatcher() {
  // 重新注册各个watcher, 并读取最新的值调用回调
  std::map<std::string, std::vector<NodeDataWatcher *> > tmp_watchers;
  {
    thread::AutoLock lock(&mutex_);
    tmp_watchers = this->watchers_;
  }
  for (auto it = tmp_watchers.begin(); it != tmp_watchers.end(); ++it) {
    const std::string &path = it->first;
    const std::vector<NodeDataWatcher *> &watchers = it->second;
    std::string data;
    if (!GetAndWatchNodeData(path, NULL, &data)) {
      LOG(ERROR) << "can not fetch newest data for path: " << path;
      continue;
    }
    for (auto iit = watchers.begin(); iit != watchers.end(); ++iit) {
      (*iit)->node_data_cb(path, data);
    }
  }
}

spider::ZKClient::ZKClient(const std::string &host, int timeout) :
    host_(host), timeout_(timeout),
    zhandle_(NULL), log_fp_(NULL), connected_(false) {

}
spider::ZKClient::~ZKClient() {
  if (zhandle_) {
    zookeeper_close(zhandle_);
  }
}
bool spider::ZKClient::Init() {
  // log级别
  ZooLogLevel log_level = ZOO_LOG_LEVEL_INFO;
  zoo_set_debug_level(log_level);
  // log目录
  if (!FLAGS_zk_log.empty()) {
    log_fp_ = fopen(FLAGS_zk_log.c_str(), "w");
    if (!log_fp_) {
      LOG(ERROR) << "can not open " << FLAGS_zk_log << " log, so init zk failed";
      return false;
    }
    zoo_set_log_stream(log_fp_);
  }
  zhandle_ = zookeeper_init(host_.c_str(), ZkDefaultWatcher, timeout_, NULL, this, 0);
  if (!zhandle_) {
    LOG(ERROR) << "failed to invoke zookeeper_init";
    return false;
  }
  for (int i = 0; i < (timeout_ + 2000) / 100; i++) {
    if (this->connected_) {
      return true;
    }
    base::SleepForMilliseconds(100);
  }
  LOG(ERROR) << "zookeeper not ready after " << timeout_ << " ms";
  return false;
}

bool spider::ZKClient::GetNodeData(const std::string &path, std::string *node_data) {
  char buf[10240];
  int buf_len = 10240;
  int rc = zoo_wget(zhandle_, path.c_str(), NULL, this, buf, &buf_len, NULL);

  if (rc == ZOK && buf_len >= 0 && buf_len < 1024) {
    buf[buf_len] = '\0';
    *node_data = buf;
    return true;
  } else {
    LOG(ERROR) << "zk read " << path << " error, rc = " << rc;
    return false;
  }
}

bool spider::ZKClient::GetChildren(const std::string &path, std::vector<std::string> *value) {
  struct String_vector strings = {0, NULL};
  int rc = zoo_wget_children(zhandle_, path.c_str(), NULL, this, &strings);
  if (rc == ZOK) {
    for (int i = 0; i < strings.count; ++i) {
      std::string path = strings.data[i];
      value->push_back(path);
    }
    deallocate_String_vector(&strings);
    return true;
  } else {
    return false;
  }
}

bool spider::ZKClient::GetAndWatchNodeData(const std::string &path, NodeDataWatcher *watcher, std::string *node_data) {
  if (watcher != NULL) {
    thread::AutoLock lock(&mutex_);
    std::vector<NodeDataWatcher *> &list = watchers_[path];
    bool already_exist = false;
    for (auto it = list.begin(); it != list.end(); ++it) {
      if (*it == watcher) {
        already_exist = true;
        break;
      }
    }
    if (!already_exist) {
      list.push_back(watcher);
    }
    LOG(INFO) << list.size() << " watchers on path: " << path;
  }
  char buf[10240];
  int buf_len = 10240;
  int rc = zoo_wget(zhandle_, path.c_str(), ZkDefaultWatcher, this, buf, &buf_len, NULL);

  if (rc == ZOK && buf_len >= 0 && buf_len < 10240) {
    buf[buf_len] = '\0';
    *node_data = buf;
    return true;
  } else {
    LOG(ERROR) << "zk read " << path << " error, rc = " << rc;
    return false;
  }
}

bool spider::ZKClient::CreatePersistentNodeIfPresent(const std::string &path, const std::string &data) {
  char buf[1024];
  int buf_len = 1024;

  int rc = zoo_create(zhandle_, path.c_str(), data.c_str(), data.size(), &ZOO_OPEN_ACL_UNSAFE, 0, buf, buf_len);
  if (rc == ZOK) {
    return true;
  } else if (rc == ZNONODE) {
    LOG(ERROR) << "zk create " << path << " error, rc = ZNONODE";
    return false;
  } else if (rc == ZNODEEXISTS) {
    return true;
  } else {
    LOG(ERROR) << "zk create " << path << " error, rc = " << rc;
    return false;
  }
}