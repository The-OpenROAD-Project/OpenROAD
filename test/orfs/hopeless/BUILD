load("@bazel-orfs//:sweep.bzl", "orfs_sweep")
load("@rules_shell//shell:sh_test.bzl", "sh_test")
load("//test/orfs:check_same.bzl", "check_same")

package(features = ["layering_check"])

orfs_sweep(
    name = "hopeless",
    # buildifier: disable=unsorted-dict-items
    arguments = {
        "CORE_AREA": "2.16 2.16 45.36 45.36",
        "DIE_AREA": "0 0 47.52 47.52",
        "PLACE_DENSITY": "0.50",
        # Clock period far below what the 8-lane × 8-deep add/xor chain can
        # ever meet. This is the whole point of the test: the flow must bail
        # on an obviously-infeasible target without grinding forever.
        "ABC_CLOCK_PERIOD_IN_PS": "50.0",
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
    verilog_files = ["hopeless.v"],
)

check_same(
    name = "hopeless_idempotent_test",
    flow = "hopeless",
    tags = ["orfs"],
    variant_a = "base",
    variant_b = "b",
)

# Asserts the WNS-stagnation gate actually fired on this infeasible design.
# Without this, the flow could "pass" simply because some other termination
# mechanism stopped repair_timing, and the real gate would quietly bit-rot.
sh_test(
    name = "hopeless_stagnation_test",
    srcs = ["check_stagnation.sh"],
    args = ["$(locations :hopeless_base_logs)"],
    data = [":hopeless_base_logs"],
    tags = ["orfs"],
)
