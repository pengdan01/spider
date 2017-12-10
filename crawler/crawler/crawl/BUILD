cc_library(name = 'crawl_util',
           srcs = [
             "crawl_util.cc",
           ],
           deps = [
             "//web/url/BUILD:url",
             "//base/encoding/BUILD:encoding",
           ],
          )

cc_library(name = 'crawl',
           srcs = ["load_controller.cc",
                   "proxy_manager.cc",
                   "page_crawler.cc",
                   "page_crawler_callback.cc",
                   "page_crawler_misc.cc",
                   "time_split_saver.cc",
                   "config_parse.cc",],
           deps = ["//base/strings/BUILD:strings",
                   "//web/url/BUILD:url",
                   "//web/url_norm/BUILD:url_norm",
                   "//base/time/BUILD:time",
                   "//base/hash_function/BUILD:hash_function",
                   "//crawler/proto2/BUILD:resource_proto",
                   "//extend/config/BUILD:config",
                   "//third_party/curl/BUILD:curl",
                   ":util",
                   ":crawl_util",
                   ],
           )

cc_library(name = 'util',
           srcs = [
             "rpc_service_controller.cc",
             "data_io.cc",
           ],
           deps = ['//rpc/stumy/BUILD:stumy',
                   ]
           )

#cc_test(name = 'crawl_test',
#        srcs = ["tests/page_crawler_test.cc",
#                "tests/load_controller_test.cc",
#                "tests/proxy_manager_test.cc",
#                "tests/config_test.cc",
#                "tests/crawl_util_test.cc",
#                ],
#        deps = ["//base/testing/BUILD:test_main",
#                "//extend/web_server/BUILD:web_server",
#                ":crawl",
#                ":crawl_util",
#                ],
#        )
