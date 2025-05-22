load("@openroad_rules_python//python:pip.bzl", "compile_pip_requirements")

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
    deps = [
        "@rules_cc//cc/runfiles",
    ],
    visibility = ["//visibility:public"],
)