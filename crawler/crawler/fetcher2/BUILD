
cc_library(name = "fetcher_util",
           srcs = ["saver.cc",
                   "data_io.cc",
                   "load_controller.cc",],
           deps = ["//base/common/BUILD:base",
                   "//base/time/BUILD:time",
                   "//base/random/BUILD:random",
                   "//base/file/BUILD:file",
                   "//web/url/BUILD:url",
                   "//web/url_norm/BUILD:url_norm",
                   "//crawler/proto2/BUILD:resource_proto",
                   ]
           )
cc_library(name = "fetcher",
           srcs = ["fetcher.cc",
                   "fetcher_callbacks.cc",
                   "fetcher_misc.cc",
                   ],
           deps = ["//base/common/BUILD:base",
                   "//base/time/BUILD:time",
                   "//base/file/BUILD:file",
                   "//base/hash_function/BUILD:hash_function",
                   "//crawler/proto2/BUILD:resource_proto",
                   "//crawler/proto2/BUILD:gen_hbase_key",
                   ":fetcher_util",
                   ]
           )

cc_binary(name = "fetcher2",
          srcs = ["fetcher_main.cc"],
          deps = ["//base/common/BUILD:base",
                  "//base/thread/BUILD:thread",
                  "//base/hash_function/BUILD:hash_function",
                  "//web/html/utils/BUILD:utils",
                  ":fetcher",
                  ":fetcher_util",
                  ]
          )

cc_binary(name = "check_fetch_status",
          srcs = ["check_fetch_status.cc"],
          deps = ["//base/common/BUILD:base",
                  "//base/thread/BUILD:thread",
                  ":fetcher",
                  ]
          )

cc_test(name =  "fetcher_test",
        srcs = ["tests/saver_test.cc",
                "tests/fetcher_test.cc",
                "tests/load_controller_test.cc",
                "tests/simple_fetcher.cc",
                ],
        deps = ["//base/testing/BUILD:test_main",
                "//base/common/BUILD:base",
                "//extend/web_server/BUILD:web_server",
                "//base/thread/BUILD:thread",
                ":fetcher_util",
                ":fetcher",
                ]
        )
