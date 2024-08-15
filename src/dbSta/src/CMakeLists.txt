###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2019, Parallax Software, Inc.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and#or other materials provided with the distribution.
##
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##
###############################################################################

include("openroad")

add_library(dbSta_lib
  dbSta.cc
  dbNetwork.cc
  dbSdcNetwork.cc
  dbReadVerilog.cc
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
    heatMap.cpp
    PathRenderer.cc
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
    gui
    utl
    dbSta_lib
)

messages(
  TARGET dbSta
  OUTPUT_DIR ..
)
