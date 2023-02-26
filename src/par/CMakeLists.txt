############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2021, The Regents of the University of California
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
##
############################################################################

include("openroad")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS                 "${CMAKE_CXX_FLAGS} -DIL_STD -m64 -Wall -fPIC")
set(CMAKE_INCLUDE_SYSTEM_FLAG_CXX   "-isystem ")
set(LINKER_OPTIONS                  "-Wl,--export-dynamic")

find_package(Threads REQUIRED)
find_package(ortools REQUIRED)

swig_lib(NAME      par
         NAMESPACE par
         I_FILE    src/partitionmgr.i
         SCRIPTS   src/partitionmgr.tcl
)

target_sources(par
  PRIVATE
    src/MakePartitionMgr.cpp
    src/PartitionMgr.cpp
    src/TritonPart.cpp
    src/TPHypergraph.cpp
    src/Utilities.cpp
    src/TPCoarsener.cpp
    src/TPMultilevel.cpp
    src/TPRefiner.cpp
    src/TPPartitioner.cpp
    src/KPMRefinement.cpp
)

messages(
  TARGET par
)

target_include_directories(par
  PUBLIC
    include
)

target_link_libraries(par
  PUBLIC
    odb
    OpenSTA
    utl
    dbSta
  PRIVATE
    ortools::ortools
    dl
)

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          par_py
           NAMESPACE     par
           LANGUAGE      python
           I_FILE        src/partitionmgr-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/include/par
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/par_py.py
  )

  target_include_directories(par_py
    PUBLIC
      include
  )

  target_link_libraries(par_py
    PUBLIC
      par
  )

endif()
