load("@bazel-orfs//:eqy.bzl", "eqy_test")
load("@bazel-orfs//:openroad.bzl", "orfs_flow", "orfs_run")
load("//test/orfs/asap7:asap7.bzl", "ASAP7_REMOVE_CELLS")
load(":sim.bzl", "sim_test")

package(
    features = ["-layering_check"],  # TODO: enable
)

orfs_flow(
    name = "ram_8x7",
    # buildifier: disable=unsorted-dict-items
    arguments = {
        # Various
        "CORE_AREA": "1.08 1.08 15.12 15.12",
        "DIE_AREA": "0 0 16.2 16.2",
        "PLACE_DENSITY": "0.35",
        "OPENROAD_HIERARCHICAL": "1",
        "HOLD_SLACK_MARGIN": "-1000",
        "MACRO_PLACE_HALO": "0 1",
        #"PDN_TCL": "$(PLATFORM_DIR)/openRoad/pdn/BLOCKS_grid_strategy.tcl",
        "RTLMP_BOUNDARY_WT": "0",
        "SETUP_SLACK_MARGIN": "-4000",
        "TAPCELL_TCL": "",
    },
    sources = {
        "RULES_JSON": [":rules-base.json"],
        "SDC_FILE": [":constraint.sdc"],
    },
    tags = ["manual"],
    test_kwargs = {
        "tags": ["orfs"],
    },
    verilog_files = ["ram_8x7.sv"],
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
    name = "ram_8x7_{stage}_verilog".format(stage = stage),
    src = ":ram_8x7_{stage}".format(stage = stage),
    outs = [
        "ram_8x7_{stage}.v".format(stage = stage),
    ],
    arguments = {
        "ASAP7_REMOVE_CELLS": " ".join(ASAP7_REMOVE_CELLS),
        "OUTPUT": "$(location :ram_8x7_{stage}.v)".format(stage = stage),
    },
    script = "//test/orfs/mock-array:write_verilog.tcl",
    tags = ["manual"],
) for stage in STAGES[2:]]

filegroup(
    name = "ram_8x7_source_files",
    srcs = [
        "ram_8x7.sv",
    ],
)

[filegroup(
    name = "ram_8x7_{stage}_files".format(stage = stage),
    srcs = [
        ":ram_8x7_{stage}_verilog".format(stage = stage),
    ],
) for stage in STAGES[2:]]

[eqy_test(
    name = "eqy_{stage}_test".format(stage = STAGES[i + 1]),
    depth = 1,
    gate_verilog_files = [
        ":ram_8x7_{stage}_files".format(stage = STAGES[i + 1]),
    ] + ([] if STAGES[i + 1] == "source" else ["//test/orfs/asap7:asap7_files"]),
    gold_verilog_files = [
        ":ram_8x7_{stage}_files".format(stage = STAGES[i]),
    ] + ([] if STAGES[i] == "source" else ["//test/orfs/asap7:asap7_files"]),
    module_top = "ram_8x7",
    tags = ["manual"],
) for i in range(len(STAGES) - 1)]

[sim_test(
    name = "ram_8x7_{stage}_sim_test".format(stage = stage),
    cc_srcs = ["ram_8x7_sim.cpp"],
    module_top = "ram_8x7",
    tags = ["manual"],
    verilog = [":ram_8x7_{stage}.v".format(stage = stage) if stage != "source" else ":ram_8x7.sv"],
) for stage in STAGES[1:]]

OTHER = [
    "good.v",
    "bad.v",
]

[sim_test(
    name = "ram_8x7_{other}_sim_test".format(other = other[:-2]),
    cc_srcs = ["ram_8x7_sim.cpp"],
    module_top = "ram_8x7",
    tags = ["manual"],
    verilog = [other],
) for other in OTHER]

eqy_test(
    name = "eqy_repair_tie_test",
    depth = 1,
    gate_verilog_files = [
        "bad.v",
        "//test/orfs/asap7:asap7_files",
    ],
    gold_verilog_files = [
        "good.v",
        "//test/orfs/asap7:asap7_files",
    ],
    module_top = "ram_8x7",
    tags = ["manual"],
)
