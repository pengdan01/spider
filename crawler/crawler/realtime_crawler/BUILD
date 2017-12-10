proto_library(name = "crawl_queue",
              srcs = ["crawl_queue.proto",],
              deps = ["//crawler/proto2/BUILD:resource_proto",
                      ],
              )

cc_library(name = "job_submitter",
           srcs = ["job_submit.cc"],
           deps = ["//rpc/job_queue/BUILD:job_queue",
                   "//base/time/BUILD:time",
                   ":crawl_queue"],
           )

cc_library(name = "realtime_crawler_lib",
          srcs = ["realtime_crawler.cc",
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
                  "//feature/web/page/BUILD:html_features",
                  "//crawler/proto/BUILD:crawled_resource",
                  "//third_party/curl/BUILD:curl",
                  ":crawl_queue",
                  ":job_submitter",
                  ],
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
                  ":crawl_queue",
                  ],
          )

cc_library(name = "realtime_extractor_lib",
           srcs = ["realtime_extractor.cc",
                   "index_request_submitter.cc",
                   "cdoc_util.cc",
                   ],
           deps = ["//base/hash_function/BUILD:hash_function",
                   "//base/strings/BUILD:strings",
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
                   '//sandbox/jiyanbing/page_miner/BUILD:news_page_miner',

                   ":crawl_queue",
                   ],
           )

cc_binary(name = "realtime_crawler",
          srcs = [
                  "realtime_crawler_main.cc",
                 ],
          deps = [
                  ":realtime_crawler_lib",
                  ":crawl_queue",
                 ],
          )

cc_binary(name = "css_fetcher",
          srcs = [
                  "css_fetcher_main.cc",
                 ],
          deps = [
                  ":crawl_queue",
                  ":css_crawl_util",
                  "//crawler/fetcher2/BUILD:fetcher",
                  "//web/html/utils/BUILD:utils",
                  ],
          )

cc_binary(name = "realtime_extractor",
          srcs = ["realtime_extractor_main.cc",
                  ],
          deps = [":realtime_extractor_lib",
                  "//rpc/http_counter_export/BUILD:export",],
          )

cc_binary(name = "task_pusher",
          srcs = ["task_pusher.cc",
                  ],
          deps = ["//crawler/proto2/BUILD:resource_proto",
                  "//rpc/job_queue/BUILD:job_queue",
                  ":realtime_crawler_lib",
                  ]
          )

cc_binary(name = "task_pusher_queue_version",
          srcs = ["task_pusher_queue_version.cc",
                  ],
          deps = ["//crawler/proto2/BUILD:resource_proto",
                  "//rpc/job_queue/BUILD:job_queue",
                  ":realtime_crawler_lib",
                  ]
          )

cc_test(name = "realtime_crawler_test",
        srcs = ["tests/realtime_crawler_test.cc",
                "tests/job_manager_test.cc",
                ],
        deps = ["//base/testing/BUILD:test_main",
                "//crawler/crawl/BUILD:util",
                "//extend/web_server/BUILD:web_server",
                ":realtime_crawler_lib",
                ":realtime_extractor_lib",
                ],
        )
