load("@bazel-orfs//:openroad.bzl", "orfs_flow")

package(features = ["layering_check"])

orfs_flow(
    name = "multiplier",
    arguments = {
        "CELL_PAD_IN_SITES_DETAIL_PLACEMENT": "0",
        "CELL_PAD_IN_SITES_GLOBAL_PLACEMENT": "1",
        "CORE_AREA": "0.27 1.35 45.31 46.44",
        "CTS_BUF_CELL": "BUFx8_ASAP7_75t_R",
        "DIE_AREA": "0 0 45.576 47.79",
        "PLACE_DENSITY": "0.41",
    },
    sources = {
        "RULES_JSON": [":multiplier-rules-base.json"],
        "SDC_FILE": [":multiplier.sdc"],
    },
    tags = ["manual"],
    test_kwargs = {
        "tags": ["orfs"],
    },
    verilog_files = ["multiplier.v"],
)

orfs_flow(
    name = "multiply_adder",
    arguments = {
        "CELL_PAD_IN_SITES_DETAIL_PLACEMENT": "0",
        "CELL_PAD_IN_SITES_GLOBAL_PLACEMENT": "1",
        "CORE_AREA": "0.27 1.35 60.318 61.56",
        "CTS_BUF_CELL": "BUFx8_ASAP7_75t_R",
        "DIE_AREA": "0 0 60.588 62.91",
        "PLACE_DENSITY": "0.67",
    },
    sources = {
        "RULES_JSON": [":multiply_adder-rules-base.json"],
        "SDC_FILE": [":multiply_adder.sdc"],
    },
    tags = ["manual"],
    test_kwargs = {
        "tags": ["orfs"],
    },
    verilog_files = ["multiply_adder.v"],
)
