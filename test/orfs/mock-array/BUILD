load("@bazel-orfs//:eqy.bzl", "eqy_test")
load("@bazel-orfs//:openroad.bzl", "orfs_run")
load("@bazel-orfs//toolchains/scala:chisel.bzl", "chisel_binary")
load(":mock-array.bzl", "ASAP7_REMOVE_CELLS", "config", "element", "mock_array", "verilog")

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

filegroup(
    name = "asap7_files",
    srcs = [
        "asap7/asap7sc7p5t_AO_RVT_TT_201020.v",
        "asap7/asap7sc7p5t_INVBUF_RVT_TT_201020.v",
        "asap7/asap7sc7p5t_OA_RVT_TT_201020.v",
        "asap7/asap7sc7p5t_SIMPLE_RVT_TT_201020.v",
        "@docker_orfs//:OpenROAD-flow-scripts/flow/platforms/asap7/verilog/stdcell/dff.v",
    ],
    visibility = ["//visibility:public"],
)

eqy_test(
    name = "MockArray_4x4_eqy_test",
    depth = 1,
    gate_verilog_files = [
        ":MockArray_4x4_floorplan.v",
        ":Element_4x4_floorplan.v",
        "src/main/resources/multiplier.v",
        ":asap7_files",
    ],
    gold_verilog_files = [
        ":4x4_array.sv",
        "src/main/resources/multiplier.v",
        ":asap7_files",
    ],
    module_top = "MockArray",
    tags = ["manual"],
)
