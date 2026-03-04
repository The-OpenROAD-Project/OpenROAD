load("@bazel_skylib//rules:build_test.bzl", "build_test")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_python//python:pip.bzl", "compile_pip_requirements")

package(features = ["layering_check"])

exports_files(
    ["install.sh"],
    visibility = ["//visibility:public"],
)

compile_pip_requirements(
    name = "requirements",
    src = "requirements.in",
    python_version = "3.13",
    requirements_txt = "requirements_lock_3_13.txt",
)

cc_library(
    name = "runfiles",
    srcs = [
        "InitRunFiles.cpp",
    ],
    data = [
        "@tcl_lang//:tcl_core",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@rules_cc//cc/runfiles",
    ],
    alwayslink = True,
)

# small build test to check if "bazel build //src/sta:StaTclInitVar" works
# Run with "bazel test //bazel:sta_tcl_encode_test"
# Tests the tcl_encode_sta.bzl rule
build_test(
    name = "sta_tcl_encode_test",
    targets = ["//src/sta:StaTclInitVar"],
)
