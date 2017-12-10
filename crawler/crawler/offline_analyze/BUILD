cc_library(name = "offline_analyze_util",
           srcs = ["offline_analyze_util.cc"],
           deps = ["//base/common/BUILD:base",
           ]
           )
cc_test(name = "offline_analyze_test",
        srcs = ["offline_analyze_test.cc"],
        deps = [ "//base/testing/BUILD:test_main",
                 "//base/common/BUILD:base",
                 "//base/strings/BUILD:strings",
                 "//crawler/proto/BUILD:crawled_resource",
                 "//crawler/selector/BUILD:selector_util",
                 "//web/html/utils/BUILD:utils",
                 ":offline_analyze_util",
               ]
        )
#cc_binary(name = "mapper_extract_image_body",
#          srcs = ["mapper_extract_image_body.cc",],
#          deps = ["//base/common/BUILD:base",
#                  "//base/strings/BUILD:strings",
#                  "//crawler/proto/BUILD:crawled_resource",
#                  "//crawler/util/BUILD:util",
#                  "//base/mapreduce/BUILD:mapreduce",
#                  ":offline_analyze_util",
#                  ]
#          )
cc_binary(name = "mr_combine_batch_delta_mapper",
          srcs = ["mr_combine_batch_delta_mapper.cc",],
          deps = ["//base/common/BUILD:base",
                  "//base/mapreduce/BUILD:mapreduce",
                  "//base/strings/BUILD:strings",
                  "//base/hash_function/BUILD:hash_function",
                  "//crawler/proto/BUILD:crawled_resource",
                  "//crawler/selector/BUILD:selector_util",
                  "//crawler/api/BUILD:base",
                  ]
          )

cc_binary(name = "mr_combine_batch_delta_reducer",
          srcs = ["mr_combine_batch_delta_reducer.cc",],
          deps = ["//base/common/BUILD:base",
                  "//base/strings/BUILD:strings",
                  "//base/mapreduce/BUILD:mapreduce",
                  "//crawler/proto/BUILD:crawled_resource",
                  ]
          )

