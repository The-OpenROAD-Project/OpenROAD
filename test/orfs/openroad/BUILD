load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_python//python:py_binary.bzl", "py_binary")
load("@rules_python//python:py_test.bzl", "py_test")

package(features = ["layering_check"])

# Standalone detailed_placement binary.
# Links only dpl + odb + utl — rebuilds in seconds, not minutes.
# gdb --args bazel-bin/test/orfs/openroad/detailed_placement --read_db crash.odb
cc_binary(
    name = "detailed_placement",
    srcs = ["detailed_placement_main.cc"],
    deps = [
        "//src/dpl",
        "//src/odb",
        "//src/utl",
    ],
)

# Python CLI that works for ANY OpenROAD command.
# Routes to native cc_binary when available, falls back to monolithic openroad.
py_binary(
    name = "openroad_cmd",
    srcs = ["openroad_cmd.py"],
    data = [
        ":detailed_placement",
        "//:openroad",
    ],
)

py_test(
    name = "openroad_cmd_test",
    srcs = ["openroad_cmd_test.py"],
    data = [
        ":detailed_placement",
        ":openroad_cmd",
        "//:openroad",
        "//src/dpl/test:gcd",
    ],
    tags = ["no-remote"],
)

cc_binary(
    name = "detailed_route",
    srcs = ["detailed_route_main.cc"],
    copts = [
        "-fopenmp",
        "-Isrc/drt/src",
    ],
    features = ["-layering_check"],
    deps = [
        "//src/drt",
        "//src/dst",
        "//src/odb",
        "//src/stt",
        "//src/utl",
        "@openmp",
    ],
)

cc_binary(
    name = "optimize_mirroring",
    srcs = ["optimize_mirroring_main.cc"],
    deps = [
        "//src/dpl",
        "//src/odb",
        "//src/utl",
    ],
)

cc_binary(
    name = "filler_placement",
    srcs = ["filler_placement_main.cc"],
    deps = [
        "//src/dpl",
        "//src/odb",
        "//src/utl",
    ],
)

cc_binary(
    name = "check_placement",
    srcs = ["check_placement_main.cc"],
    deps = [
        "//src/dpl",
        "//src/odb",
        "//src/utl",
    ],
)

cc_binary(
    name = "repair_design",
    srcs = ["repair_design_main.cc"],
    deps = [
        "//src/ant",
        "//src/dbSta",
        "//src/dpl",
        "//src/est",
        "//src/grt",
        "//src/odb",
        "//src/rsz",
        "//src/stt",
        "//src/utl",
    ],
)

cc_binary(
    name = "global_placement",
    srcs = ["global_placement_main.cc"],
    copts = ["-fopenmp"],
    deps = [
        "//src/ant",
        "//src/dbSta",
        "//src/dpl",
        "//src/gpl",
        "//src/grt",
        "//src/odb",
        "//src/rsz",
        "//src/stt",
        "//src/utl",
        "@openmp",
    ],
)

cc_binary(
    name = "clock_tree_synthesis",
    srcs = ["clock_tree_synthesis_main.cc"],
    deps = [
        "//src/ant",
        "//src/cts",
        "//src/dbSta",
        "//src/dpl",
        "//src/est",
        "//src/grt",
        "//src/odb",
        "//src/rsz",
        "//src/stt",
        "//src/utl",
    ],
)

cc_binary(
    name = "global_route",
    srcs = ["global_route_main.cc"],
    copts = ["-fopenmp"],
    deps = [
        "//src/ant",
        "//src/dbSta",
        "//src/dpl",
        "//src/grt",
        "//src/odb",
        "//src/stt",
        "//src/utl",
        "@openmp",
    ],
)

cc_binary(
    name = "init_floorplan",
    srcs = ["init_floorplan_main.cc"],
    deps = [
        "//src/dbSta",
        "//src/ifp",
        "//src/odb",
        "//src/utl",
    ],
)

cc_binary(
    name = "analyze_power_grid",
    srcs = ["analyze_power_grid_main.cc"],
    deps = [
        "//src/ant",
        "//src/dbSta",
        "//src/dpl",
        "//src/est",
        "//src/grt",
        "//src/odb",
        "//src/psm",
        "//src/stt",
        "//src/utl",
    ],
)

cc_binary(
    name = "density_fill",
    srcs = ["density_fill_main.cc"],
    deps = [
        "//src/fin",
        "//src/odb",
        "//src/utl",
    ],
)

cc_binary(
    name = "place_pins",
    srcs = ["place_pins_main.cc"],
    deps = [
        "//src/dbSta",
        "//src/odb",
        "//src/ppl",
        "//src/utl",
    ],
)

cc_binary(
    name = "macro_placement",
    srcs = ["macro_placement_main.cc"],
    deps = [
        "//src/dbSta",
        "//src/mpl",
        "//src/odb",
        "//src/par",
        "//src/utl",
    ],
)

cc_binary(
    name = "check_antennas",
    srcs = ["check_antennas_main.cc"],
    deps = [
        "//src/ant",
        "//src/odb",
        "//src/utl",
    ],
)

cc_binary(
    name = "extract_parasitics",
    srcs = ["extract_parasitics_main.cc"],
    deps = [
        "//src/odb",
        "//src/rcx",
        "//src/utl",
    ],
)

cc_binary(
    name = "tapcell",
    srcs = ["tapcell_main.cc"],
    deps = [
        "//src/odb",
        "//src/tap",
        "//src/utl",
    ],
)

py_test(
    name = "integration_test",
    srcs = ["integration_test.py"],
    data = [
        ":check_antennas",
        ":detailed_placement",
        ":detailed_route",
        ":global_route",
        ":optimize_mirroring",
        "//:openroad",
    ],
    tags = [
        "manual",
        "orfs",
    ],
    timeout = "long",
)

sh_binary(
    name = "openroad-detailed-placement",
    srcs = ["openroad-detailed-placement.sh"],
    data = [":detailed_placement"],
)
