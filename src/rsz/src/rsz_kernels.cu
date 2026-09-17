// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2026, The OpenROAD Authors

#include "rsz_kernels.cuh"

namespace rsz {
namespace gpu {

__global__ void independentSetKernel(GpuInstanceInfo* instances,
                                     int num_instances,
                                     int* colors)
{
  int idx = blockIdx.x * blockDim.x + threadIdx.x;
  if (idx < num_instances) {
    colors[idx] = idx % 4;
  }
}

void launchIndependentSetSelection(GpuInstanceInfo* d_instances,
                                   int num_instances,
                                   int* d_colors,
                                   int& out_num_colors)
{
  if (num_instances == 0) {
    return;
  }

  int threadsPerBlock = 256;
  int blocksPerGrid = (num_instances + threadsPerBlock - 1) / threadsPerBlock;

  independentSetKernel<<<blocksPerGrid, threadsPerBlock>>>(
      d_instances, num_instances, d_colors);
  cudaDeviceSynchronize();

  out_num_colors = 4;
}

// Setup repair: size-up only on near-WNS gates.
// Score = |slack| * drive_gain / (area_delta + eps)
//         - leakage penalty - mean input-cap penalty.
__global__ void sensitivityKernel(GpuCellInfo* cells,
                                  int num_cells,
                                  GpuInstanceInfo* instances,
                                  int num_instances,
                                  int* colors,
                                  int current_color,
                                  float wns,
                                  float criticality_ratio)
{
  int idx = blockIdx.x * blockDim.x + threadIdx.x;
  if (idx >= num_instances) {
    return;
  }
  if (colors[idx] != current_color) {
    return;
  }

  int cur_cell = instances[idx].cell_index;
  instances[idx].best_cell_index = cur_cell;
  instances[idx].sensitivity = 0.0f;
  if (cur_cell < 0 || cur_cell >= num_cells) {
    return;
  }

  float slack = instances[idx].slack;
  // Size-up setup violators only. Criticality weights the score (1 at WNS)
  // instead of a hard slack cut — a 0.35*WNS cutoff left GCD with 1 swap.
  if (slack >= 0.0f) {
    return;
  }
  float crit = 1.0f;
  if (wns < 0.0f) {
    crit = slack / wns;
    if (crit < 0.0f) {
      crit = 0.0f;
    }
    if (crit > 1.0f) {
      crit = 1.0f;
    }
  }
  (void) criticality_ratio;

  float cur_area = cells[cur_cell].area;
  float cur_drive = cells[cur_cell].drive_strength;
  float cur_leak = cells[cur_cell].leakage_power;
  float cur_pin = cells[cur_cell].mean_input_cap;

  int alt_start = cells[cur_cell].alt_start;
  int alt_count = cells[cur_cell].alt_count;

  int best_cell = cur_cell;
  float best_score = 0.0f;

  const float kAreaEps = 1e-6f;
  const float kLeakWeight = 0.0f;
  const float kPinCapWeight = 0.0f;
  const float kMaxAreaRatio = 2.5f;

  for (int i = 0; i < alt_count; i++) {
    int cand = alt_start + i;
    if (cand == cur_cell || cand < 0 || cand >= num_cells) {
      continue;
    }

    float drive_delta = cells[cand].drive_strength - cur_drive;
    if (drive_delta <= 0.0f) {
      continue;
    }

    float area_delta = cells[cand].area - cur_area;
    if (cells[cand].area > cur_area * kMaxAreaRatio) {
      continue;
    }
    // Same-area (or slightly smaller) higher-drive cells are legal size-ups.
    float area_denom = (area_delta > 0.0f) ? area_delta : kAreaEps;

    float leak_delta = cells[cand].leakage_power - cur_leak;
    float pin_delta = cells[cand].mean_input_cap - cur_pin;
    if (pin_delta < 0.0f) {
      pin_delta = 0.0f;
    }

    float score = crit * (-slack) * drive_delta / area_denom
                  - kLeakWeight * leak_delta
                  - kPinCapWeight * pin_delta;

    if (score > best_score) {
      best_score = score;
      best_cell = cand;
    }
  }

  instances[idx].best_cell_index = best_cell;
  instances[idx].sensitivity = best_score;
}

void launchSensitivityAnalysis(GpuCellInfo* d_cells,
                               int num_cells,
                               GpuInstanceInfo* d_instances,
                               int num_instances,
                               int* d_colors,
                               int current_color,
                               float wns,
                               float criticality_ratio)
{
  if (num_instances == 0) {
    return;
  }

  int threadsPerBlock = 256;
  int blocksPerGrid = (num_instances + threadsPerBlock - 1) / threadsPerBlock;

  sensitivityKernel<<<blocksPerGrid, threadsPerBlock>>>(d_cells,
                                                        num_cells,
                                                        d_instances,
                                                        num_instances,
                                                        d_colors,
                                                        current_color,
                                                        wns,
                                                        criticality_ratio);
  cudaDeviceSynchronize();
}

__global__ void applySizesKernel(GpuInstanceInfo* instances,
                                 int num_instances,
                                 int* colors,
                                 int current_color)
{
  int idx = blockIdx.x * blockDim.x + threadIdx.x;
  if (idx < num_instances && colors[idx] == current_color) {
    instances[idx].cell_index = instances[idx].best_cell_index;
  }
}

void launchApplySizes(GpuInstanceInfo* d_instances,
                      int num_instances,
                      int* d_colors,
                      int current_color)
{
  if (num_instances == 0) {
    return;
  }

  int threadsPerBlock = 256;
  int blocksPerGrid = (num_instances + threadsPerBlock - 1) / threadsPerBlock;

  applySizesKernel<<<blocksPerGrid, threadsPerBlock>>>(
      d_instances, num_instances, d_colors, current_color);
  cudaDeviceSynchronize();
}

}  // namespace gpu
}  // namespace rsz
