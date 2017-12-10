
cc_library(name = "evaluation",
           srcs = ["evaluation.cc"
                   ],
           deps = ["//base/strings/BUILD:strings",
                   "//base/encoding/BUILD:encoding",
                   "//web/url/BUILD:url",
                   "//crawler/exchange/BUILD:exchange",
                   ],
           );

cc_binary(name = "evaluate_uv_crawled",
          srcs = ["evaluate.cc",
                  ],
          deps = [":evaluation",
                  ],
          )
          
