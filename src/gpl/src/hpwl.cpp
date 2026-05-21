// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// HPWL (half-perimeter wirelength) — the single getHpwl() definition.
//
// This translation unit is always compiled. It holds the OpenMP CPU loop and,
// in an ENABLE_GPU build, the runtime dispatch to the Kokkos GPU kernel
// (gpu/hpwl.cpp). gpl::gpuEnabled() picks the backend per process at run time;
// the default in an ENABLE_GPU build is the GPU path, with ENABLE_GPU=0 in the
// environment forcing the CPU path. nesterovBase.h carries no preprocessor
// branches — the GPU path is a free function (gpl::getHpwlGpu).

#include <cassert>
#include <cstdint>

#include "nesterovBase.h"
#include "omp.h"

#ifdef ENABLE_GPU
#include "gpu/gpuBackend.h"
#include "gpu/hpwlGpu.h"
#endif

namespace gpl {

int64_t NesterovBaseCommon::getHpwl()
{
#ifdef ENABLE_GPU
  if (gpuEnabled()) {
    return getHpwlGpu(gNetStor_);
  }
#endif
  assert(omp_get_thread_num() == 0);
  int64_t hpwl = 0;
#pragma omp parallel for num_threads(num_threads_) reduction(+ : hpwl)
  for (auto gNet = gNetStor_.begin(); gNet < gNetStor_.end(); ++gNet) {
    // old-style loop for old OpenMP
    gNet->updateBox();
    hpwl += gNet->getHpwl();
  }
  return hpwl;
}

}  // namespace gpl
