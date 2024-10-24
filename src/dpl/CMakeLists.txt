###########################################################################
## Original authors: SangGi Do(sanggido@unist.ac.kr), Mingyu Woo(mwoo@eng.ucsd.edu)
##          (respective Ph.D. advisors: Seokhyeong Kang, Andrew B. Kahng)
## Rewrite by James Cherry, Parallax Software, Inc.
##
## BSD 3-Clause License
##
## Copyright (c) 2019, James Cherry, Parallax Software, Inc.
## Copyright (c) 2018, SangGi Do and Mingyu Woo
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
###########################################################################

include("openroad")
find_package(TCL)
find_package(Boost)

add_library(dpl_lib
  src/Opendp.cpp
  src/dbToOpendp.cpp
  src/Grid.cpp
  src/CheckPlacement.cpp
  src/Objects.cpp
  src/Padding.cpp
  src/Place.cpp
  src/FillerPlacement.cpp
  src/DecapPlacement.cpp
  src/OptMirror.cpp
  src/AbacusLegalizer.cpp
)

target_link_libraries(dpl_lib
  PUBLIC
    odb
    OpenSTA
  PRIVATE
    utl_lib
)


target_include_directories(dpl_lib
  PUBLIC
    include
)

swig_lib(NAME         dpl
         NAMESPACE    dpl
         I_FILE       src/Opendp.i
         SCRIPTS      src/Opendp.tcl
)

target_sources(dpl
  PRIVATE
    src/MakeOpendp.cpp
    src/Graphics.cpp
    src/DplObserver.h
)

target_include_directories(dpl
  PUBLIC
    include
  PRIVATE
    src
)

target_link_libraries(dpl
  PRIVATE
    ${TCL_LIBRARY}
    odb
    dpl_lib
    OpenSTA
    gui
)

messages(
  TARGET dpl
)

################################################################

add_custom_target(opendp_tags etags -o TAGS
  ${OPENDP_SRC}
  ${OPENDP_HOME}/include/dpl/*.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          dpl_py
           NAMESPACE     dpl
           LANGUAGE      python
           I_FILE        src/Opendp-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/include/dpl
                         ${ODB_HOME}/src/swig/common
                         ${ODB_HOME}/src/swig/python
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/dpl_py.py
  )

  target_link_libraries(dpl_py
    PUBLIC
      ${Boost_LIBRARIES}
      dpl_lib
      gui
      odb
  )

endif()

if(ENABLE_TESTS)
  add_subdirectory(test)
endif()
