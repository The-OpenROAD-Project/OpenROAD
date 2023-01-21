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

# For Multithread
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_FFT2D_PTHREADS=1 -DFFT_2D_MAX_THREADS=16 -O3 ${OpenMP_CXX_FLAGS}")

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Eigen3 REQUIRED)

swig_lib(NAME      gpl
         NAMESPACE gpl
         I_FILE    src/replace.i
         SCRIPTS   src/replace.tcl
)

target_sources(gpl
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
    src/routeBase.cpp
    src/timingBase.cpp
    src/graphics.cpp
    src/solver.cpp
    src/options.cpp
)

messages(TARGET gpl)

target_include_directories(gpl
  PUBLIC
    include
)

target_link_libraries(gpl
  PRIVATE
    utl
    Eigen3::Eigen
    gui
    odb
    OpenSTA
    rsz
    grt
)

# Allow users to use GPU or not
option(GPU "Enable GPU" OFF)
if (GPU)
  message(STATUS "GPU is enabled")
  # find_package(CUDA) should be replaced by "FindCUDAToolkit" for CMAKE verison >= 3.17. 
  # See https://cmake.org/cmake/help/latest/module/FindCUDAToolkit.html. 
  # find_package(CUDA) or FindCUDA is deprecated since vision 3.10.
  find_package(CUDA QUIET COMPONENTS Core Widgets)
  if (CUDA_FOUND)
    message(STATUS "CUDA is found")
    enable_language(CUDA)
    target_sources(gpl PRIVATE src/gpuSolver.cu)
    target_link_libraries(gpl
      PRIVATE
        ${CUDA_LIBRARIES}
        ${CUDA_CUBLAS_LIBRARIES}
        ${CUDA_cusparse_LIBRARY}
        ${CUDA_cusolver_LIBRARY}
    )
    target_compile_definitions( gpl
    PRIVATE
      ENABLE_GPU
    )
  else()
    message(STATUS "CUDA is not found")
  endif()
else()
  message(STATUS "GPU is not enabled")
endif()

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          gpl_py
           NAMESPACE     gpl
           LANGUAGE      python
           I_FILE        src/replace-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/include/gpl
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/gpl_py.py
  )

  target_include_directories(gpl_py
    PUBLIC
      include
  )

  target_link_libraries(gpl_py
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
