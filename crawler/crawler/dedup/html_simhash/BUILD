cc_library(name = "html_simhash",
           srcs = ["html_simhash.cc",
                   "dedup_util.cc"],
           deps = ["//base/common/BUILD:base",
                   "//base/container/BUILD:container",
                   "//base/hash_function/BUILD:hash_function",
                   "//base/strings/BUILD:strings",
                   "//nlp/common/BUILD:nlp_common",
                   "//nlp/idf/BUILD:idf",
                   "//nlp/segment/BUILD:segment",
                   "//web/url/BUILD:url",
                   "//crawler/dedup/dom_extractor/BUILD:content_collector",
                   "//extend/simhash/BUILD:simhash"]
           )
cc_test(name = "html_simhash_test",
        srcs = ["html_simhash_test.cc"],
        deps = [":html_simhash",
                "//extend/simhash/BUILD:simhash",
                "//base/strings/BUILD:strings",
                "//base/time/BUILD:time",
                "//base/testing/BUILD:test_main",
                "//crawler/dedup/dom_extractor/BUILD:content_collector",
                "//nlp/idf/BUILD:idf"]
        )

cc_data(name = "data",
        srcs = ["data/*",],
        )
