# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("//test:regression.bzl", "messages_txt")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "nangate45_fixture",
    testonly = 1,
    hdrs = ["include/tst/nangate45_fixture.h"],
    data = [
        "//test:nangate45_data",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        ":tst",
        "//src/odb/src/db",
    ],
)

cc_library(
    name = "sky130_fixture",
    testonly = 1,
    hdrs = ["include/tst/sky130_fixture.h"],
    data = [
        "//test:sky130hd_data",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        ":tst",
        "//src/odb/src/db",
    ],
)

cc_library(
    name = "db_fixture",
    testonly = 1,
    srcs = [
        "src/db_fixture.cpp",
    ],
    hdrs = [
        "include/tst/db_fixture.h",
    ],
    defines = [
        "BAZEL_BUILD",
    ],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//src/odb/src/db",
        "//src/odb/src/lefin",
        "//src/utl",
        "@bazel_tools//tools/cpp/runfiles",
        "@googletest//:gtest",
    ],
)

cc_library(
    name = "tst",
    testonly = 1,
    srcs = [
        "src/fixture.cpp",
    ],
    hdrs = [
        "include/tst/fixture.h",
    ],
    defines = [
        "BAZEL_BUILD",
    ],
    includes = [
        "include",
    ],
    deps = [
        ":db_fixture",
        "//src/dbSta",
        "//src/odb/src/db",
        "//src/odb/src/lefin",
        "//src/sta:opensta_lib",
        "//src/utl",
        "@tcl_lang//:tcl",
    ],
)

# LoadedDesign: one netlist loaded through one link mode, owning its own
# db/sta. Deliberately separate from integrated_fixture so consumers that only
# need to read/link/write a netlist do not link the optimization tool chain.
cc_library(
    name = "loaded_design",
    testonly = 1,
    srcs = ["src/loaded_design.cpp"],
    hdrs = ["include/tst/loaded_design.h"],
    data = [
        "//test:nangate45_data",
        "//test:sky130hd_data",
    ],
    defines = [
        "BAZEL_BUILD",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        ":db_fixture",
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/dbSta:dbReadVerilog",
        "//src/odb/src/db",
        "//src/odb/src/lefin",
        "//src/sta:opensta_lib",
        "//src/utl",
        "@tcl_lang//:tcl",
    ],
)

cc_library(
    name = "integrated_fixture",
    testonly = 1,
    srcs = ["src/IntegratedFixture.cpp"],
    hdrs = ["include/tst/IntegratedFixture.h"],
    data = [
        "//test:nangate45_data",
        "//test:sky130hd_data",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        ":loaded_design",
        ":tst",
        "//src/ant",
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/dbSta:dbReadVerilog",
        "//src/dpl",
        "//src/est",
        "//src/grt",
        "//src/odb/src/db",
        "//src/rsz",
        "//src/sta:opensta_lib",
        "//src/stt",
        "//src/utl",
        "@googletest//:gtest",
    ],
)

# The kepler-formal binary, built from the pinned @kepler-formal module. Add it
# to a test's `data` to get the LEC engine as a runfile; lec.cpp resolves it
# from there, falling back to $KEPLER_FORMAL / $PATH for a locally installed
# binary.
#
# A plain alias: nothing in this subgraph is built with rules_foreign_cc, so
# the build needs no accommodation for OpenROAD's global `-xc++`. That flag is
# inert for a normal Bazel C++ compile and only ever mattered because
# rules_foreign_cc copies cxxopts into CMAKE_CXX_FLAGS, which CMake then reuses
# at link time; with TBB coming from the BCR module instead of a cmake() target
# there is no such build left.
alias(
    name = "kepler_formal_bin",
    testonly = 1,
    actual = "@kepler-formal//src/bin:kepler-formal",
    visibility = ["//visibility:public"],
)

# LEC via kepler-formal, invoked as a subprocess and never linked (GPL-3.0 vs
# BSD-3-Clause). The binary is resolved at run time -- from the runfiles of the
# test that declares :kepler_formal_bin in data, or from $KEPLER_FORMAL / $PATH
# -- so this library links nothing of it.
cc_library(
    name = "lec",
    testonly = 1,
    srcs = ["src/lec.cpp"],
    hdrs = ["include/tst/lec.h"],
    # Every test that LECs gets the binary in its runfiles, so under bazel the
    # tool is never missing and the tests can never quietly degrade.
    data = [":kepler_formal_bin"],
    defines = ["BAZEL_BUILD"],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "@bazel_tools//tools/cpp/runfiles",
        "@googletest//:gtest",
    ],
)

# Test Parity Note: registered in bazel only. See src/dbSta/test/BUILD for the
# rationale behind bazel-only C++ test registration in this area.
cc_test(
    name = "TestLoadedDesign",
    srcs = [
        "test/cpp/TestLoadedDesign.cpp",
        "test/cpp/TestNetlists.h",
    ],
    deps = [
        ":db_fixture",
        ":loaded_design",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

# Test Parity Note: bazel only, as above.
cc_test(
    name = "TestLec",
    srcs = [
        "test/cpp/TestLec.cpp",
        "test/cpp/TestNetlists.h",
    ],
    deps = [
        ":db_fixture",
        ":lec",
        ":loaded_design",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

messages_txt(
    visibility = ["//docs:__pkg__"],
)
