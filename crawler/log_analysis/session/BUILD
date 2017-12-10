cc_library(name = "session_segment_alg",
           srcs = ["algorithm/*log_segmenter.cc",],
           deps = ["//base/common/BUILD:base",
                   "//web/url/BUILD:url",
                   "//base/strings/BUILD:strings",
                   "//base/time/BUILD:time",
                   "//base/hash_function/BUILD:hash_function",
                   "//nlp/segment/BUILD:segment",
                   ],
           )

cc_library(name = "session_segment",
           srcs = ["session_segment_util.cc",],
           deps = [":session_segment_alg",
                   "//base/common/BUILD:base",
                   "//nlp/common/BUILD:nlp_common",
                   "//nlp/segment/BUILD:segment",
                   "//base/strings/BUILD:strings",
                   "//base/time/BUILD:time",
                   ],
           )


cc_binary(name = "session_segment_sample",
           srcs = ["session_segment_sample.cc",],
           deps = [":session_segment",
                   "//base/strings/BUILD:strings",
                   "//base/common/BUILD:base",],
           )
