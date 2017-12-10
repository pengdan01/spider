cc_library(name = "price_recg",
           srcs = ["*.cc"],
           excludes = ["*_test.cc",],
           deps = ["//base/common/BUILD:base",
                   "//base/strings/BUILD:strings",
                   "//base/file/BUILD:file",
                   "//third_party/OpenCV/BUILD:opencv",
                   ],
           )

cc_test(name = "price_recg_test",
        srcs = ["*_test.cc",],
        deps = ["//base/testing/BUILD:test_main",
                ":price_recg"],
        )

cc_data(name = "price_recg_data",
        srcs = ["./data/training/*", "./data/testing/*"],
        )

# cc_binary(name = "cv_run",
#           srcs = ["run.cc",],
#           deps = [":simple_cv",
#                   "//base/common/BUILD:base",],
#           )
