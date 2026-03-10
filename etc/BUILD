load("@rules_python//python:defs.bzl", "py_binary")
load("@rules_shell//shell:sh_binary.bzl", "sh_binary")

package(features = ["layering_check"])

py_binary(
    name = "file_to_string",
    srcs = [
        "file_to_string.py",
    ],
    main = "file_to_string.py",
    visibility = ["//visibility:public"],
)

sh_binary(
    name = "setup-orfs",
    srcs = ["setup_orfs_bazel.sh"],
    visibility = ["//:__pkg__"],
)
