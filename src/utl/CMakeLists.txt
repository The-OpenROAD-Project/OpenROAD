###############################################################################
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

find_package(Boost 1.78 REQUIRED)

swig_lib(NAME      utl
         NAMESPACE utl
         I_FILE    src/Logger.i
         SCRIPTS   src/Utl.tcl
         SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/src
)

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          utl_py
           NAMESPACE     utl
           LANGUAGE      python
           I_FILE        src/Logger-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/include/utl
                         ${PROJECT_SOURCE_DIR}/src
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/utl_py.py
  )

  target_include_directories(utl_py
    PUBLIC
      include
    PRIVATE
      src
  )
  
  target_link_libraries(utl_py
    PUBLIC
      spdlog::spdlog
  )
endif()

add_library(utl_lib
  src/Metrics.cpp
  src/CFileUtils.cpp
  src/ScopedTemporaryFile.cpp
  src/Logger.cpp
  src/timer.cpp
)

target_include_directories(utl_lib
  PUBLIC
    include
  PRIVATE
    src
    ${Boost_INCLUDE_DIRS}
)

target_link_libraries(utl_lib
  PUBLIC
    spdlog::spdlog
)

target_sources(utl
  PRIVATE
    src/LoggerCommon.cpp
    src/MakeLogger.cpp
)
  
target_include_directories(utl
  PUBLIC
    include
  PRIVATE
    src
    ${Boost_INCLUDE_DIRS}
)

target_compile_definitions(utl
  PUBLIC
    FMT_DEPRECATED_OSTREAM=1
)

target_link_libraries(utl
  PUBLIC
    utl_lib
  PRIVATE
    OpenSTA
)

messages(
  TARGET utl
)

############################################################
# Unit testing
############################################################\
if(ENABLE_TESTS)
  add_subdirectory(test)
endif()
