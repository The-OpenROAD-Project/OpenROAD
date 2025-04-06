# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

add_library(defout
    defout.cpp
    defout_impl.cpp
)

target_include_directories(defout
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${TCL_INCLUDE_PATH}
)
target_link_libraries(defout
    db
    utl_lib
)

set_target_properties(defout
  PROPERTIES
    # python requirement
    POSITION_INDEPENDENT_CODE ON
)

