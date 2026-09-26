# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

include("openroad")

# Reads its technologies from the module's test data, so it runs from the
# source root like the fixture expects.
add_executable(TestCleanupShapes TestCleanupShapes.cpp)
target_link_libraries(TestCleanupShapes PRIVATE GTest::gtest_main tst pdn odb)
gtest_discover_tests(TestCleanupShapes WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
add_dependencies(build_and_test TestCleanupShapes)
