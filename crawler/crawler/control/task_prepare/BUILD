cc_binary(name = "uv_data",
          srcs = ["mr/uv_data.cc",],
          deps = [
            "//base/mapreduce/BUILD:mapreduce",
            "//log_analysis/common/BUILD:log_common",
            "//crawler/exchange/BUILD:exchange",
            "//crawler/fetcher2/BUILD:fetcher",
            "//base/strings/BUILD:strings",
            "//web/url/BUILD:url",
            "//web/url_category/BUILD:url_category",
          ],
          );

cc_binary(name = "web_page_urls",
          srcs = ["mr/web_page_urls.cc",],
          deps = [
            "//base/mapreduce/BUILD:mapreduce",
            "//crawler/proto2/BUILD:resource_proto",
            "//crawler/proto2/BUILD:proto_transcode",
            "//base/strings/BUILD:strings",
            "//web/url_norm/BUILD:url_norm",
          ],
          );

# cc_binary(name = "norm_for_fetcher",
#           srcs = ["mr/norm_for_fetcher.cc",],
#           deps = [
#             "//base/mapreduce/BUILD:mapreduce",
#             "//log_analysis/common/BUILD:log_common",
#             "//base/strings/BUILD:strings",
#             "//web/url/BUILD:url",
#             "//web/url_norm/BUILD:url_norm",
#           ],
#           );

cc_binary(name = "cut",
          srcs = ["mr/cut.cc",],
          deps = [
            "//base/mapreduce/BUILD:mapreduce",
            "//base/strings/BUILD:strings",
          ],
          );


