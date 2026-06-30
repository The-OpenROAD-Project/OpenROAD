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
#include "nesterovDeviceState.h"
#include "poissonSolver.h"
#include "regionDensityField_kokkos.h"

namespace gpl {
namespace densop {

namespace {
using ExecSpace = Kokkos::DefaultExecutionSpace;

// Select the x/y coord Views for a SlpSlot. Mirrors nesterovOp.cpp's getVec;
// duplicated locally to keep the two TU's anonymous namespaces independent.
struct CoordPair
{
  Kokkos::View<float*> x;
  Kokkos::View<float*> y;
};

CoordPair getSlpCoords(const KokkosNesterovState& ns, SlpSlot slot)
{
  switch (slot) {
    case SlpSlot::Cur:
      return {ns.d_cur_slp_x, ns.d_cur_slp_y};
    case SlpSlot::Prev:
      return {ns.d_prev_slp_x, ns.d_prev_slp_y};
    case SlpSlot::Next:
      return {ns.d_next_slp_x, ns.d_next_slp_y};
  }
  Kokkos::abort("getSlpCoords: invalid SlpSlot");
  return {ns.d_next_slp_x, ns.d_next_slp_y};
}

}  // namespace

void launchDensityGather(KokkosDeviceState& ds,
                         const KokkosRegionDensityField& rdf,
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
  auto d_bin_elec_x = rdf.d_bin_elec_x;
  auto d_bin_elec_y = rdf.d_bin_elec_y;
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

void launchNbDensityScatter(KokkosNesterovState& ns,
                            KokkosRegionDensityField& rdf,
                            int n_cells,
                            SlpSlot src,
                            float& overflow_area,
                            float& overflow_area_unscaled)
{
  const int bcx = ns.bin_cnt_x;
  const int bcy = ns.bin_cnt_y;
  const int nbins = bcx * bcy;
  if (n_cells == 0 || nbins == 0) {
    overflow_area = 0;
    overflow_area_unscaled = 0;
    return;
  }

  auto d_bin_inst_area = ns.d_bin_inst_area;
  auto d_bin_filler_area = ns.d_bin_filler_area;
  Kokkos::deep_copy(d_bin_inst_area, 0);
  Kokkos::deep_copy(d_bin_filler_area, 0);

  CoordPair coords = getSlpCoords(ns, src);
  auto src_x = coords.x;
  auto src_y = coords.y;
  auto d_half_ddx = ns.d_half_ddx;
  auto d_half_ddy = ns.d_half_ddy;
  auto d_density_scale = ns.d_density_scale;
  auto d_cell_kind = ns.d_cell_kind;
  auto d_bin_lx = ns.d_bin_lx;
  auto d_bin_ly = ns.d_bin_ly;
  auto d_bin_ux = ns.d_bin_ux;
  auto d_bin_uy = ns.d_bin_uy;
  auto d_bin_target_density = ns.d_bin_target_density;

  const double bsx = ns.bin_size_x;
  const double bsy = ns.bin_size_y;
  const int glx = ns.grid_lx;
  const int gly = ns.grid_ly;

  // Pass 1 — per-cell scatter. Bin index ranges replicate
  // BinGrid::getDensityMinMaxIdxX/Y in double, the overlap is exact int
  // clipping against the uploaded lround()-derived bin bounds, and each
  // contribution is truncated to int64 before the atomic add — the same
  // float→int64_t conversion Bin::add*Area performs, so the totals match
  // the serial CPU loop bit-for-bit for identical coords.
  Kokkos::parallel_for(
      "densop_nb_scatter",
      Kokkos::RangePolicy<ExecSpace>(0, n_cells),
      KOKKOS_LAMBDA(const int i) {
        const int kind = d_cell_kind(i);
        if (kind == 0) {
          return;
        }
        // GCell::setDensityCenterLocation takes ints — float coords truncate.
        const int cx = static_cast<int>(src_x(i));
        const int cy = static_cast<int>(src_y(i));
        const int d_lx = cx - d_half_ddx(i);
        const int d_ly = cy - d_half_ddy(i);
        const int d_ux = cx + d_half_ddx(i);
        const int d_uy = cy + d_half_ddy(i);

        int min_bx = static_cast<int>((d_lx - glx) / bsx);
        int max_bx = static_cast<int>(Kokkos::ceil((d_ux - glx) / bsx));
        int min_by = static_cast<int>((d_ly - gly) / bsy);
        int max_by = static_cast<int>(Kokkos::ceil((d_uy - gly) / bsy));
        min_bx = min_bx < 0 ? 0 : min_bx;
        min_by = min_by < 0 ? 0 : min_by;
        max_bx = max_bx > bcx ? bcx : max_bx;
        max_by = max_by > bcy ? bcy : max_by;

        const float scale = d_density_scale(i);
        for (int bxi = min_bx; bxi < max_bx; ++bxi) {
          for (int byi = min_by; byi < max_by; ++byi) {
            const int bi = bxi * bcy + byi;  // FFT layout
            const int r_lx = d_lx > d_bin_lx(bi) ? d_lx : d_bin_lx(bi);
            const int r_ly = d_ly > d_bin_ly(bi) ? d_ly : d_bin_ly(bi);
            const int r_ux = d_ux < d_bin_ux(bi) ? d_ux : d_bin_ux(bi);
            const int r_uy = d_uy < d_bin_uy(bi) ? d_uy : d_bin_uy(bi);
            if (r_lx >= r_ux || r_ly >= r_uy) {
              continue;
            }
            float v = static_cast<float>(r_ux - r_lx)
                      * static_cast<float>(r_uy - r_ly) * scale;
            // Truncate to int64 first (CPU's float→int64_t conversion),
            // then accumulate in double — integer-exact below 2^53.
            if (kind == 3) {
              Kokkos::atomic_add(
                  &d_bin_filler_area(bi),
                  static_cast<double>(static_cast<long long>(v)));
            } else {
              if (kind == 2) {
                v *= d_bin_target_density(bi);
              }
              Kokkos::atomic_add(
                  &d_bin_inst_area(bi),
                  static_cast<double>(static_cast<long long>(v)));
            }
          }
        }
      });

  // Pass 2 — per-bin density write + overflow reductions. Mirrors the
  // tail loop of BinGrid::updateBinsGCellDensityArea.
  auto d_bin_density = rdf.d_bin_density;
  auto d_bin_scaled_area = ns.d_bin_scaled_area;
  auto d_bin_nonplace = ns.d_bin_nonplace;
  auto d_bin_nonplace_unscaled = ns.d_bin_nonplace_unscaled;

  float ovf = 0;
  float ovf_unscaled = 0;
  Kokkos::parallel_reduce(
      "densop_nb_bin_pass",
      Kokkos::RangePolicy<ExecSpace>(0, nbins),
      KOKKOS_LAMBDA(const int bi, float& local_ovf, float& local_ovf_u) {
        const float inst = static_cast<float>(d_bin_inst_area(bi));
        const float filler = static_cast<float>(d_bin_filler_area(bi));
        const float nonplace = static_cast<float>(d_bin_nonplace(bi));
        const float nonplace_u
            = static_cast<float>(d_bin_nonplace_unscaled(bi));
        const float scaled_area = d_bin_scaled_area(bi);

        d_bin_density(bi) = (inst + filler + nonplace) / scaled_area;

        const float o = inst + nonplace - scaled_area;
        if (o > 0) {
          local_ovf += o;
        }
        const float ou = inst + nonplace_u - scaled_area;
        if (ou > 0) {
          local_ovf_u += ou;
        }
      },
      ovf,
      ovf_unscaled);

  overflow_area = ovf;
  overflow_area_unscaled = ovf_unscaled;
}

void launchNbDensityGather(KokkosNesterovState& ns,
                           const KokkosRegionDensityField& rdf,
                           int n_cells,
                           SlpSlot src)
{
  if (n_cells == 0) {
    return;
  }

  CoordPair coords = getSlpCoords(ns, src);
  auto src_x = coords.x;
  auto src_y = coords.y;
  auto d_half_ddx = ns.d_half_ddx;
  auto d_half_ddy = ns.d_half_ddy;
  auto d_density_scale = ns.d_density_scale;
  auto d_bin_lx = ns.d_bin_lx;
  auto d_bin_ly = ns.d_bin_ly;
  auto d_bin_ux = ns.d_bin_ux;
  auto d_bin_uy = ns.d_bin_uy;
  auto d_bin_elec_x = rdf.d_bin_elec_x;
  auto d_bin_elec_y = rdf.d_bin_elec_y;
  auto d_grad_x = ns.d_density_grad_x;
  auto d_grad_y = ns.d_density_grad_y;

  const int bcx = ns.bin_cnt_x;
  const int bcy = ns.bin_cnt_y;
  const double bsx = ns.bin_size_x;
  const double bsy = ns.bin_size_y;
  const int glx = ns.grid_lx;
  const int gly = ns.grid_ly;

  Kokkos::parallel_for(
      "densop_nb_gather",
      Kokkos::RangePolicy<ExecSpace>(0, n_cells),
      KOKKOS_LAMBDA(const int i) {
        const int cx = static_cast<int>(src_x(i));
        const int cy = static_cast<int>(src_y(i));
        const int d_lx = cx - d_half_ddx(i);
        const int d_ly = cy - d_half_ddy(i);
        const int d_ux = cx + d_half_ddx(i);
        const int d_uy = cy + d_half_ddy(i);

        int min_bx = static_cast<int>((d_lx - glx) / bsx);
        int max_bx = static_cast<int>(Kokkos::ceil((d_ux - glx) / bsx));
        int min_by = static_cast<int>((d_ly - gly) / bsy);
        int max_by = static_cast<int>(Kokkos::ceil((d_uy - gly) / bsy));
        min_bx = min_bx < 0 ? 0 : min_bx;
        min_by = min_by < 0 ? 0 : min_by;
        max_bx = max_bx > bcx ? bcx : max_bx;
        max_by = max_by > bcy ? bcy : max_by;

        const float scale = d_density_scale(i);
        float gx = 0.0f;
        float gy = 0.0f;
        for (int bxi = min_bx; bxi < max_bx; ++bxi) {
          for (int byi = min_by; byi < max_by; ++byi) {
            const int bi = bxi * bcy + byi;  // FFT layout
            const int r_lx = d_lx > d_bin_lx(bi) ? d_lx : d_bin_lx(bi);
            const int r_ly = d_ly > d_bin_ly(bi) ? d_ly : d_bin_ly(bi);
            const int r_ux = d_ux < d_bin_ux(bi) ? d_ux : d_bin_ux(bi);
            const int r_uy = d_uy < d_bin_uy(bi) ? d_uy : d_bin_uy(bi);
            if (r_lx >= r_ux || r_ly >= r_uy) {
              continue;
            }
            const float overlap = static_cast<float>(r_ux - r_lx)
                                  * static_cast<float>(r_uy - r_ly) * scale;
            const GplField f
                = solverToGplField(d_bin_elec_x(bi), d_bin_elec_y(bi));
            gx += overlap * f.x;
            gy += overlap * f.y;
          }
        }
        d_grad_x(i) = gx;
        d_grad_y(i) = gy;
      });
}

float launchNbSumPhi(const KokkosNesterovState& ns,
                     const KokkosRegionDensityField& rdf)
{
  const int nbins = ns.bin_cnt_x * ns.bin_cnt_y;
  if (nbins == 0) {
    return 0.0f;
  }
  auto d_bin_phi = rdf.d_bin_phi;
  auto d_bin_inst_area = ns.d_bin_inst_area;
  auto d_bin_filler_area = ns.d_bin_filler_area;
  auto d_bin_nonplace = ns.d_bin_nonplace;

  float sum_phi = 0;
  Kokkos::parallel_reduce(
      "densop_nb_sum_phi",
      Kokkos::RangePolicy<ExecSpace>(0, nbins),
      KOKKOS_LAMBDA(const int bi, float& local) {
        // CPU sums the int64 areas first, then casts once.
        const long long total = d_bin_nonplace(bi)
                                + static_cast<long long>(d_bin_inst_area(bi))
                                + static_cast<long long>(d_bin_filler_area(bi));
        local += d_bin_phi(bi) * static_cast<float>(total);
      },
      sum_phi);
  return sum_phi;
}

}  // namespace densop
}  // namespace gpl
