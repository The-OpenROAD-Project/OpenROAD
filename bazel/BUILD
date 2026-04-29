load("@bazel_skylib//rules:build_test.bzl", "build_test")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_pkg//pkg:mappings.bzl", "pkg_files", "strip_prefix")
load("@rules_pkg//pkg:zip.bzl", "pkg_zip")
load("@rules_python//python:pip.bzl", "compile_pip_requirements")
load("@rules_python//python/entry_points:py_console_script_binary.bzl", "py_console_script_binary")

package(features = ["layering_check"])

exports_files(
    [
        "bzl_fmt_test.sh",
        "bzl_lint_test.sh",
        "bzl_tidy.sh",
        "fix_lint.sh",
        "install.sh",
        "tcl_fmt_test.sh",
        "tcl_lint_test.sh",
        "tcl_tidy.sh",
    ],
    visibility = ["//visibility:public"],
)

# tclint / tclfmt binaries from pip — version pinned in requirements.in
py_console_script_binary(
    name = "tclint",
    pkg = "@openroad-pip//tclint",
    visibility = ["//visibility:public"],
)

py_console_script_binary(
    name = "tclfmt",
    pkg = "@openroad-pip//tclint",
    visibility = ["//visibility:public"],
)

compile_pip_requirements(
    name = "requirements",
    src = "requirements.in",
    python_version = "3.13",
    requirements_txt = "requirements_lock_3_13.txt",
)

# Various tcl libraries that we want to embed into the binary.
pkg_files(
    name = "tcl_core_files",
    srcs = ["@tcl_lang//:tcl_core"],
    strip_prefix = strip_prefix.from_root(),
)

pkg_files(
    name = "tclreadline_files",
    srcs = ["@tclreadline//:tclreadline_scripts"],
    prefix = "tclreadline",
    strip_prefix = strip_prefix.from_root(),
)

# Packaging up all tcl resources to one complete zip file. It can be
# directly embedded in Tcl9 in a //zipfs:/ vfs.
pkg_zip(
    name = "tcl_resources_zip",
    srcs = [
        ":tcl_core_files",
        ":tclreadline_files",
    ],
)

genrule(
    name = "tcl_resources_zip_data_h",
    srcs = [":tcl_resources_zip"],
    outs = ["tcl_resources_zip_data.h"],
    cmd = "$(location :embed) kTclResourceZip $< $@",
    tools = [":embed"],
)

cc_binary(
    name = "embed",
    srcs = ["embed.cc"],
)

cc_library(
    name = "tcl_library_init",
    srcs = [
        "tcl_library_init.cc",
        "tcl_resources_zip_data.h",
    ],
    hdrs = ["tcl_library_init.h"],
    visibility = ["//visibility:public"],
    deps = ["@tcl_lang//:tcl"],
)

# small build test to check if "bazel build //src/sta:StaTclInitVar" works
# Run with "bazel test //bazel:sta_tcl_encode_test"
# Tests the tcl_encode_sta.bzl rule
build_test(
    name = "sta_tcl_encode_test",
    targets = ["//src/sta:StaTclInitVar"],
)
