// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "deviceState.h"

#include <Kokkos_Core.hpp>
#include <cstddef>
#include <vector>

#include "deviceState_kokkos.h"
#include "gpuRuntime.h"
#include "nesterovBase.h"

namespace gpl {

namespace {

// Resolve a GPin's owning GCell to its index in gCellStor_.
// Linear scan over gCellStor_ once, indexed via a small map built on the
// stack — adequate at init time (a few hundred us on large01). After init,
// this map is discarded.
int indexOfGCell(const std::vector<GCell>& gCellStor, const GCell* gCell)
{
  // Pointer arithmetic into the contiguous storage vector. gCell must point
  // into gCellStor.
  const GCell* base = gCellStor.data();
  return static_cast<int>(gCell - base);
}

// Deleter passed to the type-erased unique_ptr in deviceState.h. Defined
// here where KokkosDeviceState is complete.
void deleteKokkosDeviceState(KokkosDeviceState* p)
{
  delete p;
}

}  // namespace

DeviceState::DeviceState(const std::vector<GCell>& gCellStor,
                         const std::vector<GPin>& gPinStor,
                         const std::vector<GNet>& gNetStor)
    : kokkos_(new KokkosDeviceState(), &deleteKokkosDeviceState)
{
  ensureKokkosInitialized();

  num_insts_ = static_cast<int>(gCellStor.size());
  num_pins_ = static_cast<int>(gPinStor.size());
  num_nets_ = static_cast<int>(gNetStor.size());

  auto& s = *kokkos_;
  s.d_inst_cx = Kokkos::View<int*>("ds_inst_cx", num_insts_);
  s.d_inst_cy = Kokkos::View<int*>("ds_inst_cy", num_insts_);
  s.h_inst_cx = Kokkos::create_mirror_view(s.d_inst_cx);
  s.h_inst_cy = Kokkos::create_mirror_view(s.d_inst_cy);

  s.d_pin_offset_cx = Kokkos::View<int*>("ds_pin_offset_cx", num_pins_);
  s.d_pin_offset_cy = Kokkos::View<int*>("ds_pin_offset_cy", num_pins_);
  s.d_pin_inst_id = Kokkos::View<int*>("ds_pin_inst_id", num_pins_);
  s.d_pin_net_id = Kokkos::View<int*>("ds_pin_net_id", num_pins_);
  s.d_pin_cx = Kokkos::View<int*>("ds_pin_cx", num_pins_);
  s.d_pin_cy = Kokkos::View<int*>("ds_pin_cy", num_pins_);

  s.d_net_pin_off = Kokkos::View<int*>("ds_net_pin_off", num_nets_ + 1);

  // WA wirelength gradient buffers (per-pin A/B/C).
  s.d_pin_a_pos_x = Kokkos::View<float*>("ds_pin_a_pos_x", num_pins_);
  s.d_pin_a_neg_x = Kokkos::View<float*>("ds_pin_a_neg_x", num_pins_);
  s.d_pin_a_pos_y = Kokkos::View<float*>("ds_pin_a_pos_y", num_pins_);
  s.d_pin_a_neg_y = Kokkos::View<float*>("ds_pin_a_neg_y", num_pins_);
  s.d_pin_grad_x = Kokkos::View<float*>("ds_pin_grad_x", num_pins_);
  s.d_pin_grad_y = Kokkos::View<float*>("ds_pin_grad_y", num_pins_);

  s.d_net_lx = Kokkos::View<int*>("ds_net_lx", num_nets_);
  s.d_net_ly = Kokkos::View<int*>("ds_net_ly", num_nets_);
  s.d_net_ux = Kokkos::View<int*>("ds_net_ux", num_nets_);
  s.d_net_uy = Kokkos::View<int*>("ds_net_uy", num_nets_);

  s.d_net_b_pos_x = Kokkos::View<float*>("ds_net_b_pos_x", num_nets_);
  s.d_net_b_neg_x = Kokkos::View<float*>("ds_net_b_neg_x", num_nets_);
  s.d_net_b_pos_y = Kokkos::View<float*>("ds_net_b_pos_y", num_nets_);
  s.d_net_b_neg_y = Kokkos::View<float*>("ds_net_b_neg_y", num_nets_);
  s.d_net_c_pos_x = Kokkos::View<float*>("ds_net_c_pos_x", num_nets_);
  s.d_net_c_neg_x = Kokkos::View<float*>("ds_net_c_neg_x", num_nets_);
  s.d_net_c_pos_y = Kokkos::View<float*>("ds_net_c_pos_y", num_nets_);
  s.d_net_c_neg_y = Kokkos::View<float*>("ds_net_c_neg_y", num_nets_);

  s.d_net_weight = Kokkos::View<float*>("ds_net_weight", num_nets_);

  s.d_inst_pin_off = Kokkos::View<int*>("ds_inst_pin_off", num_insts_ + 1);
  s.d_inst_wl_grad_x = Kokkos::View<float*>("ds_inst_wl_grad_x", num_insts_);
  s.d_inst_wl_grad_y = Kokkos::View<float*>("ds_inst_wl_grad_y", num_insts_);
  s.h_inst_wl_grad_x = Kokkos::create_mirror_view(s.d_inst_wl_grad_x);
  s.h_inst_wl_grad_y = Kokkos::create_mirror_view(s.d_inst_wl_grad_y);

  // ---- Build host CSR + static pin attributes ----
  // I/O pins (BTerm) have no owning GCell — their absolute coords come from
  // the DB pin position and never move during placement. Mark them with
  // inst_id = -1 so updatePinLocations() leaves d_pin_cx/d_pin_cy alone and
  // the initial absolute coord we seed below stands forever.
  std::vector<int> h_pin_offset_cx(num_pins_);
  std::vector<int> h_pin_offset_cy(num_pins_);
  std::vector<int> h_pin_inst_id(num_pins_);
  std::vector<int> h_pin_net_id(num_pins_, -1);
  std::vector<int> h_pin_cx_init(num_pins_);
  std::vector<int> h_pin_cy_init(num_pins_);
  const GNet* net_base = gNetStor.data();
  for (int i = 0; i < num_pins_; ++i) {
    const GPin& gPin = gPinStor[i];
    h_pin_offset_cx[i] = gPin.offsetCx();
    h_pin_offset_cy[i] = gPin.offsetCy();
    const GCell* gCell = gPin.getGCell();
    h_pin_inst_id[i] = gCell ? indexOfGCell(gCellStor, gCell) : -1;
    // Net index (or -1 for unconnected pins). gPin->getGNet() returns
    // pointer into gNetStor_; use pointer arithmetic to recover the index.
    const GNet* gNet = gPin.getGNet();
    h_pin_net_id[i] = gNet ? static_cast<int>(gNet - net_base) : -1;
    // GPin::cx()/cy() return absolute coords (set in the GPin ctor from the
    // DB pin position; later refreshed by updateLocation for instance pins
    // as cells move). For I/O pins they are the final value; for instance
    // pins this initial value is overwritten by updatePinLocations() once
    // syncInstCoordsFromHost() runs.
    h_pin_cx_init[i] = gPin.cx();
    h_pin_cy_init[i] = gPin.cy();
  }

  // Net→pin CSR (offsets only; per-net pin index list assembled below).
  std::vector<int> h_net_pin_off(num_nets_ + 1, 0);
  for (int n = 0; n < num_nets_; ++n) {
    h_net_pin_off[n + 1]
        = h_net_pin_off[n] + static_cast<int>(gNetStor[n].getGPins().size());
  }
  const int total_net_pins = h_net_pin_off[num_nets_];
  s.d_net_pin_idx = Kokkos::View<int*>("ds_net_pin_idx", total_net_pins);

  std::vector<int> h_net_pin_idx(total_net_pins);
  for (int n = 0; n < num_nets_; ++n) {
    int off = h_net_pin_off[n];
    for (const GPin* gPin : gNetStor[n].getGPins()) {
      // gPin is a pointer into gPinStor_; convert to index.
      const int pin_idx = static_cast<int>(gPin - gPinStor.data());
      h_net_pin_idx[off++] = pin_idx;
    }
  }

  // Inst→pin CSR. Reverse of net→pin, but bucketed by inst_id. I/O pins
  // (inst_id == -1) are excluded — they carry no gradient back to any cell.
  // Two-pass build: count per inst, then prefix-sum to offsets, then fill.
  std::vector<int> h_inst_pin_off(num_insts_ + 1, 0);
  for (int p = 0; p < num_pins_; ++p) {
    const int inst = h_pin_inst_id[p];
    if (inst >= 0) {
      h_inst_pin_off[inst + 1]++;
    }
  }
  for (int i = 0; i < num_insts_; ++i) {
    h_inst_pin_off[i + 1] += h_inst_pin_off[i];
  }
  const int total_inst_pins = h_inst_pin_off[num_insts_];
  s.d_inst_pin_idx = Kokkos::View<int*>("ds_inst_pin_idx", total_inst_pins);

  std::vector<int> h_inst_pin_idx(total_inst_pins);
  // Scratch cursor per inst — we'll increment in place during fill.
  std::vector<int> cursor(num_insts_, 0);
  for (int p = 0; p < num_pins_; ++p) {
    const int inst = h_pin_inst_id[p];
    if (inst >= 0) {
      h_inst_pin_idx[h_inst_pin_off[inst] + cursor[inst]++] = p;
    }
  }

  // Per-net total weight. Refreshed by DeviceState::refreshNetWeights — see
  // the TODO there for the missing rsz/grt-driven caller wiring.
  std::vector<float> h_net_weight(num_nets_);
  for (int n = 0; n < num_nets_; ++n) {
    h_net_weight[n] = gNetStor[n].getTotalWeight();
  }

  Kokkos::View<int*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> h_offset_cx_v(
      h_pin_offset_cx.data(), num_pins_);
  Kokkos::View<int*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> h_offset_cy_v(
      h_pin_offset_cy.data(), num_pins_);
  Kokkos::View<int*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> h_inst_id_v(
      h_pin_inst_id.data(), num_pins_);
  Kokkos::View<int*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> h_net_id_v(
      h_pin_net_id.data(), num_pins_);
  Kokkos::View<int*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> h_net_off_v(
      h_net_pin_off.data(), num_nets_ + 1);
  Kokkos::View<int*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> h_net_idx_v(
      h_net_pin_idx.data(), total_net_pins);
  Kokkos::View<int*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged>
      h_inst_pin_off_v(h_inst_pin_off.data(), num_insts_ + 1);
  Kokkos::View<int*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged>
      h_inst_pin_idx_v(h_inst_pin_idx.data(), total_inst_pins);
  Kokkos::View<float*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged>
      h_net_weight_v(h_net_weight.data(), num_nets_);

  Kokkos::deep_copy(s.d_pin_offset_cx, h_offset_cx_v);
  Kokkos::deep_copy(s.d_pin_offset_cy, h_offset_cy_v);
  Kokkos::deep_copy(s.d_pin_inst_id, h_inst_id_v);
  Kokkos::deep_copy(s.d_pin_net_id, h_net_id_v);
  Kokkos::deep_copy(s.d_net_pin_off, h_net_off_v);
  Kokkos::deep_copy(s.d_net_pin_idx, h_net_idx_v);
  Kokkos::deep_copy(s.d_inst_pin_off, h_inst_pin_off_v);
  Kokkos::deep_copy(s.d_inst_pin_idx, h_inst_pin_idx_v);
  Kokkos::deep_copy(s.d_net_weight, h_net_weight_v);

  // Seed pin coords (absolute). For I/O pins this is the final value
  // (inst_id == -1, skipped by updatePinLocations); for instance pins this
  // is the starting value, overwritten every iteration by the kernel.
  Kokkos::View<int*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> h_pin_cx_v(
      h_pin_cx_init.data(), num_pins_);
  Kokkos::View<int*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> h_pin_cy_v(
      h_pin_cy_init.data(), num_pins_);
  Kokkos::deep_copy(s.d_pin_cx, h_pin_cx_v);
  Kokkos::deep_copy(s.d_pin_cy, h_pin_cy_v);

  // Initial coord push so the device buffers are not garbage on the first
  // updatePinLocations() before any host iteration has occurred.
  syncInstCoordsFromHost(gCellStor);
}

// ~DeviceState() is inline-defaulted in deviceState.h thanks to the
// function-pointer deleter on kokkos_.

void DeviceState::initBinViews(const BinGrid& binGrid,
                               const std::vector<GCell>& gCellStor)
{
  bin_cnt_x_ = binGrid.getBinCntX();
  bin_cnt_y_ = binGrid.getBinCntY();
  bin_size_x_ = static_cast<float>(binGrid.getBinSizeX());
  bin_size_y_ = static_cast<float>(binGrid.getBinSizeY());
  grid_lx_ = binGrid.lx();
  grid_ly_ = binGrid.ly();
  num_bins_ = bin_cnt_x_ * bin_cnt_y_;

  auto& s = *kokkos_;
  s.d_bin_density = Kokkos::View<float*>("ds_bin_density", num_bins_);
  s.d_bin_phi = Kokkos::View<float*>("ds_bin_phi", num_bins_);
  s.d_bin_elec_x = Kokkos::View<float*>("ds_bin_elec_x", num_bins_);
  s.d_bin_elec_y = Kokkos::View<float*>("ds_bin_elec_y", num_bins_);
  s.h_bin_density = Kokkos::create_mirror_view(s.d_bin_density);
  s.h_bin_phi = Kokkos::create_mirror_view(s.d_bin_phi);
  s.h_bin_elec_x = Kokkos::create_mirror_view(s.d_bin_elec_x);
  s.h_bin_elec_y = Kokkos::create_mirror_view(s.d_bin_elec_y);

  s.d_inst_density_half_dx
      = Kokkos::View<int*>("ds_inst_density_half_dx", num_insts_);
  s.d_inst_density_half_dy
      = Kokkos::View<int*>("ds_inst_density_half_dy", num_insts_);
  s.d_inst_density_scale
      = Kokkos::View<float*>("ds_inst_density_scale", num_insts_);
  s.d_inst_density_grad_x
      = Kokkos::View<float*>("ds_inst_density_grad_x", num_insts_);
  s.d_inst_density_grad_y
      = Kokkos::View<float*>("ds_inst_density_grad_y", num_insts_);
  s.h_inst_density_grad_x = Kokkos::create_mirror_view(s.d_inst_density_grad_x);
  s.h_inst_density_grad_y = Kokkos::create_mirror_view(s.d_inst_density_grad_y);

  std::vector<int> h_half_dx(num_insts_);
  std::vector<int> h_half_dy(num_insts_);
  std::vector<float> h_scale(num_insts_);
  for (int i = 0; i < num_insts_; ++i) {
    h_half_dx[i] = gCellStor[i].dDx() / 2;
    h_half_dy[i] = gCellStor[i].dDy() / 2;
    h_scale[i] = gCellStor[i].getDensityScale();
  }
  Kokkos::View<int*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> hv_dx(
      h_half_dx.data(), num_insts_);
  Kokkos::View<int*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> hv_dy(
      h_half_dy.data(), num_insts_);
  Kokkos::View<float*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> hv_s(
      h_scale.data(), num_insts_);
  Kokkos::deep_copy(s.d_inst_density_half_dx, hv_dx);
  Kokkos::deep_copy(s.d_inst_density_half_dy, hv_dy);
  Kokkos::deep_copy(s.d_inst_density_scale, hv_s);
}

void DeviceState::syncInstCoordsFromHost(const std::vector<GCell>& gCellStor)
{
  auto& s = *kokkos_;
  // IMPORTANT: read DENSITY centers (dCx/dCy), not regular centers (cx/cy).
  // During Nesterov iterations, only density coords mutate
  // (updateGCellDensityCenterLocation calls setDensityCenterLocation). The
  // "regular" lx_/ux_ are only ever set by updateGCellCenterLocation, which
  // is not part of the inner loop. The CPU getHpwl path reads gPin->cx_,
  // which is refreshed to dCx_-based by gPin->updateDensityLocation — i.e.,
  // CPU also effectively uses density coords during the iter loop.
  for (int i = 0; i < num_insts_; ++i) {
    s.h_inst_cx(i) = gCellStor[i].dCx();
    s.h_inst_cy(i) = gCellStor[i].dCy();
  }
  Kokkos::deep_copy(s.d_inst_cx, s.h_inst_cx);
  Kokkos::deep_copy(s.d_inst_cy, s.h_inst_cy);
}

void DeviceState::updatePinLocations()
{
  auto& s = *kokkos_;
  // Local refs so the lambda captures by value, not via implicit `this`.
  auto d_inst_cx = s.d_inst_cx;
  auto d_inst_cy = s.d_inst_cy;
  auto d_pin_offset_cx = s.d_pin_offset_cx;
  auto d_pin_offset_cy = s.d_pin_offset_cy;
  auto d_pin_inst_id = s.d_pin_inst_id;
  auto d_pin_cx = s.d_pin_cx;
  auto d_pin_cy = s.d_pin_cy;

  using ExecSpace = Kokkos::DefaultExecutionSpace;
  Kokkos::parallel_for(
      "ds_update_pin_loc",
      Kokkos::RangePolicy<ExecSpace>(0, num_pins_),
      KOKKOS_LAMBDA(const int i) {
        const int inst = d_pin_inst_id(i);
        // I/O pins (inst < 0) keep the absolute coord seeded at construction.
        if (inst >= 0) {
          d_pin_cx(i) = d_inst_cx(inst) + d_pin_offset_cx(i);
          d_pin_cy(i) = d_inst_cy(inst) + d_pin_offset_cy(i);
        }
      });
}

void DeviceState::refreshNetWeights(const std::vector<GNet>& gNetStor)
{
  auto& s = *kokkos_;
  std::vector<float> h_weights(num_nets_);
  for (int n = 0; n < num_nets_; ++n) {
    h_weights[n] = gNetStor[n].getTotalWeight();
  }
  Kokkos::View<float*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> hv(
      h_weights.data(), num_nets_);
  Kokkos::deep_copy(s.d_net_weight, hv);
}

void DeviceState::refreshDensityParams(const std::vector<GCell>& gCellStor)
{
  auto& s = *kokkos_;
  std::vector<int> h_half_dx(num_insts_);
  std::vector<int> h_half_dy(num_insts_);
  std::vector<float> h_scale(num_insts_);
  for (int i = 0; i < num_insts_; ++i) {
    h_half_dx[i] = gCellStor[i].dDx() / 2;
    h_half_dy[i] = gCellStor[i].dDy() / 2;
    h_scale[i] = gCellStor[i].getDensityScale();
  }
  Kokkos::View<int*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> hv_dx(
      h_half_dx.data(), num_insts_);
  Kokkos::View<int*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> hv_dy(
      h_half_dy.data(), num_insts_);
  Kokkos::View<float*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> hv_s(
      h_scale.data(), num_insts_);
  Kokkos::deep_copy(s.d_inst_density_half_dx, hv_dx);
  Kokkos::deep_copy(s.d_inst_density_half_dy, hv_dy);
  Kokkos::deep_copy(s.d_inst_density_scale, hv_s);
}

int DeviceState::numInsts() const
{
  return num_insts_;
}

int DeviceState::numPins() const
{
  return num_pins_;
}

int DeviceState::numNets() const
{
  return num_nets_;
}

int DeviceState::numBins() const
{
  return num_bins_;
}

void DeviceState::ensureCoordsFresh(const std::vector<GCell>& gCellStor)
{
  // Fast path: NB device context already scattered fresh inst coords (and
  // ran updatePinLocations()) this iteration via commitCoordsToDeviceState.
  // Skip the host→device round-trip — host gCellStor_::dCx/dCy is
  // int-truncated and would lose the sub-integer precision the GPU
  // coord-update kernel produced.
  if (coords_fresh_) {
    coords_fresh_ = false;
    return;
  }
  syncInstCoordsFromHost(gCellStor);
  updatePinLocations();
}

}  // namespace gpl
