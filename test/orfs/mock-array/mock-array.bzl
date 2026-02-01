"""
Generates mock-array test cases
"""

load("@bazel-orfs//:generate.bzl", "fir_library")
load("@bazel-orfs//:openroad.bzl", "orfs_flow", "orfs_run")
load("@bazel-orfs//:verilog.bzl", "verilog_directory", "verilog_single_file_library")
load("@rules_cc//cc:defs.bzl", "cc_binary")
load("@rules_shell//shell:sh_test.bzl", "sh_test")
load("@rules_verilator//verilator:defs.bzl", "verilator_cc_library")
load("@rules_verilator//verilog:defs.bzl", "verilog_library")
load("//test/orfs:eqy-flow.bzl", "eqy_flow_test")

FIRTOOL_OPTIONS = [
    "-disable-all-randomization",
    "-strip-debug-info",
    "-enable-layers=Verification",
    "-enable-layers=Verification.Assert",
    "-enable-layers=Verification.Assume",
    "-enable-layers=Verification.Cover",
]

def verilog(name, rows, cols):
    """Generate mock array verilog

    Args:
        name: Name of the verilog target
        rows: Number of rows in the array
        cols: Number of columns in the array
    """

    fir_library(
        name = "generate_{name}_fir".format(name = name),
        data = [
        ],
        generator = "//test/orfs/mock-array:generate_verilog",
        opts = [
            "--width=" + str(cols),
            "--height=" + str(rows),
            "--dataWidth=64",
            "--",
            # Imagine Chisel arguments here
            "--",
        ] + FIRTOOL_OPTIONS,
        tags = ["manual"],
    )

    verilog_directory(
        name = "generate_{name}_split".format(name = name),
        srcs = [":generate_{name}_fir".format(name = name)],
        opts = FIRTOOL_OPTIONS,
        tags = ["manual"],
    )

    verilog_single_file_library(
        name = "{name}_array.sv".format(name = name),
        srcs = [":generate_{name}_split".format(name = name)],
        tags = ["manual"],
        visibility = ["//visibility:public"],
    )

    native.filegroup(
        name = "{name}_verilog".format(name = name),
        srcs = [
            "src/main/resources/multiplier.v",
            ":{name}_array.sv".format(name = name),
        ],
        tags = ["manual"],
    )

def element(name, config):
    orfs_flow(
        name = "Element",
        arguments = {
            "CORE_AREA": "{} {} {} {}".format(
                ce_margin_x,
                ce_margin_y,
                config["ce_width"] - ce_margin_x,
                config["ce_height"] - ce_margin_y,
            ),
            "DETAILED_ROUTE_END_ITERATION": "6",
            "DIE_AREA": "0 0 {} {}".format(
                config["ce_width"],
                config["ce_height"],
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
        verilog_files = [":{name}_verilog".format(name = name)],
        variant = "{name}_base".format(name = name),
    )
    eqy_flow_test(
        name = "Element_eqy_{variant}".format(variant = name),
        flow = "Element_{variant}_base".format(variant = name),
        verilog_files = [":{name}_verilog".format(name = name)],
        tags = ["manual"],
        module_top = "Element",
    )

POWER_STAGES = {
    "cts": {
        "stage": "4",
    },
    "final": {
        "stage": "6",
    },
}

POWER_STAGE_STEM = {
    stage: POWER_STAGES[stage]["stage"] + "_" + stage
    for stage in POWER_STAGES
}

# single source of truth for defaults.
# each number is a unit
# current unit is configured as 2.16 which is on the routing grid for M5

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

# Use smallest working value for minimum area of test-case
ce_x = 8

ce_y = 8

# top level core offset
margin_x = placement_grid_x * 2

margin_y = placement_grid_y * 2

# Element core margin
ce_margin_x = placement_grid_x * 0.5

ce_margin_y = placement_grid_y * 0.5

# PDN problems if it is smaller. Not investigated.
array_spacing_x = placement_grid_x * 4

array_spacing_y = placement_grid_y * 4

def config(name, rows, cols):
    """Generate mock array co   nfiguration object

    Args:
        name: Name of the configuration
        rows: Number of rows in the array
        cols: Number of columns in the array
    Returns:
        A dictionary with configuration parameters
    """
    config = {}
    config["name"] = name

    # number of Elements in row and column, can be control by user via environment variable
    #
    #  rows, cols       - number of Element in rows, cols
    #  width, height    - width and height of each Element
    #
    # When the pitch is equal to the width/height, we have routing by abutment
    # https://en.wikipedia.org/wiki/Pitch#Linear_measurement
    #
    #  pitch_x, pitch_y - placement pitch for each Element, in x and y direction
    # specification are in unit of placement grid
    config["rows"] = rows

    config["cols"] = cols

    # Element size is set to multiple of placement grid above
    config["ce_width"] = ce_x * placement_grid_x

    config["ce_height"] = ce_y * placement_grid_y

    # Routing by abutment horizontally
    config["pitch_x"] = config["ce_width"]

    # Some routing space vertically for clock routing
    config["pitch_y"] = config["ce_height"] + (placement_grid_y * 2)

    # top level core and die size, we need some space around the
    # array to put some flip flops and buffers for top level io pins
    config["core_width"] = (
        config["pitch_x"] * (cols - 1) + config["ce_width"] + margin_x * 2
    )

    config["core_height"] = (
        config["pitch_y"] * (rows - 1) + config["ce_height"] + margin_y * 2
    )

    config["die_width"] = config["core_width"] + (array_spacing_x * 2)

    config["die_height"] = config["core_height"] + (array_spacing_y * 2)

    return config

MACROS = [
    "Element",
    "MockArray",
]

POWER_TESTS = [
    "openroad",
    "power",
    "power_instances",
    "path_groups",
]

ELEMENT_POWER_TESTS = [
    "power_modules",
]

def mock_array(name, config):
    """Defines the mock array build targets for OpenROAD-flow-scripts.

    Args:
        name: Name of the configuration
        config: Configuration object from config()
    """

    variants = {v: "{name}_{v}".format(name = name, v = v) for v in [
        "base",
        "flat",
    ]}

    for v, variant in variants.items():
        orfs_flow(
            name = "MockArray",
            arguments = {
                "CORE_AREA": "{} {} {} {}".format(
                    array_spacing_x,
                    array_spacing_y,
                    array_spacing_x + config["core_width"],
                    array_spacing_y + config["core_height"],
                ),
                "DETAILED_ROUTE_END_ITERATION": "6",
                "DIE_AREA": "0 0 {} {}".format(
                    config["die_width"],
                    config["die_height"],
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
                "OPENROAD_HIERARCHICAL": "1" if v == "base" else "0",
                "PDN_TCL": "$(PLATFORM_DIR)/openRoad/pdn/BLOCKS_grid_strategy.tcl",
                "PLACE_DENSITY": "0.30",
                "PLACE_PINS_ARGS": "-annealing",
                "PWR_NETS_VOLTAGES": "",
                "RTLMP_BOUNDARY_WT": "0",
                "RTLMP_MAX_INST": "250",
                "RTLMP_MAX_MACRO": "64",
                "RTLMP_MIN_INST": "50",
                "RTLMP_MIN_MACRO": "8",
                "RTLMP_NOTCH_WT": "0",
            },
            macros = ["Element_{name}_base_generate_abstract".format(name = name)],
            sources = {
                "IO_CONSTRAINTS": [":mock-array-io"],
                "RULES_JSON": [":rules-{variant}.json".format(variant = variant)],
                "SDC_FILE": [":mock-array-constraints"],
            } | ({
                "IO_CONSTRAINTS": [":write_pin_placement"],
                "MACRO_PLACEMENT_TCL": [":write_macro_placement"],
            } if variant == "4x4_flat" else {}),
            tags = ["manual"],
            test_kwargs = {
                "tags": ["orfs"],
            },
            variant = variant,
            verilog_files = [":{name}_verilog".format(name = name)],
        )
        eqy_flow_test(
            name = "MockArray_eqy_{variant}".format(variant = variant),
            flow = "MockArray_{variant}".format(variant = variant),
            verilog_files = [":{name}_verilog".format(name = name)],
            other_verilog_files = [":Element_eqy_{name}_final_verilog".format(name = name)],
            tags = ["manual"],
            module_top = "MockArray",
        )

        for stage in POWER_STAGES:
            for macro in MACROS:
                if macro == "Element" and v != "base":
                    continue
                short_variant = (name + "_base") if macro == "Element" else variant
                if stage != "final":
                    orfs_run(
                        name = "{variant}_{macro}_parasitics".format(variant = variant, macro = macro),
                        src = ":{macro}_{variant}_{stage}".format(
                            macro = macro,
                            variant = short_variant,
                            stage = stage,
                        ),
                        outs = [
                            "results/asap7/{macro}/{variant}/{stem}.spef".format(
                                macro = macro,
                                variant = short_variant,
                                stem = POWER_STAGE_STEM[stage],
                            ),
                            "results/asap7/{macro}/{variant}/{stem}.v".format(
                                macro = macro,
                                variant = short_variant,
                                stem = POWER_STAGE_STEM[stage],
                            ),
                        ],
                        script = ":parasitics.tcl",
                        tags = ["manual"],
                        visibility = ["//visibility:public"],
                    )
                else:
                    native.filegroup(
                        name = "{variant}_{macro}_netlist".format(variant = variant, macro = macro),
                        srcs = [
                            ":{macro}_{variant}_{stage}".format(
                                variant = short_variant,
                                macro = macro,
                                stage = stage,
                            ),
                        ],
                        output_group = POWER_STAGE_STEM[stage] + ".v",
                    )

        for stage in POWER_STAGES:
            verilog_library(
                name = "array_{variant}_{stage}".format(variant = variant, stage = stage),
                srcs = [
                    ("results/asap7/{macro}/{variant}/{stem}.v".format(
                        macro = macro,
                        variant = (name + "_base") if macro == "Element" else variant,
                        stem = POWER_STAGE_STEM[stage],
                    ) if stage != "final" else "{variant}_{macro}_netlist".format(
                        macro = macro,
                        variant = (name + "_base") if macro == "Element" else variant,
                    ))
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
                name = "array_verilator_{variant}_{stage}".format(variant = variant, stage = stage),
                copts = [
                    # Don't care about warnings from Verilator generated C++
                    "-Wno-unused-variable",
                ],
                module = ":array_{variant}_{stage}".format(variant = variant, stage = stage),
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
                name = "simulator_{variant}_{stage}".format(variant = variant, stage = stage),
                srcs = [
                    "simulate.cpp",
                ],
                # Best way to refer to static names in Verilator generated code?
                copts = [
                    "-DARRAY_COLS=\"{}\"".format(
                        ",".join([
                            "&top->io_ins_down_{col}, &top->io_ins_up_{col}".format(col = col)
                            for col in range(config["cols"])
                        ]),
                    ),
                    "-DARRAY_ROWS=\"{}\"".format(
                        ",".join([
                            "&top->io_ins_left_{row}, &top->io_ins_right_{row}".format(row = row)
                            for row in range(config["rows"])
                        ]),
                    ),
                ],
                cxxopts = [
                    "-std=c++23",
                ],
                tags = ["manual"],
                deps = [
                    ":array_verilator_{variant}_{stage}".format(variant = variant, stage = stage),
                ],
            )

            native.genrule(
                name = "vcd_{variant}_{stage}".format(variant = variant, stage = stage),
                srcs = [
                    # FIXME move to tools, using target configuration for now to avoid rebuilds
                    ":simulator_{variant}_{stage}".format(variant = variant, stage = stage),
                ],
                outs = ["MockArrayTestbench_{variant}_{stage}.vcd".format(variant = variant, stage = stage)],
                cmd = "$(execpath :simulator_{variant}_{stage}) $(location :MockArrayTestbench_{variant}_{stage}.vcd)".format(variant = variant, stage = stage),
                tags = ["manual"],
                tools = [
                ],
            )

        # If we want to measure power after final, instead of with estimated parasitics,
        # we'll need this.
        # buildifier: disable=unused-variable
        SPEFS_AND_NETLISTS = [
            ":results/asap7/{variant}/{stem}.{ext}".format(
                ext = ext,
                macro = macro,
                stem = POWER_STAGE_STEM[stage],
                variant = variant,
            )
            for macro in MACROS
            for ext in [
                "spef",
                "v",
            ]
            for stage in POWER_STAGES
        ]

        for power_test in POWER_TESTS:
            for stage in POWER_STAGES:
                orfs_run(
                    name = "MockArray_{variant}_{stage}_{power_test}".format(
                        variant = variant,
                        power_test = power_test,
                        stage = stage,
                    ),
                    src = ":MockArray_{variant}_{stage}".format(variant = variant, stage = stage),
                    outs = [
                        "{variant}_{power_test}_{stage}.txt".format(
                            variant = variant,
                            power_test = power_test,
                            stage = stage,
                        ),
                    ],
                    arguments = {
                        "ARRAY_COLS": str(config["cols"]),
                        "ARRAY_ROWS": str(config["rows"]),
                        "LOAD_MOCK_ARRAY_TCL": "$(location :load_mock_array.tcl)",
                        "OUTPUT": "$(location :{variant}_{power_test}_{stage}.txt)".format(
                            variant = variant,
                            power_test = power_test,
                            stage = stage,
                        ),
                        "POWER_STAGE_NAME": stage,
                        "POWER_STAGE_STEM": POWER_STAGE_STEM[stage],
                        "VCD_STIMULI": "$(location :vcd_{variant}_{stage})".format(variant = variant, stage = stage),
                    } | ({"openroad": {}}.get(
                        power_test,
                        {
                            "OPENROAD_EXE": "$(location //src/sta:opensta)",
                        },
                    )),
                    data = [
                               # FIXME this is a workaround to ensure that the OpenSTA runfiles are available
                               ":opensta_runfiles",
                               ":vcd_{variant}_{stage}".format(variant = variant, stage = stage),
                               ":load_mock_array.tcl",
                           ] + ["{macro}_{variant}_{stage}".format(
                               variant = (name + "_base") if macro == "Element" else variant,
                               macro = macro,
                               stage = stage,
                           ) for macro in MACROS] +
                           (["{variant}_{macro}_parasitics".format(
                               variant = (name + "_base") if macro == "Element" else variant,
                               macro = macro,
                           ) for macro in MACROS] if stage != "final" else []),
                    script = ":{power_test}.tcl".format(power_test = power_test if power_test != "openroad" else "power"),
                    tags = ["manual"],
                    tools = ["//src/sta:opensta"],
                    visibility = ["//visibility:public"],
                )

                sh_test(
                    name = "MockArray_{variant}_{stage}_{power_test}_test".format(
                        variant = variant,
                        power_test = power_test,
                        stage = stage,
                    ),
                    srcs = ["ok.sh"],
                    args = [
                        "$(location :MockArray_{variant}_{stage}_{power_test})".format(
                            variant = variant,
                            stage = stage,
                            power_test = power_test,
                        ),
                    ],
                    data = [
                        ":MockArray_{variant}_{stage}_{power_test}".format(
                            variant = variant,
                            power_test = power_test,
                            stage = stage,
                        ),
                    ],
                )

        for power_test in ELEMENT_POWER_TESTS:
            if v != "base":
                continue
            for stage in POWER_STAGES:
                orfs_run(
                    name = "Element_{variant}_{stage}_{power_test}".format(
                        variant = variant,
                        power_test = power_test,
                        stage = stage,
                    ),
                    src = ":Element_{variant}_{stage}".format(variant = variant, stage = stage),
                    outs = [
                        "{variant}_{power_test}_{stage}.txt".format(
                            variant = variant,
                            power_test = power_test,
                            stage = stage,
                        ),
                    ],
                    arguments = {
                        "OUTPUT": "$(location :{variant}_{power_test}_{stage}.txt)".format(
                            variant = variant,
                            power_test = power_test,
                            stage = stage,
                        ),
                        "POWER_STAGE_NAME": stage,
                        "POWER_STAGE_STEM": POWER_STAGE_STEM[stage],
                        "VCD_STIMULI": "$(location :vcd_{variant}_{stage})".format(variant = variant, stage = stage),
                    },
                    data = [
                        ":vcd_{variant}_{stage}".format(variant = variant, stage = stage),
                    ],
                    script = ":{power_test}.tcl".format(power_test = power_test),
                    tags = ["manual"],
                    visibility = ["//visibility:public"],
                )

                sh_test(
                    name = "Element_{variant}_{power_test}_{stage}_test".format(
                        variant = variant,
                        power_test = power_test,
                        stage = stage,
                    ),
                    srcs = ["ok.sh"],
                    args = [
                        "$(location :Element_{variant}_{stage}_{power_test})".format(
                            variant = variant,
                            stage = stage,
                            power_test = power_test,
                        ),
                    ],
                    data = [
                        ":Element_{variant}_{stage}_{power_test}".format(
                            variant = variant,
                            power_test = power_test,
                            stage = stage,
                        ),
                    ],
                )
