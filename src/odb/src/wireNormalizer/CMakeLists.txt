# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, Precision Innovations Inc.

add_library(wireNormalizer
    connectionGraph.cpp
    normalizeWireConnectivity.cpp
    shapeSearch.cpp
    wireNormalizer.cpp
)

target_include_directories(wireNormalizer
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(wireNormalizer
    db
    utl_lib
)

set_target_properties(wireNormalizer
  PROPERTIES
    # python requirement
    POSITION_INDEPENDENT_CODE ON
)
