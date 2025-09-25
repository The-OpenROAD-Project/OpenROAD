load("@bazel-orfs//:openroad.bzl", "orfs_flow")

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
