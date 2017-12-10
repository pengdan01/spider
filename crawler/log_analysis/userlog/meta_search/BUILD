proto_library(name = "meta_proto",
              srcs = [
                "meta.proto",
              ],)

cc_binary(name = "gen_batch_meta",
          srcs = ["gen_batch_meta.cc"],
           deps = ["//base/common/BUILD:base",
                   "//base/strings/BUILD:strings",
                   "//base/encoding/BUILD:encoding",
                   "//log_analysis/common/BUILD:log_common",
                   ":meta_proto",
                   ]
         )

cc_binary(name = "to_protobuf",
          srcs = ["to_protobuf.cc"],
           deps = ["//base/common/BUILD:base",
                   "//base/strings/BUILD:strings",
                   "//base/encoding/BUILD:encoding",
                   "//log_analysis/common/BUILD:log_common",
                   "//query/parser2/BUILD:query_parser",
                   "//web/url_norm/BUILD:url_norm",
                   "//web/url_category/BUILD:url_category",
                   ":meta_proto",
                   ]
         )

