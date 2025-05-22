load("@bazel-orfs//:openroad.bzl", "orfs_flow", "orfs_run")
load("@rules_cc//cc:defs.bzl", "cc_binary")
load("@rules_verilator//verilator:defs.bzl", "verilator_cc_library")
load("@rules_verilator//verilog:defs.bzl", "verilog_library")
load("chisel.bzl", "chisel_binary")

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

# Element'd data width
MOCK_ARRAY_DATAWIDTH = 64

# Must be zero for routing by abutment
MACRO_BLOCKAGE_HALO = 0

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

array_offset_x = array_spacing_x + margin_x

array_offset_y = array_spacing_y + margin_y

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

orfs_flow(
    name = "MockArray",
    arguments = {
        "PLACE_PINS_ARGS": "-annealing",
        "PLACE_DENSITY": "0.30",
        "CORE_AREA": "{} {} {} {}".format(
            margin_x,
            margin_y,
            core_width + margin_x,
            core_height + margin_y,
        ),
        "DIE_AREA": "0 0 {} {}".format(
            die_width,
            die_height,
        ),
        "MACRO_PLACE_HALO": "0 2.16",
        "RTLMP_BOUNDARY_WT": "0",
        "PDN_TCL": "$(PLATFORM_DIR)/openRoad/pdn/BLOCKS_grid_strategy.tcl",
        "MACRO_ROWS_HALO_X": "0.5",
        "MACRO_ROWS_HALO_Y": "0.5",
        "MACRO_BLOCKAGE_HALO": "0",
        "MAX_ROUTING_LAYER": "M9",
        "GDS_ALLOW_EMPTY": "Element",
        "PWR_NETS_VOLTAGES": "",
        "GND_NETS_VOLTAGES": "",
        "IO_PLACER_V": "M5 M7",
        "IO_PLACER_H": "M4 M6",
        "DETAILED_ROUTE_END_ITERATION": "6",
    },
    macros = ["Element_generate_abstract"],
    sources = {
        "SDC_FILE": [":mock-array-constraints"],
        "IO_CONSTRAINTS": [":mock-array-io"],
    },
    verilog_files = [":verilog"],
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

orfs_flow(
    name = "Element",
    abstract_stage = POWER_STAGE_NAME,
    arguments = {
        "PLACE_DENSITY": "0.82",
        "MOCK_ARRAY_ROWS": "8",
        "MOCK_ARRAY_COLS": "8",
        "DETAILED_ROUTE_END_ITERATION": "6",
        "MIN_ROUTING_LAYER": "M2",
        "MAX_ROUTING_LAYER": "M5",
        "IO_PLACER_H": "M2 M4",
        "IO_PLACER_V": "M3 M5",
        "PLACE_PINS_ARGS": "-annealing",
        "GND_NETS_VOLTAGES": "",
        "PWR_NETS_VOLTAGES": "",
        "CORE_AREA": "{} {} {} {}".format(
            ce_margin_x,
            ce_margin_y,
            ce_width - ce_margin_x,
            ce_height - ce_margin_y,
        ),
        "DIE_AREA": "0 0 {} {}".format(
            ce_width,
            ce_height,
        ),
        "PDN_TCL": "$(PLATFORM_DIR)/openRoad/pdn/BLOCK_grid_strategy.tcl",
    },
    sources = {
        "IO_CONSTRAINTS": [":mock-array-element-io"],
        "SDC_FILE": [":mock-array-constraints"],
    },
    verilog_files = [":verilog"],
)

chisel_binary(
    name = "generate_verilog",
    srcs = glob(["src/main/scala/**/*.scala"]),
    main_class = "GenerateMockArray",
    resources = glob(["src/main/resources/**/*"]),
)

genrule(
    name = "genverilog",
    srcs = [],
    outs = ["array.sv"],
    cmd = "$(execpath :generate_verilog) --width 8 --height 8 --dataWidth 64 -- --firtool-binary-path $(execpath @circt//:bin/firtool) -- --default-layer-specialization=disable -o $(location :array.sv)",
    tools = [
        ":generate_verilog",
        "@circt//:bin/firtool",
    ],
)

filegroup(
    name = "verilog",
    srcs = [
        "src/main/resources/multiplier.v",
        ":genverilog",
    ],
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
        "@docker_orfs//:OpenROAD-flow-scripts/flow/platforms/asap7/work_around_yosys/asap7sc7p5t_OA_RVT_TT_201020.v",
    ],
)

verilator_cc_library(
    name = "array_verilator",
    copts = [
        # Don't care about warnings from Verilator generated C++
        "-Wno-unused-variable",
    ],
    module = ":array",
    module_top = "MockArray",
    trace = True,
    vopts = [
        "--timescale 1ps/1ps",
        "-Wall",
        "-Wno-DECLFILENAME",
        "-Wno-UNUSEDSIGNAL",
        "-Wno-PINMISSING",
        "--trace-underscore",
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
    tools = [
    ],
)

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

orfs_run(
    name = "MockArray_power",
    src = ":MockArray_{stage}".format(stage = POWER_STAGE_NAME),
    outs = [
        "power.txt",
    ],
    arguments = {
        "OUTPUT": "$(location :power.txt)",
        "VCD_STIMULI": "$(location :vcd)",
        "OPENROAD_EXE": "$(location //:opensta)",
        "POWER_STAGE_NAME": POWER_STAGE_NAME,
        "POWER_STAGE_STEM": POWER_STAGE_STEM,
    },
    data = [
               ":vcd",
               "//:opensta",
           ] + ["{macro}_{stage}".format(
               macro = macro,
               stage = POWER_STAGE_NAME,
           ) for macro in MACROS] +
           (["{macro}_parasitics".format(macro = macro) for macro in MACROS] if POWER_STAGE_NAME != "final" else []),
    script = ":power.tcl",
    tags = ["manual"],
    visibility = ["//visibility:public"],
)

# FIXME a orfs_run_test() rule would be nicer
sh_test(
    name = "MockArray_power_test",
    srcs = ["ok.sh"],
    args = [
        "$(location :MockArray_power)",
    ],
    data = [
        ":MockArray_power",
    ],
)
