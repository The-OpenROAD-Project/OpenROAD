load("@bazel-orfs//:eqy.bzl", "eqy_test")
load("@bazel-orfs//:openroad.bzl", "orfs_flow", "orfs_run")

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

orfs_run(
    name = "gcd_floorplan_verilog",
    src = ":gcd_floorplan",
    outs = [
        "gcd_floorplan.v",
    ],
    arguments = {
        "OUTPUT": "$(location :gcd_floorplan.v)",
    },
    script = "//test/orfs/mock-array:write_verilog.tcl",
    tags = ["manual"],
)

filegroup(
    name = "gcd_synth_files",
    srcs = [
        ":gcd_synth",
    ],
    output_group = "1_synth.v",
)

eqy_test(
    name = "eqy_test",
    depth = 1,
    gate_verilog_files = [
        ":gcd_floorplan.v",
        "//test/orfs/mock-array:asap7_files",
    ],
    gold_verilog_files = [
        ":gcd_synth_files",
        "//test/orfs/mock-array:asap7_files",
    ],
    module_top = "gcd",
    tags = ["manual"],
)
