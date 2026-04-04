load("@bazel_skylib//rules:build_test.bzl", "build_test")
load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_pkg//pkg:mappings.bzl", "pkg_files", "strip_prefix")
load("@rules_pkg//pkg:zip.bzl", "pkg_zip")
load("@rules_python//python:pip.bzl", "compile_pip_requirements")
load("@rules_python//python/entry_points:py_console_script_binary.bzl", "py_console_script_binary")
load("//bazel:extract_zip.bzl", "extract_zip")

package(features = ["layering_check"])

exports_files(
    [
        "fix_lint.sh",
        "install.sh",
        "tcl_lint_test.sh",
        "tcl_fmt_test.sh",
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

# ... however, Tcl8 still needs access to a directory, so unpack in that case.
extract_zip(
    name = "tcl_resources_dir",
    src = ":tcl_resources_zip",
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

# As long as we support tcl8 (using runfiles) we need this flag.
bool_flag(
    name = "use_zipfs",
    build_setting_default = False,  # False until we can use tcl9
)

config_setting(
    name = "zipfs_config",
    flag_values = {":use_zipfs": "true"},
)

# Library initialization is a bit different for tcl8 and tcl9.
# In tcl9, we can use a zipped version of the needed include files, but
# for tcl8, we have to use the unpacked zip file and point our runfiles
# to it.
cc_library(
    name = "tcl_library_init",
    srcs = ["tcl_library_init.cc"] +
           select({
               ":zipfs_config": ["tcl_resources_zip_data.h"],
               "//conditions:default": [],
           }),
    hdrs = ["tcl_library_init.h"],
    copts = select({
        ":zipfs_config": [],
        "//conditions:default": ["-DUSE_TCL_RUNFILE_INIT"],
    }),
    data = select({
        ":zipfs_config": [],
        "//conditions:default": [":tcl_resources_dir"],
    }),
    visibility = ["//visibility:public"],
    deps = [
        "@tcl_lang//:tcl",
    ] + select({
        ":zipfs_config": [],
        "//conditions:default": ["@rules_cc//cc/runfiles"],
    }),
)

# shim old library as it is still referenced in src/sta, implementing the
# old behavior.
cc_library(
    name = "runfiles",
    srcs = ["InitRunFiles.cpp"],
    data = [":tcl_resources_dir"],
    visibility = ["//visibility:public"],
    deps = [
        "@rules_cc//cc/runfiles",
        "@tcl_lang//:tcl",
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
