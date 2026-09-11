// SPDX-FileCopyrightText: Copyright Contributors to the Kokkos project
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// Manually mirrors Kokkos 5.2.2's generated KokkosCore_config.h for a static
// C++20 build with Serial, CUDA, CUDA_CONSTEXPR, and deprecated APIs enabled.
// DEPRECATED_CODE_4 is required by gpl's View::HostMirror usage. Recompare this
// file with cmake/KokkosCore_config.h.in and a CMake-generated header whenever
// the Kokkos pin in MODULE.bazel changes.

#if !defined(KOKKOS_MACROS_HPP) || defined(KOKKOS_CORE_CONFIG_H)
#error "Do not include KokkosCore_config.h directly; include Kokkos_Macros.hpp instead."
#else
#define KOKKOS_CORE_CONFIG_H
#endif

#define KOKKOS_VERSION 50202
#define KOKKOS_VERSION_MAJOR 5
#define KOKKOS_VERSION_MINOR 2
#define KOKKOS_VERSION_PATCH 2

#define KOKKOS_ENABLE_SERIAL
#define KOKKOS_ENABLE_CUDA
#define KOKKOS_ENABLE_CXX20
#define KOKKOS_ENABLE_CUDA_CONSTEXPR
#define KOKKOS_ENABLE_DEPRECATED_CODE_4
#define KOKKOS_ENABLE_DEPRECATED_CODE_5
#define KOKKOS_ENABLE_DEPRECATION_WARNINGS
#define KOKKOS_ENABLE_COMPLEX_ALIGN
#define KOKKOS_ENABLE_IMPL_MDSPAN
#define KOKKOS_ENABLE_IMPL_REF_COUNT_BRANCH_UNLIKELY
#define KOKKOS_ENABLE_LIBDL

#define KOKKOS_IMPL_DESUL_VERSION "83d26f34db50b4fefe30ed4db38b10e04c7a9384"
#define KOKKOS_IMPL_MDSPAN_VERSION "5d4eb209c77f4744980c0b0c2af44636cc81b08b"

@OPENROAD_KOKKOS_ARCH_DEFINES@
