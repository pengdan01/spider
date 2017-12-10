cc_library(name = "log_common",
           srcs = ["*.cc"],
           excludes = ["*_test.cc", "demo.cc"],
           deps = ["//base/common/BUILD:base",
                   "//base/strings/BUILD:strings",
                   "//base/encoding/BUILD:encoding",
                   "//web/url/BUILD:url",
                   "//base/mapreduce/BUILD:mapreduce",
                   "//base/time/BUILD:time",
                   "//extend/regexp/re3/BUILD:re3",
                   "//nlp/common/BUILD:nlp_common"
                   ]
           )

cc_test(name = "log_common_test",
        srcs = ["*_test.cc"],
        deps = ["//base/common/BUILD:base",
                "//base/strings/BUILD:strings",
                "//base/encoding/BUILD:encoding",
                "//base/testing/BUILD:test_main",
                ":log_common"
                ]
        )

cc_binary(name = "demo",
        srcs = ["demo.cc"],
        deps = ["//base/common/BUILD:base",
                ":log_common"
                ]
        )
