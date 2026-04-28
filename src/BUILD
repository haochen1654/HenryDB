load(
    "@rules_cc//cc:defs.bzl",
    "cc_binary",
)

cc_binary(
    name = "client",
    srcs = ["client.cc"],
    # We define the visibilty explicitly,
    # so it can be accessed from other packages.
    # https://bazel.build/concepts/visibility
    visibility = ["//visibility:public"],
    deps = [
        "@abseil-cpp//absl/log:check",
        "@abseil-cpp//absl/log:log",
    ],
)

cc_binary(
    name = "server",
    srcs = ["server.cc"],
    # We define the visibilty explicitly,
    # so it can be accessed from other packages.
    # https://bazel.build/concepts/visibility
    visibility = ["//visibility:public"],
    deps = [
        "@abseil-cpp//absl/log:check",
        "@abseil-cpp//absl/log:log",
    ],
)
