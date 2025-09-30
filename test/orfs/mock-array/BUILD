load("@bazel-orfs//:generate.bzl", "fir_library")
load("@bazel-orfs//:openroad.bzl", "orfs_flow", "orfs_run")
load("@bazel-orfs//:verilog.bzl", "verilog_directory", "verilog_single_file_library")
load("@bazel-orfs//toolchains/scala:chisel.bzl", "chisel_binary")
load("@rules_cc//cc:defs.bzl", "cc_binary")
load("@rules_shell//shell:sh_test.bzl", "sh_test")
load("@rules_verilator//verilator:defs.bzl", "verilator_cc_library")
load("@rules_verilator//verilog:defs.bzl", "verilog_library")

POWER_STAGE_NAME = "cts"

POWER_STAGE_STEM = {
    "cts": "4",
    "final": "6",
}[POWER_STAGE_NAME] + "_" + POWER_STAGE_NAME

# single source of truth for defaults.
# each number is a unit
# current unit is configured as 2.16 which is on the routing grid for M5

# table of Elements - (rows cols width height pitch_x pitch_y)
MOCK_ARRAY_TABLE = [
    8,
    8,
    20,
    20,
    20,
    22,
]

MOCK_ARRAY_SCALE = 45

# Routing pitches for relevant metal layers.
#  For x, this is M5; for y, this is M4.
#  Pitches are specified in OpenROAD-flow-scripts/flow/platforms/asap7/lef/asap7_tech_1x_201209.lef.
#  For asap7, x and y pitch is the same.
#
# make_tracks M5 -x_offset 0.012 -x_pitch 0.048 -y_offset 0.012 -y_pitch 0.048
#
# the macro needs to be on a multiple of the track pattern
placement_grid_x = 0.048 * MOCK_ARRAY_SCALE

placement_grid_y = 0.048 * MOCK_ARRAY_SCALE

# number of Elements in row and column, can be control by user via environment variable
# MOCK_ARRAY_TABLE (rows, cols, width, height, pitch_x, pitch_y)
#  rows, cols       - number of Element in rows, cols
#  width, height    - width and height of each Element
#
# When the pitch is equal to the width/height, we have routing by abutment
# https://en.wikipedia.org/wiki/Pitch#Linear_measurement
#
#  pitch_x, pitch_y - placement pitch for each Element, in x and y direction
# specification are in unit of placement grid
rows, cols, ce_x, ce_y, pitch_x, pitch_y = MOCK_ARRAY_TABLE

# Element size is set to multiple of placement grid above
ce_width = ce_x * placement_grid_x

ce_height = ce_y * placement_grid_y

# top level core offset
margin_x = placement_grid_x

margin_y = placement_grid_y

# Element core margin
ce_margin_x = placement_grid_x * 0.5

ce_margin_y = placement_grid_y * 0.5

# PDN problems if it is smaller. Not investigated.
array_spacing_x = margin_x * 2

array_spacing_y = margin_y * 2

# top level core and die size
core_width = (
    2 * array_spacing_x + ((placement_grid_x * pitch_x) * (cols - 1)) + ce_width
)

core_height = (
    2 * array_spacing_y + ((placement_grid_y * pitch_y) * (rows - 1)) + ce_height
)

die_width = core_width + (margin_x * 2)

die_height = core_height + (margin_y * 2)

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

VARIANTS = [
    "base",
    "flat",
]

[orfs_flow(
    name = "MockArray",
    arguments = {
        "CORE_AREA": "{} {} {} {}".format(
            margin_x,
            margin_y,
            core_width + margin_x,
            core_height + margin_y,
        ),
        "DETAILED_ROUTE_END_ITERATION": "6",
        "DIE_AREA": "0 0 {} {}".format(
            die_width,
            die_height,
        ),
        "GDS_ALLOW_EMPTY": "Element",
        "GND_NETS_VOLTAGES": "",
        "IO_PLACER_H": "M4 M6",
        "IO_PLACER_V": "M5 M7",
        "MACRO_BLOCKAGE_HALO": "0",
        "MACRO_PLACE_HALO": "0 2.16",
        "MACRO_ROWS_HALO_X": "0.5",
        "MACRO_ROWS_HALO_Y": "0.5",
        "MAX_ROUTING_LAYER": "M9",
        "OPENROAD_HIERARCHICAL": "1" if variant == "base" else "0",
        "PDN_TCL": "$(PLATFORM_DIR)/openRoad/pdn/BLOCKS_grid_strategy.tcl",
        "PLACE_DENSITY": "0.30",
        "PLACE_PINS_ARGS": "-annealing",
        "PWR_NETS_VOLTAGES": "",
        "RTLMP_BOUNDARY_WT": "0",
        "RTLMP_MAX_INST": "250",
        "RTLMP_MAX_MACRO": "64",
        "RTLMP_MIN_INST": "50",
        "RTLMP_MIN_MACRO": "8",
    },
    macros = ["Element_generate_abstract"],
    sources = {
        "IO_CONSTRAINTS": [":mock-array-io"],
        "RULES_JSON": [":rules-base.json"],
        "SDC_FILE": [":mock-array-constraints"],
    },
    tags = ["manual"],
    test_kwargs = {
        "tags": ["orfs"],
    },
    variant = variant,
    verilog_files = [":verilog"],
) for variant in VARIANTS]

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

orfs_flow(
    name = "Element",
    abstract_stage = POWER_STAGE_NAME,
    arguments = {
        "CORE_AREA": "{} {} {} {}".format(
            ce_margin_x,
            ce_margin_y,
            ce_width - ce_margin_x,
            ce_height - ce_margin_y,
        ),
        "DETAILED_ROUTE_END_ITERATION": "6",
        "DIE_AREA": "0 0 {} {}".format(
            ce_width,
            ce_height,
        ),
        "GND_NETS_VOLTAGES": "",
        "IO_PLACER_H": "M2 M4",
        "IO_PLACER_V": "M3 M5",
        "MAX_ROUTING_LAYER": "M5",
        "MIN_ROUTING_LAYER": "M2",
        # We want to report power per module using hierarhical .odb
        "OPENROAD_HIERARCHICAL": "1",
        "PDN_TCL": "$(PLATFORM_DIR)/openRoad/pdn/BLOCK_grid_strategy.tcl",
        "PLACE_DENSITY": "0.82",
        "PLACE_PINS_ARGS": "-annealing",
        "PWR_NETS_VOLTAGES": "",
        # Keep only one module, enough for testing, faster builds. There's
        # some width in multiply reduction going on here to speed up
        # builds, so we want to keep this exact module.
        "SYNTH_KEEP_MODULES": "Multiplier",
    },
    sources = {
        "IO_CONSTRAINTS": [":mock-array-element-io"],
        "SDC_FILE": [":mock-array-constraints"],
    },
    tags = ["manual"],
    verilog_files = [":verilog"],
)

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

fir_library(
    name = "generate_fir",
    data = [
    ],
    generator = ":generate_verilog",
    opts = [
        "--width=8",
        "--height=8",
        "--dataWidth=64",
        "--",
        # Imagine Chisel arguments here
        "--",
        "-disable-all-randomization",
        "-strip-debug-info",
        "-enable-layers=Verification",
        "-enable-layers=Verification.Assert",
        "-enable-layers=Verification.Assume",
        "-enable-layers=Verification.Cover",
    ],
    tags = ["manual"],
)

verilog_directory(
    name = "generate_split",
    srcs = [":generate_fir"],
    tags = ["manual"],
)

verilog_single_file_library(
    name = "array.sv",
    srcs = [":generate_split"],
    tags = ["manual"],
    visibility = ["//visibility:public"],
)

filegroup(
    name = "verilog",
    srcs = [
        "src/main/resources/multiplier.v",
        ":array.sv",
    ],
    tags = ["manual"],
)

MACROS = [
    "Element",
    "MockArray",
]

[
    orfs_run(
        name = "{macro}_parasitics".format(macro = macro),
        src = ":{macro}_{stage}".format(
            macro = macro,
            stage = POWER_STAGE_NAME,
        ),
        outs = [
            "results/asap7/{macro}/base/{stem}.spef".format(
                macro = macro,
                stem = POWER_STAGE_STEM,
            ),
            "results/asap7/{macro}/base/{stem}.v".format(
                macro = macro,
                stem = POWER_STAGE_STEM,
            ),
        ],
        script = ":parasitics.tcl",
        tags = ["manual"],
        visibility = ["//visibility:public"],
    )
    for macro in MACROS
    if POWER_STAGE_NAME != "final"
]

[
    filegroup(
        name = "{macro}_netlist".format(macro = macro),
        srcs = [
            ":{macro}_{stage}".format(
                macro = macro,
                stage = POWER_STAGE_NAME,
            ),
        ],
        output_group = POWER_STAGE_STEM + ".v",
    )
    for macro in MACROS
    if POWER_STAGE_NAME == "final"
]

verilog_library(
    name = "array",
    srcs = [
        ("results/asap7/{macro}/base/{stem}.v".format(
            macro = macro,
            stem = POWER_STAGE_STEM,
        ) if POWER_STAGE_NAME != "final" else "{macro}_netlist".format(macro = macro))
        for macro in MACROS
    ] + [
        "@docker_orfs//:OpenROAD-flow-scripts/flow/platforms/asap7/verilog/stdcell/asap7sc7p5t_AO_RVT_TT_201020.v",
        "@docker_orfs//:OpenROAD-flow-scripts/flow/platforms/asap7/verilog/stdcell/asap7sc7p5t_INVBUF_RVT_TT_201020.v",
        "@docker_orfs//:OpenROAD-flow-scripts/flow/platforms/asap7/verilog/stdcell/asap7sc7p5t_SIMPLE_RVT_TT_201020.v",
        "@docker_orfs//:OpenROAD-flow-scripts/flow/platforms/asap7/verilog/stdcell/dff.v",
        "@docker_orfs//:OpenROAD-flow-scripts/flow/platforms/asap7/verilog/stdcell/empty.v",
    ],
    tags = ["manual"],
)

verilator_cc_library(
    name = "array_verilator",
    copts = [
        # Don't care about warnings from Verilator generated C++
        "-Wno-unused-variable",
    ],
    module = ":array",
    module_top = "MockArray",
    tags = ["manual"],
    trace = True,
    vopts = [
        "--timescale 1ps/1ps",
        "-Wall",
        "-Wno-DECLFILENAME",
        "-Wno-UNUSEDSIGNAL",
        "-Wno-PINMISSING",
        "--trace-underscore",
        # inline all PDK modules to speed up compilation
        "--flatten",
        # No-op option to retrigger a build
        # "-Wfuture-blah",
    ],
)

cc_binary(
    name = "simulator",
    srcs = [
        "simulate.cpp",
    ],
    cxxopts = [
        "-std=c++23",
    ],
    tags = ["manual"],
    deps = [
        ":array_verilator",
    ],
)

genrule(
    name = "vcd",
    srcs = [
        # FIXME move to tools, using target configuration for now to avoid rebuilds
        ":simulator",
    ],
    outs = ["MockArrayTestbench.vcd"],
    cmd = "$(execpath :simulator) $(location :MockArrayTestbench.vcd)",
    tags = ["manual"],
    tools = [
    ],
)

# If we want to measure power after final, instead of with estimated parasitics,
# we'll need this.
# buildifier: disable=unused-variable
SPEFS_AND_NETLISTS = [
    ":results/asap7/{macro}/{stem}.{ext}".format(
        ext = ext,
        macro = macro,
        stem = POWER_STAGE_STEM,
    )
    for macro in MACROS
    for ext in [
        "spef",
        "v",
    ]
]

POWER_TESTS = [
    "power",
    "power_instances",
]

[orfs_run(
    name = "MockArray_{name}".format(name = name),
    src = ":MockArray_{stage}".format(stage = POWER_STAGE_NAME),
    outs = [
        "{name}.txt".format(name = name),
    ],
    arguments = {
        "LOAD_POWER_TCL": "$(location :load_power.tcl)",
        "OPENROAD_EXE": "$(location //src/sta:opensta)",
        "OUTPUT": "$(location :{name}.txt)".format(name = name),
        "POWER_STAGE_NAME": POWER_STAGE_NAME,
        "POWER_STAGE_STEM": POWER_STAGE_STEM,
        "VCD_STIMULI": "$(location :vcd)",
    },
    data = [
               # FIXME this is a workaround to ensure that the OpenSTA runfiles are available
               ":opensta_runfiles",
               ":vcd",
               ":load_power.tcl",
           ] + ["{macro}_{stage}".format(
               macro = macro,
               stage = POWER_STAGE_NAME,
           ) for macro in MACROS] +
           (["{macro}_parasitics".format(macro = macro) for macro in MACROS] if POWER_STAGE_NAME != "final" else []),
    script = ":{name}.tcl".format(name = name),
    tags = ["manual"],
    tools = ["//src/sta:opensta"],
    visibility = ["//visibility:public"],
) for name in POWER_TESTS]

[sh_test(
    name = "MockArray_{name}_test".format(name = name),
    srcs = ["ok.sh"],
    args = [
        "$(location :MockArray_{name})".format(name = name),
    ],
    data = [
        ":MockArray_{name}".format(name = name),
    ],
) for name in POWER_TESTS]

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
