# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

include(openroad)

add_library(cut_test_helper helper.cpp)

target_link_libraries(cut_test_helper
  PRIVATE
    cut
    tst
)


target_include_directories(cut_test_helper
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)
