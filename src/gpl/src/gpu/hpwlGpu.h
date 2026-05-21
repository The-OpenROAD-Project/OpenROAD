// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// HPWL GPU backend entry point.
//
// Declares the free function that the CPU dispatch site in ../hpwl.cpp calls
// when the GPU backend is selected at runtime. The definition lives in
// gpu/hpwl.cpp (a CUDA/HIP translation unit, compiled only when
// ENABLE_GPU=ON). nesterovBase.h is included for the GNet type; routing the
// GPU path through a free function keeps nesterovBase.h preprocessor-free.

#pragma once

#include <cstdint>
#include <vector>

#include "nesterovBase.h"

namespace gpl {

// Computes the total HPWL over gNetStor and writes each net's bounding box
// back via GNet::setBox, exactly as the CPU loop does. Bit-identical to the
// CPU result (integer arithmetic).
int64_t getHpwlGpu(std::vector<GNet>& gNetStor);

}  // namespace gpl
