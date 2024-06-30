###########################################################################
## BSD 3-Clause License
##
## Copyright (c) 2021, Andrew Kennings
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

swig_lib(NAME         dpo
         NAMESPACE    dpo
         I_FILE       src/Optdp.i
         SCRIPTS      src/Optdp.tcl
)

# https://github.com/The-OpenROAD-Project/OpenROAD/issues/1186
find_package(LEMON NAMES LEMON lemon REQUIRED)

target_sources(dpo
  PRIVATE
    src/Optdp.cpp
    src/MakeOptdp.cpp
    src/architecture.cxx
    src/color.cxx
    src/network.cxx
    src/router.cxx
    src/utility.cxx
    src/legalize_shift.cxx
    src/detailed.cxx
    src/detailed_manager.cxx
    src/detailed_mis.cxx
    src/detailed_reorder.cxx
    src/detailed_global.cxx
    src/detailed_vertical.cxx
    src/detailed_displacement.cxx
    src/detailed_hpwl.cxx
    src/detailed_abu.cxx
    src/detailed_random.cxx
    src/detailed_orient.cxx
)

target_include_directories(dpo
  PUBLIC
    include
    ${LEMON_INCLUDE_DIRS}
)

target_link_libraries(dpo
  PRIVATE
    odb
    OpenSTA
    utl
    dpl_lib
)

messages(
  TARGET dpo
)

################################################################

add_custom_target(optdp_tags etags -o TAGS
  ${OPTDP_SRC}
  ${OPTDP_HOME}/include/dpo/*.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          dpo_py
           NAMESPACE     dpo
           LANGUAGE      python
           I_FILE        src/Optdp-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/include/dpo
                         ${ODB_HOME}/src/swig/common
                         ${ODB_HOME}/src/swig/python
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/dpo_py.py
  )

  target_include_directories(dpo_py
    PUBLIC
      include
      include/dpo
  )

  target_link_libraries(dpo_py
    PUBLIC
      dpo
      odb
  )

endif()

add_subdirectory(test)
