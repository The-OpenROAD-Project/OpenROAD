// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// DeviceState — owns the device-resident pool of cell coordinates, per-pin
// offsets, and the net→pin CSR. Built once per NesterovBaseCommon after the
// gCellStor_ / gPinStor_ / gNetStor_ vectors are populated; reused across
// every Nesterov iteration to keep coordinate data on the device.
//
// This is the foundation for moving the gpl hot path off the host:
//   - HPWL (Phase 1, this file): reads device pin coords directly, no host
//     re-pack per iteration.
//   - WA wirelength gradient (Phase 2): same device pool + per-pin A/B/C
//     buffers (owned by the gradient backend).
//   - Density scatter+gather (Phase 3): same instance coords drive the
//     density bin update.
//   - Nesterov coord update (Phase 4): inst coords mutate device-side,
//     `syncInstCoordsFromHost` becomes the one-time init load.
//
// PIMPL: Kokkos types are hidden in gpu/deviceState_kokkos.h, included only
// by Kokkos-aware translation units. This header is plain C++, so consumer
// TUs (nesterovBase.cpp in particular) need not be compiled by nvcc.
//
// Compiled only when ENABLE_GPU=ON.

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace gpl {

class BinGrid;
class GCell;
class GNet;
class GPin;

struct KokkosDeviceState;  // gpu/deviceState_kokkos.h

class DeviceState
{
 public:
  // Reads instance coords, pin offsets, pin→inst id, and net→pin CSR from
  // the supplied host storage. Static data (offsets, CSRs) is pushed once;
  // coords loaded each iter via syncInstCoordsFromHost().
  DeviceState(const std::vector<GCell>& gCellStor,
              const std::vector<GPin>& gPinStor,
              const std::vector<GNet>& gNetStor);
  ~DeviceState();

  // Phase 3: allocate bin grid Views + push per-inst density params. Called
  // once from NesterovBase after the BinGrid is initialized (initDensity1).
  // Must precede any density gather kernel or GpuFftBackend solve.
  void initBinViews(const BinGrid& binGrid,
                    const std::vector<GCell>& gCellStor);

  // Re-push current instance centers (= GCell::cx()/cy()) to the device.
  // Used at the start of every gpu kernel that reads pin coords in Phases
  // 1-3, where Nesterov updates still run on the host. After Phase 4 this
  // shrinks to a one-time initial load.
  void syncInstCoordsFromHost(const std::vector<GCell>& gCellStor);

  // Compute absolute pin centers on the device:
  //   d_pin_cx[i] = d_inst_cx[d_pin_inst_id[i]] + d_pin_offset_cx[i]
  //   d_pin_cy[i] = d_inst_cy[d_pin_inst_id[i]] + d_pin_offset_cy[i]
  // Must be called after syncInstCoordsFromHost() and before any consumer
  // (HPWL bbox, WA gradient, ...) reads d_pin_cx / d_pin_cy.
  void updatePinLocations();

  // Re-push per-net total weights to the device. Net weights change only on
  // the timing-driven / routability-driven boundary, not inside the Nesterov
  // inner loop, so they are loaded once at construction. This API exists as
  // a TODO hook for those boundary callers — currently no caller wires it.
  // FIXME(phase 2): hook from rsz/grt-driven net-weight update path.
  void refreshNetWeights(const std::vector<GNet>& gNetStor);

  // Re-push per-inst density params (half_dx, half_dy, density_scale) after
  // the resize callback changes them. Static during the main Nesterov loop.
  // FIXME(phase 3): hook from resize callback path.
  void refreshDensityParams(const std::vector<GCell>& gCellStor);

  // Counts (for backends to size their own per-net / per-pin buffers).
  int numInsts() const;
  int numPins() const;
  int numNets() const;
  int numBins() const;

  // Bin grid geometry (for kernels that compute bin indices on-the-fly).
  int binCntX() const { return bin_cnt_x_; }
  int binCntY() const { return bin_cnt_y_; }
  float binSizeX() const { return bin_size_x_; }
  float binSizeY() const { return bin_size_y_; }
  int gridLx() const { return grid_lx_; }
  int gridLy() const { return grid_ly_; }

  // Accessor for Kokkos-aware backend translation units. Consumers must
  // also #include "deviceState_kokkos.h" to use the returned reference.
  KokkosDeviceState& kokkos() { return *kokkos_; }
  const KokkosDeviceState& kokkos() const { return *kokkos_; }

 private:
  std::unique_ptr<KokkosDeviceState> kokkos_;

  // Cached host-side sizes; used by numInsts/Pins/Nets without needing to
  // include the Kokkos header.
  int num_insts_ = 0;
  int num_pins_ = 0;
  int num_nets_ = 0;
  int num_bins_ = 0;

  // Bin grid geometry (plain scalars, no Kokkos dependency).
  int bin_cnt_x_ = 0;
  int bin_cnt_y_ = 0;
  float bin_size_x_ = 0;
  float bin_size_y_ = 0;
  int grid_lx_ = 0;
  int grid_ly_ = 0;
};

}  // namespace gpl
