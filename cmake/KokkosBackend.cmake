# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

# Kokkos GPU backend wiring for OpenROAD. Included from the root
# CMakeLists.txt only when ENABLE_GPU=ON; not loaded otherwise.
#
# Discovers the user's Kokkos install, inherits its compute backend, turns
# on the matching CMake language so downstream targets can mark kernel
# sources with set_source_files_properties(... LANGUAGE CUDA|HIP), and
# applies the small set of nvcc / fmt / host-compiler workarounds that the
# CUDA backend currently needs in modern Linux toolchains. Per-module
# CMakeLists (e.g. src/gpl) key off ENABLE_GPU and Kokkos_ENABLE_*; they
# do not need to call find_package(Kokkos) or enable_language() themselves.

find_package(Kokkos QUIET)
if(NOT Kokkos_FOUND)
  message(FATAL_ERROR
    "OpenROAD: ENABLE_GPU=ON requires the Kokkos package to be "
    "installed and discoverable by CMake, but Kokkos was not found.\n"
    "  - If Kokkos is already installed: pass "
    "-DKokkos_ROOT=/path/to/kokkos (or extend CMAKE_PREFIX_PATH).\n"
    "  - If not: build and install Kokkos from "
    "https://github.com/kokkos/kokkos with the desired backend "
    "(CUDA / HIP / SYCL / OpenMP) and a target architecture that "
    "matches the host GPU.\n"
    "  - A future etc/DependencyInstaller.sh -gpu option will "
    "automate this step.")
endif()
message(STATUS "OpenROAD: GPU acceleration enabled (Kokkos ${Kokkos_VERSION})")

if(Kokkos_ENABLE_CUDA)
  # Auto-discover nvcc when the user has CUDA installed at a standard
  # location but their environment does not expose it on PATH (common
  # with IDE-launched configures: the bundled CMake does not inherit
  # the shell PATH). enable_language(CUDA) below would otherwise abort
  # with "No CMAKE_CUDA_COMPILER could be found" even though Kokkos's
  # find_package already located the toolkit.
  if(NOT DEFINED CMAKE_CUDA_COMPILER AND NOT DEFINED ENV{CUDACXX})
    find_program(_OPENROAD_NVCC nvcc
      HINTS ENV CUDA_HOME ENV CUDA_PATH ENV CUDA_ROOT
            /usr/local/cuda/bin
            /usr/local/cuda-13.0/bin
            /usr/local/cuda-12.8/bin /usr/local/cuda-12.0/bin
            /opt/cuda/bin
    )
    if(_OPENROAD_NVCC)
      set(CMAKE_CUDA_COMPILER "${_OPENROAD_NVCC}" CACHE FILEPATH "")
      message(STATUS "OpenROAD: auto-discovered nvcc at ${_OPENROAD_NVCC}")
    endif()
  endif()
  # nvcc < 13 cannot parse glibc 2.38+'s _Float128 type that ships with
  # gcc 13+'s C++ standard library headers (math.h template specialization
  # for __iseqsig_type<_Float128>). When a known-broken pairing is detected,
  # pin a compatible older g++ as the CUDA host compiler (the system C++
  # compiler stays unchanged for non-CUDA TUs). Override is always
  # available via -DCMAKE_CUDA_HOST_COMPILER or CUDAHOSTCXX.
  if(NOT DEFINED CMAKE_CUDA_HOST_COMPILER AND NOT DEFINED ENV{CUDAHOSTCXX}
     AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
     AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "13.0"
     AND _OPENROAD_NVCC)
    execute_process(
      COMMAND "${_OPENROAD_NVCC}" --version
      OUTPUT_VARIABLE _OPENROAD_NVCC_VERSION_OUTPUT
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(_OPENROAD_NVCC_VERSION_OUTPUT MATCHES "release ([0-9]+)")
      set(_OPENROAD_NVCC_MAJOR "${CMAKE_MATCH_1}")
      if(_OPENROAD_NVCC_MAJOR LESS 13)
        foreach(_OPENROAD_GXX_VER 12 11)
          find_program(_OPENROAD_CUDAHOST g++-${_OPENROAD_GXX_VER}
            HINTS /usr/bin /usr/local/bin)
          if(_OPENROAD_CUDAHOST)
            set(CMAKE_CUDA_HOST_COMPILER "${_OPENROAD_CUDAHOST}"
              CACHE FILEPATH "")
            message(STATUS
              "OpenROAD: pinning CUDA host compiler to "
              "${_OPENROAD_CUDAHOST} (nvcc ${_OPENROAD_NVCC_MAJOR}.x + "
              "glibc/gcc 13+ _Float128 compat)")
            break()
          endif()
          unset(_OPENROAD_CUDAHOST CACHE)
        endforeach()
        if(NOT DEFINED CMAKE_CUDA_HOST_COMPILER)
          message(FATAL_ERROR
            "OpenROAD: nvcc ${_OPENROAD_NVCC_MAJOR}.x cannot parse "
            "_Float128 declarations in glibc 2.38+ system headers used "
            "by gcc ${CMAKE_CXX_COMPILER_VERSION}, and no compatible "
            "g++-12 / g++-11 was found in /usr/bin or /usr/local/bin. "
            "Install one (e.g. apt install g++-12) or set "
            "-DCMAKE_CUDA_HOST_COMPILER=/path/to/older-g++ explicitly.")
        endif()
      endif()
    endif()
  endif()
  if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES OR "${CMAKE_CUDA_ARCHITECTURES}" STREQUAL "")
    if(DEFINED Kokkos_CUDA_ARCHITECTURES
       AND NOT "${Kokkos_CUDA_ARCHITECTURES}" STREQUAL "")
      set(CMAKE_CUDA_ARCHITECTURES "${Kokkos_CUDA_ARCHITECTURES}")
    else()
      message(FATAL_ERROR
        "OpenROAD: ENABLE_GPU=ON with Kokkos CUDA backend, but the "
        "Kokkos package does not advertise Kokkos_CUDA_ARCHITECTURES "
        "and CMAKE_CUDA_ARCHITECTURES was not provided. Set "
        "-DCMAKE_CUDA_ARCHITECTURES=<arch> explicitly (e.g. 89 for "
        "RTX 4070, 120 for RTX 5090) or rebuild Kokkos with the "
        "target architecture baked in.")
    endif()
  endif()
  enable_language(CUDA)
  find_package(CUDAToolkit REQUIRED)
  message(STATUS "OpenROAD: CUDA backend (arch=${CMAKE_CUDA_ARCHITECTURES})")
  # nvcc 12.8 cannot parse fmt 11's nontype-template-parameter user-defined
  # literals (fmt/bundled/format.h: operator""_a with fixed_string). The
  # legacy literal fallback is still available; opt into it for CUDA TUs
  # only. Project-wide CXX compilation is unaffected.
  add_compile_definitions(
    $<$<COMPILE_LANGUAGE:CUDA>:FMT_USE_NONTYPE_TEMPLATE_ARGS=0>)
elseif(Kokkos_ENABLE_HIP)
  enable_language(HIP)
  message(STATUS "OpenROAD: HIP backend")
elseif(Kokkos_ENABLE_SYCL)
  message(STATUS "OpenROAD: SYCL backend (driven by Kokkos host compiler)")
else()
  message(STATUS
          "OpenROAD: host-only Kokkos backend (Serial / OpenMP / Threads)")
endif()
