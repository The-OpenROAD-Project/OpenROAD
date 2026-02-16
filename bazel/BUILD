load("@openroad_rules_python//python:pip.bzl", "compile_pip_requirements")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

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
