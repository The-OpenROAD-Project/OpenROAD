load("@bazel-orfs//:sweep.bzl", "orfs_sweep")
load("@rules_shell//shell:sh_test.bzl", "sh_test")
load("//test/orfs:check_same.bzl", "check_same")
load("//test/orfs:eqy-flow.bzl", "eqy_flow_test")

package(features = ["layering_check"])

orfs_sweep(
    name = "gcd",
    # buildifier: disable=unsorted-dict-items
    arguments = {
        # Various
        "CORE_AREA": "1.08 1.08 15.12 15.12",
        "DIE_AREA": "0 0 16.2 16.2",
        "PLACE_DENSITY": "0.35",
        "OPENROAD_HIERARCHICAL": "1",
        # Demonstrate clock period specified in arguments, useful
        # for Optuna scripts.
        "ABC_CLOCK_PERIOD_IN_PS": "310.001",
        # Smoketest
        "SWAP_ARITH_OPERATORS": "1",
    },
    sources = {
        "RULES_JSON": [":rules-base.json"],
        "SDC_FILE": [":constraint.sdc"],
    },
    stage = "final",
    sweep = {
        "b": {},
        "base": {},
    },
    tags = ["manual"],
    test_kwargs = {
        "tags": ["orfs"],
    },
    verilog_files = ["gcd.v"],
)

eqy_flow_test(
    name = "gcd_eqy",
    flow = "gcd",
    module_top = "gcd",
    tags = ["manual"],
    verilog_files = ["gcd.v"],
)

filegroup(
    name = "gcd_floorplan_odb",
    srcs = [":gcd_floorplan"],
    output_group = "2_floorplan.odb",
    tags = ["manual"],
)

sh_test(
    name = "gcd_whittle_test",
    timeout = "long",
    srcs = ["test_whittle.sh"],
    args = [
        "$(rootpath //:openroad)",
        "$(rootpath //etc:whittle.py)",
        "$(rootpath :gcd_floorplan_odb)",
    ],
    data = [
        ":gcd_floorplan_odb",
        "//:openroad",
        "//etc:whittle.py",
        "//etc:whittle_cleanup.tcl",
        "//etc:whittle_cut.tcl",
    ],
    tags = ["orfs"],
)

check_same(
    name = "gcd_idempotent_test",
    flow = "gcd",
    tags = ["orfs"],
    variant_a = "base",
    variant_b = "b",
)
