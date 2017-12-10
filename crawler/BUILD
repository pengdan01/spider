cc_library(name = "job_submitter",
           srcs = ["job_submit.cc"],
           deps = ["//rpc/job_queue/BUILD:job_queue",
                   "//base/time/BUILD:time",
                   "//crawler/realtime_crawler/BUILD:crawl_queue",
                   ],
           )

cc_library(name = "kafka_client",
           srcs = ["kafka/kafka_common.cc",
                   "kafka/kafka_reader.cc",
                   "kafka/kafka_writer.cc"],
           deps = ["//base/common/BUILD:base",
                   "//base/strings/BUILD:strings",
                   "//base/thread/BUILD:thread",
                   "//base/thread/BUILD:rw_mutex",
                   "//storage/base/BUILD:storage_base",
                   "//serving_base/rw_mutex/BUILD:rw_mutex",
                   ],
           )

cc_library(name = "zk_client",
           srcs = ["zookeeper/zkclient.cc"],
           deps = ["//third_party/zookeeper/BUILD:zookeeper",
                   "//base/thread/BUILD:thread",
                   "//base/thread/BUILD:rw_mutex",
                   "//base/common/BUILD:base",
                   ],
           )

cc_library(name = "crawler_lib",
           srcs = ["crawler.cc",
                   "fetch_result_handler.cc",
                   "config_parse.cc",
                   "job_manager.cc",
                   "amonitor_reporter.cc",
                   "util.cc",
                   ],
           deps = ["//base/encoding/BUILD:encoding",
                   "//crawler/crawl/BUILD:crawl",
                   "//rpc/job_queue/BUILD:job_queue",
                   "//rpc/http_counter_export/BUILD:export",
                   "//net/rpc/BUILD:rpc",
                   "//crawler/realtime_crawler/BUILD:crawl_queue",
                   "//crawler2/general_crawler/BUILD:util",
                   "//third_party/curl/BUILD:curl",
                   "//serving_base/http_communicator/BUILD:http_communicator",
                   ":job_submitter",
                   ":kafka_client",
                   ":zk_client",
                   ],
           )

cc_binary(name = "crawler",
          srcs = [
            "crawler_main.cc",
          ],
          deps = [
            "//crawler/realtime_crawler/BUILD:crawl_queue",
            "//serving_base/utility/BUILD:signal",
            ":crawler_lib",
          ],
          ldflags = ["-L./spider/crawler/kafka/librdkafka", "-lrdkafka++", "-lrdkafka", "-lz", "-lpthread", "-lrt"],
          )

cc_binary(name = "zk_test_bin",
          srcs = [
            "zookeeper/zk_test_bin.cc",
          ],
          deps = [
            "//serving_base/utility/BUILD:signal",
            ":zk_client",
          ],
          )