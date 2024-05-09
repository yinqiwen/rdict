
package(default_visibility = ["//visibility:public"])
LINKOPTS = [
    "-L/usr/local/lib",
    "-L/usr/local/lib64",
    "-lfolly",
    "-lfmt",
    "-lgflags",
    "-lglog",
    "-ldouble-conversion",
    "-liberty",
    "-levent",
    "-lunwind",
    "-lcrypto",
    "-lssl",
    "-ldl",
    "-lrt",
    "-lpthread",
    "-lstdc++fs",
    "-lboost_context",
    "-lboost_filesystem",
]

cc_library(
    name = "mmap_file",
    srcs = [
        "mmap_file.cc",
    ],
    hdrs = [
        "mmap_file.h",
    ],
    deps = [
        "@com_google_absl//absl/status:statusor",
    ],
)

cc_library(
    name = "rdict",
    hdrs = [
        "kv.h",
        "list.h",
        "common.h",
        "fbs_kv.h",
        "fbs_list.h",
    ],
    srcs = [
        "list.cc",
    ],
    deps = [
        ":mmap_file",
        "@com_google_absl//absl/status:statusor",
        "@com_github_google_flatbuffers//:flatbuffers",
    ],
)

cc_library(
    name = "fbs_builder",
    srcs = ["fbs_builder.cc"],
    hdrs = [
        "fbs_builder.h",
    ],
    deps = [
        ":rdict",
        "@com_github_google_flatbuffers//:flatbuffers",
        "@com_google_absl//absl/status:statusor",
    ],
)

cc_binary(
    name = "rdict_builder",
    srcs = ["rdict_builder.cc"],
    deps = [
        ":fbs_builder",
    ],
     linkopts = LINKOPTS,
)

