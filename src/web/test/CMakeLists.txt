# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

or_integration_tests(
  "web"
  PASSFAIL_TESTS
    cpp_tests
)

add_subdirectory(cpp)
