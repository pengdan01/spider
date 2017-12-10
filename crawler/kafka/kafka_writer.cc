//
// Created by yumeng on 2017/3/18.
//

#include "spider/crawler/kafka/kafka_writer.h"

DEFINE_int64_counter(kafka_client, total_kafka_write_succ, 0, "Kafka发送成功数");
DEFINE_int64_counter(kafka_client, total_kafka_write_fail, 0, "Kafka发送失败数");

spider::KafkaWriter::KafkaWriter(const std::string &kafka_brokers) {
  this->kafka_brokers_ = kafka_brokers;
}

bool spider::KafkaWriter::Init() {
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
  this->producer_ = RdKafka::Producer::create(this->conf_, err_str);
  if (this->producer_ == NULL) {
    LOG(FATAL) << "Failed to create producer: " << err_str;
    return false;
  }
  LOG(INFO) << "Succ to create producer " << producer_->name() << ", brokers: " + kafka_brokers_;
  return true;
}

bool spider::KafkaWriter::Close() {
  RdKafka::ErrorCode err = this->producer_->flush(10000);
  if (err) {
    LOG(ERROR) << "Failed to real close kafka producer: " << RdKafka::err2str(err);
    return false;
  }
  return true;
}

bool spider::KafkaWriter::Send(const std::string &topic_name,
                               const std::string &key,
                               const std::string &value,
                               int32_t priority) {
  int32_t *in_build_priority_ptr = NULL;
  if (priority < 0) {
    in_build_priority_ptr = NULL;
  } else if (priority < 3) {
    in_build_priority_ptr = &KafkaPriority::HIGH_PRIORITY;
  } else if (priority < 7) {
    in_build_priority_ptr = &KafkaPriority::MIDDLE_PRIORITY;
  } else {
    in_build_priority_ptr = &KafkaPriority::LOW_PRIORITY;
  }

  for (int i = 0; i < 3; i++) {
    bool succ = this->Produce(topic_name, key, value, in_build_priority_ptr);
    if (succ) {
      return true;
    }
    base::SleepForSeconds(1);
  }
  return false;
}

bool spider::KafkaWriter::SendWithPartition(const std::string &topic_name,
                                            const std::string &key,
                                            const std::string &value,
                                            int32_t partition_idx) {
  int32_t *in_build_parition_idx_ptr = NULL;
  if (partition_idx < 0 || partition_idx >= KafkaCommon::max_partition_cnt_) {
    in_build_parition_idx_ptr = NULL;
  } else {
    if (KafkaCommon::partition_idx_[partition_idx] != partition_idx) {
      KafkaCommon::partition_idx_[partition_idx] = partition_idx;
    }
    in_build_parition_idx_ptr = &(KafkaCommon::partition_idx_[partition_idx]);
  }
  for (int i = 0; i < 3; i++) {
    bool succ = this->Produce(topic_name, key, value, in_build_parition_idx_ptr);
    if (succ) {
      return true;
    }
    base::SleepForSeconds(1);
  }
  return false;
}

bool spider::KafkaWriter::Produce(const std::string &topic_name,
                                  const std::string &key,
                                  const std::string &value,
                                  int32_t *priority) {
  std::string err_str;
  RdKafka::Topic *topic = GetOrCreateTopic(topic_name, err_str);
  if (!topic) {
    LOG(ERROR) << "Failed to create topic: " << err_str << std::endl;
    return false;
  }
  RdKafka::ErrorCode err = producer_->produce(topic,
                                              RdKafka::Topic::PARTITION_UA,
                                              RdKafka::Producer::RK_MSG_COPY,
                                              const_cast<char *>(value.c_str()),
                                              value.size(),
                                              &key,
                                              priority);
  // delete topic;
  if (err == RdKafka::ERR_NO_ERROR) {
    LOG_EVERY_N(INFO, 1000) << "Succ to send message to kafka, topic=" <<
                            topic_name << ", key=" << key << ", value size: " << value.length();
    producer_->poll(0);
    return true;
  } else {
    LOG(ERROR) << "Failed to send message: " << RdKafka::err2str(err);
    producer_->poll(0);
    return false;
  }
}

RdKafka::Topic *spider::KafkaWriter::GetOrCreateTopic(const std::string &topic_name, std::string &err_str) {
  {
    serving_base::ReaderAutoLock(&this->topic_map_mutex_);
    auto iter = topic_map_.find(topic_name);
    if (iter != topic_map_.end()) {
      return iter->second;
    }
  }
  serving_base::WriterAutoLock(&this->topic_map_mutex_);
  auto iter = topic_map_.find(topic_name);
  if (iter != topic_map_.end()) {
    return iter->second;
  }
  RdKafka::Topic *topic = RdKafka::Topic::create(producer_, topic_name, tconf_, err_str);
  topic_map_[topic_name] = topic;
  return topic;
}

spider::KafkaWriter::~KafkaWriter() {
  Close();
  if (this->producer_ != NULL) {
    delete this->producer_;
    this->producer_ = NULL;
  }
  if (this->tconf_ != NULL) {
    delete this->tconf_;
    this->tconf_ = NULL;
  }
  if (this->conf_ != NULL) {
    delete this->conf_;
    this->conf_ = NULL;
  }
  this->topic_map_mutex_.WriterLock();
  auto iter = this->topic_map_.begin();
  for (auto iter = this->topic_map_.begin(); iter != topic_map_.end(); ++iter) {
    delete iter->second;
  }
  this->topic_map_.clear();
  this->topic_map_mutex_.WriterUnlock();
}

void spider::KafkaWriter::dr_cb(RdKafka::Message &message) { // NOLINT
  std::string key;
  if (message.key() && message.key_len() > 0) {
    key = *(message.key());
  }

  if (message.err() != RdKafka::ERR_NO_ERROR) {
    COUNTERS_kafka_client__total_kafka_write_fail.Increase(1);
    LOG(ERROR) << "kafka msg delivery fail, key:" << key
               << ", err:" << message.err()
               << ", errstr:" << message.errstr()
               << ", info:" << RdKafka::err2str(message.err());
    return;
  }
  COUNTERS_kafka_client__total_kafka_write_succ.Increase(1);
  LOG_EVERY_N(INFO, 1000) << "sample log: msg delivery, topic:" << message.topic_name()
                          << ", part:" << message.partition()
                          << ", offset:" << message.offset()
                          << ", key:" << key
                          << ", msg_len:" << message.len();
}

bool spider::KafkaWriter::CreateConf() {
  conf_ = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
  tconf_ = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);
  std::string err_str;

  if (conf_->set("metadata.broker.list", kafka_brokers_, err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error: " << err_str;
    return false;
  }

  if (conf_->set("event_cb", &KafkaCommon::event_cb_, err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  if (conf_->set("dr_cb", this, err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  if (conf_->set("delivery.report.only.error", "false", err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  // 64MB，爬虫整体流程就是单条消息最大64MB
  if (conf_->set("message.max.bytes", "67108864", err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  // 64MB，爬虫整体流程就是单条消息最大64MB, 这个配置的单位是kb
  if (conf_->set("queue.buffering.max.kbytes", "65536", err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  if (conf_->set("message.send.max.retries", "20", err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  if (conf_->set("retry.backoff.ms", "2000", err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  if (conf_->set("batch.num.messages", "1000", err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  if (conf_->set("queue.buffering.max.messages", "1000000", err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  if (tconf_->set("partitioner_cb", &KafkaCommon::partition_cb_, err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  if (tconf_->set("produce.offset.report", "true", err_str) != RdKafka::Conf::CONF_OK) {
    LOG(ERROR) << "set conf error:" << err_str;
    return false;
  }

  if (FLAGS_debug_kafka) {
    if (conf_->set("debug", "all", err_str) != RdKafka::Conf::CONF_OK) {
      LOG(ERROR) << "set conf error:" << err_str;
      return false;
    }
  }

  return true;
}
