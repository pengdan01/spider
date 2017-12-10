cc_library(name = "updater_util",
           srcs = ["updater_util.cc",
                  ],
           excludes = ["./*_test.cc", "./*_main.cc"],
           deps = ["//base/common/BUILD:base",
                   "//base/hash_function/BUILD:hash_function",
                   "//crawler/proto/BUILD:crawled_resource",
                   "//web/url/BUILD:url"]
           )
cc_test(name = "updater_util_test",
        srcs = ["updater_util_test.cc",],
        deps = [":updater_util",
                "//base/testing/BUILD:test_main",
                "//base/file/BUILD:file",
                ]
        )
cc_binary(name = "mr_updater_mapper",
           srcs = ["mr_updater_mapper.cc",
                  ],
           excludes = ["./*_test.cc", "./*_main.cc"],
           deps = ["//base/common/BUILD:base",
                   "//base/hash_function/BUILD:hash_function",
                   "//base/mapreduce/BUILD:mapreduce",
                   "//crawler/proto/BUILD:crawled_resource",
                   "//crawler/api/BUILD:base",
                   ":updater_util"]
           )
cc_binary(name = "mr_updater_reducer",
           srcs = ["mr_updater_reducer.cc",
                  ],
           excludes = ["./*_test.cc", "./*_main.cc"],
           deps = ["//base/common/BUILD:base",
                   "//base/hash_function/BUILD:hash_function",
                   "//base/mapreduce/BUILD:mapreduce",
                   "//crawler/proto/BUILD:crawled_resource",
                   ":updater_util"]
           )


