# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "clock_domain",
    srcs = [
        "ClockDomain.cpp",
        "ClockDomainHash.cpp",
    ],
    hdrs = [
        "ClockDomain.hh",
        "ClockDomainHash.hh",
    ],
    includes = [
        ".",
    ],
    deps = [
        "//src/dft/src/config",
        "//src/utl",
    ],
)
