###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2021, University of California, San Diego.
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

option(USE_CIMG_LIB "Use CImg drawing library for image drawing" OFF)

# For Multithread
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_FFT2D_PTHREADS=1 -DFFT_2D_MAX_THREADS=16 -O3 ${OpenMP_CXX_FLAGS}")

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Eigen3 REQUIRED)

swig_lib(NAME      replace
         NAMESPACE gpl
         I_FILE    src/replace.i
         SCRIPTS   src/replace.tcl
)

target_sources(replace
  PRIVATE
    src/MakeReplace.cpp
    src/replace.cpp
    src/initialPlace.cpp
    src/nesterovPlace.cpp
    src/placerBase.cpp
    src/nesterovBase.cpp
    src/fft.cpp
    src/fftsg.cpp
    src/fftsg2d.cpp
    src/point.cpp
    src/plot.cpp
    src/routeBase.cpp
    src/timingBase.cpp
    src/graphics.cpp
)

messages(TARGET replace)

target_include_directories(replace
  PUBLIC
    include
)

target_link_libraries(replace
  PRIVATE
    utl
    Eigen3::Eigen
    gui
    opendb
    OpenSTA
    sst
    rsz
    grt
)

# CIMG_USE case
if( USE_CIMG_LIB )
  find_package(X11 REQUIRED)
  find_package(JPEG REQUIRED)
  if ( X11_FOUND AND JPEG_FOUND )
    target_link_libraries( replace
      PRIVATE
        ${X11_LIBRARIES}
        ${JPEG_LIBRARIES}
    )
    target_include_directories( replace
      PUBLIC
        ${X11_INCLUDE_DIR}
        ${JPEG_INCLUDE_DIR}
    )
    target_compile_definitions( replace
      PRIVATE
        ENABLE_CIMG_LIB
        cimg_use_jpeg=1
    )
  endif()
endif()
