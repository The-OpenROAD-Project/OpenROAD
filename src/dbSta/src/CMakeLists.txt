# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

include("openroad")

add_library(dbSta_lib
  dbSta.cc
  dbNetwork.cc
  dbSdcNetwork.cc
  dbEditHierarchy.cc
  dbReadVerilog.cc
  SpefWriter.cc
  IpChecker.cc
)

target_include_directories(dbSta_lib
  PUBLIC
    ../include
    ${PROJECT_SOURCE_DIR}/include
)


target_link_libraries(dbSta_lib
  PUBLIC
    odb
    OpenSTA
  PRIVATE
    utl_lib
)

swig_lib(NAME          dbSta
         NAMESPACE     sta
         I_FILE        dbSta.i
         SWIG_INCLUDES ${OPENSTA_HOME}
                       ${ODB_HOME}/src/swig/common
                       ${ODB_HOME}/src/swig/tcl
                       ${ODB_HOME}/include
         SCRIPTS       ${OPENSTA_HOME}/tcl/CmdArgs.tcl
                       ${OPENSTA_HOME}/tcl/CmdUtil.tcl
                       ${OPENSTA_HOME}/tcl/Property.tcl
                       ${OPENSTA_HOME}/spice/WriteSpice.tcl
                       ${OPENSTA_HOME}/tcl/Variables.tcl
                       ${OPENSTA_HOME}/tcl/Sta.tcl
                       ${OPENSTA_HOME}/tcl/Splash.tcl
                       ${OPENSTA_HOME}/dcalc/DelayCalc.tcl
                       ${OPENSTA_HOME}/graph/Graph.tcl
                       ${OPENSTA_HOME}/liberty/Liberty.tcl
                       ${OPENSTA_HOME}/network/Network.tcl
                       ${OPENSTA_HOME}/network/NetworkEdit.tcl
                       ${OPENSTA_HOME}/parasitics/Parasitics.tcl
                       ${OPENSTA_HOME}/power/Power.tcl
                       ${OPENSTA_HOME}/sdc/Sdc.tcl
                       ${OPENSTA_HOME}/sdf/Sdf.tcl
                       ${OPENSTA_HOME}/search/Search.tcl
                       ${OPENSTA_HOME}/tcl/Util.tcl
                       ${DBSTA_HOME}/src/dbReadVerilog.tcl
                       ${DBSTA_HOME}/src/dbSta.tcl
)

target_sources(dbSta
  PRIVATE
    MakeDbSta.cc
)

# sta warnings
target_compile_options(dbSta
  PRIVATE
    -Wno-redundant-decls
    -Wno-unused-function
)

target_include_directories(dbSta
  PUBLIC
    ../include
    ${PROJECT_SOURCE_DIR}/include

  PRIVATE
    # Unfortunate side-effect of swig include StaTcl.i
    ${OPENSTA_HOME}/include/sta
    ${OPENSTA_HOME}
)

target_link_libraries(dbSta
  PRIVATE
    OpenSTA
    odb
    utl_lib
    dbSta_lib
)

messages(
  TARGET dbSta
  OUTPUT_DIR ..
)
