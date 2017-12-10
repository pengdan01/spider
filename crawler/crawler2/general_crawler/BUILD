proto_library(name = "url",
              srcs = [
                "url.proto",
              ],
              deps = [
                "//crawler/proto2/BUILD:resource_proto",
              ],
              )
cc_library(name = "util",
           srcs = [
             "util/url_util.cc",
             "util/url_extract_rule.cc",
           ],
           deps = [
             "//web/url/BUILD:url",
             "//log_analysis/common/BUILD:log_common",
             "//third_party/jsoncpp/BUILD:jsoncpp",
             ":url",
           ],
           )

cc_library(name = "scheduler_lib",
           srcs = ["scheduler.cc",
                   ],
           deps = ["//base/hash_function/BUILD:hash_function",
                   "//web/url/BUILD:url",
                   "//web/url_norm/BUILD:url_norm",
                   "//crawler/crawl/BUILD:crawl",
                   "//extend/config/BUILD:config",
                   "//rpc/job_queue/BUILD:job_queue",
                   "//rpc/redis/BUILD:redis_control",
                   '//web/url_category/BUILD:url_category',
                   "//crawler/realtime_crawler/BUILD:crawl_queue",
                   "//log_analysis/page_importance/index_model/BUILD:index_model",
                   "//crawler2/price_recg/BUILD:price_recg",
                   ":url",
                   ],
           )

cc_binary(name = "scheduler",
          srcs = ["scheduler_main.cc",
                  ],
          deps = [":scheduler_lib",
                  "//rpc/http_counter_export/BUILD:export",],
          )

cc_library(name = "extractor_lib",
           srcs = ["extractor.cc",
                   #"cdoc_util.cc",
                   ],
           deps = ["//base/hash_function/BUILD:hash_function",
                   "//web/url/BUILD:url",
                   "//web/url_norm/BUILD:url_norm",
                   "//crawler/crawl/BUILD:crawl",
                   "//crawler/proto/BUILD:crawled_resource",
                   "//crawler/proto2/BUILD:resource_proto",
                   "//crawler/proto2/BUILD:proto_transcode",
                   "//crawler/proto2/BUILD:gen_hbase_key",
                   "//extend/config/BUILD:config",
                   "//rpc/job_queue/BUILD:job_queue",
                   "//rpc/redis/BUILD:redis_control",
                   "//feature/web/extractor/api/BUILD:webpage_feature_extractor",
                   "//feature/web/page/BUILD:html_features",
                   "//feature/web/page/BUILD:html_info_util",
                   "//feature/site/BUILD:site_feature",
                   "//serving/serving_proto/BUILD:serving_proto",
                   "//sandbox/yuanjinhui/page_temporal/BUILD:page_temporal",
                   '//web/url_category/BUILD:url_category',
                   "//crawler/realtime_crawler/BUILD:crawl_queue",
                   "//crawler2/price_recg/BUILD:price_recg",
                   "//third_party/jsoncpp/BUILD:jsoncpp",
                   ":url",
                   ":util",
                   ],
           )

cc_binary(name = "extractor",
          srcs = ["extractor_main.cc",
                  ],
          deps = [":extractor_lib",
                  "//rpc/http_counter_export/BUILD:export",],
          )

cc_library(name = "css_crawl_util",
           srcs = [
             "css_fetch_result_handler.cc",
             "css_job_manager.cc",
           ],
           deps = ["//base/encoding/BUILD:encoding",
                   "//rpc/stumy/BUILD:stumy",
                   "//rpc/job_queue/BUILD:job_queue",
                   "//rpc/http_counter_export/BUILD:export",
                   "//serving/serving_proto/BUILD:serving_proto",
                   "//third_party/curl/BUILD:curl",
                   "//crawler/realtime_crawler/BUILD:crawl_queue",
                   ],
           )

cc_binary(name = "css_fetcher",
          srcs = [
            "css_fetcher_main.cc",
          ],
          deps = [
            ":css_crawl_util",
            ":util",
            "//crawler/realtime_crawler/BUILD:crawl_queue",
            "//crawler/fetcher2/BUILD:fetcher",
            "//web/html/utils/BUILD:utils",
            "//web/html/api/BUILD:api",
            "//web/html/api/BUILD:parser",
          ],
          )

cc_library(name = "job_submitter",
           srcs = ["job_submit.cc"],
           deps = ["//rpc/job_queue/BUILD:job_queue",
                   "//base/time/BUILD:time",
                   "//crawler/realtime_crawler/BUILD:crawl_queue",
                   ],
           )

cc_library(name = "crawler_lib",
           srcs = ["crawler.cc",
                   "fetch_result_handler.cc",
                   "config_parse.cc",
                   "job_manager.cc",
                   ],
           deps = ["//base/encoding/BUILD:encoding",
                   "//crawler/crawl/BUILD:crawl",
                   "//rpc/stumy/BUILD:stumy",
                   "//rpc/job_queue/BUILD:job_queue",
                   "//rpc/http_counter_export/BUILD:export",
                   "//serving/serving_proto/BUILD:serving_proto",
                   "//third_party/curl/BUILD:curl",
                   "//crawler/realtime_crawler/BUILD:crawl_queue",
                   "//feature/redis_dict/BUILD:redis_dict_util",
                   "//crawler2/price_recg/BUILD:price_recg",
                   ":job_submitter",
                   ":url",
                   ],
           )

cc_binary(name = "crawler",
          srcs = [
            "crawler_main.cc",
          ],
          deps = [
            ":crawler_lib",
            "//crawler/realtime_crawler/BUILD:crawl_queue",
          ],
          )

cc_binary(name = "task_pusher_for_schedule",
          srcs = [
            "task_pusher_for_schedule.cc",
          ],
          deps = [
            "//base/common/BUILD:base",
            "//base/file/BUILD:file",
            "//rpc/job_queue/BUILD:job_queue",
            ":util",
            ":url",
          ],
          )

cc_binary(name = "task_pusher_for_schedule_queue_version",
          srcs = [
            "task_pusher_for_schedule_queue_version.cc",
          ],
          deps = [
            "//base/common/BUILD:base",
            "//base/file/BUILD:file",
            "//rpc/job_queue/BUILD:job_queue",
            ":util",
            ":url",
          ],
          )

cc_binary(name = "task_pusher",
          srcs = [
            "task_pusher.cc",
          ],
          deps = [
            "//base/common/BUILD:base",
            "//base/file/BUILD:file",
            "//rpc/job_queue/BUILD:job_queue",
            ":job_submitter",
            ":util",
            ":url",
          ],
          )

cc_binary(name = "task_pusher_queue_version",
          srcs = [
            "task_pusher_queue_version.cc",
          ],
          deps = [
            "//base/common/BUILD:base",
            "//base/file/BUILD:file",
            "//rpc/job_queue/BUILD:job_queue",
            ":job_submitter",
            ":util",
            ":url",
          ],
          )

cc_test(name = "url_util_test",
        srcs = [
          "util/url_util_test.cc"
        ],
        deps = [
          "//base/testing/BUILD:test_main",
          ":util",
        ],
        )
cc_test(name = "url_extract_rule_test",
        srcs = [
          "util/url_extract_rule_test.cc"
        ],
        deps = [
          "//base/testing/BUILD:test_main",
          ":util",
        ],
        )
