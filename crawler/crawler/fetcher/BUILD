cc_library(name = "multifetch",
           srcs = ["multi_fetcher.cc",
                  "fetcher_thread.cc",
                   "resource_saver.cc"],
           deps = ["//base/common/BUILD:base",
                   "//base/hash_function/BUILD:hash_function",
                   "//crawler/api/BUILD:base",
                   "//crawler/util/BUILD:util",
                   "//web/url/BUILD:url",
                   "//crawler/proto/BUILD:crawled_resource",
                   "//crawler/exchange/BUILD:exchange",
                   "//third_party/curl/BUILD:curl",
                   ":crawler_analyzer",
                   ]
           )

cc_library(name = "crawler_analyzer",
           srcs = ["crawler_analyzer.cc",],
           deps = ["//base/common/BUILD:base",
                   "//base/hash_function/BUILD:hash_function",
                   "//crawler/util/BUILD:util",
                   "//web/url/BUILD:url",
                   "//crawler/proto/BUILD:crawled_resource",
                   "//third_party/curl/BUILD:curl",
                   ]
           )

cc_binary(name = "crawler_fetcher",
          srcs = ["fetcher_worker_main.cc",
                  ],
          deps = ["//base/common/BUILD:base",
                  "//base/thread/BUILD:thread",
                  ":multifetch",
                  ":crawler_analyzer",
                  ]
          )

cc_binary(name = "proto_buffer",
          srcs = ["resolve_page_from_proto_buffer.cc"],
          deps = ["//base/common/BUILD:base",
                  "//base/thread/BUILD:thread",
                  ":multifetch",
                  ]
          )

cc_binary(name = "file_upload",
          srcs = ["file_upload.cc"],
          deps = ["//base/common/BUILD:base",
                  ":multifetch",
                  ]
          )

cc_binary(name = "fetcher_tool",
          srcs = ["fetcher_tool.cc"],
          deps = ["//base/common/BUILD:base",
                  "//base/thread/BUILD:thread",
                  ":multifetch",
                  ]
          )
cc_test(name =  "crawler_analyzer_test",
        srcs = ["crawler_analyzer_test.cc"],
        deps = ["//base/testing/BUILD:test_main",
                "//crawler/selector/BUILD:selector_util",
                ":crawler_analyzer",
               ]
        )
cc_test(name =  "fetcher_test",
        srcs = ["fetcher_test.cc"],
        deps = ["//base/testing/BUILD:test_main",
                ":multifetch",
               ]
        )
