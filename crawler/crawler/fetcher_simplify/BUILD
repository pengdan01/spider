cc_library(name = "fetcher",
           srcs = ["fetcher.cc",
                   "fetcher_callbacks.cc",
                   "fetcher_misc.cc",
                   ],
           deps = ["//base/common/BUILD:base",
                   "//base/time/BUILD:time",
                   "//base/file/BUILD:file",
                   "//base/hash_function/BUILD:hash_function",
                   "//crawler/proto2/BUILD:gen_hbase_key",
                   "//third_party/curl_wrapper/BUILD:curl_wrapper",
                   ]
           )

cc_library(name = "fetcher_simplify",
           srcs = ["fetcher_simplify.cc",
                   ],
           deps = ["//base/common/BUILD:base",
                   "//base/time/BUILD:time",
                   "//base/file/BUILD:file",
                   "//base/hash_function/BUILD:hash_function",
                   "//third_party/curl/BUILD:curl",
                   ":fetcher"
                   ]
           )

cc_test(name =  "fetcher_simplify_test",
        srcs = [
                "tests/fetcher_simplify_test.cc",
                ],
        deps = ["//base/testing/BUILD:test_main",
                "//base/common/BUILD:base",
                "//base/thread/BUILD:thread",
                ":fetcher_simplify"
                ]
        )
