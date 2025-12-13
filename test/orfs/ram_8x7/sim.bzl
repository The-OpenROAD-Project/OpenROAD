"""
Simulate on netlist
"""

load("@rules_cc//cc:defs.bzl", "cc_test")
load("@rules_verilator//verilator:defs.bzl", "verilator_cc_library")
load("@rules_verilator//verilog:defs.bzl", "verilog_library")

def sim_test(
        name,
        cc_srcs,
        module_top,
        verilog,
        **kwargs):
    """Creates a cc_test() for a verilog file

    Args:
        name: The name of the test target.
        cc_srcs: C++ driver source
        module_top: The top module name in the verilog file.
        verilog: The verilog source file to simulate.
        **kwargs: Additional keyword arguments passed to cc_test().
    """
    verilog_library(
        name = "{name}_split".format(name = name),
        srcs = [
            "//test/orfs/asap7:asap7_files",
            #    "@docker_orfs//:OpenROAD-flow-scripts/flow/platforms/asap7/verilog/stdcell/asap7sc7p5t_AO_RVT_TT_201020.v",
            #    "@docker_orfs//:OpenROAD-flow-scripts/flow/platforms/asap7/verilog/stdcell/asap7sc7p5t_INVBUF_RVT_TT_201020.v",
            #    "@docker_orfs//:OpenROAD-flow-scripts/flow/platforms/asap7/verilog/stdcell/asap7sc7p5t_SIMPLE_RVT_TT_201020.v",
            #    "@docker_orfs//:OpenROAD-flow-scripts/flow/platforms/asap7/verilog/stdcell/dff.v",
        ] + verilog,
        visibility = ["//visibility:public"],
        tags = ["manual"],
    )

    verilator_cc_library(
        name = "{name}_simulator".format(name = name),
        copts = [
            # Don't care about warnings from Verilator generated C++
            "-Wno-unused-variable",
        ],
        module = "{name}_split".format(name = name),
        module_top = module_top,
        tags = ["manual"],
        trace = True,
        visibility = ["//visibility:public"],
        vopts = [
            "--runtime-debug",
            "--timescale 1ps/1ps",
            "-Wall",
            "-Wno-DECLFILENAME",
            "-Wno-UNUSEDSIGNAL",
            "-Wno-PINMISSING",
            "--trace-underscore",
            # inline all PDK modules to speed up compilation
            "--flatten",
            # No-op option to retrigger a build
            # "-Wfuture-blah",
        ],
    )

    cc_test(
        name = "{name}".format(name = name),
        size = "small",
        srcs = cc_srcs,
        cxxopts = [
            "-std=c++23",
        ],
        deps = [
            ":{name}_simulator".format(name = name),
            "@googletest//:gtest_main",
        ],
        **kwargs
    )
