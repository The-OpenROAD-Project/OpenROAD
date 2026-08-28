# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors

# Test Parity Note: All tests from CMakeLists.txt are included in this BUILD file.
# No tests are intentionally excluded. See .kiro/specs/bazel-cmake-test-parity/intentional_exclusions.md

load("@bazel_skylib//rules:write_file.bzl", "write_file")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("//test:regression.bzl", "regression_test")
load(
    ":hier_case_test.bzl",
    "hier_case_test",
    "hier_case_tests",
    "render_top_overrides",
)
load(
    ":hier_expected_fail.bzl",
    "CONFORMANCE_EXPECTED_FAIL",
    "STRUCTURAL_EXPECTED_FAIL",
    "conformance_manifest_by_netlist",
    "manifest_lines",
    "structural_manifest_by_netlist",
)

package(features = ["layering_check"])

exports_files(["synthetic_linear.lib"])

# From CMakeLists.txt or_integration_tests(TESTS
ALL_TESTS = [
    "3dic_cross",
    "3dic_get_cells",
    "3dic_read_db",
    "block_sta1",
    "check_axioms",
    "clock_pin",
    "constant1",
    "escape_slash",
    "escape_slash_hier",
    "find_clks1",
    "find_clks2",
    "get_nets1_hier",
    "get_ports1",
    "get_ports1_hier",
    "hier2",
    "hier3",
    "hier_escape_port",
    "hierclock",
    "hierwrite",
    "hier_deep",
    "hier_highest_connected_net",
    "hier_weird_port",
    "levelize_drvr_vertices1",
    "make_port",
    "modnet_port_alias",
    "network_edit1",
    "power1",
    "read_def_bus_port",
    "read_liberty1",
    "read_vcd",
    "read_verilog1",
    "read_verilog2",
    "read_verilog3",
    "read_verilog4",
    "read_verilog5",
    "read_verilog6",
    "read_verilog7",
    "read_verilog8",
    "read_verilog9",
    "read_verilog10",
    "read_verilog10_no_prop",
    "read_verilog11",
    "readdb_hier",
    "report_cell_usage",
    "report_cell_usage_file",
    "report_cell_usage_modinsts",
    "report_cell_usage_modinsts_metrics",
    "report_cell_usage_physical_only",
    "report_cell_usage_wide_columns",
    "report_json1",
    "report_timing_histogram",
    "report_logic_depth_histogram",
    "sdc_get1",
    "sdc_modinst_slash",
    "sdc_names1",
    "sdc_names2",
    "spef_def_names",
    "sta1",
    "sta2",
    "sta3",
    "sta4",
    "sta5",
    "sdc_in_db1",
    "sdc_in_db2",
    "sdc_in_db3",
    "sta_to_db1",
    "sta_to_db1_hier",
    "write_sdc1",
    "write_verilog1",
    "write_verilog2",
    "write_verilog3",
    "write_verilog4",
    "write_verilog5",
    "write_verilog6",
    "write_verilog7",
    "write_verilog8",
    "write_verilog9",
    "write_verilog9_hier",
]

filegroup(
    name = "regression_resources",
    srcs = [
        "Nangate45/Nangate45.lef",
        "Nangate45/Nangate45.pdn.tcl",
        "Nangate45/Nangate45.rc",
        "Nangate45/Nangate45.tracks",
        "Nangate45/Nangate45.vars",
        "Nangate45/Nangate45_fast.lib",
        "Nangate45/Nangate45_slow.lib",
        "Nangate45/Nangate45_stdcell.lef",
        "Nangate45/Nangate45_tech.lef",
        "Nangate45/Nangate45_typ.lib",
        "helpers.tcl",
        "liberty1.lef",
        "liberty1.lib",
    ],
)

[
    filegroup(
        name = test_name + "_resources",
        srcs = [":regression_resources"] + glob(
            [
                test_name + ".*",
            ],
        ) + {
            "3dic_cross": [
                "//src/odb/test:regression_resources",
                "3dic_cross_flop.3dbv",
                "3dic_cross_flop_a.bmap",
                "3dic_cross_flop_a.def",
                "3dic_cross_flop_b.bmap",
                "3dic_cross_flop_b.def",
                "3dic_cross_top.v",
            ],
            "3dic_get_cells": [
                "//src/odb/test:regression_resources",
            ],
            "3dic_read_db": [
                "//src/odb/test:regression_resources",
                "3dic_cross.3dbx",
                "3dic_cross_flop.3dbv",
                "3dic_cross_flop_a.bmap",
                "3dic_cross_flop_a.def",
                "3dic_cross_flop_b.bmap",
                "3dic_cross_flop_b.def",
                "3dic_cross_top.v",
            ],
            "block_sta1": [
                "example1_typ.lib",
                "example1.def",
                "example1.lef",
            ],
            "find_clks1": [
                "pad.lef",
                "pad.lib",
            ],
            "get_nets1_hier": [
                "hier1.v",
            ],
            "get_ports1_hier": [
                "get_ports1.v",
            ],
            "hier2": [
                "example1_typ.lib",
                "example1.lef",
                "hier2_out.vok",
            ],
            "hier3": [
                "example1_typ.lib",
                "example1.lef",
            ],
            "hier_deep": [
                "example1_typ.lib",
                "example1.lef",
            ],
            "hierclock": [
                "hierclock_gate.v",
                "hierclock_out.vok",
            ],
            "levelize_drvr_vertices1": [
                "gcd_asap7.v",
                "asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz",
                "asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz",
                "asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz",
                "asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz",
                "asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib",
                "asap7/asap7_tech_1x_201209.lef",
                "asap7/asap7sc7p5t_28_R_1x_220121a.lef",
            ],
            "network_edit1": [
                "reg3.def",
            ],
            "read_vcd": glob(
                [
                    "Element.*",
                    "MockArray.*",
                    "asap7/*",
                ],
            ),
            "read_verilog1": [
                "reg1.v",
            ],
            "read_verilog10": [
                "hier1.v",
            ],
            "read_verilog10_no_prop": [
                "hier1.v",
            ],
            "read_verilog11": [
                "hier1.v",
            ],
            "read_verilog2": [
                "hier1.v",
            ],
            "read_verilog3": [
                "bus1.lef",
                "bus1.lib",
                "bus1.v",
            ],
            "read_verilog5": [
                "bidir1.lef",
                "bidir1.lib",
            ],
            "read_verilog9": [
                "reg1.v",
            ],
            "readdb_hier": [
                "hier1.v",
            ],
            "report_cell_usage_file": [
                "report_cell_usage.def",
            ],
            "report_cell_usage_modinsts": [
                "hier1.v",
            ],
            "report_cell_usage_modinsts_metrics": [
                "hier1.v",
            ],
            "report_cell_usage_physical_only": [
                "report_cell_usage_no_taps.def",
            ],
            "report_cell_usage_wide_columns": [
                "Nangate45/fakeram45_1024x32.lef",
                "report_cell_usage.def",
            ],
            "report_json1": [
                "reg6.def",
            ],
            "report_timing_histogram": [
                "example1_slow.lib",
                "example1.def",
                "example1.lef",
            ],
            "sdc_get1": [
                "example1_slow.lib",
                "example1.def",
                "example1.lef",
            ],
            "sdc_in_db1": [
                "hier1.sdc",
                "hier1.v",
            ],
            "sdc_in_db2": [
                "sdc_in_db.odb",
            ],
            "sdc_in_db3": [
                "sdc_in_db.odb",
            ],
            "sdc_names1": [
                "hier1.def",
            ],
            "spef_def_names": [
                "example1_slow.lib",
                "example1.lef",
            ],
            "sta1": [
                "example1_slow.lib",
                "example1.def",
                "example1.lef",
            ],
            "sta2": [
                "example1_slow.lib",
                "example1.def",
                "example1.lef",
                "example1.sdf",
                "example1.v",
            ],
            "sta3": [
                "example1_slow.lib",
                "example1_fast.lib",
                "example1.def",
                "example1.lef",
            ],
            "sta4": [
                "example1_slow.lib",
                "example1.dspef",
                "example1.def",
                "example1.lef",
            ],
            "sta5": [
                "example1_fast.lib",
                "example1_slow.lib",
                "example1_typ.lib",
                "example1.def",
                "example1.lef",
            ],
            "sta_to_db1": [
                "example1_slow.lib",
                "example1.def",
                "example1.lef",
            ],
            "sta_to_db1_hier": [
                "example1_typ.lib",
                "example1.lef",
                "hier3.v",
            ],
            "write_sdc1": [
                "hier1.sdc",
                "hier1.v",
            ],
            "write_verilog1": [
                "reg1.def",
            ],
            "write_verilog2": [
                "reg4.def",
            ],
            "write_verilog3": [
                "bus1.def",
                "bus1.lef",
                "bus1.lib",
            ],
            "write_verilog6": [
                "write_verilog5.def",
            ],
            "write_verilog8": [
                "reg5.v",
            ],
            "write_verilog9_hier": [
                "write_verilog9.v",
            ],
        }.get(test_name, []),
    )
    for test_name in ALL_TESTS
]

[
    regression_test(
        name = test_name,
        data = [":" + test_name + "_resources"],
        visibility = ["//visibility:public"],
    )
    for test_name in ALL_TESTS
]

cc_test(
    name = "dbsta_hconn_unittest",
    srcs = ["cpp/TestHconn.cpp"],
    data = [
        "Nangate45/Nangate45.lef",
        "Nangate45/Nangate45_typ.lib",
    ],
    deps = [
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/odb/src/db",
        "//src/odb/src/lefin",
        "//src/sta:opensta_lib",
        "//src/tst",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

# Both hierarchy suites below take their whole corpus from a directory scan:
# every netlist in cpp/hier_cases/ (and its inherited/ and structural/
# subdirectories) is a case, so adding one means adding a file and nothing else.
# What a folder listing cannot carry -- a case's top module, and which defects
# it is known to hit -- lives here in the build rules instead, so none of it
# sits in a text file that a contributor has to find and hand-edit.
#
# Top-level module of each netlist. Nearly every case uses "top"; the exceptions
# are cases where the top module's odd name IS the hazard under test (a keyword,
# a digit-leading name, a name that collides with a uniquified clone).
HIER_TOP_OVERRIDES = {
    "bx_collisions_uniq_clone_steals_top.v": "sub_i2",
    "bx_naming_escaped_top_kw.v": "module",
    "bx_naming_escaped_top_special.v": "t+p",
    "bx_naming_plain_dollar_top.v": "top$1",
    "bx_naming_plain_topname_TOP_sub_top.v": "TOP",
    "s2_escaping_esc_top_over_hier_d3.v": "t*p",
    "wb_dbsta_link_uniq_clone_eq_top_name.v": "sub_u1",
    "wb_dbsta_link_uniq_clone_eq_top_name2.v": "sub_u2",
    "wb_writer_digit_topmodule.v": "1top",
}

# The corpus, as three groups of netlists. Both suites below turn these into one
# test target per netlist: bazel caches per target, so a case that shares a
# target with 1200 others cannot be cached, rerun or read in isolation.
HIER_NETLISTS = glob(
    ["cpp/hier_cases/*.v"],
    allow_empty = True,
)

# Symlinks to fixtures owned by other suites. The symlink is what the corpus
# scan sees; the filegroups below are where the bytes come from, so a case here
# depends on both.
HIER_INHERITED_NETLISTS = glob(
    ["cpp/hier_cases/inherited/*.v"],
    allow_empty = True,
)

HIER_INHERITED_FIXTURES = [
    "//src/odb/test:hier_corpus_netlists",
    "//src/rsz/test:hier_corpus_netlists",
]

# Cases only the structural suite runs: a LEC cannot adjudicate them (several
# are refused by the reader outright), while a structural diff can.
HIER_STRUCTURAL_NETLISTS = glob(
    ["cpp/hier_cases/structural/*.v"],
    allow_empty = True,
)

# cpp/hier_cases/crash/*.v is a data dependency of no target here -- those
# netlists kill the process.

# The corpus by name, keyed the way each suite's manifest keys it: the structural
# manifest keeps a subdirectory case's prefix (its gtest name and manifest key
# stay unambiguous that way), the conformance manifest keys every case by its
# bare file name.
CONFORMANCE_CORPUS = [f.rpartition("/")[2] for f in HIER_NETLISTS] + [
    f.rpartition("/")[2]
    for f in HIER_INHERITED_NETLISTS
]

STRUCTURAL_CORPUS = [f.rpartition("/")[2] for f in HIER_NETLISTS] + [
    "inherited/" + f.rpartition("/")[2]
    for f in HIER_INHERITED_NETLISTS
] + [
    "structural/" + f.rpartition("/")[2]
    for f in HIER_STRUCTURAL_NETLISTS
]

# Each suite's known failures, grouped by the case they name. A per-case target
# carries its own rows in its environment, so editing one defect's entry
# invalidates the cases it names and nothing else; rendering the same rows into a
# manifest file gives the corpus-wide target the whole list, which is what its
# staleness check needs. Generating both from hier_expected_fail.bzl rather than
# checking a manifest in keeps every piece of a case's metadata -- its top
# module, and which defects it is known to hit -- in the build rules, and lets
# Starlark reject a malformed entry, an unknown check, or an entry naming no
# corpus case when the package loads, instead of the C++ parser dropping it
# silently at run time.
CONFORMANCE_XFAIL_ROWS = conformance_manifest_by_netlist(
    CONFORMANCE_EXPECTED_FAIL,
)

STRUCTURAL_XFAIL_ROWS = structural_manifest_by_netlist(
    STRUCTURAL_EXPECTED_FAIL,
    STRUCTURAL_CORPUS,
)

write_file(
    name = "hier_expected_fail_manifest",
    out = "cpp/hier_cases/expected_fail.txt",
    content = manifest_lines(CONFORMANCE_XFAIL_ROWS, CONFORMANCE_CORPUS),
)

write_file(
    name = "hier_structural_expected_fail_manifest",
    out = "cpp/hier_cases/structural_expected_fail.txt",
    content = manifest_lines(STRUCTURAL_XFAIL_ROWS, STRUCTURAL_CORPUS),
)

# Hierarchy conformance: both link modes LEC'd against the input netlist.
#
# Needs the external kepler-formal binary at run time (resolved from
# $KEPLER_FORMAL or the test's runfiles -- see src/tst/include/tst/lec.h);
# without it the tests fail with an actionable message rather than skipping. The
# binary rides along in the runner's runfiles, so every case target gets it.
#
# Test Parity Note: this suite and dbsta_hier_structural below are registered in
# bazel ONLY, deliberately. Both read a generated manifest and take their
# per-case metadata from HIER_TOP_OVERRIDES, this one needs the kepler-formal
# binary, and the per-case targets are generated by a Starlark macro -- none of
# which a CMake registration can reproduce today. A CMake registration would
# build and then fail on every case for want of them, so there is nothing to
# gain by adding one until the metadata has a build-system-independent home.
cc_binary(
    name = "hier_conformance_runner",
    testonly = 1,
    srcs = ["cpp/TestHierConformance.cpp"],
    deps = [
        "//src/tst:db_fixture",
        "//src/tst:lec",
        "//src/tst:loaded_design",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

# The corpus-wide checks: is the corpus loaded, are its TARGETS tags unique, is
# the manifest well formed. They need the whole corpus, which is why they are one
# target and not 1200; the inverted filter is what keeps them from also running
# the per-case tests below.
hier_case_test(
    name = "dbsta_hier_conformance_corpus",
    size = "small",
    data = [
        ":hier_expected_fail_manifest",
    ] + HIER_NETLISTS + HIER_INHERITED_NETLISTS + HIER_INHERITED_FIXTURES,
    gtest_filter = "-*MatchesInput*",
    runner = ":hier_conformance_runner",
    top_overrides = render_top_overrides(HIER_TOP_OVERRIDES),
)

test_suite(
    name = "dbsta_hier_conformance",
    tests = [":dbsta_hier_conformance_corpus"] + hier_case_tests(
        name = "dbsta_hier_conformance_case",
        expected_fail = CONFORMANCE_XFAIL_ROWS,
        netlists = HIER_NETLISTS,
        runner = ":hier_conformance_runner",
        top_overrides = HIER_TOP_OVERRIDES,
    ) + hier_case_tests(
        name = "dbsta_hier_conformance_case",
        data = HIER_INHERITED_FIXTURES,
        expected_fail = CONFORMANCE_XFAIL_ROWS,
        netlists = HIER_INHERITED_NETLISTS,
        runner = ":hier_conformance_runner",
        subdir = "inherited/",
        top_overrides = HIER_TOP_OVERRIDES,
    ),
)

# Structural conformance: both link modes' emitted netlists diffed against the
# input netlist for names, port order, module set, declared objects and instance
# bindings -- the fidelity aspects a LEC is blind to. Needs no external tool.
cc_binary(
    name = "hier_structural_runner",
    testonly = 1,
    srcs = ["cpp/TestHierStructural.cpp"],
    deps = [
        "//src/tst:db_fixture",
        "//src/tst:loaded_design",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

# The corpus-wide checks, plus this suite's scanner and detector unit tests: they
# own no netlist, so they belong with the corpus rather than with a case.
hier_case_test(
    name = "dbsta_hier_structural_corpus",
    size = "small",
    data = [
               ":hier_structural_expected_fail_manifest",
           ] + HIER_NETLISTS + HIER_INHERITED_NETLISTS + HIER_INHERITED_FIXTURES +
           HIER_STRUCTURAL_NETLISTS,
    gtest_filter = "-*MatchesInput*",
    runner = ":hier_structural_runner",
    top_overrides = render_top_overrides(HIER_TOP_OVERRIDES),
)

test_suite(
    name = "dbsta_hier_structural",
    tests = [":dbsta_hier_structural_corpus"] + hier_case_tests(
        name = "dbsta_hier_structural_case",
        expected_fail = STRUCTURAL_XFAIL_ROWS,
        netlists = HIER_NETLISTS,
        runner = ":hier_structural_runner",
        top_overrides = HIER_TOP_OVERRIDES,
    ) + hier_case_tests(
        name = "dbsta_hier_structural_case",
        data = HIER_INHERITED_FIXTURES,
        expected_fail = STRUCTURAL_XFAIL_ROWS,
        netlists = HIER_INHERITED_NETLISTS,
        runner = ":hier_structural_runner",
        subdir = "inherited/",
        top_overrides = HIER_TOP_OVERRIDES,
    ) + hier_case_tests(
        name = "dbsta_hier_structural_case",
        expected_fail = STRUCTURAL_XFAIL_ROWS,
        netlists = HIER_STRUCTURAL_NETLISTS,
        runner = ":hier_structural_runner",
        subdir = "structural/",
        top_overrides = HIER_TOP_OVERRIDES,
    ),
)

cc_test(
    name = "dbsta_unittest",
    srcs = ["cpp/TestDbSta.cc"],
    data = [
        "Nangate45/Nangate45.lef",
        "Nangate45/Nangate45_typ.lib",
        "cpp/TestDbSta_0.v",
        "cpp/TestDbSta_StalePrevPath.v",
    ],
    deps = [
        "//src/dbSta:dbNetwork",
        "//src/odb/src/db",
        "//src/sta:opensta_lib",
        "//src/tst:integrated_fixture",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_read_verilog",
    srcs = ["cpp/TestReadVerilog.cpp"],
    data = glob(["cpp/TestReadVerilog*.v"]),
    deps = [
        "//src/odb/src/db",
        "//src/tst:integrated_fixture",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_write_verilog",
    srcs = ["cpp/TestWriteVerilog.cpp"],
    data = glob(["cpp/TestWriteVerilog*.v"]),
    deps = [
        "//src/sta:opensta_lib",
        "//src/tst:integrated_fixture",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "dbsta_lambertw_test",
    srcs = ["cpp/TestLambertW.cc"],
    data = [
        "Nangate45/Nangate45.lef",
        "Nangate45/Nangate45_typ.lib",
        "cpp/TestDbSta_0.v",
    ],
    deps = [
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/dbSta:lambertw_delay_calc",
        "//src/odb/src/db",
        "//src/sta:opensta_lib",
        "//src/tst:integrated_fixture",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
