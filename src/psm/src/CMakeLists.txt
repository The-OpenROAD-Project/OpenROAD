# BSD 3-Clause License
# 
# Copyright (c) 2021, The Regents of the University of Minnesota
# 
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
# 
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# 
# * Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

include("openroad")

find_package(Eigen3 REQUIRED)

swig_lib(NAME      psm
         NAMESPACE psm
         I_FILE    pdnsim.i
         SCRIPTS   pdnsim.tcl
)

target_sources(psm
  PRIVATE
    MakePDNSim.cpp
    pdnsim.cpp
    heatMap.cpp
    ir_solver.cpp
    ir_network.cpp
    connection.cpp
    shape.cpp
    node.cpp

    debug_gui.cpp
)

target_include_directories( psm
  PUBLIC
    ../include
)

target_link_libraries(psm
  PRIVATE
    utl
    odb
    OpenSTA
    dbSta
    dpl_lib
    rsz_lib
    Eigen3::Eigen
    gui
    pad
    Boost::boost
)

messages(
  TARGET psm
  OUTPUT_DIR ..
)

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          psm_py
           NAMESPACE     psm
           LANGUAGE      python
           I_FILE        pdnsim-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/../include
                         ${ODB_HOME}/src/swig/common
                         ${ODB_HOME}/src/swig/python
                         ${ODB_HOME}/include
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/psm_py.py
  )

  target_include_directories(psm_py
    PUBLIC
      ../include
  )

  target_link_libraries(psm_py
    PUBLIC
      odb
      psm
      dpl_lib
  )

endif()
