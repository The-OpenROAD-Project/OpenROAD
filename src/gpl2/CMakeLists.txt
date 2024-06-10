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
set(CMAKE_CUDA_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --extended-lambda --expt-relaxed-constexpr -Wno-redundant-decls")
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Xcompiler=-Wno-redundant-decls")
set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS}" --expt-relaxed-constexpr)
find_package(Eigen3 REQUIRED)
set(CMAKE_C_COMPILER "g++")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -ggdb -O3 -DIL_STD -ldl")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -g -ggdb -O3 -DIL_STD -ldl")
set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_PTHREADS_INIT 1)
set(CMAKE_CUDA_SEPARABLE_COMPILATION ON)

swig_lib(NAME      gpl2
         NAMESPACE gpl2
         I_FILE    src/gpu_replace.i
         SCRIPTS   src/gpu_replace.tcl
)

target_sources(gpl2
  PRIVATE
    src/MakeGpuReplace.cpp
    src/gpu_replace.cpp
    src/util.cu
    src/wirelengthOp.cu
    src/wirelengthOp.cpp
    src/densityOp.cu
    src/densityOp.cpp
    src/initialPlace.cpp    
    src/nesterovPlace.cpp
    src/cudaDCT.cu
    src/poissonSolver.cpp
    src/poissonSolver.cu
    src/placerObjects.cpp
    src/placerBase.cu
    src/cudaPlacerBase.cu
    src/gpuRouteBase.cu
    src/gpuTimingBase.cu
  )

messages(TARGET gpl2)

set_property(TARGET gpl2 
             PROPERTY CUDA_SEPARABLE_COMPILATION ON)
set_target_properties(gpl2 PROPERTIES CUDA_RESOLVE_DEVICE_SYMBOLS ON)
set_target_properties(gpl2 PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
set_target_properties(gpl2 PROPERTIES POSITION_INDEPENDENT_CODE ON)

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
)

# Set CUDA host compiler and link executable
set(CMAKE_CUDA_HOST_COMPILER /home/tool/gcc/gcc-10.3.0/bin/g++) # replace with your host compiler
set(CMAKE_CUDA_LINK_EXECUTABLE "<CMAKE_CUDA_COMPILER> <FLAGS> <CMAKE_CUDA_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")


message(STATUS "GPU is enabled") 
# find_package(CUDA) should be replaced by "FindCUDAToolkit" for CMAKE verison >= 3.17. 
# See https://cmake.org/cmake/help/latest/module/FindCUDAToolkit.html. 
# find_package(CUDA) or FindCUDA is deprecated since vision 3.10.
find_package(CUDAToolkit)
if (CUDAToolkit_FOUND)
  message(STATUS "CUDA is found")
  enable_language(CUDA)
  target_link_libraries(gpl2
    PRIVATE
      CUDA::cudart
      CUDA::cublas
      CUDA::cusparse
      CUDA::cusolver
      CUDA::cufft
  )  
else()
  message(STATUS "CUDA is not found")
endif()


if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          gpl2_py
           NAMESPACE     gpl2
           LANGUAGE      python
           I_FILE        src/gpu_replace-py.i
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
