# BSD 3-Clause License
#
# Copyright (c) 2020, MICL, DD-Lab, University of Michigan
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
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

include("openroad")

find_package(OpenMP REQUIRED)

swig_lib(NAME      ant
         NAMESPACE ant
         I_FILE    AntennaChecker.i
         SCRIPTS   AntennaChecker.tcl
)

add_library(ant_lib
    AntennaChecker.cc
    Polygon.cc
)

target_sources(ant
  PRIVATE
    MakeAntennaChecker.cc
)

target_include_directories(ant_lib
  PUBLIC
    ../include
)

target_link_libraries(ant_lib
  PUBLIC
    odb
    OpenSTA
    utl_lib
    OpenMP::OpenMP_CXX
)

target_link_libraries(ant
  PUBLIC
    ant_lib
    grt_lib
    utl
    OpenMP::OpenMP_CXX
)

messages(
  TARGET ant
  OUTPUT_DIR ..
)

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          ant_py
           NAMESPACE     ant
           LANGUAGE      python
           I_FILE        AntennaChecker-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/../include
                         ${ODB_HOME}/src/swig/common
                         ${ODB_HOME}/src/swig/python
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/ant_py.py
  )

  target_include_directories(ant_py
    PUBLIC
      ../include
  )

  target_link_libraries(ant_py
    PUBLIC
      odb
      OpenSTA
      grt
      utl
  )

endif()
