#pragma once

//
// 给爬虫用的kafka consumer，因为爬虫对kafka使用消息体较大，reco下面的kafka client参数不好定制，所以重新写一个，防止命令冲突
// 所以叫kafka reader
//
// Created by yumeng on 2017/3/18.
//

#include <string>
#include <set>
#include <vector>
#include <cstdatomic>
#include "spider/crawler/kafka/kafka_common.h"

namespace spider {

class KafkaReader : public RdKafka::RebalanceCb {

 public:
  // 同时读多个topic
  explicit KafkaReader(const std::string &kafka_brokers,
                       const std::set<std::string> &topics,
                       const std::string &consumer_group_name,
                       const std::string &kafka_assignor,
                       const bool kafka_seek_end);

  // 只读一个topic
  explicit KafkaReader(const std::string &kafka_brokers,
                       const std::string &topic,
                       const std::string &consumer_group_name,
                       const std::string &kafka_assignor,
                       const bool kafka_seek_end);

  // 初始化操作
  bool Init();

  // 关闭
  bool Close();

  // 触发停止读取, 并没有真的停止，只是Fetch返回NULL了，需要调用Close真正关闭
  void SetStopped();

  // 是否已触发停止读取
  bool IsStopped();

  // 从Kafka取出一条记录，是指针，用完需要自己去delete, 本函数是阻塞式，会不停地读取
  RdKafka::Message* Fetch();

  // 析构函数
  virtual ~KafkaReader();

 public:
  // Override
  void rebalance_cb(RdKafka::KafkaConsumer *consumer,
                    RdKafka::ErrorCode err,
                    std::vector<RdKafka::TopicPartition *> &partitions);

 public:
  // inline getter or setter
  const std::string& GetAssignor() {
    return kafka_assignor_;
  }

 private:
  bool CreateConf();

  // 通过参数返回是否需要sleep一会再调用
  RdKafka::Message* Consume(bool &need_sleep);

 private:
  // 底层的真正的消费对象
  RdKafka::KafkaConsumer *consumer_;
  RdKafka::Conf *conf_;
  RdKafka::Conf *tconf_;
  std::string kafka_brokers_;
  std::vector<std::string> topics_;
  std::string consumer_group_name_;
  std::string kafka_assignor_;

  std::atomic<bool> is_stopped_;

  // 如果发生连续消费超时，证明着这个kafka consumer内部出现了问题，需要重启服务
  std::atomic<int> continuous_timeout_times_; // 连续地消费超时的个数
  int64 consumer_start_time_; // consumer启动的时间
  bool kafka_seek_end_; // 是否需要在第一次启动时，seek到队尾
};

} // namespace spider 
