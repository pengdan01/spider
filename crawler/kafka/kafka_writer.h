#pragma once

//
// 给爬虫用的kafka producer，因为爬虫对kafka使用消息体较大，reco下面的kafka client参数不好定制，所以重新写一个，防止命令冲突
// 所以叫kafka writer
//
// Created by yumeng on 2017/3/18.
//

#include <string>
#include <set>
#include <vector>
#include <map>
#include "spider/crawler/kafka/kafka_common.h"

namespace spider {

class KafkaWriter : public RdKafka::DeliveryReportCb {

 public:

  explicit KafkaWriter(const std::string &kafka_brokers);

  // 初始化操作
  bool Init();

  // 关闭
  bool Close();

  // 发送一条记录出去
  bool Send(const std::string &topic, const std::string &key, const std::string &value, int32_t priority = -1);

  // 发送一条记录出去
  bool SendWithPartition(const std::string &topic, const std::string &key, const std::string &value, int32_t partition_idx = -1);

  // 析构函数
  virtual ~KafkaWriter();

 public:
  // 底层发送出去数据后的回调
  void dr_cb(RdKafka::Message &message);

 private:
  bool CreateConf();

  // 直正发送的方法
  bool Produce(const std::string &topic, const std::string &key, const std::string &value, int32_t *priority);

  // 获取缓存中的topic对象，若缓存没有，则创建一个新的并放入缓存
  RdKafka::Topic* GetOrCreateTopic(const std::string &topic_name, std::string& err_str);

 private:
  // 底层的真正的producer对象
  RdKafka::Producer *producer_;
  RdKafka::Conf *conf_;
  RdKafka::Conf *tconf_;
  std::string kafka_brokers_;

  // topic对象缓存用的锁
  serving_base::RWMutex topic_map_mutex_;
  // topic对象的缓存
  std::map<std::string, RdKafka::Topic *> topic_map_;
};

} // namespace spider 
