# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, The OpenROAD Authors

include("openroad")

add_executable(TestPowerDomainRegions TestPowerDomainRegions.cc)

target_link_libraries(TestPowerDomainRegions
    GTest::gtest
    GTest::gtest_main
    upf_lib
    tst
    dbSta_lib
    utl_lib
    OpenSTA
    ${TCL_LIBRARY}
)

# Nangate45Fixture resolves its LEF relative to the working directory (bazel
# goes through runfiles instead), and src/upf/test has no Nangate45 of its own.
gtest_discover_tests(TestPowerDomainRegions
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test
)

add_dependencies(build_and_test TestPowerDomainRegions)
