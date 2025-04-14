# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

find_package(Boost REQUIRED)

add_subdirectory(src)
if (ENABLE_TESTS)
    add_subdirectory(test)
endif()
