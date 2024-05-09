load("@com_github_google_flatbuffers//:build_defs.bzl", "flatbuffer_cc_library")
load("@rules_cc//cc:defs.bzl", "cc_test")

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

flatbuffer_cc_library(
    name = "monster_fbs",
    srcs = ["monster.fbs"],
    flatc_args = [
        "--gen-object-api",
    ],
)
flatbuffer_cc_library(
    name = "simple_fbs",
    srcs = ["simple_kv.fbs", "simple_list.fbs"],
    flatc_args = [
        "--gen-object-api",
    ],
)

cc_binary(
    name = "test_fbs",
    srcs = ["test.cc"],
    linkopts = LINKOPTS,
    deps = [
        ":monster_fbs",
        ":simple_fbs",
        "//rdict:rdict",
        "@com_github_google_flatbuffers//:flatbuffers",
    ],
)

cc_test(
    name = "test_rdict_kv",
    size = "small",
    srcs = ["test_rdict_kv.cc"],
    linkopts = LINKOPTS,
    deps = [
        "//rdict:rdict",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_rdict_list",
    size = "small",
    srcs = ["test_rdict_list.cc"],
    linkopts = LINKOPTS,
    deps = [
          "//rdict:rdict",
        "@com_google_googletest//:gtest_main",
    ],
)
