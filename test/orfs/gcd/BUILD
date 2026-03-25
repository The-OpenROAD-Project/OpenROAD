load("@bazel-orfs//:openroad.bzl", "orfs_run")
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

# --- Static HTML report targets ---

_STAGES = {
    "cts": "4_cts",
    "floorplan": "2_floorplan",
    "grt": "5_1_grt",
    "place": "3_place",
    "route": "5_route",
    "synth": "1_synth",
}

# Extract JSON payload + generate static HTML for each stage
[
    (
        orfs_run(
            name = "gcd_{}_json".format(stage),
            src = ":gcd_{}".format(stage),
            outs = ["gcd_{}.json".format(stage)],
            arguments = {
                "DESIGN_NAME": "gcd",
                "GUI_TIMING": "1",
                "OUTPUT": "$(location :gcd_{}.json)".format(stage),
                "STAGE_NAME": orfs_name,
            },
            script = "//src/web:src/export_json.tcl",
            tags = ["manual"],
        ),
        sh_binary(
            name = "gcd_{}_html".format(stage),
            srcs = ["//src/web:src/html_open.sh"],
            data = [
                ":gcd_{}_json".format(stage),
                "//src/web:web_assets",
            ],
            tags = ["manual"],
        ),
    )
    for stage, orfs_name in _STAGES.items()
]

# Generate histogram-only demo report (A/B prototype)
sh_binary(
    name = "gcd_route_demo_html",
    srcs = ["//src/web:src/demo_html_open.sh"],
    data = [
        ":gcd_route_json",
        "//src/web:render_static",
    ],
    tags = ["manual"],
)

# Launch live web viewer for the routed design
sh_binary(
    name = "gcd_route_web",
    srcs = ["//src/web:src/web_open.sh"],
    data = [
        ":gcd_route",
        "//:openroad",
        "//src/web:web_assets",
    ],
    tags = ["manual"],
)
