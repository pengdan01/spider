cc_library(name = "base",
           srcs = ["base.cc",
                  ],
           excludes = ["./*_test.cc", "./*_main.cc"],
           deps = ["//base/common/BUILD:base",
                   "//base/hash_function/BUILD:hash_function",
                   "//crawler/proto/BUILD:crawled_resource",
                   "//web/url/BUILD:url"]
           )
cc_test(name = "base_test",
        srcs = ["base_test.cc",],
        deps = [":base",
                "//base/testing/BUILD:test_main",
                ]
        )
cc_data(name = "base_data",
        srcs = ["tld.dat"],
        )
