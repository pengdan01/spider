proto_library(name = "resource_proto",
              srcs = ["crawl_info.proto",
                      "resource.proto",
                      "probe.proto",
                      ],
              )

cc_library(name = "proto_transcode",
           srcs = ["proto_transcode.cc"],
           deps = ["//crawler/proto/BUILD:crawled_resource",
                   "//crawler/proto2/BUILD:resource_proto",
                   "//base/strings/BUILD:strings",
                   "//base/hash_function/BUILD:hash_function",
                   ],
           )

cc_library(name = "gen_hbase_key",
           srcs = ["gen_hbase_key.cc"],
          deps = ["//crawler/proto/BUILD:crawled_resource",
                  "//crawler/proto2/BUILD:resource_proto",
                  "//base/common/BUILD:base",
                  "//base/mapreduce/BUILD:mapreduce",
                  "//web/url/BUILD:url",
                  "//web/url_norm/BUILD:url_norm",
                  ":proto_transcode",
                  ],
           )

cc_binary(name = "mr_proto_to_hbase",
          srcs = ["mr_proto_to_hbase.cc"],
          deps = ["//crawler/proto/BUILD:crawled_resource",
                  "//crawler/proto2/BUILD:resource_proto",
                  "//base/common/BUILD:base",
                  "//base/mapreduce/BUILD:mapreduce",
                  "//web/url/BUILD:url",
                  "//web/url_norm/BUILD:url_norm",
                  ":proto_transcode",
                  ":gen_hbase_key",
                  ],
          )

cc_binary(name = "mr_proto_to_hbase_dump",
          srcs = ["mr_proto_to_hbase_dump.cc"],
          deps = ["//crawler/proto/BUILD:crawled_resource",
                  "//crawler/proto2/BUILD:resource_proto",
                  "//base/common/BUILD:base",
                  "//base/mapreduce/BUILD:mapreduce",
                  "//web/url/BUILD:url",
                  "//web/url_norm/BUILD:url_norm",
                  ":proto_transcode",
                  ":gen_hbase_key",
                  ],
          )

cc_binary(name = "mr_proto_to_hbase2",
          srcs = ["mr_proto_to_hbase2.cc"],
          deps = ["//crawler/proto/BUILD:crawled_resource",
                  "//crawler/proto2/BUILD:resource_proto",
                  "//base/common/BUILD:base",
                  "//base/mapreduce/BUILD:mapreduce",
                  "//web/url/BUILD:url",
                  "//web/url_norm/BUILD:url_norm",
                  ":proto_transcode",
                  ":gen_hbase_key",
                  ],
          )

cc_binary(name = "mr_hbase_dump_to_hbase_import",
          srcs = ["mr_hbase_dump_to_hbase_import.cc"],
          deps = ["//crawler/proto2/BUILD:resource_proto",
                  "//base/common/BUILD:base",
                  "//base/zip/BUILD:zip",
                  "//base/mapreduce/BUILD:mapreduce",
                  ":proto_transcode",
                  ":gen_hbase_key",
                  ],
          )

cc_binary(name = "mr_hbase_dump_to_snappy",
          srcs = ["mr_hbase_dump_to_snappy.cc"],
          deps = ["//crawler/proto2/BUILD:resource_proto",
                  "//base/common/BUILD:base",
                  "//base/zip/BUILD:zip",
                  "//base/mapreduce/BUILD:mapreduce",
                  "//web/html/api/BUILD:parser",
                  ":proto_transcode",
                  ":gen_hbase_key",
                  ],
          )

cc_binary(name = "mr_url_to_hbase_key",
          srcs = ["mr_url_to_hbase_key.cc"],
          deps = ["//base/mapreduce/BUILD:mapreduce",
                  "//web/url_norm/BUILD:url_norm",
                  "//web/url/BUILD:url",
                  "//base/common/BUILD:base",
                  ":gen_hbase_key",
                  ],
          )

cc_test(name = "gen_hbase_key_test",
        srcs = ["gen_hbase_key_test.cc",],
        deps = ["//base/testing/BUILD:test_main",
                "//web/url/BUILD:url",
                "//web/url_norm/BUILD:url_norm",
                ":proto_transcode",
                ":gen_hbase_key",
                ]
        )

cc_binary(name = "gen_hbase_key_demo",
          srcs = ["gen_hbase_key_demo.cc",],
          deps = [":gen_hbase_key",],
          )

cc_test(name = "proto_transcode_test",
        srcs = ["proto_transcode_test.cc",],
        deps = ["//base/testing/BUILD:test_main",
                "//web/url/BUILD:url",
                "//web/url_norm/BUILD:url_norm",
                ":proto_transcode",
                ]
        )

cc_binary(name = "mr_hbase_dump_crawled_resource_css_mapper",
          srcs = ["mr_hbase_dump_crawled_resource_css_mapper.cc"],
          deps = ["//crawler/proto2/BUILD:resource_proto",
                  "//base/common/BUILD:base",
                  "//base/mapreduce/BUILD:mapreduce",
                  "//web/html/api/BUILD:parser",
                  "//web/url/BUILD:url",
                  ":proto_transcode",
                  ":gen_hbase_key",
                  ],
          )

cc_binary(name = "mr_hbase_dump_crawled_resource_css_reducer",
          srcs = ["mr_hbase_dump_crawled_resource_css_reducer.cc"],
          deps = ["//crawler/proto2/BUILD:resource_proto",
                  "//base/common/BUILD:base",
                  "//base/mapreduce/BUILD:mapreduce",
                  "//web/html/api/BUILD:parser",
                  "//web/url/BUILD:url",
                  ":proto_transcode",
                  ":gen_hbase_key",
                  ],
          )
