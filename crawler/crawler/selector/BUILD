cc_library(name = "selector_util",
           srcs = ["crawler_selector_util.cc"],
           deps = ["//base/common/BUILD:base",
                   "//web/url/BUILD:url",
                   "//web/url_norm/BUILD:url_norm",
                   "//crawler/api/BUILD:base",
                   "//crawler/proto/BUILD:crawled_resource",
                   "//log_analysis/common/BUILD:log_common",
                   ]
           )

cc_test(name =  "selector_test",
        srcs = ["crawler_selector_test.cc"],
        deps = ["//base/testing/BUILD:test_main",
                ":selector_util",
               ]
        )

cc_binary(name = "mr_selector_r1_mapper",
          srcs = ["mr_selector_r1_mapper.cc",],
          deps = ["//base/common/BUILD:base",
                  "//base/mapreduce/BUILD:mapreduce",
                  "//base/strings/BUILD:strings",
                  "//crawler/proto/BUILD:crawled_resource",
                  "//crawler/api/BUILD:base",
                  "//feature/url/BUILD:urlscore",
                  "//log_analysis/common/BUILD:log_common",
                  ":selector_util",
                  ]
          )

cc_binary(name = "mr_selector_r1_reducer",
          srcs = ["mr_selector_r1_reducer.cc",],
          deps = ["//base/common/BUILD:base",
                  "//base/mapreduce/BUILD:mapreduce",
                  "//base/strings/BUILD:strings",
                  "//web/url/BUILD:url",
                  "//crawler/proto/BUILD:crawled_resource",
                  "//feature/url/BUILD:urlscore",
                  ":selector_util",
                  ]
          )
