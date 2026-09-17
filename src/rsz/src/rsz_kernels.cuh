// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2026, The OpenROAD Authors

#pragma once

#include <cuda_runtime.h>

#include "rsz_gpu_db.cuh"

namespace rsz {
namespace gpu {

void launchIndependentSetSelection(GpuInstanceInfo* d_instances,
                                   int num_instances,
                                   int* d_colors,
                                   int& out_num_colors);

// Setup-repair sensitivity (size-up only):
//   slack < 0 and slack <= criticality_ratio * WNS → size UP
//   size-down is disabled (it caused thousands of non-repair swaps)
void launchSensitivityAnalysis(GpuCellInfo* d_cells,
                               int num_cells,
                               GpuInstanceInfo* d_instances,
                               int num_instances,
                               int* d_colors,
                               int current_color,
                               float wns,
                               float criticality_ratio);

void launchApplySizes(GpuInstanceInfo* d_instances,
                      int num_instances,
                      int* d_colors,
                      int current_color);

}  // namespace gpu
}  // namespace rsz
