#############################################################################
##
## Copyright (c) 2019, OpenROAD
## All rights reserved.
##
## BSD 3-Clause License
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and/or other materials provided with the distribution.
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
#############################################################################

set(PARQUET_HOME ${MPLACE_HOME}/src/ParquetFP )

add_subdirectory(${PARQUET_HOME})

############################################################
# Create a library 
############################################################

set(MPLACE_WRAP ${CMAKE_CURRENT_BINARY_DIR}/MacroPlacer_wrap.cc)
set(MPLACE_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/MacroPlacerTclInitVar.cc)


add_custom_command(OUTPUT ${MPLACE_WRAP}
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -namespace -prefix mpl -o ${MPLACE_WRAP} -I${MPLACE_HOME}/include ${MPLACE_HOME}/src/MacroPlacer.i   
  WORKING_DIRECTORY ${MPLACE_HOME}
  DEPENDS ${MPLACE_HOME}/src/MacroPlacer.i "include/mpl/MacroPlacer.h"
)

add_custom_command(OUTPUT ${MPLACE_TCL_INIT}
  COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${MPLACE_TCL_INIT} macro_placer_tcl_inits ${MPLACE_HOME}/src/MacroPlacer.tcl
  WORKING_DIRECTORY ${MPLACE_HOME}/src
  DEPENDS ${MPLACE_HOME}/src/MacroPlacer.tcl ${OPENSTA_HOME}/etc/TclEncode.tcl
)

############################################################
# Create an library 
############################################################
add_library( TritonMacroPlace 
  src/MacroPlacer.cpp
  src/MakeMacroPlacer.cpp
  src/Partition.cpp
  ${MPLACE_WRAP}
  ${MPLACE_TCL_INIT}
)

############################################################
# include directories 
############################################################

target_include_directories( TritonMacroPlace 
PUBLIC
  ${MPLACE_HOME}/src
  ${MPLACE_HOME}/include
  utility

PRIVATE
  ${PARQUET_HOME}/src

  ${OPENSTA_HOME}/include

  ${OPENDB_HOME}/include
  ${OPENROAD_HOME}/include
  ${DBSTA_HOME}/include

  ${TCL_INCLUDE_PATH}
  )

target_link_libraries( TritonMacroPlace
  utility
  )
