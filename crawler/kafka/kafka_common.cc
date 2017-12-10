//
// Created by yumeng on 2017/3/18.
//

#include "storage/base/process_util.h"
#include "base/strings/string_number_conversions.h"
#include "spider/crawler/kafka/kafka_common.h"

DEFINE_bool(debug_kafka, false, "");

DEFINE_int64_counter(kafka_client, total_kafka_read_cnt, 0, "Kafka读取成功数");
DEFINE_int64_counter(kafka_client, total_kafka_skip_cnt, 0, "Kafka跳过的记录数");
DEFINE_int64_counter(kafka_client, total_kafka_broker_disconnect, 0, "Kafka跟broker断开连接的次数");
DEFINE_int64_counter(kafka_client, kafka_assigned_partition_cnt, 0, "Kafka被分配的partition个数");

namespace spider {

// 定义各种回调函数
CustomEventCb KafkaCommon::event_cb_;
CustomPartitionerCb KafkaCommon::partition_cb_;
int32_t KafkaCommon::max_partition_cnt_ = 10000;
int32_t KafkaCommon::partition_idx_[10000];

int32_t KafkaPriority::LOW_PRIORITY = -2;
int32_t KafkaPriority::MIDDLE_PRIORITY = -3;
int32_t KafkaPriority::HIGH_PRIORITY = -4;


std::string get_kafka_client_id() {
  const char* docker_host_ip = getenv("DOCKER_HOST_IP");
  const char* docker_container_name = getenv("DOCKER_CONTAINER_NAME");
  if (docker_host_ip != NULL && docker_container_name != NULL) {
    // 如果在docker容器内，则使用docker宿主机ip + docker容器名来做唯一标识
    const std::string local_ip = docker_host_ip;
    const std::string pid = docker_container_name;
    return local_ip + "-" + pid;
  } else {
    // 如果不在docker容器内，就用ip + 进程号来标识
    const std::string local_ip = base::GetLocalIP();
    const std::string pid = base::Int64ToString(getpid());
    return local_ip + "-" + pid;
  }
}
}