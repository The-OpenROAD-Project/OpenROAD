# BSD 3-Clause License
#
# Copyright (c) 2023, Google LLC
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
project("dft")

swig_lib(NAME dft
         NAMESPACE dft
         I_FILE src/dft.i
         SCRIPTS src/dft.tcl
)

#################################################################
#
# Source files.
#
################################################################
target_sources(dft
  PRIVATE
  # Keep sorted
    src/Dft.cpp
    src/MakeDft.cpp
)


target_include_directories(dft
  PUBLIC
    include
)

target_link_libraries(dft
  PUBLIC
    odb
    dbSta
    OpenSTA
)

target_link_libraries(dft
  PRIVATE
    dft_cells_lib
    dft_architect_lib
    dft_config_lib
    dft_replace_lib
    dft_clock_domain_lib
    dft_utils_lib
    dft_stitch_lib
)


if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          dft_py
           NAMESPACE     dft
           LANGUAGE      python
           I_FILE        src/DftGen-py.i
           SWIG_INCLUDES ${ODB_HOME}/src/swig/common
                         ${ODB_HOME}/src/swig/python
                         ${ODB_HOME}/include
                         ${PROJECT_SOURCE_DIR}/include
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/dft_py.py
  )

  target_include_directories(dft_py
    PUBLIC
      ../include
  )
  
  target_link_libraries(dft_py
    PUBLIC
      dft
    )

endif()

# Keep sorted
add_subdirectory(src/architect)
add_subdirectory(src/cells)
add_subdirectory(src/clock_domain)
add_subdirectory(src/config)
add_subdirectory(src/replace)
add_subdirectory(src/stitch)
add_subdirectory(src/utils)
if(ENABLE_TESTS)
  add_subdirectory(test)
endif()

messages(
  TARGET dft
)
