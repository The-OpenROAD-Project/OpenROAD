# Copyright (c) 2019, Parallax Software, Inc.
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

include("openroad")

swig_lib(NAME          dbSta
         NAMESPACE     sta
         I_FILE        dbSta.i
         SWIG_INCLUDES ${OPENSTA_HOME}
                       ${OPENDB_HOME}/src/swig/common
                       ${OPENDB_HOME}/src/swig/tcl
                       ${OPENDB_HOME}/include/opendb
         SCRIPTS       ${OPENSTA_HOME}/tcl/Graph.tcl
                        ${OPENSTA_HOME}/tcl/Liberty.tcl
                       ${OPENSTA_HOME}/tcl/Network.tcl
                       ${OPENSTA_HOME}/tcl/NetworkEdit.tcl
                       ${OPENSTA_HOME}/tcl/Sdc.tcl
                       ${OPENSTA_HOME}/tcl/Search.tcl
                       ${OPENSTA_HOME}/tcl/Cmds.tcl
                       ${OPENSTA_HOME}/tcl/Variables.tcl
                       ${OPENSTA_HOME}/tcl/Sta.tcl
                       ${OPENSTA_HOME}/tcl/Power.tcl
                       ${OPENSTA_HOME}/tcl/Splash.tcl
                       ${OPENSTA_HOME}/dcalc/DelayCalc.tcl
                       ${OPENSTA_HOME}/parasitics/Parasitics.tcl
                       ${OPENSTA_HOME}/sdf/Sdf.tcl
                       ${DBSTA_HOME}/src/dbReadVerilog.tcl
                       ${DBSTA_HOME}/src/dbSta.tcl
)

target_sources(dbSta
  PRIVATE
    dbSta.cc
    dbNetwork.cc
    dbSdcNetwork.cc
    dbReadVerilog.cc
)

target_include_directories(dbSta
  PUBLIC
    ../include

  PRIVATE
    # Unfortunate side-effect of swig include StaTcl.i
    ${OPENSTA_HOME}/include/sta
    ${OPENSTA_HOME}
)

target_link_libraries(dbSta
  PRIVATE
    OpenSTA
    opendb
    gui
    utl
)

messages(
  TARGET dbSta
  OUTPUT_DIR ..
)
