load("@bazel-orfs//:openroad.bzl", "orfs_flow")

orfs_flow(
    name = "gcd",
    arguments = {
        # Faster builds
        "SKIP_INCREMENTAL_REPAIR": "1",
        "GPL_TIMING_DRIVEN": "0",
        "SKIP_LAST_GASP": "1",
        # Various
        "DIE_AREA": "0 0 16.2 16.2",
        "CORE_AREA": "1.08 1.08 15.12 15.12",
        "PLACE_DENSITY": "0.35",
    },
    sources = {
        "RULES_JSON": [":rules-base.json"],
        "SDC_FILE": [":constraint.sdc"],
    },
    verilog_files = ["gcd.v"],
    tags = ["manual"],
)
