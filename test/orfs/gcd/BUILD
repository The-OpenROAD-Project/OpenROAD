load("@bazel-orfs//:eqy.bzl", "eqy_test")
load("@bazel-orfs//:openroad.bzl", "orfs_flow", "orfs_run")
load("//test/orfs/mock-array:mock-array.bzl", "ASAP7_REMOVE_CELLS")

package(features = ["layering_check"])

orfs_flow(
    name = "gcd",
    # buildifier: disable=unsorted-dict-items
    arguments = {
        # Faster builds
        "GPL_TIMING_DRIVEN": "0",
        "SKIP_INCREMENTAL_REPAIR": "1",
        "SKIP_LAST_GASP": "1",
        # Various
        "CORE_AREA": "1.08 1.08 15.12 15.12",
        "DIE_AREA": "0 0 16.2 16.2",
        "PLACE_DENSITY": "0.35",
        "OPENROAD_HIERARCHICAL": "1",
    },
    sources = {
        "RULES_JSON": [":rules-base.json"],
        "SDC_FILE": [":constraint.sdc"],
    },
    tags = ["manual"],
    test_kwargs = {
        "tags": ["orfs"],
    },
    verilog_files = ["gcd.v"],
)

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

[orfs_run(
    name = "gcd_{stage}_verilog".format(stage = stage),
    src = ":gcd_{stage}".format(stage = stage),
    outs = [
        "gcd_{stage}.v".format(stage = stage),
    ],
    arguments = {
        "ASAP7_REMOVE_CELLS": " ".join(ASAP7_REMOVE_CELLS),
        "OUTPUT": "$(location :gcd_{stage}.v)".format(stage = stage),
    },
    script = "//test/orfs/mock-array:write_verilog.tcl",
    tags = ["manual"],
) for stage in STAGES[2:]]

filegroup(
    name = "gcd_source_files",
    srcs = [
        "gcd.v",
    ],
)

[filegroup(
    name = "gcd_{stage}_files".format(stage = stage),
    srcs = [
        ":gcd_{stage}_verilog".format(stage = stage),
    ],
) for stage in STAGES[2:]]

[eqy_test(
    name = "eqy_{stage}_test".format(stage = STAGES[i + 1]),
    depth = 1,
    gate_verilog_files = [
        ":gcd_{stage}_files".format(stage = STAGES[i + 1]),
    ] + ([] if STAGES[i + 1] == "source" else ["//test/orfs/mock-array:asap7_files"]),
    gold_verilog_files = [
        ":gcd_{stage}_files".format(stage = STAGES[i]),
    ] + ([] if STAGES[i] == "source" else ["//test/orfs/mock-array:asap7_files"]),
    module_top = "gcd",
    tags = ["manual"],
) for i in range(len(STAGES) - 1)]
