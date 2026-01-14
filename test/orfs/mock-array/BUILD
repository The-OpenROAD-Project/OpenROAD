load("@bazel-orfs//:openroad.bzl", "orfs_run")
load("@bazel-orfs//toolchains/scala:chisel.bzl", "chisel_binary")
load("//test/orfs/asap7:asap7.bzl", "ASAP7_REMOVE_CELLS")
load(":mock-array.bzl", "config", "element", "mock_array", "verilog")

package(
    features = ["-layering_check"],  # TODO: enable
)

exports_files(["write_verilog.tcl"])

chisel_binary(
    name = "generate_verilog",
    srcs = glob(["src/main/scala/**/*.scala"]),
    main_class = "GenerateMockArray",
    resources = glob(["src/main/resources/**/*"]),
    tags = ["manual"],
    deps = [
        "@maven//:com_github_scopt_scopt_2_13",
    ],
)

filegroup(
    name = "mock-array-element-io",
    srcs = [
        "element-io.tcl",
    ],
    data = [
        ":util.tcl",
    ],
    visibility = [":__subpackages__"],
)

filegroup(
    name = "mock-array-constraints",
    srcs = [
        "constraints.sdc",
    ],
    visibility = [":__subpackages__"],
)

filegroup(
    name = "mock-array-io",
    srcs = [
        "io.tcl",
    ],
    data = [
        ":util.tcl",
    ],
    visibility = [":__subpackages__"],
)

# FIXME why is this needed to ensure that cfg=exec of OpenSTA has runfiles?
genrule(
    name = "opensta_runfiles",
    srcs = [],
    outs = ["dummy"],
    cmd = """
    $(location //src/sta:opensta) >$@
    """,
    tags = ["manual"],
    tools = ["//src/sta:opensta"],
)

CONFIGS = {name: config(name, rows, cols) for name, rows, cols in [
    ("4x4", 4, 4),
    ("8x8", 8, 8),
]}

[verilog(
    name = name,
    cols = config["cols"],
    rows = config["rows"],
) for name, config in CONFIGS.items()]

[element(
    name = name,
    config = config,
) for name, config in CONFIGS.items()]

[mock_array(
    name = name,
    config = config,
) for name, config in CONFIGS.items()]

test_suite(
    name = "eqy_tests",
    tags = ["manual"],
    tests = ["Element_eqy_{variant}".format(variant = name) for name in CONFIGS] +
            [
                "MockArray_eqy_{name}_{variant}".format(
                    name = name,
                    variant = variant,
                )
                for name in CONFIGS
                for variant in [
                    "flat",
                    "base",
                ]
            ],
)

orfs_run(
    name = "write_macro_placement",
    src = ":MockArray_4x4_base_floorplan",
    outs = [
        ":macro-placement.tcl",
    ],
    script = ":write_macros.tcl",
    tags = ["manual"],
)

orfs_run(
    name = "write_pin_placement",
    src = ":MockArray_4x4_base_floorplan",
    outs = [
        ":io-placement.tcl",
    ],
    script = ":write_pin_placement.tcl",
    tags = ["manual"],
)

[
    orfs_run(
        name = "{}_floorplan_verilog".format(module),
        src = ":{}_base_floorplan".format(module),
        outs = [
            "{}_floorplan.v".format(module),
        ],
        arguments = {
            "ASAP7_REMOVE_CELLS": " ".join(ASAP7_REMOVE_CELLS),
            "OUTPUT": "$(location :{}_floorplan.v)".format(module),
        },
        script = ":write_verilog.tcl",
        tags = ["manual"],
    )
    for module in [
        "MockArray_4x4",
        "Element_4x4",
    ]
]
