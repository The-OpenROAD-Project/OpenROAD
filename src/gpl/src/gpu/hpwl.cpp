// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// HPWL (half-perimeter wirelength) Kokkos backend.
//
// Compiled only when ENABLE_GPU=ON. This translation unit exposes the GPU
// kernel as the free function gpl::getHpwlGpu(); the CPU dispatch site in
// ../hpwl.cpp calls it at runtime when gpl::gpuEnabled() is true. Both the
// CPU and GPU paths coexist in an ENABLE_GPU build — selection is a runtime
// choice, not a link-time one.
//
// Determinism: integer arithmetic; bit-exact across Kokkos backends
// (Serial / OpenMP / Threads / CUDA) and against the OpenMP CPU loop.

#include <Kokkos_Core.hpp>
#include <climits>
#include <cstdint>
#include <vector>

#include "gpuBackend.h"
#include "hpwlGpu.h"
#include "nesterovBase.h"

namespace gpl {

int64_t getHpwlGpu(std::vector<GNet>& gNetStor)
{
  const int n_nets = static_cast<int>(gNetStor.size());
  if (n_nets == 0) {
    return 0;
  }

  ensureKokkosInitialized();

  // ---- 1. Flatten net→pins to CSR on host ----
  std::vector<int> h_net_off(n_nets + 1, 0);
  for (int i = 0; i < n_nets; ++i) {
    h_net_off[i + 1]
        = h_net_off[i] + static_cast<int>(gNetStor[i].getGPins().size());
  }
  const int total_pins = h_net_off[n_nets];

  std::vector<int> h_pin_cx(total_pins);
  std::vector<int> h_pin_cy(total_pins);
  for (int i = 0; i < n_nets; ++i) {
    int off = h_net_off[i];
    for (auto* gPin : gNetStor[i].getGPins()) {
      h_pin_cx[off] = gPin->cx();
      h_pin_cy[off] = gPin->cy();
      ++off;
    }
  }

  // ---- 2. Mirror inputs to device ----
  using ExecSpace = Kokkos::DefaultExecutionSpace;
  Kokkos::View<int*, ExecSpace> d_net_off("hpwl_net_off", n_nets + 1);
  Kokkos::View<int*, ExecSpace> d_pin_cx("hpwl_pin_cx", total_pins);
  Kokkos::View<int*, ExecSpace> d_pin_cy("hpwl_pin_cy", total_pins);

  Kokkos::View<int*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> h_net_off_view(
      h_net_off.data(), n_nets + 1);
  Kokkos::View<int*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> h_pin_cx_view(
      h_pin_cx.data(), total_pins);
  Kokkos::View<int*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> h_pin_cy_view(
      h_pin_cy.data(), total_pins);

  Kokkos::deep_copy(d_net_off, h_net_off_view);
  Kokkos::deep_copy(d_pin_cx, h_pin_cx_view);
  Kokkos::deep_copy(d_pin_cy, h_pin_cy_view);

  // Per-net bbox outputs (kept on device for reduction; mirrored back at end).
  Kokkos::View<int*, ExecSpace> d_lx("hpwl_net_lx", n_nets);
  Kokkos::View<int*, ExecSpace> d_ly("hpwl_net_ly", n_nets);
  Kokkos::View<int*, ExecSpace> d_ux("hpwl_net_ux", n_nets);
  Kokkos::View<int*, ExecSpace> d_uy("hpwl_net_uy", n_nets);

  // ---- 3. Compute per-net bbox in parallel; serial inner over pins ----
  Kokkos::parallel_for(
      "hpwl_bbox",
      Kokkos::RangePolicy<ExecSpace>(0, n_nets),
      KOKKOS_LAMBDA(const int i) {
        int lx = INT_MAX;
        int ly = INT_MAX;
        int ux = INT_MIN;
        int uy = INT_MIN;
        const int begin = d_net_off(i);
        const int end = d_net_off(i + 1);
        // Serial over pins for determinism (sgizler 80b04e1c1 pattern: do not
        // rely on parallel_reduce ordering even though min/max are commutative
        // — keeps results bit-identical to the CPU updateBox() loop).
        for (int j = begin; j < end; ++j) {
          const int x = d_pin_cx(j);
          const int y = d_pin_cy(j);
          if (x < lx) {
            lx = x;
          }
          if (y < ly) {
            ly = y;
          }
          if (x > ux) {
            ux = x;
          }
          if (y > uy) {
            uy = y;
          }
        }
        d_lx(i) = lx;
        d_ly(i) = ly;
        d_ux(i) = ux;
        d_uy(i) = uy;
      });

  // ---- 4. Sum HPWL across nets (int64 reduction → backend-deterministic) ----
  int64_t total_hpwl = 0;
  Kokkos::parallel_reduce(
      "hpwl_sum",
      Kokkos::RangePolicy<ExecSpace>(0, n_nets),
      KOKKOS_LAMBDA(const int i, int64_t& acc) {
        const int lx = d_lx(i);
        const int ly = d_ly(i);
        const int ux = d_ux(i);
        const int uy = d_uy(i);
        // Dangling net (no pins): GNet::getHpwl() returns 0 in this case.
        if (ux < lx) {
          return;
        }
        acc += static_cast<int64_t>(ux - lx) + static_cast<int64_t>(uy - ly);
      },
      total_hpwl);

  // ---- 5. Mirror per-net bbox back to host GNet objects ----
  // Subsequent code paths (e.g. routeBase, timing-driven weights) read
  // gNet->lx() / ly() / ux() / uy() and expect them updated.
  auto h_lx = Kokkos::create_mirror_view(d_lx);
  auto h_ly = Kokkos::create_mirror_view(d_ly);
  auto h_ux = Kokkos::create_mirror_view(d_ux);
  auto h_uy = Kokkos::create_mirror_view(d_uy);
  Kokkos::deep_copy(h_lx, d_lx);
  Kokkos::deep_copy(h_ly, d_ly);
  Kokkos::deep_copy(h_ux, d_ux);
  Kokkos::deep_copy(h_uy, d_uy);

  for (int i = 0; i < n_nets; ++i) {
    gNetStor[i].setBox(h_lx(i), h_ly(i), h_ux(i), h_uy(i));
  }

  return total_hpwl;
}

}  // namespace gpl
