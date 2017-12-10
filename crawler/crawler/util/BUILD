cc_library(name = "util",
           srcs = ["auto_split_file.cc",
                   "command.cc",
                   "text_handler.cc",
                   "ip_scope.cc",
                   "text_handler_main.cc",
                   "dns_resolve.cc",
                   ],
           excludes = ["./*_test.cc", "./*_main.cc"],
           deps = ["//base/common/BUILD:base",
                   "//base/hash_function/BUILD:hash_function",
                   "//base/thread/BUILD:thread",
                   "//base/encoding/BUILD:encoding",
                   "//extend/regexp/re3/BUILD:re3",
                   "//web/url/BUILD:url",
                   "//third_party/curl/BUILD:curl",
                   ]
           )
cc_test(name = "auto_split_file_test",
        srcs = ["auto_split_file_test.cc",],
        deps = ["//base/common/BUILD:base",
                "//base/testing/BUILD:test_main",
                ":util"]
        )
cc_test(name = "dns_resolve_test",
        srcs = ["dns_resolve_test.cc",],
        deps = ["//base/common/BUILD:base",
                "//base/testing/BUILD:test_main",
                ":util"]
        )

cc_test(name = "text_handler_test",
        srcs = ["text_handler_test.cc",],
        deps = ["//base/common/BUILD:base",
                "//base/testing/BUILD:test_main",
                "//base/file/BUILD:file",
                ":util"]
        )

cc_test(name = "ip_scope_test",
        srcs = ["ip_scope_test.cc",],
        deps = [":util",
                "//base/testing/BUILD:test_main",
                ]
        )

cc_test(name = "command_test",
        srcs = ["./command_test.cc",],
        deps = ["//base/common/BUILD:base",
                ":util",
                "//base/testing/BUILD:test_main",
                ]
        )

cc_binary(name = "test_regexp",
          srcs = ["test_regexp.cc",],
          deps = ["//base/common/BUILD:base",
                  ":util",
                  "//base/file/BUILD:file",]
          )

#cc_binary(name = "url_test",
#          srcs = ["url_test.cc"],
#          deps = ["//base/common/BUILD:base",":util"]
#          )
