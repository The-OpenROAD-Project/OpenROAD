load("@bazel-orfs//:openroad.bzl", "orfs_flow")
load("//test/orfs:eqy-flow.bzl", "eqy_flow_test")

package(features = ["layering_check"])

orfs_flow(
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
