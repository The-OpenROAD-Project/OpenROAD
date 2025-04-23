# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

include("openroad")

swig_lib(NAME          odbtcl
         NAMESPACE     odb
         I_FILE        ../common/odb.i
         SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/include
                       ${PROJECT_SOURCE_DIR}/src/swig/tcl
         SCRIPTS       odb.tcl
)

target_sources(odbtcl
  PRIVATE
    MakeOdb.cpp
)
  
target_include_directories(odbtcl
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include/odb
)

target_link_libraries(odbtcl
    PUBLIC
        odb
        utl_lib
        cdl
        odb_swig_common
        Boost::boost
        ${TCL_LIBRARY}
)

# Executable
add_executable(odbtcl-bin
    main.cpp
    stub.cpp
)

target_include_directories(odbtcl-bin
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include/odb
    PRIVATE
        ${OPENROAD_HOME}/include
)

target_link_libraries(odbtcl-bin
    PUBLIC
        odbtcl
        dl
)

set_target_properties(odbtcl-bin
    PROPERTIES 
        OUTPUT_NAME odbtcl
)
