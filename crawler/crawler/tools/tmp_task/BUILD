#cc_binary(name = "mr_domain_url_mapper",
#          srcs = ["mr_domain_url_mapper.cc",],
#          deps = ["//base/common/BUILD:base",
#                  "//base/strings/BUILD:strings",
#                  "//base/mapreduce/BUILD:mapreduce",
#                  "//crawler/api/BUILD:base",
#                  "//web/url/BUILD:url",
#                  "//web/url_norm/BUILD:url_norm",
#                  "//crawler/selector/BUILD:selector_util",
#                  ]
#          )
#cc_binary(name = "mr_domain_url_reducer",
#          srcs = ["mr_domain_url_reducer.cc",],
#          deps = ["//base/common/BUILD:base",
#                  "//base/strings/BUILD:strings",
#                  "//base/mapreduce/BUILD:mapreduce",
#                  "//web/url/BUILD:url",
#                  ]
#          )
#cc_binary(name = "fetcher_main",
#          srcs = ["fetcher.cc",],
#          deps = ["//base/common/BUILD:base",
#                  "//base/strings/BUILD:strings",
#                  "//crawler/fetcher/BUILD:multifetch",
#                  ]
#          )
#
#cc_binary(name = "test_kv",
#          srcs = ["test_kv.cc",],
#          deps = ["//base/common/BUILD:base",
#                  "//base/strings/BUILD:strings",
#                  "//crawler/proto/BUILD:crawled_resource",
#                  ]
#          )
cc_binary(name = "mapper_test_protobuf",
          srcs = ["test_protobufer/mr_test_protobufer_mapper.cc",],
          deps = ["//base/common/BUILD:base",
                  "//base/mapreduce/BUILD:mapreduce",
                  "//base/strings/BUILD:strings",
                  "//crawler/proto2/BUILD:resource_proto",
                  "//crawler/proto2/BUILD:proto_transcode",
                  "//web/url/BUILD:url",
                  ]
          )
cc_binary(name = "reducer_test_protobuf",
          srcs = ["test_protobufer/mr_test_protobufer_reducer.cc",],
          deps = ["//base/common/BUILD:base",
                  "//base/mapreduce/BUILD:mapreduce",
                  "//base/strings/BUILD:strings",
                  "//crawler/proto2/BUILD:resource_proto",
                  ]
          )


cc_binary(name = "mr_stat_crawl_status_mapper",
          srcs = ["stat_crawl_status/mr_stat_crawl_status_mapper.cc",],
          deps = ["//base/common/BUILD:base",
                  "//base/mapreduce/BUILD:mapreduce",
                  "//base/strings/BUILD:strings",
                  "//crawler/proto/BUILD:crawled_resource",
                  "//web/url/BUILD:url",
                  ]
          )
cc_binary(name = "mr_stat_crawl_status_reducer",
          srcs = ["stat_crawl_status/mr_stat_crawl_status_reducer.cc",],
          deps = ["//base/common/BUILD:base",
                  "//base/mapreduce/BUILD:mapreduce",
                  "//base/strings/BUILD:strings",
                  "//crawler/proto/BUILD:crawled_resource",
                  "//web/url/BUILD:url",
                  ]
          )

#cc_binary(name = "mr_bad_urls_mapper",
#          srcs = ["bad_urls_not_in_crawl_task/mr_bad_urls_mapper.cc",],
#          deps = ["//base/common/BUILD:base",
#                  "//base/mapreduce/BUILD:mapreduce",
#                  "//base/strings/BUILD:strings",
#                  "//crawler/proto/BUILD:crawled_resource",
#                  "//web/url/BUILD:url",
#                  ]
#          )
#cc_binary(name = "mr_bad_urls_reducer",
#          srcs = ["bad_urls_not_in_crawl_task/mr_bad_urls_reducer.cc",],
#          deps = ["//base/common/BUILD:base",
#                  "//base/mapreduce/BUILD:mapreduce",
#                  "//base/strings/BUILD:strings",
#                  "//crawler/proto/BUILD:crawled_resource",
#                  "//web/url/BUILD:url",
#                  ]
#          )
#
#cc_binary(name = "mr_check_http200_page_content_mapper",
#          srcs = ["check_page_content/mr_check_http200_page_content_mapper.cc",],
#          deps = ["//base/common/BUILD:base",
#                  "//base/mapreduce/BUILD:mapreduce",
#                  "//base/strings/BUILD:strings",
#                  "//crawler/proto2/BUILD:resource_proto",
#                  "//crawler/proto2/BUILD:proto_transcode",
#                  "//crawler/dedup/html_simhash/BUILD:html_simhash",
#                  "//web/url/BUILD:url",
#                  ]
#          )
#cc_binary(name = "mr_check_http200_page_content_reducer",
#          srcs = ["check_page_content/mr_check_http200_page_content_reducer.cc",],
#          deps = ["//base/common/BUILD:base",
#                  "//base/mapreduce/BUILD:mapreduce",
#                  "//base/strings/BUILD:strings",
#                  "//web/url/BUILD:url",
#                  ]
#          )
#
cc_binary(name = "mr_check_valid_page_mapper",
          srcs = ["check_page_content/mr_check_valid_page_mapper.cc",],
          deps = ["//base/common/BUILD:base",
                  "//base/mapreduce/BUILD:mapreduce",
                  "//base/strings/BUILD:strings",
                  "//crawler/proto2/BUILD:resource_proto",
                  "//crawler/proto2/BUILD:proto_transcode",
                  "//web/url/BUILD:url",
                  "//crawler2/price_recg/BUILD:price_recg",
                  ]
          )
#cc_binary(name = "mr_check_valid_page_reducer",
#          srcs = ["check_page_content/mr_check_valid_page_reducer.cc",],
#          deps = ["//base/common/BUILD:base",
#                  "//base/mapreduce/BUILD:mapreduce",
#                  "//base/strings/BUILD:strings",
#                  "//web/url/BUILD:url",
#                  ]
#          )
#
cc_binary(name = "mr_check_dead_link_mapper",
          srcs = ["check_dead_link/mr_check_dead_link_mapper.cc",],
          deps = ["//base/common/BUILD:base",
                  "//base/mapreduce/BUILD:mapreduce",
                  "//base/strings/BUILD:strings",
                  "//crawler/proto2/BUILD:resource_proto",
                  "//crawler/proto2/BUILD:proto_transcode",
                  "//web/url/BUILD:url",
                  ]
          )
cc_binary(name = "mr_check_dead_link_reducer",
          srcs = ["check_dead_link/mr_check_dead_link_reducer.cc",],
          deps = ["//base/common/BUILD:base",
                  "//base/mapreduce/BUILD:mapreduce",
                  "//base/strings/BUILD:strings",
                  "//web/url/BUILD:url",
                  ]
          )

#cc_binary(name = "checker",
#          srcs = ["check_dead_link/checker.cc",],
#          deps = ["//base/common/BUILD:base",
#                  "//base/strings/BUILD:strings",
#                  ]
#          )

cc_binary(name = "total_mapper",
          srcs = ["stat_total_url/mr_stat_crawl_status_mapper.cc",],
          deps = ["//base/common/BUILD:base",
                  "//base/mapreduce/BUILD:mapreduce",
                  "//base/strings/BUILD:strings",
                  "//crawler/proto/BUILD:crawled_resource",
                  "//web/url/BUILD:url",
                  ]
          )
cc_binary(name = "total_reducer",
          srcs = ["stat_total_url/mr_stat_crawl_status_reducer.cc",],
          deps = ["//base/common/BUILD:base",
                  "//base/mapreduce/BUILD:mapreduce",
                  "//base/strings/BUILD:strings",
                  "//crawler/proto/BUILD:crawled_resource",
                  "//web/url/BUILD:url",
                  ]
          )

#cc_binary(name = "diff_mapper",
#          srcs = ["diff/mr_diff_mapper.cc",],
#          deps = ["//base/common/BUILD:base",
#                  "//base/mapreduce/BUILD:mapreduce",
#                  "//base/strings/BUILD:strings",
#                  "//crawler/proto/BUILD:crawled_resource",
#                  "//web/url/BUILD:url",
#                  ]
#          )
#cc_binary(name = "diff_reducer",
#          srcs = ["diff/mr_diff_reducer.cc",],
#          deps = ["//base/common/BUILD:base",
#                  "//base/mapreduce/BUILD:mapreduce",
#                  "//base/strings/BUILD:strings",
#                  "//crawler/proto/BUILD:crawled_resource",
#                  "//web/url/BUILD:url",
#                  ]
#          )

#cc_binary(name = "douban_mapper",
#          srcs = ["find_douban/mr_douban_mapper.cc",],
#          deps = ["//base/common/BUILD:base",
#                  "//base/mapreduce/BUILD:mapreduce",
#                  "//base/strings/BUILD:strings",
#                  "//crawler/proto/BUILD:crawled_resource",
#                  "//web/url/BUILD:url",
#                  ]
#          )
#cc_binary(name = "douban_reducer",
#          srcs = ["find_douban//mr_douban_reducer.cc",],
#          deps = ["//base/common/BUILD:base",
#                  "//base/mapreduce/BUILD:mapreduce",
#                  "//base/strings/BUILD:strings",
#                  "//crawler/proto/BUILD:crawled_resource",
#                  "//web/url/BUILD:url",
#                  ]
#          )
