cc_binary(name = "data_prepare",
          srcs = ["data_prepare.cc",],
          deps = ["//base/common/BUILD:base",
                  "//base/mapreduce/BUILD:mapreduce",
                  "//base/strings/BUILD:strings",
                  "//base/encoding/BUILD:encoding",
                  "//web/url/BUILD:url",
                  "//base/zip/BUILD:zip",
                  "//base/random/BUILD:random",
                  "//base/hash_function/BUILD:hash_function",
#                   "//crawler/proto2/BUILD:resource_proto",
#                   "//crawler/proto2/BUILD:proto_transcode",
                  ],
          )

ss_test(name = "robots_test",
        srcs = ["*_test.sh"],
        )
