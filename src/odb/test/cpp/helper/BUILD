# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

package(
    features = ["layering_check"],
)

cc_library(
    name = "helper",
    srcs = [
        "env.cpp",
        "helper.cpp",
    ],
    hdrs = [
        "env.h",
        "helper.h",
    ],
    includes = ["."],  # not needed after no cmake
    visibility = ["//visibility:public"],
    deps = [
        "//src/odb",
        "//src/utl",
    ],
)
