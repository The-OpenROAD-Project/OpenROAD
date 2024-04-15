################################################################################
## BSD 3-Clause License
##
## Copyright (c) 2020, The Regents of the University of California
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
################################################################################

include("openroad")

add_library(rcx_lib
  ext.cpp
  extBench.cpp
  extBenchDB.cpp
  extCC.cpp
  extCoords.cpp
  extFlow.cpp
  extRCmodel.cpp
  extSpef.cpp
  extSpefIn.cpp
  extmain.cpp
  extmeasure.cpp
  extDebugPrint.cpp
  extstats.cpp
  extprocess.cpp
  netRC.cpp
  extmeasure_res.cpp
  name.cpp
  grids.cpp
  gs.cpp
  dbUtil.cpp
)

target_include_directories(rcx_lib
  PUBLIC
    ../include
)

target_link_libraries(rcx_lib
  PUBLIC
    odb
    utl
)

swig_lib(NAME      rcx
         NAMESPACE rcx
         I_FILE    ext.i
         SCRIPTS   OpenRCX.tcl
)

target_sources(rcx
  PRIVATE
    MakeOpenRCX.cpp
)

target_include_directories(rcx
  PUBLIC
    ../include
)

target_link_libraries(rcx
  PUBLIC
    rcx_lib
    utl
  PRIVATE
    OpenSTA
)

messages(
  TARGET rcx
  OUTPUT_DIR ..
)

############################################################
# Unit testing
############################################################
if(ENABLE_TESTS)
  enable_testing()

  add_executable(rcxUnitTest
    ${PROJECT_SOURCE_DIR}/test/ext2dBoxTest.cpp
  )

  target_include_directories(rcxUnitTest
    PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${OPENROAD_HOME}/include
  )

  target_link_libraries(rcxUnitTest
    rcx_lib
  )

  # Use the shared library if found.  We need to pass this info to
  # the code to select the corresponding include.  Using the shared
  # library speeds up compilation.
  if (Boost_unit_test_framework_FOUND)
    message(STATUS "Boost unit_test_framework library found")
    target_link_libraries(rcxUnitTest
      Boost::unit_test_framework
    )
    target_compile_definitions(rcxUnitTest
      PRIVATE
      HAS_BOOST_UNIT_TEST_LIBRARY
    )
  endif()

  add_test(NAME rcxUnitTest COMMAND rcxUnitTest)
  add_dependencies(build_and_test rcxUnitTest)
endif()

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          rcx_py
           NAMESPACE     rcx
           LANGUAGE      python
           I_FILE        ext-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/../include
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/rcx_py.py
  )

  target_include_directories(rcx_py
    PUBLIC
      ../include
  )

  target_link_libraries(rcx_py
    PUBLIC
      rcx_lib
  )

endif()
