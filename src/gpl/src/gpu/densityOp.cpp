// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Density gradient gather — Kokkos kernel.
//
// K_density_gather: per-inst, find overlapping bins via density half-sizes,
// compute clipped rectangle overlap area, accumulate overlap × E_field ×
// density_scale. The solver→gpl axis swap + 0.5× field scale come from the
// shared adapter in poissonSolver.h (same constant used by the host unpack
// in GpuFftBackend::solve).

#include "densityOp.h"

#include <Kokkos_Core.hpp>
#include <algorithm>

#include "deviceState_kokkos.h"
#include "poissonSolver.h"

namespace gpl {
namespace densop {

namespace {
using ExecSpace = Kokkos::DefaultExecutionSpace;
}  // namespace

void launchDensityGather(KokkosDeviceState& ds,
                         int n_insts,
                         int bin_cnt_x,
                         int bin_cnt_y,
                         float bin_size_x,
                         float bin_size_y,
                         int grid_lx,
                         int grid_ly)
{
  if (n_insts == 0) {
    return;
  }

  auto d_inst_cx = ds.d_inst_cx;
  auto d_inst_cy = ds.d_inst_cy;
  auto d_inst_density_half_dx = ds.d_inst_density_half_dx;
  auto d_inst_density_half_dy = ds.d_inst_density_half_dy;
  auto d_inst_density_scale = ds.d_inst_density_scale;
  auto d_bin_elec_x = ds.d_bin_elec_x;
  auto d_bin_elec_y = ds.d_bin_elec_y;
  auto d_inst_density_grad_x = ds.d_inst_density_grad_x;
  auto d_inst_density_grad_y = ds.d_inst_density_grad_y;

  const float inv_bsx = 1.0f / bin_size_x;
  const float inv_bsy = 1.0f / bin_size_y;
  const int bcx = bin_cnt_x;
  const int bcy = bin_cnt_y;
  const int glx = grid_lx;
  const int gly = grid_ly;
  const float bsx = bin_size_x;
  const float bsy = bin_size_y;

  Kokkos::parallel_for(
      "densop_gather",
      Kokkos::RangePolicy<ExecSpace>(0, n_insts),
      KOKKOS_LAMBDA(const int i) {
        const int cx = d_inst_cx(i);
        const int cy = d_inst_cy(i);
        const int half_dx = d_inst_density_half_dx(i);
        const int half_dy = d_inst_density_half_dy(i);
        const float scale = d_inst_density_scale(i);

        const int d_lx = cx - half_dx;
        const int d_ly = cy - half_dy;
        const int d_ux = cx + half_dx;
        const int d_uy = cy + half_dy;

        // Bin index range (same logic as BinGrid::getDensityMinMaxIdxX/Y).
        int min_bx = static_cast<int>((d_lx - glx) * inv_bsx);
        int max_bx = static_cast<int>((static_cast<float>(d_ux - glx) * inv_bsx)
                                      + 0.9999f);
        int min_by = static_cast<int>((d_ly - gly) * inv_bsy);
        int max_by = static_cast<int>((static_cast<float>(d_uy - gly) * inv_bsy)
                                      + 0.9999f);

        if (min_bx < 0) {
          min_bx = 0;
        }
        if (min_by < 0) {
          min_by = 0;
        }
        if (max_bx > bcx) {
          max_bx = bcx;
        }
        if (max_by > bcy) {
          max_by = bcy;
        }

        float gx = 0.0f;
        float gy = 0.0f;

        for (int bxi = min_bx; bxi < max_bx; ++bxi) {
          for (int byi = min_by; byi < max_by; ++byi) {
            // Bin bounds.
            const int b_lx = glx + static_cast<int>(bxi * bsx);
            const int b_ly = gly + static_cast<int>(byi * bsy);
            const int b_ux = glx + static_cast<int>((bxi + 1) * bsx);
            const int b_uy = gly + static_cast<int>((byi + 1) * bsy);

            // Clipped rectangle overlap area.
            const int r_lx = d_lx > b_lx ? d_lx : b_lx;
            const int r_ly = d_ly > b_ly ? d_ly : b_ly;
            const int r_ux = d_ux < b_ux ? d_ux : b_ux;
            const int r_uy = d_uy < b_uy ? d_uy : b_uy;
            if (r_lx >= r_ux || r_ly >= r_uy) {
              continue;
            }
            const float overlap = static_cast<float>(r_ux - r_lx)
                                  * static_cast<float>(r_uy - r_ly);

            // FFT Views are indexed [x * binCntY + y] (X-major, matching
            // the PoissonSolver's flat layout). NOT the bin grid's
            // [y * binCntX + x] layout.
            const int fft_idx = bxi * bcy + byi;
            // Axis swap + 0.5× scale via shared adapter.
            const GplField f = solverToGplField(d_bin_elec_x(fft_idx),
                                                d_bin_elec_y(fft_idx));

            gx += overlap * scale * f.x;
            gy += overlap * scale * f.y;
          }
        }
        d_inst_density_grad_x(i) = gx;
        d_inst_density_grad_y(i) = gy;
      });
}

}  // namespace densop
}  // namespace gpl
