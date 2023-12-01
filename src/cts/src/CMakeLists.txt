###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2019, The Regents of the University of California
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

# https://github.com/The-OpenROAD-Project/OpenROAD/issues/1186
find_package(LEMON NAMES LEMON lemon REQUIRED)

add_library(cts_lib
    Clock.cpp
    TreeBuilder.cpp
    HTreeBuilder.cpp
    SinkClustering.cpp
    TechChar.cpp
    TritonCTS.cpp
    Clustering.cpp
    LevelBalancer.cpp
)

swig_lib(NAME      cts
         NAMESPACE cts
         I_FILE    TritonCTS.i
         SCRIPTS   TritonCTS.tcl
)

target_sources(cts
  PRIVATE
    MakeTritoncts.cpp
    CtsGraphics.cpp
)

target_include_directories(cts_lib
  PUBLIC
    ../include
  PRIVATE
    .
  ${LEMON_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
 )

target_include_directories(cts
  PUBLIC
    ../include
  PRIVATE
    .
  ${Boost_INCLUDE_DIRS}
 )

target_link_libraries(cts_lib
  PUBLIC
    rsz_lib
  PRIVATE
    odb
    dbSta_lib
    OpenSTA
    stt_lib
    utl_lib
)

target_link_libraries(cts
  PRIVATE
    odb
    cts_lib
    dbSta_lib
    rsz_lib
    OpenSTA
    gui
 )

messages(
  TARGET cts
  OUTPUT_DIR ..
)

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          cts_py
           NAMESPACE     cts
           LANGUAGE      python
           I_FILE        TritonCTS-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/../include/cts
                         ${PROJECT_SOURCE_DIR}
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/cts_py.py
  )

  target_include_directories(cts_py
    PUBLIC
      .
  )
  
  target_link_libraries(cts_py
    PUBLIC
      cts
      dbSta
      OpenSTA
      odb
      rsz
  )

endif()
