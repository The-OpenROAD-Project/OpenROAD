# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

package(
    features = ["layering_check"],
)

cc_test(
    name = "TestAbstractLef",
    srcs = [
        "TestAbstractLef.cc",
        "sky130_test_fixture.h",
    ],
    data = [
        "//src/odb/test/data/sky130hd:lef-test-data",
    ],
    defines = [
        'DATA_PREFIX=\\"src/odb/test/\\"',  # TODO: not needed after no cmake
    ],
    linkstatic = True,  # TODO: remove once deps define all symbols
    deps = [
        "//src/odb",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestDbWire",
    srcs = ["TestDbWire.cc"],
    data = [
        "//src/odb/test/data/sky130hd:lef-test-data",
    ],
    defines = [
        'DATA_PREFIX=\\"src/odb/test/\\"',  # TODO: not needed after no cmake
    ],
    linkstatic = True,  # TODO: remove once deps define all symbols
    deps = [
        "//src/odb",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

# TODO: more to come.
