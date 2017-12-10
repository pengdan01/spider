#pragma once

//
// Created by yumeng on 2017/3/18.
//

#include <string>
#include <vector>
#include <sstream>
#include <sys/types.h>
#include <unistd.h>
#include <cstdlib>
#include "base/strings/string_util.h"
#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/common/sleep.h"
#include "base/hash_function/city.h"
#include "base/time/timestamp.h"
#include "base/strings/string_printf.h"
#include "spider/crawler/kafka/librdkafka/rdkafkacpp.h"
#include "spider/crawler/kafka/librdkafka/rdkafka.h"
#include "serving_base/rw_mutex/mutex.h"
#include "spider/crawler/util.h"

DECLARE_bool(debug_kafka);
DECLARE_int64_counter(kafka_client, total_kafka_write_succ);
DECLARE_int64_counter(kafka_client, total_kafka_write_fail);
DECLARE_int64_counter(kafka_client, total_kafka_read_cnt);
DECLARE_int64_counter(kafka_client, total_kafka_skip_cnt);
DECLARE_int64_counter(kafka_client, total_kafka_broker_disconnect);
DECLARE_int64_counter(kafka_client, kafka_assigned_partition_cnt);

namespace spider {

class KafkaPriority {
 public:
  // 低优先级
  static int32_t LOW_PRIORITY;
  // 中优先级
  static int32_t MIDDLE_PRIORITY;
  // 高优先级
  static int32_t HIGH_PRIORITY;
};

class CustomEventCb : public RdKafka::EventCb {
 public:
  void event_cb(RdKafka::Event &event) {
    switch (event.type()) {
      case RdKafka::Event::EVENT_ERROR: {
        if (event.err() != RdKafka::ERR__ALL_BROKERS_DOWN) {
          if (base::StartsWith(event.str(), "Disconnected", true)) {
            COUNTERS_kafka_client__total_kafka_broker_disconnect.Increase(1);
          }
          LOG(INFO) << "EVENT " << event.type() << "(" << RdKafka::err2str(event.err()) << "):" << event.str();
        } else {
          LOG(ERROR) << "error:" << RdKafka::err2str(event.err());
        }
        break;
      }
      case RdKafka::Event::EVENT_STATS: {
        LOG(INFO) << "stats:" << event.str();
        break;
      }
      case RdKafka::Event::EVENT_LOG: {
        LOG(INFO) << base::StringPrintf("LOG-%i-%s:%s",
                                        event.severity(),
                                        event.fac().c_str(),
                                        event.str().c_str());
        break;
      }
      case RdKafka::Event::EVENT_THROTTLE: {
        LOG(INFO) << "THROTTLED: " << event.throttle_time()
                  << "ms by " << event.broker_name()
                  << " id " << event.broker_id();
        break;
      }
      default: {
        LOG(INFO) << "EVENT " << event.type() << "(" << RdKafka::err2str(event.err())
                  << "):" << event.str();
        break;
      }
    }
  }
};

// 划分partition时使用, 如果传入的了优先级信息，则使用对partition划分出不同的优先级区间
class CustomPartitionerCb : public RdKafka::PartitionerCb {
 public:
  int32_t partitioner_cb(const RdKafka::Topic *topic, const std::string *key,
                         int32_t partition_cnt, void *msg_opaque) {
    if (msg_opaque == NULL || partition_cnt < 3) {
      if (key && key->size() > 0) {
        return ((uint32_t) base::CityHash64(key->c_str(), key->size())) % partition_cnt;
      }
      return ((uint32_t) base::GetTimestamp()) % partition_cnt;
    }

    int32_t priority = *static_cast<int32_t *>(msg_opaque);
    if (priority == KafkaPriority::HIGH_PRIORITY) {
      return 0;
    } else if (priority == KafkaPriority::LOW_PRIORITY) {
      return partition_cnt - 1;
    } else if (priority == KafkaPriority::MIDDLE_PRIORITY) {
      if (key && key->size() > 0) {
        return (((uint32_t) base::CityHash64(key->c_str(), key->size())) % (partition_cnt - 2)) + 1;
      }
      return (((uint32_t) base::GetTimestamp()) % (partition_cnt - 2)) + 1;
    } else if (priority >= 0 && priority < partition_cnt) {
      // priority也可能是直接写明的partition index
      return ((uint32_t) priority);
    } else {
      if (key && key->size() > 0) {
        return ((uint32_t) base::CityHash64(key->c_str(), key->size())) % partition_cnt;
      }
      return ((uint32_t) base::GetTimestamp()) % partition_cnt;
    }
  }
};

class KafkaCommon {
 public:
  static CustomEventCb event_cb_;
  static CustomPartitionerCb partition_cb_;
  static int32_t max_partition_cnt_;
  static int32_t partition_idx_[10000];
};

// 获取kafka的client_id，兼容在docker里的情况
std::string get_kafka_client_id();

} // namespace spider
