# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

# Locate Cap'n Proto package
find_package(CapnProto REQUIRED)

# Set Cap'n Proto executables dynamically
find_program(CAPNP_EXECUTABLE capnp)
find_program(CAPNPC_CXX_EXECUTABLE capnpc-c++)

# Include directories
find_path(CAPNP_INCLUDE_DIRECTORY capnp/capnp.h)
include_directories(${CAPNP_INCLUDE_DIRECTORY})

# Link directories
find_library(CAPNP_LIBRARY_DIR capnp)
link_directories(${CAPNP_LIBRARY_DIR})

# Include Cap'n Proto macros
include(${CapnProto_DIR}/CapnProtoMacros.cmake)

# Define schema paths
set(CAPNPC_SRC_PREFIX "${PROJECT_SOURCE_DIR}/third-party/naja-if/schema" CACHE STRING "" FORCE)
set(CAPNPC_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})


capnp_generate_cpp(najaCommonSources najaCommonHeaders ${CAPNPC_SRC_PREFIX}/naja_common.capnp)
capnp_generate_cpp(najaNLInterfaceSources najaNLInterfaceHeaders ${CAPNPC_SRC_PREFIX}/naja_nl_interface.capnp)
capnp_generate_cpp(najaNLImplementationSources najaNLImplementationHeaders ${CAPNPC_SRC_PREFIX}/naja_nl_implementation.capnp)

add_library(naja_nl_dump STATIC
    ${najaCommonSources} ${najaNLInterfaceSources}
    ${najaNLImplementationSources}
    dbReadNajaImpl.cpp dbReadNajaInterface.cpp)

set_target_properties(naja_nl_dump PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(naja_nl_dump PRIVATE CapnProto::capnp )
target_link_libraries(naja_nl_dump PUBLIC naja_nl)
target_include_directories(naja_nl_dump PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(naja_nl_dump PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
