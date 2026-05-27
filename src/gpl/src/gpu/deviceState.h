// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// DeviceState — owns the device-resident pool of cell coordinates, per-pin
// offsets, and the net→pin CSR. Built once per NesterovBaseCommon after the
// gCellStor_ / gPinStor_ / gNetStor_ vectors are populated; reused across
// every Nesterov iteration to keep coordinate data on the device.
//
// Consumers of this pool:
//   - HPWL: reads device pin coords directly, no host re-pack per iteration.
//   - WA wirelength gradient: same device pool + per-pin A/B/C buffers
//     (owned by the gradient backend).
//   - Density scatter+gather: same instance coords drive the density bin
//     update; FFT solve writes electric field Views back here.
//   - Nesterov coord update: inst coords mutate device-side via the NB
//     device context; `syncInstCoordsFromHost` is a one-time init load.
//
// PIMPL: Kokkos types are hidden in gpu/deviceState_kokkos.h, included only
// by Kokkos-aware translation units. This header is plain C++, so consumer
// TUs (nesterovBase.cpp in particular) need not be compiled by nvcc.
//
// Compiled only when ENABLE_GPU=ON.

#pragma once

#include <cstdint>
#include <memory>
#include <type_traits>
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
  // coords loaded each iter via syncInstCoordsFromHost(). The only public
  // ctor — default-construction is deleted so kokkos_ can never start out
  // null with a null deleter.
  DeviceState(const std::vector<GCell>& gCellStor,
              const std::vector<GPin>& gPinStor,
              const std::vector<GNet>& gNetStor);
  DeviceState() = delete;
  // Default destructor — the function-pointer deleter on kokkos_ (see
  // below) lets this stay inline without requiring KokkosDeviceState to be
  // complete here. CPU-only builds (no ENABLE_GPU) never construct the
  // unique_ptr, so the deleter is never invoked.
  ~DeviceState() = default;

  // Non-copyable, non-movable: the implicit move would inherit a possibly
  // null deleter from a moved-from instance, masking the "must construct
  // via the GPU ctor" invariant captured by the unique_ptr field below.
  DeviceState(const DeviceState&) = delete;
  DeviceState& operator=(const DeviceState&) = delete;
  DeviceState(DeviceState&&) = delete;
  DeviceState& operator=(DeviceState&&) = delete;

  // Allocate bin grid Views + push per-inst density params. Called once
  // from NesterovBase after the BinGrid is initialized (initDensity1).
  // Must precede any density gather kernel or GpuFftBackend solve.
  void initBinViews(const BinGrid& binGrid,
                    const std::vector<GCell>& gCellStor);

  // Re-push current instance centers (= GCell::cx()/cy()) to the device.
  // Now used only on the init path; once nb_device_ctx_ exists, that
  // context scatters fresh inst coords each iteration via
  // scatterToDeviceState and this host-side path becomes redundant.
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
  // TODO: hook from the rsz/grt-driven net-weight update path.
  void refreshNetWeights(const std::vector<GNet>& gNetStor);

  // Re-push per-inst density params (half_dx, half_dy, density_scale) after
  // the resize callback changes them. Static during the main Nesterov loop.
  // TODO: hook from the resize callback path.
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

  // Coord-sync manager. The NB device context scatters fresh inst coords
  // to the device before updateWireLengthForceWA, so a subsequent
  // host→device sync would be redundant (and lossy: gCellStor_::dCx/dCy is
  // int-truncated). The methods below encapsulate that fast-path skip so
  // HPWL and WA gradient consumers can stay symmetric.
  //
  // Thread safety: these methods are called only from the master thread
  // (Nesterov outer loop + getHpwl / updateWireLengthForceWA entry points).
  // The OMP parallel regions in the backends do not touch this flag — they
  // run after the sync decision is made. No atomic is needed.
  //
  // Usage:
  //   - ensureCoordsFresh(gCellStor) — call before any consumer that reads
  //     device pin coords (HPWL, WA gradient). No-op if coords are already
  //     fresh (NB scatter ran this iteration). Otherwise syncs from host
  //     and updates pin locations. Clears the fresh flag on exit so the
  //     next iteration's NB scatter sets it again.
  //   - markCoordsFresh() — called by NesterovBase::commitCoordsToDeviceState
  //     after scatterToDeviceState + updatePinLocations.
  //   - invalidateCoords() — call after host-side mutation of gCellStor
  //     that happens outside the Nesterov inner loop, to force the next
  //     ensureCoordsFresh() to re-sync.
  void ensureCoordsFresh(const std::vector<GCell>& gCellStor);
  void markCoordsFresh() { coords_fresh_ = true; }
  void invalidateCoords() { coords_fresh_ = false; }

  // Accessor for Kokkos-aware backend translation units. Consumers must
  // also #include "deviceState_kokkos.h" to use the returned reference.
  KokkosDeviceState& kokkos() { return *kokkos_; }
  const KokkosDeviceState& kokkos() const { return *kokkos_; }

 private:
  // Master-thread-only; see ensureCoordsFresh() for the thread-safety
  // rationale. No atomic.
  bool coords_fresh_ = false;
  // Type-erased deleter: a plain function pointer instead of
  // std::default_delete<KokkosDeviceState>. This lets ~DeviceState() be
  // synthesized in CPU-only TUs (Bazel, ENABLE_GPU=OFF) where
  // KokkosDeviceState is incomplete — the unique_ptr destructor only ever
  // calls the deleter through the stored pointer, never through a typed
  // expression that requires the impl to be complete. The deleter is set
  // by the GPU-only constructor in gpu/deviceState.cpp; default-constructed
  // unique_ptrs hold a null pointer + null deleter and never invoke it.
  using KokkosDeleter = void (*)(KokkosDeviceState*);
  std::unique_ptr<KokkosDeviceState, KokkosDeleter> kokkos_{nullptr, nullptr};

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

// Lock the "must construct via the GPU ctor" invariant at compile time so a
// future refactor that re-enables default/copy/move construction also fails
// to build instead of silently regressing the null-deleter footgun.
static_assert(!std::is_default_constructible_v<DeviceState>);
static_assert(!std::is_copy_constructible_v<DeviceState>);
static_assert(!std::is_move_constructible_v<DeviceState>);

}  // namespace gpl
