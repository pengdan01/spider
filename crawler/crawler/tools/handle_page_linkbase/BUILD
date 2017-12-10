#cc_binary(name = "mr_assign_page_simhash_mapper",
#          srcs = ["mr_assign_page_simhash_mapper.cc",],
#          deps = ["//base/common/BUILD:base",
#                  "//base/strings/BUILD:strings",
#                  "//base/mapreduce/BUILD:mapreduce",
#                  "//crawler/proto/BUILD:crawled_resource",
#                  "//crawler/api/BUILD:base",
#                  "//web/url/BUILD:url",
#                  "//web/html/utils/BUILD:utils",
#                  "//extend/simhash/BUILD:html_simhash",
#                  ]
#          )
cc_binary(name = "mr_rawtoclick_mapper",
          srcs = ["mr_rawtoclick_mapper.cc",],
          deps = ["//base/common/BUILD:base",
                  "//base/strings/BUILD:strings",
                  "//base/mapreduce/BUILD:mapreduce",
                  "//crawler/proto/BUILD:crawled_resource",
                  "//crawler/api/BUILD:base",
                  "//web/url/BUILD:url",
                  ]
          )
#cc_binary(name = "mr_rawtoclick_reducer",
#          srcs = ["mr_rawtoclick_reducer.cc",],
#          deps = ["//base/common/BUILD:base",
#                  "//base/strings/BUILD:strings",
#                  "//base/mapreduce/BUILD:mapreduce",
#                  "//crawler/proto/BUILD:crawled_resource",
#                  "//crawler/api/BUILD:base",
#                  "//web/url/BUILD:url",
#                  ]
#          )
cc_binary(name = "mr_assign_page_simhash_reducer",
          srcs = ["mr_assign_page_simhash_reducer.cc",],
          deps = ["//base/common/BUILD:base",
                  "//base/strings/BUILD:strings",
                  "//base/mapreduce/BUILD:mapreduce",
                  "//crawler/api/BUILD:base",
                  ]
          )
cc_binary(name = "mr_rebuild_linkbase_from_page_mapper",
          srcs = ["mr_rebuild_linkbase_from_page_mapper.cc",],
          deps = ["//base/common/BUILD:base",
                  "//base/strings/BUILD:strings",
                  "//base/mapreduce/BUILD:mapreduce",
                  "//crawler/proto/BUILD:crawled_resource",
                  "//crawler/api/BUILD:base",
                  ]
          )
cc_binary(name = "mr_rebuild_linkbase_from_page_reducer",
          srcs = ["mr_rebuild_linkbase_from_page_reducer.cc",],
          deps = ["//base/common/BUILD:base",
                  "//base/strings/BUILD:strings",
                  "//base/mapreduce/BUILD:mapreduce",
                  "//crawler/proto/BUILD:crawled_resource",
                  "//crawler/api/BUILD:base",
                  ]
          )

cc_binary(name = "mr_hfile_debug",
          srcs = ["mr_hfile_debug.cc",],
          deps = ["//base/common/BUILD:base",
                  "//base/strings/BUILD:strings",
                  "//base/mapreduce/BUILD:mapreduce",
                  "//crawler/proto/BUILD:crawled_resource",
                  "//crawler/api/BUILD:base",
                  ]
          )

cc_binary(name = "mr_normalize_linkbase_mapper",
          srcs = ["mr_normalize_linkbase_mapper.cc",],
          deps = ["//base/common/BUILD:base",
                  "//base/strings/BUILD:strings",
                  "//base/mapreduce/BUILD:mapreduce",
                  "//crawler/api/BUILD:base",
                  ]
          )
cc_binary(name = "mr_normalize_linkbase_reducer",
          srcs = ["mr_normalize_linkbase_reducer.cc",],
          deps = ["//base/common/BUILD:base",
                  "//base/strings/BUILD:strings",
                  "//base/mapreduce/BUILD:mapreduce",
                  "//crawler/api/BUILD:base",
                  ]
          )
