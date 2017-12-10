//
// Created by yumeng on 2017/3/18.
//

#include <unistd.h>
#include "spider/crawler/kafka/kafka_reader.h"
#include "storage/base/process_util.h"
#include "base/strings/string_number_conversions.h"

DECLARE_int64_counter(kafka_client, total_kafka_read_cnt);
DEFINE_int32(kafka_max_timeout_times, 10, "max continuous kafka timeout times");

spider::KafkaReader::KafkaReader(const std::string &kafka_brokers,
                                 const std::set<std::string> &topics,
                                 const std::string &consumer_group_name,
                                 const std::string &kafka_assignor,
                                 const bool kafka_seek_end) {
  this->kafka_brokers_ = kafka_brokers;
  this->kafka_assignor_ = kafka_assignor;
  this->topics_.clear();
  for (auto iter = topics.begin(); iter != topics.end(); ++iter) {
    this->topics_.push_back(*iter);
  }
  this->consumer_group_name_ = consumer_group_name;
  this->continuous_timeout_times_.store(0);
  this->is_stopped_.store(false);
  this->consumer_start_time_ = base::GetTimestamp() / 1000;
  this->kafka_seek_end_ = kafka_seek_end;
}

spider::KafkaReader::KafkaReader(const std::string &kafka_brokers,
                                 const std::string &topic,
                                 const std::string &consumer_group_name,
                                 const std::string &kafka_assignor,
                                 const bool kafka_seek_end) {
  this->kafka_brokers_ = kafka_brokers;
  this->kafka_assignor_ = kafka_assignor;
  this->topics_.clear();
  this->topics_.push_back(topic);
  this->consumer_group_name_ = consumer_group_name;
  this->continuous_timeout_times_.store(0);
  this->is_stopped_.store(false);
  this->consumer_start_time_ = base::GetTimestamp() / 1000;
  this->kafka_seek_end_ = kafka_seek_end;
}

bool spider::KafkaReader::Init() {
  if (!CreateConf()) {
    if (this->conf_ != NULL) {
      delete this->conf_;
      this->conf_ = NULL;
    }
    if (this->tconf_ != NULL) {
      delete this->tconf_;
      this->tconf_ = NULL;
    }
    return false;
  }
  std::string err_str;
  this->consumer_ = RdKafka::KafkaConsumer::create(this->conf_, err_str);
  if (this->consumer_ == NULL) {
    LOG(FATAL) << "Failed to create consumer , brokers: " << this->kafka_brokers_ << ", error: " << err_str;
    return false;
  }
  LOG(INFO) << "Succ to create kafka consumer: " << consumer_->name() << ", brokers: " << this->kafka_brokers_
            << ", kafka_seek_end: " << this->kafka_seek_end_ << ", start at: " << this->consumer_start_time_;
  RdKafka::ErrorCode err = this->consumer_->subscribe(this->topics_);
  if (err) {
    LOG(ERROR) << "Failed to subscribe topics: " << Collection2String(this->topics_) << ", error: "
               << RdKafka::err2str(err);
    return false;
  }
  LOG(INFO) << "Succ to subscribe topics: " << Collection2String(this->topics_) + ", consumer group name: "
            << this->consumer_group_name_;
  return true;
}

bool spider::KafkaReader::Close() {
  if (this->consumer_ == NULL) {
    return true;
  }
  // consumer_->commitSync();
  // consumer_->unsubscribe();
  // 最后poll一下，把该触发的事件都触发掉，防止死锁关闭不上
  // consumer_->poll(0);
  LOG(INFO) << "Begin to close kafka consumer topics: " << Collection2String(this->topics_) << " consumer: "
            << this->consumer_group_name_;
  RdKafka::ErrorCode err = this->consumer_->close();
  RdKafka::wait_destroyed(5000);
  if (err) {
    LOG(ERROR) << "Failed to real close kafka consumer topics: " << Collection2String(this->topics_) << ", error: "
               << RdKafka::err2str(err);
    return false;
  }
  LOG(INFO) << "Succ to close kafka consumer topics: " << Collection2String(this->topics_);
  delete this->consumer_;
  this->consumer_ = NULL;
  return true;
}

void spider::KafkaReader::SetStopped() {
  this->is_stopped_.store(true);
}

bool spider::KafkaReader::IsStopped() {
  return this->is_stopped_.load();
}

RdKafka::Message *spider::KafkaReader::Fetch() {
  // 如果Close以后consumer_指标应该是指向NULL的
  while (this->consumer_ != NULL && !this->is_stopped_.load()) {
    bool need_sleep = true;
    RdKafka::Message *msg = this->Consume(need_sleep);
    if (msg != NULL) {
      // 如果设定了要seek to end，而消息的时间又比启动时间早，说明消息是老消息，干掉
      if (this->kafka_seek_end_ == true && msg->timestamp().timestamp < this->consumer_start_time_) {
        // 一定要释放msg，否则内存泄漏啦
        delete msg;
        COUNTERS_kafka_client__total_kafka_skip_cnt.Increase(1);
        continue;
      }
      COUNTERS_kafka_client__total_kafka_read_cnt.Increase(1);
      return msg;
    }
    // 如果读不到消息，并且需要暂停一会，那就sleep再继续读取
    if (need_sleep) {
      base::SleepForSeconds(1);
    }
  }
  // 已关闭kafka reader，所以返回NULL
  return NULL;
}

RdKafka::Message *spider::KafkaReader::Consume(bool &need_sleep) {
  need_sleep = true;
  RdKafka::Message *msg = consumer_->consume(60000);
  if (msg == NULL) {
    return NULL;
  }

  switch (msg->err()) {
    case RdKafka::ERR__TIMED_OUT: {
      this->continuous_timeout_times_.fetch_add(1);
      LOG(INFO) << "Kafka consume timeout for " << this->continuous_timeout_times_.load()
                             << " times, topic: "
                             << Collection2String(this->topics_) + ", consumer group name: "
                             << this->consumer_group_name_;
      delete msg;
      if (this->continuous_timeout_times_.load() >= FLAGS_kafka_max_timeout_times) {
        LOG(FATAL) << "Kafka consumer continuous timeout times exceed " << FLAGS_kafka_max_timeout_times
                   << " times, topic: " << Collection2String(this->topics_) + ", consumer group name: "
                   << this->consumer_group_name_;
      }
      return NULL;
    }

    case RdKafka::ERR_NO_ERROR: {
      this->continuous_timeout_times_.store(0);
      need_sleep = false;
      return msg;
    }

    case RdKafka::ERR__PARTITION_EOF: {
      LOG(INFO) << "Kafka EOF reached for topic: " << msg->topic_name() << "-" << msg->partition() << ", offset: "
                << msg->offset();
      this->continuous_timeout_times_.store(0);
      delete msg;
      need_sleep = false;
      return NULL;
    }

    case RdKafka::ERR__UNKNOWN_TOPIC:
    case RdKafka::ERR__UNKNOWN_PARTITION: {
      LOG(FATAL)
          << "Kafka unknown topic or partition: " << msg->topic_name() << "-" << msg->partition();
      this->continuous_timeout_times_.store(0);
      delete msg;
      return NULL;
    }

    default: {
      LOG(FATAL) << "Consume failed, " << msg->errstr() << " for " << msg->topic_name() << "-" << msg->partition();
      this->continuous_timeout_times_.store(0);
      delete msg;
      return NULL;
    }
  }
}

spider::KafkaReader::~KafkaReader() {
  Close();
  if (this->consumer_ != NULL) {
    delete this->consumer_;
    this->consumer_ = NULL;
  }
  if (this->tconf_ != NULL) {
    delete this->tconf_;
    this->tconf_ = NULL;
  }
  if (this->conf_ != NULL) {
    delete this->conf_;
    this->conf_ = NULL;
  }
}

void spider::KafkaReader::rebalance_cb(RdKafka::KafkaConsumer *consumer,
                  RdKafka::ErrorCode err,
                  std::vector<RdKafka::TopicPartition *> &partitions) {
  // if (this->is_stopped_.load() && err == RdKafka::ERR__ASSIGN_PARTITIONS) {
    // 如果已经stop了，就不要再响应这些rebalance的事件了
    // return;
  // }

  std::ostringstream oss;
  if (err == RdKafka::ERR__ASSIGN_PARTITIONS) {
    oss << "Assign partitions: [";
    for (auto i = 0u; i < partitions.size(); ++i) {
      if (i != 0u) {
        oss << ", ";
      }
      oss << partitions[i]->topic() << ":" << partitions[i]->partition();
    }
    oss << "]";
    LOG(INFO) << "RebalanceCb: " << oss.str();
    COUNTERS_kafka_client__kafka_assigned_partition_cnt.Reset(partitions.size());
    consumer->assign(partitions);
  } else if (err == RdKafka::ERR__REVOKE_PARTITIONS) {
    oss << "Unassign partitions: [";
    for (auto i = 0u; i < partitions.size(); ++i) {
      if (i != 0u) {
        oss << ", ";
      }
      oss << partitions[i]->topic() << ":" << partitions[i]->partition();
    }
    oss << "]";
    LOG(INFO) << "RebalanceCb: " << oss.str();
    consumer->unassign();
  } else {
    LOG(ERROR) << "Rebalancing error: " << RdKafka::err2str(err) << std::endl;
    consumer->unassign();
  }
}

bool spider::KafkaReader::CreateConf() {
  conf_ = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
  tconf_ = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);
  std::string err_str;
  if (conf_->set("metadata.broker.list", kafka_brokers_, err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error: " << err_str;
    return false;
  }

  const std::string client_id = get_kafka_client_id();
  if (conf_->set("client.id", client_id, err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  if (conf_->set("group.id", consumer_group_name_, err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  // 64MB，爬虫整体流程就是单条消息最大64MB
  if (conf_->set("message.max.bytes", "67108864", err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  if (conf_->set("receive.message.max.bytes", "500000000", err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  if (conf_->set("log.thread.name", "true", err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  if (conf_->set("event_cb", &KafkaCommon::event_cb_, err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  if (conf_->set("rebalance_cb", this, err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  if (conf_->set("api.version.request", "true", err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  if (conf_->set("socket.keepalive.enable", "true", err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  if (conf_->set("broker.address.ttl", "5000", err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  if (conf_->set("fetch.wait.max.ms", "5000", err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  if (FLAGS_debug_kafka) {
    if (conf_->set("debug", "all", err_str) != RdKafka::Conf::CONF_OK) {
      LOG(ERROR) << "set conf error:" << err_str;
      return false;
    }
  }

  // session过期设置为30秒，30秒无消费，自动断开
  if (conf_->set("session.timeout.ms", "30000", err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  if (conf_->set("heartbeat.interval.ms", "2000", err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  // 开始设置topic默认配置
  if (tconf_->set("enable.auto.commit", "true", err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  if (tconf_->set("auto.commit.interval.ms", "5000", err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  if (tconf_->set("offset.store.method", "broker", err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  if (tconf_->set("auto.offset.reset", "largest", err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  std::string assignor = "range,roundrobin,range-r,roundrobin-r";
  if (base::StartsWith(this->kafka_assignor_, "range-r", false)) {
    assignor = "range-r,roundrobin,range,roundrobin-r";
  } else if (base::StartsWith(this->kafka_assignor_, "roundrobin-r", false)) {
    assignor = "roundrobin-r,range,roundrobin,range-r";
  } else if (base::StartsWith(this->kafka_assignor_, "roundrobin", false)) {
    assignor = "roundrobin,range-r,roundrobin-r,range";
  }

  if (conf_->set("partition.assignment.strategy", assignor, err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  if (conf_->set("default_topic_conf", tconf_, err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  return true;
}
