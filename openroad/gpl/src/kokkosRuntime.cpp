// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "kokkosRuntime.h"

#include <Kokkos_Core.hpp>
#include <algorithm>
#include <cstdlib>
#include <mutex>

#ifdef KOKKOS_ENABLE_CUDA
#include <cuda_runtime.h>
#endif

#include "utl/Logger.h"

namespace gpl {

namespace {

// Kokkos::initialize starts every compiled backend, so a CUDA-enabled Kokkos
// needs a device even when gpl's GPU path is off (ENABLE_GPU=0). Check up
// front so a missing device is an OpenROAD error, not a Kokkos abort.
void checkKokkosDevice(utl::Logger* logger)
{
#ifdef KOKKOS_ENABLE_CUDA
  if (Kokkos::is_initialized()) {
    return;
  }
  int count = 0;
  const cudaError_t err = cudaGetDeviceCount(&count);
  if (err != cudaSuccess || count == 0) {
    logger->error(
        utl::GPL,
        336,
        "OpenROAD was built with GPU support (--config=gpu) and "
        "needs a usable CUDA device, even with ENABLE_GPU=0: {}.",
        err != cudaSuccess ? cudaGetErrorString(err) : "no CUDA device found");
  }
#else
  (void) logger;
#endif
}

}  // namespace

void ensureKokkosInitialized(std::optional<int> num_threads)
{
  static std::once_flag once;
  std::call_once(once, [=] {
    if (Kokkos::is_initialized()) {
      return;
    }
    Kokkos::InitializationSettings settings;
    // Keep Kokkos warnings enabled on the GPU build. In particular, Kokkos
    // reports when the compiled CUDA architecture differs from the selected
    // device. Compatible architectures can still run, but the mismatch may
    // reduce performance.
#ifndef ENABLE_GPU
    settings.set_disable_warnings(true);
#endif
    if (num_threads) {
      settings.set_num_threads(*num_threads);
    }
    Kokkos::initialize(settings);
    std::atexit([] {
      if (Kokkos::is_initialized() && !Kokkos::is_finalized()) {
        Kokkos::finalize();
      }
    });
  });
}

Kokkos::DefaultHostExecutionSpace hostExecutionSpace(utl::Logger* logger,
                                                     int num_threads)
{
  checkKokkosDevice(logger);
#ifdef KOKKOS_ENABLE_OPENMP
  num_threads = std::clamp(
      num_threads, 1, Kokkos::Impl::OpenMPTraits::MAX_THREAD_COUNT);
#endif
#ifdef KOKKOS_ENABLE_THREADS
  num_threads = std::clamp(
      num_threads, 1, Kokkos::Impl::ThreadsInternal::MAX_THREAD_COUNT);
#endif
  ensureKokkosInitialized(num_threads);
#ifdef KOKKOS_ENABLE_OPENMP
  return Kokkos::DefaultHostExecutionSpace(num_threads);
#else
  return Kokkos::DefaultHostExecutionSpace();
#endif
}

}  // namespace gpl
