cc_library(
  name = "content_collector",
  srcs = [
    "node_cluster.cc",
    "node_extras.cc",
    "content_collector.cc",
  ],
  deps = [
    "//base/common/BUILD:base",
    "//nlp/common/BUILD:nlp_common",
    "//web/html/api/BUILD:parser",
  ],
);
cc_test(name = "content_collector_test",
        srcs = ["content_collector_test.cc"],
        deps = [":content_collector",
                "//extend/simhash/BUILD:simhash",
                "//base/strings/BUILD:strings",
                "//base/time/BUILD:time",
                "//base/testing/BUILD:test_main",
                ]
        )
cc_binary(
  name = "content_from_url",
  srcs = [
    "content_from_url.cc",
  ],
  deps = [
    "//base/common/BUILD:base",
    "//base/strings/BUILD:strings",
    "//base/testing/BUILD:test_main",
    ":content_collector",
  ],
);
#cc_binary(
#  name = "eval_html_content",
#  srcs = [
#    "eval_html_content.cc",
#  ],
#  deps = [
#    "//base/common/BUILD:base",
#    "//base/file/BUILD:file",
#    "//nlp/common/BUILD:nlp_common",
#    "//feature/web/extractor/evaluation/BUILD:evaluation",
#    ":content_collector",
#  ],
#);
  
  
