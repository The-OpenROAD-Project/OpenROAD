# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

add_library(stb_truetype
  stb_truetype.cpp
)

target_include_directories(stb_truetype
  PUBLIC
    ${OPENROAD_HOME}
)
