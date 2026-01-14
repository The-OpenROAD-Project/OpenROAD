load("@openroad_rules_python//python:defs.bzl", "py_library", "py_test")

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors
load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

py_library(
    name = "helper",
    srcs = [
        "helper.py",
    ],
    imports = ["."],
)

py_library(
    name = "odbUnitTest",
    srcs = [
        "odbUnitTest.py",
    ],
    imports = ["."],
)

py_test(
    name = "test_block",
    srcs = [
        "test_block.py",
    ],
    deps = [
        ":helper",
        ":odbUnitTest",
        "//openroad:openroadpy",
        "//src/odb:odb_py",
    ],
)

py_test(
    name = "test_bterm",
    srcs = [
        "test_bterm.py",
    ],
    deps = [
        ":helper",
        ":odbUnitTest",
        "//openroad:openroadpy",
        "//src/odb:odb_py",
    ],
)

py_test(
    name = "test_destroy",
    srcs = [
        "test_destroy.py",
    ],
    deps = [
        ":helper",
        ":odbUnitTest",
        "//openroad:openroadpy",
        "//src/odb:odb_py",
    ],
)

py_test(
    name = "test_group",
    srcs = [
        "test_group.py",
    ],
    deps = [
        ":helper",
        ":odbUnitTest",
        "//openroad:openroadpy",
        "//src/odb:odb_py",
    ],
)

py_test(
    name = "test_inst",
    srcs = [
        "test_inst.py",
    ],
    deps = [
        ":helper",
        ":odbUnitTest",
        "//openroad:openroadpy",
        "//src/odb:odb_py",
    ],
)

py_test(
    name = "test_iterm",
    srcs = [
        "test_iterm.py",
    ],
    deps = [
        ":helper",
        ":odbUnitTest",
        "//openroad:openroadpy",
        "//src/odb:odb_py",
    ],
)

py_test(
    name = "test_module",
    srcs = [
        "test_module.py",
    ],
    deps = [
        ":helper",
        ":odbUnitTest",
        "//openroad:openroadpy",
        "//src/odb:odb_py",
    ],
)

py_test(
    name = "test_net",
    srcs = [
        "test_net.py",
    ],
    deps = [
        ":helper",
        ":odbUnitTest",
        "//openroad:openroadpy",
        "//src/odb:odb_py",
    ],
)

py_test(
    name = "test_wire_codec",
    srcs = [
        "test_wire_codec.py",
    ],
    deps = [
        ":helper",
        ":odbUnitTest",
        "//openroad:openroadpy",
        "//src/odb:odb_py",
    ],
)

# From CMakeLists.txt or_integration_tests(TESTS
COMPULSORY_TESTS = [
    "abstract_origin",
    "all_pins_placed1",
    "all_pins_placed2",
    "bterm_hier_create",
    "check_3dblox",
    "check_routing_tracks",
    "create_blockage",
    "create_obstruction",
    "create_sboxes",
    "def_dup_net",
    "def_parser",
    "design_is_routed1",
    "design_is_routed2",
    "design_is_routed3",
    "design_is_routed_fail1",
    "design_is_routed_fail2",
    "dont_touch",
    "dump_netlists",
    "dump_netlists_withfill",
    "dump_nets",
    "dump_via_rules",
    "dump_vias",
    "edit_def",
    "edit_via_params",
    "floorplan_initialize",
    "floorplan_initialize2",
    "floorplan_initialize3",
    "gcd_abstract_lef",
    "gcd_abstract_lef_with_power",
    "gcd_def_access",
    "gcd_pdn_def_access",
    "import_package",
    "lef_data_access",
    "lef_mask",
    "multi_tech",
    "ndr",
    "polygon",
    "read_abstract_lef",
    "read_3dbv",
    "read_3dbx",
    "read_db",
    "read_def",
    "read_def58",
    "read_lef",
    "read_zipped",
    "replace_hier_mod1",
    "replace_hier_mod2",
    "replace_hier_mod3",
    "replace_hier_mod4",
    "replace_hier_mod5",
    "replace_hier_mod6",
    "replace_hier_mod_report_checks",
    "rounding",
    "row_settings",
    "sky130hd_multi_patterned",
    "smash_vias",
    "transform",
    "wire_encoder",
    "write_cdl",
    "write_3dbv",
    "write_3dbx",
    "write_def58",
    "write_def58_gzip",
    "write_lef_and_def",
    "write_lef_polygon",
    "write_macro_placement",
]

PASSFAIL_TESTS = [
    # pass-fail
    "test_block",
    "test_bterm",
    "test_destroy",
    "test_group",
    "test_inst",
    "test_iterm",
    "test_module",
    "test_net",
    "test_wire_codec",
]

# Disabled in CMakeLists.txt
MANUAL_TESTS = [
    "odb_man_tcl_check",
    "odb_readme_msgs_check",
]

# TODO: Enable once difference between bazel and ctest is resolved.
MANUAL_FOR_BAZEL_TESTS = [
    "replace_hier_mod1",
]

ALL_TESTS = COMPULSORY_TESTS + MANUAL_TESTS + PASSFAIL_TESTS

filegroup(
    name = "regression_resources",
    # Dependencies could be specified more narrowly per test case,
    # but at least it is not a glob of everything and there are
    # per test glob patterns below.
    srcs = [
        "Nangate45/Nangate45.lef",
        "Nangate45/Nangate45.pdn.tcl",
        "Nangate45/Nangate45.rc",
        "Nangate45/Nangate45.rcx_rules",
        "Nangate45/Nangate45.tracks",
        "Nangate45/Nangate45.vars",
        "Nangate45/Nangate45_fast.lib",
        "Nangate45/Nangate45_lvt.lef",
        "Nangate45/Nangate45_lvt.lib",
        "Nangate45/Nangate45_slow.lib",
        "Nangate45/Nangate45_stdcell.lef",
        "Nangate45/Nangate45_tech.lef",
        "Nangate45/Nangate45_typ.lib",
        "Nangate45/fake_bumps.lef",
        "Nangate45/fake_macros.lef",
        "Nangate45/fake_macros.lib",
        "Nangate45/fakeram45.cfg",
        "Nangate45/fakeram45_1024x32.lef",
        "Nangate45/fakeram45_1024x32.lib",
        "Nangate45/fakeram45_256x16.lef",
        "Nangate45/fakeram45_256x16.lib",
        "Nangate45/fakeram45_512x64.lef",
        "Nangate45/fakeram45_512x64.lib",
        "Nangate45/fakeram45_64x32.lef",
        "Nangate45/fakeram45_64x32.lib",
        "Nangate45/fakeram45_64x7.lef",
        "Nangate45/fakeram45_64x7.lib",
        "Nangate45/fakeram45_64x96.lef",
        "Nangate45/fakeram45_64x96.lib",
        "Nangate45/work_around_yosys/cells.v",
        "NangateOpenCellLibrary.cdl",
        "data/ICEWall/dummy_pads.lef",
        "data/ICEWall/octilinear.def",
        "data/design.def",
        "data/design.def.gz",
        "data/design.odb",
        "data/design58.def",
        "data/example.3dbv",
        "data/example.3dbx",
        "data/example.bmap",
        "data/example1.bmap",
        "data/example2.bmap",
        "data/example3.bmap",
        "data/floorplan_initialize.def",
        "data/floorplan_initialize.v",
        "data/floorplan_initialize2.def",
        "data/floorplan_initialize2.v",
        "data/gcd/floorplan.def",
        "data/gcd/gcd.def",
        "data/gcd/gcd_nangate45_route.def",
        "data/gcd/gcd_nangate45_route_with_power_pins.def",
        "data/gcd/gcd_pdn.def",
        "data/gscl45nm.lef",
        "data/gscl45nm.lef.gz",
        "data/gscl45nm_ext_macros.lef",
        "data/gscl45nm_polygon.lef",
        "data/lef58class_gscl45nm.lef",
        "data/mask.lef",
        "data/ndr.def",
        "data/parser_test.def",
        "data/rounding.def",
        "data/sky130_fd_sc_hd__inv_1.gds",
        "data/sky130hd_multi_patterned.def",
        "dump_netlists.tcl",
        "dump_netlists_withfill.tcl",
        "fake_macros.def",
        "gcd_adder.v",
        "gcd_adder2.v",
        "gcd_adder4.v",
        "gcd_adder5.v",
        "helper.tcl",
        "helpers.tcl",
        "replace_hier_mod1.v",
        "replace_hier_mod6.v",
        "replace_hier_mod_undo.tcl",
        "replace_hier_mod_undo2.tcl",
        "sky130hd/sky130_fd_sc_hd__ff_n40C_1v95.lib",
        "sky130hd/sky130_fd_sc_hd__ss_n40C_1v40.lib",
        "sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib",
        "sky130hd/sky130_fd_sc_hd_merged.lef",
        "sky130hd/sky130hd.pdn.tcl",
        "sky130hd/sky130hd.rc",
        "sky130hd/sky130hd.rcx_rules",
        "sky130hd/sky130hd.tlef",
        "sky130hd/sky130hd.tracks",
        "sky130hd/sky130hd.vars",
        "sky130hd/sky130hd_multi_patterned.tlef",
        "sky130hd/sky130hd_std_cell.lef",
        "sky130hd/sky130hd_std_cell_vt.lef",
        "sky130hd/sky130hd_tt.lib",
        "sky130hd/sky130hd_vt.tlef",
        "sky130hd/work_around_yosys/formal_pdk.v",
    ],
    visibility = ["//src/odb/test/cpp:__subpackages__"],
)

[
    filegroup(
        name = test_name + "_resources",
        srcs = [
            ":regression_resources",
            "//src/odb/test/data/sky130hd:lef-test-data",
        ] + glob(
            [
                test_name + ".*",
            ],
        ) + {
            "all_pins_placed1": [
                "floorplan_initialize3.def",
            ],
            "all_pins_placed2": [
                "design_is_routed1.def",
            ],
            "create_obstruction": [
                "create_blockage.def",
            ],
            "dump_netlists": [
                "dump_netlists_cdl.ok",
            ],
            "dump_netlists_withfill": [
                "dump_netlists_withfill_cdl.ok",
            ],
            "read_abstract_lef": [
                "gcd_abstract_lef.lefok",
            ],
            "replace_hier_mod4": [
                "gcd_abstract_lef.lefok",
            ],
            "write_cdl": [
                "write_cdl_escaped.cdl",
            ],
        }.get(
            test_name,
            [],
        ),
    )
    for test_name in ALL_TESTS
]

[
    regression_test(
        name = test_name,
        check_log = False if test_name in PASSFAIL_TESTS else True,
        check_passfail = True if test_name in PASSFAIL_TESTS else False,
        data = [":" + test_name + "_resources"],
        tags = ["manual"] if test_name in MANUAL_TESTS or
                             test_name in MANUAL_FOR_BAZEL_TESTS else [],
        visibility = ["//visibility:public"],
    )
    for test_name in ALL_TESTS
]
