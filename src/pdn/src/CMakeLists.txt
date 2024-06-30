###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2022, The Regents of the University of California
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

swig_lib(NAME      pdn
         NAMESPACE pdn
         I_FILE    PdnGen.i
         SWIG_INCLUDES ${ODB_HOME}/src/swig/common
                       ${ODB_HOME}/src/swig/tcl
                       ${ODB_HOME}/include
         SCRIPTS   pdn.tcl
)

target_sources(pdn
  PRIVATE
    PdnGen.cc
    MakePdnGen.cc
    
    techlayer.cpp
    domain.cpp
    power_cells.cpp
    grid.cpp
    grid_component.cpp
    rings.cpp
    straps.cpp
    shape.cpp
    via.cpp
    connect.cpp
    renderer.cpp
    sroute.cpp
    via_repair.cpp
)

target_include_directories(pdn
  PUBLIC
    ../include
)

target_link_libraries(pdn
  PRIVATE
    odb
    utl
    gui
    Boost::boost
)

messages(
  TARGET pdn
  OUTPUT_DIR ..
)

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          pdn_py
           NAMESPACE     pdn
           LANGUAGE      python
           I_FILE        PdnGen-py.i
           SWIG_INCLUDES ${ODB_HOME}/src/swig/common
                         ${ODB_HOME}/src/swig/python
                         ${ODB_HOME}/include
                         ${PROJECT_SOURCE_DIR}/../include/pdn
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/pdn_py.py
  )

  target_include_directories(pdn_py
    PUBLIC
      .
  )
  
  target_link_libraries(pdn_py
    PUBLIC
      pdn
      odb
      gui
    )

endif()
