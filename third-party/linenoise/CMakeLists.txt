# SPDX-License-Identifier: BSD-2-Clause
# Copyright (c) 2010-2023, Salvatore Sanfilippo <antirez at gmail dot com>

add_library(linenoise
  linenoise.c
)

target_include_directories(linenoise
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(linenoise PROPERTIES POSITION_INDEPENDENT_CODE ON)
