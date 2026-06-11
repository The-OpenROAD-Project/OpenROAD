// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// GpuHpwlBackend — the Kokkos GPU implementation of HpwlBackend.
//
// Compiled only when ENABLE_GPU=ON. makeHpwlBackend() (in ../hpwl.cpp)
// constructs a GpuHpwlBackend when the GPU path is selected at run time
// (gpl::gpuEnabled()); CpuHpwlBackend stays the default. Both backends coexist
// in an ENABLE_GPU build — the choice is a runtime one.
//
// Reads pin coords from a DeviceState shared with the owning
// NesterovBaseCommon; owns only the per-net bbox / reduction buffers + their
// host mirrors.
//
// Determinism: integer arithmetic; bit-exact across Kokkos backends
// (Serial / OpenMP / Threads / CUDA) and against the OpenMP CPU loop.

#include "gpuHpwlBackend.h"

#include <Kokkos_Core.hpp>
#include <climits>
#include <cstdint>
#include <memory>
#include <vector>

#include "deviceState.h"
#include "deviceState_kokkos.h"
#include "gpuRuntime.h"
#include "nesterovBase.h"

namespace gpl {

// Persistent backend-private state: only the per-net bbox outputs and their
// host mirrors. The pin coords, pin→net CSR, and inst coords live in the
// shared DeviceState (gpu/deviceState.h).
struct GpuHpwlBackend::Impl
{
  DeviceState* device_state;  // borrowed
  Kokkos::View<int*> d_lx;
  Kokkos::View<int*> d_ly;
  Kokkos::View<int*> d_ux;
  Kokkos::View<int*> d_uy;
  Kokkos::View<int*>::HostMirror h_lx;
  Kokkos::View<int*>::HostMirror h_ly;
  Kokkos::View<int*>::HostMirror h_ux;
  Kokkos::View<int*>::HostMirror h_uy;
};

GpuHpwlBackend::GpuHpwlBackend(DeviceState* device_state)
    : impl_(std::make_unique<Impl>())
{
  impl_->device_state = device_state;
}

GpuHpwlBackend::~GpuHpwlBackend() = default;

int64_t GpuHpwlBackend::computeHpwl(std::vector<GNet>& gNetStor)
{
  const int n_nets = static_cast<int>(gNetStor.size());
  if (n_nets == 0) {
    return 0;
  }

  ensureKokkosInitialized();

  Impl& s = *impl_;
  KokkosDeviceState& ds = s.device_state->kokkos();

  // ---- 1. Lazy (re)allocate per-net bbox buffers ----
  // n_nets is fixed across Nesterov iterations, so this is one-shot in
  // practice.
  if (s.d_lx.extent(0) != static_cast<size_t>(n_nets)) {
    s.d_lx = Kokkos::View<int*>("hpwl_net_lx", n_nets);
    s.d_ly = Kokkos::View<int*>("hpwl_net_ly", n_nets);
    s.d_ux = Kokkos::View<int*>("hpwl_net_ux", n_nets);
    s.d_uy = Kokkos::View<int*>("hpwl_net_uy", n_nets);
    s.h_lx = Kokkos::create_mirror_view(s.d_lx);
    s.h_ly = Kokkos::create_mirror_view(s.d_ly);
    s.h_ux = Kokkos::create_mirror_view(s.d_ux);
    s.h_uy = Kokkos::create_mirror_view(s.d_uy);
  }

  // Local refs so the lambdas below capture by value (no implicit `this`).
  auto d_net_pin_off = ds.d_net_pin_off;
  auto d_net_pin_idx = ds.d_net_pin_idx;
  auto d_pin_cx = ds.d_pin_cx;
  auto d_pin_cy = ds.d_pin_cy;
  auto d_lx = s.d_lx;
  auto d_ly = s.d_ly;
  auto d_ux = s.d_ux;
  auto d_uy = s.d_uy;

  using ExecSpace = Kokkos::DefaultExecutionSpace;

  // ---- 2. Compute per-net bbox in parallel; serial inner over pins ----
  // Pin coords are already on the device (DeviceState::updatePinLocations
  // ran beforehand). Indirection through d_net_pin_idx — the CSR stores
  // global pin indices into d_pin_cx/d_pin_cy. Nets above
  // kHighFanoutThreshold are deferred to a team-parallel pass below — one
  // thread walking a clock/reset-class net serializes the launch. Int
  // min/max is associative, so both passes stay bit-identical to the CPU
  // updateBox() loop regardless of reduction order.
  Kokkos::parallel_for(
      "hpwl_bbox",
      Kokkos::RangePolicy<ExecSpace>(0, n_nets),
      KOKKOS_LAMBDA(const int i) {
        const int begin = d_net_pin_off(i);
        const int end = d_net_pin_off(i + 1);
        if (end - begin > kHighFanoutThreshold) {
          return;  // handled by the team-parallel pass below
        }
        int lx = INT_MAX;
        int ly = INT_MAX;
        int ux = INT_MIN;
        int uy = INT_MIN;
        for (int j = begin; j < end; ++j) {
          const int pin = d_net_pin_idx(j);
          const int x = d_pin_cx(pin);
          const int y = d_pin_cy(pin);
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

  const int n_high_fanout
      = static_cast<int>(ds.d_high_fanout_net_idx.extent(0));
  if (n_high_fanout > 0) {
    auto d_high_fanout_net_idx = ds.d_high_fanout_net_idx;
    using TeamPolicy = Kokkos::TeamPolicy<ExecSpace>;
    Kokkos::parallel_for(
        "hpwl_bbox_hf",
        TeamPolicy(n_high_fanout, Kokkos::AUTO),
        KOKKOS_LAMBDA(const TeamPolicy::member_type& team) {
          const int i = d_high_fanout_net_idx(team.league_rank());
          const int begin = d_net_pin_off(i);
          const int end = d_net_pin_off(i + 1);
          int lx, ly, ux, uy;
          Kokkos::parallel_reduce(
              Kokkos::TeamThreadRange(team, begin, end),
              [&](const int j, int& v) {
                const int x = d_pin_cx(d_net_pin_idx(j));
                v = x < v ? x : v;
              },
              Kokkos::Min<int>(lx));
          Kokkos::parallel_reduce(
              Kokkos::TeamThreadRange(team, begin, end),
              [&](const int j, int& v) {
                const int y = d_pin_cy(d_net_pin_idx(j));
                v = y < v ? y : v;
              },
              Kokkos::Min<int>(ly));
          Kokkos::parallel_reduce(
              Kokkos::TeamThreadRange(team, begin, end),
              [&](const int j, int& v) {
                const int x = d_pin_cx(d_net_pin_idx(j));
                v = x > v ? x : v;
              },
              Kokkos::Max<int>(ux));
          Kokkos::parallel_reduce(
              Kokkos::TeamThreadRange(team, begin, end),
              [&](const int j, int& v) {
                const int y = d_pin_cy(d_net_pin_idx(j));
                v = y > v ? y : v;
              },
              Kokkos::Max<int>(uy));
          Kokkos::single(Kokkos::PerTeam(team), [&]() {
            d_lx(i) = lx;
            d_ly(i) = ly;
            d_ux(i) = ux;
            d_uy(i) = uy;
          });
        });
  }

  // ---- 3. Sum HPWL across nets (int64 reduction → backend-deterministic) ----
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
        acc += (static_cast<int64_t>(ux) - lx)
               + (static_cast<int64_t>(uy) - ly);
      },
      total_hpwl);

  // ---- 4. Mirror per-net bbox back to host GNet objects ----
  // Subsequent code paths (e.g. routeBase, timing-driven weights) read
  // gNet->lx() / ly() / ux() / uy() and expect them updated.
  Kokkos::deep_copy(s.h_lx, s.d_lx);
  Kokkos::deep_copy(s.h_ly, s.d_ly);
  Kokkos::deep_copy(s.h_ux, s.d_ux);
  Kokkos::deep_copy(s.h_uy, s.d_uy);

  for (int i = 0; i < n_nets; ++i) {
    gNetStor[i].setBox(s.h_lx(i), s.h_ly(i), s.h_ux(i), s.h_uy(i));
  }

  return total_hpwl;
}

}  // namespace gpl
