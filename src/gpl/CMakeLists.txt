# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021-2025, The OpenROAD Authors

include("openroad")

# For Multithread
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_FFT2D_PTHREADS=1 -DFFT_2D_MAX_THREADS=16 -O3 ${OpenMP_CXX_FLAGS}")

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Eigen3 REQUIRED)
find_package(ortools REQUIRED)
find_package(LEMON NAMES LEMON lemon REQUIRED)
find_package(OpenMP REQUIRED)

message(STATUS "Found OR-Tools: ${ortools_DIR} (version: ${ortools_VERSION})")

swig_lib(NAME      gpl
         NAMESPACE gpl
         I_FILE    src/replace.i
         SWIG_INCLUDES ${ODB_HOME}/src/swig/common
                       ${ODB_HOME}/src/swig/tcl
                       ${ODB_HOME}/include
         SCRIPTS   src/replace.tcl
)

add_library(gpl_lib
  src/AbstractGraphics.cpp
  src/replace.cpp
  src/initialPlace.cpp
  src/nesterovPlace.cpp
  src/placerBase.cpp
  src/nesterovBase.cpp
  src/fft.cpp
  src/fftsg.cpp
  src/fftsg2d.cpp
  src/hpwl.cpp
  src/routeBase.cpp
  src/timingBase.cpp
  src/graphicsNone.cpp
  src/solver.cpp
  src/mbff.cpp
)

# --- HPWL backend: runtime switch ---
# src/hpwl.cpp holds the single NesterovBaseCommon::getHpwl() definition and
# is always compiled. When ENABLE_GPU=ON, the Kokkos kernel in src/gpu/hpwl.cpp
# is also compiled in and getHpwl() dispatches between the CPU loop and the GPU
# kernel at run time (gpl::gpuEnabled(), driven by the ENABLE_GPU env var).
# ENABLE_GPU is a compile definition gating the #ifdef blocks in gpl_lib's .cpp
# files; the consumer-facing headers (nesterovBase.h) stay preprocessor-free.
# gpu/ is a file-layout subdirectory only (no nested CMakeLists.txt) so kernel
# build settings stay in this module's CMakeLists with the rest of gpl_lib.
if(ENABLE_GPU)
  target_sources(gpl_lib PRIVATE src/gpu/hpwl.cpp src/gpu/gpuBackend.cpp)
  target_compile_definitions(gpl_lib PRIVATE ENABLE_GPU)
  # nesterovBase.h and other private gpl headers live in src/; sources
  # under src/gpu/ need that on the include path explicitly because
  # the compiler's default same-dir lookup points into src/gpu/ instead.
  target_include_directories(gpl_lib PRIVATE src)
  # gpu/hpwl.cpp is the device kernel TU. gpu/gpuBackend.cpp carries no device
  # code itself, but it includes <Kokkos_Core.hpp> for the lazy Kokkos
  # initialize()/finalize(): when Kokkos is built with the CUDA (or HIP)
  # backend, that header bakes KOKKOS_ENABLE_CUDA into its config and refuses
  # to compile under a plain host compiler (it requires __CUDACC__). So both
  # TUs are flagged with the device language to match the Kokkos backend.
  if(Kokkos_ENABLE_CUDA)
    set_source_files_properties(src/gpu/hpwl.cpp src/gpu/gpuBackend.cpp
                                PROPERTIES LANGUAGE CUDA)
  elseif(Kokkos_ENABLE_HIP)
    set_source_files_properties(src/gpu/hpwl.cpp src/gpu/gpuBackend.cpp
                                PROPERTIES LANGUAGE HIP)
  endif()
  # Disable FP contraction for kernels that share gpl_lib's compile
  # context so they stay bit-stable across compilers. Scoped to gpl_lib
  # but the CXX flag is also harmless on the existing CPU TUs.
  target_compile_options(gpl_lib PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-ffp-contract=off>
    $<$<COMPILE_LANGUAGE:CUDA>:--fmad=false>
    $<$<COMPILE_LANGUAGE:HIP>:-ffp-contract=off>
  )
  target_link_libraries(gpl_lib Kokkos::kokkos)
  if(Kokkos_ENABLE_CUDA)
    # cuda runtime symbols are referenced from the CUDA TU; expose cudart
    # so that gpl_lib (and the openroad binary) link against libcudart.
    target_link_libraries(gpl_lib CUDA::cudart)
  endif()
endif()

target_sources(gpl
  PRIVATE
    src/MakeReplace.cpp
    src/graphicsImpl.cpp
)

messages(TARGET gpl)
messages(TARGET gpl_lib)

target_include_directories(gpl
  PUBLIC
    include
)

target_include_directories(gpl_lib
  PUBLIC
    include
    ${LEMON_INCLUDE_DIRS}
)

target_link_libraries(gpl_lib
    utl_lib
    Eigen3::Eigen
    odb
    OpenSTA
    rsz_lib
    grt_lib
    ortools::ortools
    Threads::Threads
    OpenMP::OpenMP_CXX
)

target_link_libraries(gpl
  PRIVATE
    gpl_lib
    utl_lib
    Eigen3::Eigen
    gui
    odb
    OpenSTA
    rsz
    grt
    ortools::ortools
    Threads::Threads
    OpenMP::OpenMP_CXX
)

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
      utl_lib
      Eigen3::Eigen
      gui
      odb
      OpenSTA
      rsz
      grt
  )

endif()

if(ENABLE_TESTS)
  add_subdirectory(test)
endif()
