cc_library(name = "common",
          srcs = ["common.cc"],
           deps = ["//base/common/BUILD:base",
                   "//base/strings/BUILD:strings",
                   "//base/encoding/BUILD:encoding",
                   "//log_analysis/common/BUILD:log_common"
                   ]
         )

cc_binary(name = "search_plus",
          srcs = ["search_plus.cc"],
           deps = ["//base/common/BUILD:base",
                   "//base/strings/BUILD:strings",
                   "//base/encoding/BUILD:encoding",
                   "//log_analysis/common/BUILD:log_common",
                   ":common",
                   ]
         )

cc_binary(name = "pv_plus",
          srcs = ["pv_plus.cc"],
           deps = ["//base/common/BUILD:base",
                   "//base/strings/BUILD:strings",
                   "//base/encoding/BUILD:encoding",
                   "//log_analysis/common/BUILD:log_common",
                   ":common",
                   ]
         )

