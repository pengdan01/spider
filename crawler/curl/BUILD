cc_library(name = "curl",
           cppflags = ["-DHAVE_CONFIG_H",
                       "-DCURL_HIDDEN_SYMBOLS",
                       "-Ithird_party/curl/include/curl",
                       ],
           ext_cppflags = ["-Ithird_party/curl/include",
                           ],
           cflags = ["-fno-strict-aliasing"],
           deps = ["//third_party/c-ares/BUILD:c-ares"],
)

