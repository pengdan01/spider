
cc_library(name = "url_rule",
           srcs = ["url_rule.cc",],
           deps = ["//base/strings/BUILD:strings",
                   "//base/encoding/BUILD:encoding",
                   "//web/url/BUILD:url",
                   "//web/url_norm/BUILD:url_norm",
                   "//log_analysis/common/BUILD:log_common"
                   ],
           );

cc_test(name = "url_rule_test",
        srcs = ["url_rule_test.cc"],
        deps = ["//base/testing/BUILD:test_main",
                ":url_rule",
                ],
        );
