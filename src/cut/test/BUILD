# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors

load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(features = ["layering_check"])

cc_test(
    name = "cut_abc_test",
    srcs = ["cpp/TestAbc.cc"],
    data = [
        "Nangate45/Nangate45_stdcell.lef",
        "Nangate45/Nangate45_tech.lef",
        "Nangate45/Nangate45_typ.lib",
        "aes_nangate45.v",
        "asap7/asap7_tech_1x_201209.lef",
        "asap7/asap7sc7p5t_28_R_1x_220121a.lef",
        "asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz",
        "asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz",
        "asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz",
        "asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib",
        "asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz",
        "empty_cut_set.v",
        "side_outputs_extract.v",
        "side_outputs_extract_logic_depth.v",
        "simple_and_gate_extract.v",
        "sky130/sky130_fd_sc_hd__ss_n40C_1v40.lib",
        "sky130/sky130hd.tlef",
        "sky130/sky130hd_std_cell.lef",
        "sky130_const_cell.v",
    ],
    deps = [
        "//src/cut",
        "//src/dbSta",
        "//src/dbSta:dbReadVerilog",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/tst",
        "//src/utl",
        "@abc",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
