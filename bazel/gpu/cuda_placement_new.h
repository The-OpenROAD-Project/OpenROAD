// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Host+device placement new/delete for the CUDA translation units, force
// included ahead of every other header (see -include in bazel/gpu/copts.bzl).
//
// libc++ defines the placement forms as plain inline functions in
// <__new/placement_new_delete.h>. In clang's CUDA mode those are __host__
// only, so any __host__ __device__ function that placement-news -- which
// Kokkos does in ViewValueFunctor (Kokkos_ViewAlloc.hpp) and in
// FunctorAnalysis's reducer init (Kokkos_FunctorAnalysis.hpp) -- fails with
//
//     error: reference to __host__ function 'operator new'
//            in __host__ __device__ function
//
// clang's own fix, cuda_wrappers/new, adds __device__ placement overloads
// whenever <new> is included, and that is enough for libstdc++. libc++'s
// granular headers (__memory/construct_at.h and friends) include
// <__new/placement_new_delete.h> directly and never pass through that
// wrapper, so std::construct_at & co. only ever see the host-only forms.
//
// Defining libc++'s include guard and supplying the operators here is the
// narrowest fix available: the declarations libc++ needs internally (e.g.
// std::__construct_at) stay visible, and only their host/device attributes
// change. Suppressing the header without replacing it breaks <string>.
//
// The guard name and the <__cstddef/size_t.h> include are libc++
// implementation details, so this shim is pinned to the libc++ major that
// the llvm module in MODULE.bazel ships. On a toolchain bump, compare the
// new <__new/placement_new_delete.h> with the four operators below (and
// the guard name), then widen the version range.
//
// Drop this file once libc++ annotates the placement forms for CUDA.

#include <__config>

#if !defined(_LIBCPP_VERSION) || _LIBCPP_VERSION < 220000 \
    || _LIBCPP_VERSION >= 230000
#error \
    "bazel/gpu/cuda_placement_new.h was verified against libc++ 22 only. Re-check it against this libc++'s <__new/placement_new_delete.h> and widen the version range."
#endif

#ifndef _LIBCPP___NEW_PLACEMENT_NEW_DELETE_H
#define _LIBCPP___NEW_PLACEMENT_NEW_DELETE_H

#include <__cstddef/size_t.h>

// Spelled as attributes rather than the __host__/__device__ macros: this
// header is force included before CUDA's host_defines.h has run.
#define OPENROAD_CUDA_HD __attribute__((host)) __attribute__((device))

[[nodiscard]] inline OPENROAD_CUDA_HD void* operator new(std::size_t,
                                                         void* p) noexcept
{
  return p;
}

[[nodiscard]] inline OPENROAD_CUDA_HD void* operator new[](std::size_t,
                                                           void* p) noexcept
{
  return p;
}

inline OPENROAD_CUDA_HD void operator delete(void*, void*) noexcept
{
}

inline OPENROAD_CUDA_HD void operator delete[](void*, void*) noexcept
{
}

#undef OPENROAD_CUDA_HD

#endif  // _LIBCPP___NEW_PLACEMENT_NEW_DELETE_H
