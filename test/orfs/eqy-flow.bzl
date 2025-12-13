"""
This module defines the ORFS equivalence for orfs_flow() using EQY and OpenROAD.
"""

load("@bazel-orfs//:eqy.bzl", "eqy_test")
load("@bazel-orfs//:openroad.bzl", "orfs_run")
load("//test/orfs/asap7:asap7.bzl", "ASAP7_REMOVE_CELLS")

STAGES = [
    "source",
    # _source tests original source to source transition,
    # which checks that the eqy setup works.
    "source",
    # _synth tests synthesis output, and so on
    # for the next stages.
    "synth",
    "floorplan",
    "place",
    "cts",
    "grt",
    "route",
    "final",
]

def eqy_flow_test(name, flow, verilog_files, module_top, other_verilog_files = [], **kwargs):
    """Defines ORFS equivalence checking flow tests for a given design.

    Args:
        name: name stem for rules setup by this macro
        flow: orfs_flow(name, ...)
        verilog_files: List of verilog files needed for synthesis.
        module_top: Top-level module name for the design.
        other_verilog_files: other gate netlists needed, typically macros, default is [].
        **kwargs: Additional keyword arguments passed to eqy_test and test_suite.
    """
    for stage in STAGES[2:]:
        orfs_run(
            name = "{name}_{stage}_verilog".format(stage = stage, name = name),
            src = ":{flow}_{stage}".format(stage = stage, flow = flow),
            outs = [
                "{name}_{stage}.v".format(stage = stage, name = name),
            ],
            arguments = {
                "ASAP7_REMOVE_CELLS": " ".join(ASAP7_REMOVE_CELLS),
                "OUTPUT": "$(location :{name}_{stage}.v)".format(stage = stage, name = name),
            },
            script = "//test/orfs/mock-array:write_verilog.tcl",
            tags = ["manual"],
        )

        native.filegroup(
            name = "{name}_{stage}_files".format(stage = stage, name = name),
            srcs = [
                ":{name}_{stage}_verilog".format(stage = stage, name = name),
            ],
            tags = ["manual"],
        )

    native.filegroup(
        name = "{name}_source_files".format(name = name),
        srcs = verilog_files + other_verilog_files,
        tags = ["manual"],
    )

    for i in range(len(STAGES) - 1):
        eqy_test(
            name = "{name}_{stage}_test".format(stage = STAGES[i + 1], name = name),
            depth = 1,
            gate_verilog_files = [
                ":{name}_{stage}_files".format(stage = STAGES[i + 1], name = name),
                "//test/orfs/asap7:asap7_files",
            ] + other_verilog_files,
            gold_verilog_files = [
                ":{name}_{stage}_files".format(stage = STAGES[i], name = name),
                "//test/orfs/asap7:asap7_files",
            ] + other_verilog_files,
            module_top = module_top,
            **kwargs
        )

    native.test_suite(
        name = "{name}_tests".format(name = name),
        tests = [
            "{name}_{stage}_test".format(stage = STAGES[i + 1], name = name)
            for i in range(len(STAGES) - 1)
        ],
        **kwargs
    )
