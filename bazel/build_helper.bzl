# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

"""Source Tracking for OpenROAD"""

# This file should go away eventually. It hides crucial information
# for developers and tools.
#
# Right now, there is a lot of duplicate use of the same dependencies
# and sources in the toplevel BUILD file, this is why these
# variables exist.
#
# Once this is a well-defined bazel project, there is no need for
# variables anymore.

OPENROAD_BINARY_SRCS_WITHOUT_MAIN = [
    #Root OpenRoad
    ":openroad_swig",
    ":openroad_tcl",
    #RMP
    ":rmp_swig",
    ":rmp_tcl",
    #dft
    ":dft_swig",
    ":dft_tcl",
]

OPENROAD_LIBRARY_HDRS_INCLUDE = [
    #STA
    "src/sta/include/sta/*.hh",
    #RMP
    "src/rmp/src/*.h",
    "src/rmp/include/rmp/*.h",
    #dft
    "src/dft/include/dft/*.hh",
]

# Once we properly include headers relative to project-root,
# this will not be needed anymore.
OPENROAD_LIBRARY_INCLUDES = [
    #Root OpenRoad
    "include",
    #STA
    "src/sta",
    "src/sta/include/sta",
    #RMP
    "src/rmp/include",
    #dft
    "src/dft/include",
    "src/dft/src/clock_domain",
    "src/dft/src/config",
    "src/dft/src/utils",
    "src/dft/src/cells",
    "src/dft/src/replace",
    "src/dft/src/architect",
    "src/dft/src/stitch",
]

OPENROAD_LIBRARY_SRCS_EXCLUDE = [
    "src/Main.cc",
    "src/OpenRoad.cc",
]

OPENROAD_LIBRARY_SRCS_INCLUDE = [
    #Root OpenRoad
    "src/*.cc",
    "src/*.cpp",
    #OpenRCX
    "src/rcx/src/*.cpp",
    "src/rcx/src/*.h",
    #RMP
    "src/rmp/src/*.cpp",
    #dft
    "src/dft/src/**/*.cpp",
    "src/dft/src/**/*.hh",
]
