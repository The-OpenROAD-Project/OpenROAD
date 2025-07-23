###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2023, The Regents of the University of California
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
find_package(Eigen3 REQUIRED)
find_package(OpenMP REQUIRED)

if(NOT (${Kokkos_FOUND} AND ${KokkosFFT_FOUND}))
  add_subdirectory(kokkos-fft)
endif()

swig_lib(NAME      gpl2
         NAMESPACE gpl2
         I_FILE    src/dg_replace.i
         SCRIPTS   src/dg_replace.tcl
)

target_sources(gpl2
  PRIVATE
    src/MakeDgReplace.cpp
    src/dg_replace.cpp
    src/wirelengthOp.cpp
    src/densityOp.cpp
    src/initialPlace.cpp
    src/nesterovPlace.cpp
    src/dct.cpp
    src/poissonSolver.cpp
    src/placerObjects.cpp
    src/placerBase.cpp
    src/routeBase.cpp
    src/timingBase.cpp
  )

messages(TARGET gpl2)

target_include_directories(gpl2
  PUBLIC
    include
)

target_link_libraries(gpl2
  PRIVATE
    utl
    Eigen3::Eigen
    gui
    odb
    OpenSTA
    rsz
    grt
    KokkosFFT::fft
)

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          gpl2_py
           NAMESPACE     gpl2
           LANGUAGE      python
           I_FILE        src/dg_replace-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/include/gpl2
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/gpl2_py.py
  )

  target_include_directories(gpl2_py
    PUBLIC
      include
  )

  target_link_libraries(gpl2_py
    PUBLIC
      utl
      Eigen3::Eigen
      gui
      odb
      OpenSTA
      rsz
      grt
  )

endif()
