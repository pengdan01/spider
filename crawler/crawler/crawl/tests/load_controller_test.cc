
#include "base/testing/gtest.h"
#include "base/common/basic_types.h"
#include "base/common/sleep.h"
#include "base/common/base.h"
#include "base/time/time.h"
#include "crawler/crawl/load_controller.h"

using ::crawler2::crawl::LoadController;

TEST(TestLoadControl, LoadIPLoadControll) {
  LoadController::Options options;
  options.default_max_qps = 100;
  options.default_max_connections = 100;
  options.check_frequency = 100;
  options.max_connections_in_all = 100;
  options.min_holdon_duration_after_failed = 1000;
  options.max_holdon_duration_after_failed = 1000;
  options.failed_times_threadhold = 10;

  struct TestCase {
    std::string ip;
    std::string time;
    int max_qps;
  };

  TestCase cases[] = {
    {"*", "2012-01-01 00:00:00", 99},
    {"*", "2012-01-01 00:02:00", 100},
    {"*", "2012-01-01 13:02:00", 111},
    {"127.0.0.1", "2012-01-01 00:00:00", 99},
    {"127.0.0.1", "2012-01-01 00:02:00", 100},
    {"127.0.0.1", "2012-01-01 13:02:00", 111},
    {"114.80.172.231", "2012-01-01 00:00:00", 2},
    {"114.80.172.231", "2012-01-01 00:03:00", 100},
    {"114.80.172.231", "2012-01-01 13:02:00", 111},
  };

  const char* file_data =
      "114.80.172.231\t77\t2\t00:00-00:02\n"
      "220.181.35.202\t88\t20\t00:00-00:01\n"
      "*\t99\t99\t00:00-00:01\n"
      "*\t111\t111\t13:00-23:59\n";

  LoadController load_control(options);
  ASSERT_TRUE(load_control.SetIpLoadRecords(file_data));
  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
    ::base::Time st;
    ASSERT_TRUE(::base::Time::FromStringInSeconds(cases[i].time.c_str(), &st));
    const LoadController::IpLoadRecord* ipload
        = load_control.FindIpLoadRecord(cases[i].ip, st.ToDoubleT() * 1000 * 1000);
    ASSERT_TRUE(NULL != ipload);
    ASSERT_EQ(ipload->max_qps, cases[i].max_qps);
  }
}

TEST(TestLoadControl, LoadIPLoadControlReadFile) {
  struct IPControl {
    const char *ip;
    int64 timestamp;
    int max_qps;
    int max_connections;
  };

  struct IPLoadControlFile {
    int ip_number;
    const char *file_data;
    IPControl ip_control[100];
  } files[] = {
    {1, "114.80.172.231\t10\t4\t00:00-23:59", { {"114.80.172.231", 0, 4, 10} } },
    {2, "114.80.172.231\t15\t2\t00:00-23:59\n220.181.35.202\t10\t6\t00:00-23:59",
      { {"114.80.172.231", 0, 2, 15}, {"220.181.35.202", 0, 6, 10} },
    }
  };

  LoadController::Options options;
  options.default_max_qps = 100;
  options.default_max_connections = 100;
  options.check_frequency = 100;
  options.max_connections_in_all = 100;
  options.min_holdon_duration_after_failed = 1000;
  options.max_holdon_duration_after_failed = 1000;
  options.failed_times_threadhold = 10;

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(files); ++i) {
    LoadController load_control(options);
    load_control.SetIpLoadRecords(files[i].file_data);
    // for each ip
    for (int ip_i = 0; ip_i < files[i].ip_number; ++ip_i) {
      const LoadController::IpLoadRecord *record =
          load_control.FindIpLoadRecord(files[i].ip_control[ip_i].ip,
                                        files[i].ip_control[ip_i].timestamp);
      ASSERT_EQ(files[i].ip_control[ip_i].max_qps, record->max_qps);
      ASSERT_EQ(files[i].ip_control[ip_i].max_connections, record->max_connections);
    }
  }
}

struct TestEvent {
  std::string ip;
  int64 timestamp_ms;  // 事件发生的时间点, 从启动测试开始的 毫秒 数
  int type;  // 事件类型：1: register, 2: unresgister and success, 3: unregister and failed
  bool crash;  // 该事件是否导致 load controller 崩溃
};

void CheckEvents(size_t event_num, LoadController *controller, TestEvent *events);

// Check QPS events
TEST(TestLoadControl, EventsQPS) {
  LoadController::Options options;
  options.default_max_qps = 30;
  options.default_max_connections = 100;
  options.check_frequency = 3;
  options.max_connections_in_all = 100;
  options.max_holdon_duration_after_failed = 200;
  options.min_holdon_duration_after_failed = 200;
  options.failed_times_threadhold = 10;
  LoadController load_controller(options);

  TestEvent test_events[] = {
    {"127.0.0.1", 0, 1, false},
    {"127.0.0.1", 33, 1, false},
    {"127.0.0.1", 66, 1, false},
    {"127.0.0.1", 99, 1, true},
  };

  CheckEvents(arraysize(test_events), &load_controller, test_events);
}

// Check QPS events
TEST(TestLoadControl, EventsQPSNoCrash) {
  LoadController::Options options;
  options.default_max_qps = 30;
  options.default_max_connections = 100;
  options.check_frequency = 3;
  options.max_connections_in_all = 100;
  options.max_holdon_duration_after_failed = 20000;
  options.min_holdon_duration_after_failed = 20000;
  options.failed_times_threadhold = 10;
  LoadController load_controller(options);
  TestEvent test_events[] = {
    {"127.0.0.1", 0, 1, false},
    {"127.0.0.1", 33, 1, false},
    {"127.0.0.1", 66, 1, false},
    {"127.0.0.1", 134, 1, false},
  };

  CheckEvents(arraysize(test_events), &load_controller, test_events);
}

TEST(TestLoadControl, EventsNodefaultIPQPS) {
  LoadController::Options options;
  options.default_max_qps = 20;
  options.default_max_connections = 100;
  options.check_frequency = 3;
  options.max_connections_in_all = 100;
  options.max_holdon_duration_after_failed = 20000;
  options.min_holdon_duration_after_failed = 20000;
  options.failed_times_threadhold = 10;
  const char * file_data = "114.80.172.231\t15\t2\t00:00-23:59\n220.181.35.202\t100\t20\t00:00-23:59";
  LoadController load_controller(options);
  load_controller.SetIpLoadRecords(file_data);
  TestEvent test_events[] = {
    {"220.181.35.202", 0, 1, false},
    {"220.181.35.202", 50, 1, false},
    {"220.181.35.202", 99, 1, false},
    {"220.181.35.202", 101, 1, true},
  };

  CheckEvents(arraysize(test_events), &load_controller, test_events);
}

TEST(TestLoadControl, EventsTotalConnections) {
  LoadController::Options options;
  options.default_max_qps = 100;
  options.default_max_connections = 9;
  options.check_frequency = 100;
  options.max_connections_in_all = 10;
  options.max_holdon_duration_after_failed = 20000;
  options.min_holdon_duration_after_failed = 20000;
  options.failed_times_threadhold = 10;
  LoadController load_controller(options);

  TestEvent test_events_total_conn[] = {
    {"127.0.0.1", 0, 1, false},
    {"10.1.1.1", 1, 1, false},
    {"122.0.0.1", 2, 1, false},
    {"127.0.0.1", 3, 1, false},
    {"128.0.0.1", 4, 1, false},
    {"10.11.2.2", 5, 1, false},
    {"127.0.0.1", 6, 1, false},
    {"127.0.4.1", 7, 1, false},
    {"127.0.0.1", 8, 1, false},
    {"122.0.0.1", 9, 1, false},
    {"1l7.0.0.1", 10, 1, true},
  };

  CheckEvents(arraysize(test_events_total_conn), &load_controller, test_events_total_conn);
}

TEST(TestLoadControl, EventsTotalConnectionsUnregisiter) {
  LoadController::Options options;
  options.default_max_qps = 100;
  options.default_max_connections = 9;
  options.check_frequency = 100;
  options.max_connections_in_all = 10;
  options.max_holdon_duration_after_failed = 20000;
  options.min_holdon_duration_after_failed = 20000;
  options.failed_times_threadhold = 10;

  LoadController load_controller(options);
  TestEvent test_events_total_conn[] = {
    {"127.0.0.1", 0, 1, false},
    {"10.1.1.1", 1, 1, false},
    {"122.0.0.1", 2, 1, false},
    {"127.0.0.1", 3, 1, false},
    {"128.0.0.1", 4, 1, false},
    {"10.11.2.2", 5, 1, false},
    {"127.0.0.1", 6, 1, false},
    {"127.0.4.1", 7, 1, false},
    {"127.0.0.1", 8, 1, false},
    {"122.0.0.1", 9, 1, false},
    {"122.0.0.1", 10, 2, false},
    {"1l7.0.0.1", 11, 1, false},
    {"1l7.0.0.1", 12, 1, true},
  };

  CheckEvents(arraysize(test_events_total_conn), &load_controller, test_events_total_conn);
}

TEST(TestLoadControl, EventsTaskNoDefaultIPConnections) {
  LoadController::Options options;
  options.default_max_qps = 100;
  options.default_max_connections = 5;
  options.check_frequency = 100;
  options.max_connections_in_all = 10;
  options.max_holdon_duration_after_failed = 20000;
  options.min_holdon_duration_after_failed = 20000;
  options.failed_times_threadhold = 10;

  const char * file_data = "114.80.172.231\t15\t2\t00:00-23:59\n220.181.35.202\t5\t100\t00:00-23:59";
  LoadController load_controller(options);
  load_controller.SetIpLoadRecords(file_data);
  // check qps
  TestEvent test_events[] = {
    {"220.181.35.202", 0, 1, false},
    {"220.181.35.202", 3, 1, false},
    {"220.181.35.202", 6, 1, false},
    {"220.181.35.202", 9, 1, false},
    {"220.181.35.202", 10, 1, false},
    {"220.181.35.202", 11, 1, true},
  };

  CheckEvents(arraysize(test_events), &load_controller, test_events);
}

TEST(TestLoadControl, EventsTaskConnections) {
  LoadController::Options options;
  options.default_max_qps = 100;
  options.default_max_connections = 5;
  options.check_frequency = 100;
  options.max_connections_in_all = 10;
  options.max_holdon_duration_after_failed = 20000;
  options.min_holdon_duration_after_failed = 20000;
  options.failed_times_threadhold = 10;
  LoadController load_controller(options);

  // check qps
  TestEvent test_events[] = {
    {"127.0.0.1", 0, 1, false},
    {"127.0.0.1", 3, 1, false},
    {"127.0.0.1", 6, 1, false},
    {"127.0.0.1", 9, 1, false},
    {"127.0.0.1", 10, 1, false},
    {"127.0.0.1", 11, 1, true},
  };

  CheckEvents(arraysize(test_events), &load_controller, test_events);
}

TEST(TestLoadControl, EventsTaskConnectionsUnregisiter) {
  LoadController::Options options;
  options.default_max_qps = 100;
  options.default_max_connections = 5;
  options.check_frequency = 100;
  options.max_connections_in_all = 10;
  options.max_holdon_duration_after_failed = 20000;
  options.min_holdon_duration_after_failed = 20000;
  options.failed_times_threadhold = 10;
  LoadController load_controller(options);

  // check qps
  TestEvent test_events[] = {
    {"127.0.0.1", 0, 1, false},
    {"127.0.0.1", 1, 1, false},
    {"127.0.0.1", 2, 1, false},
    {"127.0.0.1", 3, 1, false},
    {"127.0.0.1", 4, 1, false},
    {"127.0.0.1", 5, 2, false},
    {"127.0.0.1", 6, 1, false},
    {"127.0.0.1", 7, 1, true},
  };

  CheckEvents(arraysize(test_events), &load_controller, test_events);
}

void CheckFetcherEvents(size_t event_num, LoadController *controller, TestEvent *events) {
  for (size_t i = 0; i < event_num; ++i) {
    const TestEvent &e = events[i];
    LOG(INFO) << "event, ip: " << e.ip
              << ", timestamp_ms: " << e.timestamp_ms
              << ", type:" << e.type
              << ", crash: " << e.crash;

    if (i > 0) {
      int64 to_sleep = events[i].timestamp_ms - events[i-1].timestamp_ms;
      // 先检查一下事件是顺序发生的
      CHECK_GE(to_sleep, 0);
    }

    switch (e.type) {
      case 1:
        if (e.crash) {
          ASSERT_NE(controller->CheckFetch(e.ip, e.timestamp_ms * 1000), 0);
        } else {
          ASSERT_EQ(controller->CheckFetch(e.ip, e.timestamp_ms * 1000), 0);
          controller->RegisterIp(e.ip, e.timestamp_ms * 1000);
        }
        break;
      case 2:
        controller->UnregisterIp(e.ip, e.timestamp_ms * 1000, true);
        break;
      default:
        NOT_REACHED();
    }
  }
}

// Check Faileds events
TEST(TestLoadControl, EventsWithFailed) {
  LoadController::Options options;
  options.default_max_qps = 5000;
  options.default_max_connections = 100;
  options.check_frequency = 3;
  options.max_connections_in_all = 100;
  options.max_holdon_duration_after_failed = 20000;
  options.min_holdon_duration_after_failed = 20000;
  options.failed_times_threadhold = 2;
  LoadController load_controller(options);

  TestEvent test_events[] = {
    {"127.0.0.1", 0, 1, false},
    {"127.0.0.1", 1, 3, false},
    {"127.0.0.1", 2, 1, false},
    {"127.0.0.1", 3, 3, false},
    {"127.0.0.1", 6, 1, true},
  };

  CheckEvents(arraysize(test_events), &load_controller, test_events);
}

TEST(TestLoadControl, EventsWithFailedAfterReset) {
  LoadController::Options options;
  options.default_max_qps = 5000;
  options.default_max_connections = 100;
  options.check_frequency = 3;
  options.max_connections_in_all = 100;
  options.max_holdon_duration_after_failed = 5000;
  options.min_holdon_duration_after_failed = 5000;
  options.failed_times_threadhold = 2;
  LoadController load_controller(options);

  TestEvent test_events[] = {
    {"127.0.0.1", 0, 1, false},
    {"127.0.0.1", 1, 3, false},
    {"127.0.0.1", 2, 1, false},
    {"127.0.0.1", 3, 3, false},
    {"127.0.0.1", 9, 1, false},
    {"127.0.0.1", 10, 2, false},
    {"127.0.0.1", 11, 1, false},
    {"127.0.0.1", 12, 3, false},
    {"127.0.0.1", 13, 1, false},
    {"127.0.0.1", 14, 3, false},
    {"127.0.0.1", 15, 1, true},
  };

  CheckEvents(arraysize(test_events), &load_controller, test_events);
}

TEST(TestLoadControl, EventsWithFailedBelowLowerBound) {
  LoadController::Options options;
  options.default_max_qps = 5000;
  options.default_max_connections = 100;
  options.check_frequency = 3;
  options.max_connections_in_all = 100;
  options.max_holdon_duration_after_failed = 4000;
  options.min_holdon_duration_after_failed = 2000;
  options.failed_times_threadhold = 2;
  LoadController load_controller(options);

  TestEvent test_events[] = {
    {"127.0.0.1", 0, 1, false},
    {"127.0.0.1", 1, 3, false},
    {"127.0.0.1", 2, 1, false},
    {"127.0.0.1", 3, 3, false},
    {"127.0.0.1", 5, 1, true},
  };

  CheckEvents(arraysize(test_events), &load_controller, test_events);
}

TEST(TestLoadControl, EventsWithFaildAboveUpperBound) {
  LoadController::Options options;
  options.default_max_qps = 5000;
  options.default_max_connections = 100;
  options.check_frequency = 3;
  options.max_connections_in_all = 100;
  options.max_holdon_duration_after_failed = 4000;
  options.min_holdon_duration_after_failed = 2000;
  options.failed_times_threadhold = 2;
  LoadController load_controller(options);

  TestEvent test_events[] = {
    {"127.0.0.1", 0, 1, false},
    {"127.0.0.1", 1, 3, false},
    {"127.0.0.1", 2, 1, false},
    {"127.0.0.1", 3, 3, false},
    {"127.0.0.1", 7, 1, false},
  };

  CheckEvents(arraysize(test_events), &load_controller, test_events);
}

TEST(TestLoadControl, FailedTooMuch) {
  LoadController::Options options;
  options.default_max_qps = 5000;
  options.default_max_connections = 100;
  options.check_frequency = 100;
  options.max_connections_in_all = 100;
  options.max_holdon_duration_after_failed = 4000;
  options.min_holdon_duration_after_failed = 2000;
  options.failed_times_threadhold = 5;
  options.max_failed_times = 5;
  LoadController controller(options);

  for (int i = 0; i < options.max_failed_times; ++i) {
    controller.RegisterIp("127.0.0.1", (2 * i) * 1000);
    controller.UnregisterIp("127.0.0.1", (2 * i + 1) * 1000, false);
  }

  CHECK_EQ(controller.CheckFetch("127.0.0.1", (2 * options.max_failed_times)), -2);
  CHECK_EQ(controller.CheckFetch("127.0.0.1", (2 * options.max_failed_times) + 1), -2);
  CHECK_EQ(controller.CheckFetch("127.0.0.1", (2 * options.max_failed_times) + 2), -2);
  CHECK_EQ(controller.CheckFetch("127.0.0.1", (2 * options.max_failed_times) + 3), -2);
}


void CheckEvents(size_t event_num, LoadController *controller, TestEvent *events) {
  for (size_t i = 0; i < event_num; ++i) {
    const TestEvent &e = events[i];
    LOG(INFO) << "event, ip: " << e.ip
              << ", timestamp_ms: " << e.timestamp_ms
              << ", type:" << e.type
              << ", crash: " << e.crash;

    if (i > 0) {
      int64 to_sleep = events[i].timestamp_ms - events[i-1].timestamp_ms;
      // 先检查一下事件是顺序发生的
      CHECK_GE(to_sleep, 0);
    }

    switch (e.type) {
      case 1:
        if (e.crash) {
          ASSERT_DEATH(controller->RegisterIp(e.ip, e.timestamp_ms * 1000), "");
          break;
        } else {
          controller->RegisterIp(e.ip, e.timestamp_ms * 1000);
        }
        break;
      case 2:
        if (e.crash) {
          ASSERT_DEATH(controller->UnregisterIp(e.ip, e.timestamp_ms * 1000, true), "");
          break;
        } else {
          controller->UnregisterIp(e.ip, e.timestamp_ms * 1000, true);
        }
        break;
      case 3:
        if (e.crash) {
          ASSERT_DEATH(controller->UnregisterIp(e.ip, e.timestamp_ms * 1000, false), "");
          break;
        } else {
          controller->UnregisterIp(e.ip, e.timestamp_ms * 1000, false);
        }
        break;
      default:
        NOT_REACHED();
    }
  }
}

TEST(TestLoadControl, EventsRandomQPS) {
  // srand(time(NULL));
  LoadController::Options options;
  options.default_max_qps = 50;
  options.default_max_connections = 1000;
  options.check_frequency = 3;
  options.max_connections_in_all = 1000;
  options.max_holdon_duration_after_failed = 20000;
  options.min_holdon_duration_after_failed = 20000;
  options.failed_times_threadhold = 10;
  static const double death_percetage = 0.2;
  TestEvent *event_list = new TestEvent[options.default_max_connections];
  LoadController load_controller(options);
  // TestEvent test_events[] = {
  //   {"127.0.0.1", 0, 1, false},
  //   {"127.0.0.1", 0, 1, false},
  //   {"127.0.0.1", 0, 1, false},
  // };
  int64_t current_time = 0;
  int test_num = 0;
  uint32 seed = 0xBAD;
  for (int i = 0; i < options.default_max_connections; i+=3) {
    TestEvent test_events;
    test_events.ip = "127.0.0.1";
    test_events.type = 1;
    int rand_num = rand_r(&seed) % 10;
    bool death = rand_num < death_percetage * 10;
    if (death && (i != 0)) {
      test_events.timestamp_ms = current_time;
      test_events.crash = true;
      event_list[i] = test_events;
      test_num += 1;
      break;
    } else {
      test_events.crash = false;
      test_events.timestamp_ms = current_time + 1000 / options.default_max_qps * 3;
      test_events.crash = false;
      test_events.timestamp_ms = current_time + 1000 / options.default_max_qps * 3;
      test_events.crash = false;
      test_events.timestamp_ms = current_time + 1000 / options.default_max_qps * 3;
      current_time += 1000 / options.default_max_qps * 3;
      event_list[i] = test_events;
      event_list[i+1] = test_events;
      event_list[i+2] = test_events;
      test_num += 3;
    }
  }
  CheckEvents(test_num, &load_controller, event_list);
  delete []event_list;
}

TEST(TestLoadControl, FetcherEventsRandomQPS) {
  srand(time(NULL));
  static const double death_percetage = 0.4;
  LoadController::Options options;
  options.default_max_qps = 50;
  options.default_max_connections = 1000;
  options.check_frequency = 3;
  options.max_connections_in_all = 1000;
  options.max_holdon_duration_after_failed = 20000;
  options.min_holdon_duration_after_failed = 20000;
  options.failed_times_threadhold = 10;
  TestEvent *event_list = new TestEvent[options.default_max_connections];
  LoadController load_controller(options);
  int64_t current_time = 0;
  int test_num = 0;
  int i = 0;
  uint32 seed = 0xBAD;
  while (i < options.default_max_connections/2) {
    TestEvent test_events;
    test_events.ip = "127.0.0.1";
    test_events.type = 1;
    int rand_num = rand_r(&seed) % 10;
    bool death = rand_num < death_percetage * 10;
    if (death && (i != 0)) {
      test_events.timestamp_ms = current_time;
      test_events.crash = true;
      event_list[i] = test_events;
      LOG(INFO) << event_list[i].ip;
      test_num += 1;
      ++i;
    } else {
      test_events.crash = false;
      test_events.timestamp_ms = current_time + 1000 / options.default_max_qps * 3;
      test_events.crash = false;
      test_events.timestamp_ms = current_time + 1000 / options.default_max_qps * 3;
      test_events.crash = false;
      test_events.timestamp_ms = current_time + 1000 / options.default_max_qps * 3;
      current_time += 1000 / options.default_max_qps * 3;
      event_list[i] = test_events;
      event_list[i+1] = test_events;
      event_list[i+2] = test_events;
      LOG(INFO) << event_list[i].ip;
      LOG(INFO) << event_list[i+1].ip;
      LOG(INFO) << event_list[i+2].ip;
      test_num += 3;
      i+=3;
    }
  }
  CheckFetcherEvents(test_num, &load_controller, event_list);
  delete []event_list;
}

TEST(TestLoadControl, RegisterIp) {
  struct LoadControlTestData {
    const char *file_content;

    int default_max_qps;
    int default_max_connections;
    int check_frequency;
    int max_connections_in_all;

    const char *ip;

    int sleep_time;  // 每发一个请求 sleep 多长时间
    int safe_times;  // 可以安全抓取的次数
    bool crash_at_end;  // 最后多的一次抓取是否会挂
  } test_data[] = {
    {
      "114.80.172.231\t15\t2\t00:00-23:59\n"
      "220.181.35.202\t10\t6\t00:00-23:59",
      3, 10, 10, 100, "220.181.35.202", 0, 10, true,
    }, {
      "114.80.172.231\t15\t2\t00:00-23:59\n"
      "220.181.35.202\t5\t3\t00:00-23:59",
      30, 200, 10, 200, "127.0.0.1", 40, 100, false
    }, {
      "114.80.172.231\t15\t2\t00:00-23:59\n"
      "220.181.35.202\t200\t30\t00:00-23:59",
      30, 200, 10, 200, "220.181.35.202", 40, 100, false
    }, {
      "114.80.172.231\t15\t2\t00:00-23:59\n"
      "220.181.35.202\t10\t6\t00:00-23:59",
      3, 10, 10, 5, "220.181.35.202", 0, 5, true,
    },
  };
  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(test_data); ++i) {
    auto &test = test_data[i];
    LoadController::Options options;
    options.default_max_qps = test.default_max_qps;
    options.default_max_connections = test.default_max_connections;
    options.check_frequency = test.check_frequency;
    options.max_connections_in_all = test.max_connections_in_all;
    options.max_holdon_duration_after_failed = 20000;
    options.min_holdon_duration_after_failed = 20000;
    options.failed_times_threadhold = 10;
    LoadController load_control(options);
    load_control.SetIpLoadRecords(test.file_content);
    int64 timestamp = base::GetTimestamp();
    for (int query_i = 0; query_i < test.safe_times; ++query_i) {
      load_control.RegisterIp(test.ip, timestamp);
      timestamp += test.sleep_time * 1000;
    }
    if (test.crash_at_end) {
      ASSERT_DEATH(load_control.RegisterIp(test.ip, timestamp), "Check failed:");
    } else {
      load_control.RegisterIp(test.ip, timestamp);
    }
  }
}
