load("@rules_python//python:defs.bzl", "py_binary")

package(features = ["layering_check"])

py_binary(
    name = "file_to_string",
    srcs = [
        "file_to_string.py",
    ],
    main = "file_to_string.py",
    visibility = ["//visibility:public"],
)

exports_files(
    [
        "find_dup_ids.sh",
        "find_messages.py",
    ],
    visibility = ["//visibility:public"],
)
