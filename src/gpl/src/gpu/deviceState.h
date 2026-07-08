// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// DeviceState — owns the device-resident pool of cell coordinates, per-pin
// offsets, and the net→pin CSR. Built per NesterovBaseCommon after the
// gCellStor_ / gPinStor_ / gNetStor_ vectors are populated; reused across
// every Nesterov iteration to keep coordinate data on the device. Rebuilt
// in place via rebuild() on the timing-driven boundary, where repair_design
// callbacks grow, shrink, and permute (swap-remove) the host storage.
//
// Consumers of this pool:
//   - HPWL: reads device pin coords directly, no host re-pack per iteration.
//   - WA wirelength gradient: same device pool + per-pin A/B/C buffers
//     (owned by the gradient backend).
//   - Density scatter+gather: same instance coords drive the density bin
//     update, and the per-inst density params + gather output live here. The
//     FFT field Views (density / phi / electric field) live per-region in
//     RegionDensityField (gpu/regionDensityField.h), not here.
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

  // Reload everything from the (mutated) host storage: sizes, CSRs, pin
  // offsets, net weights, and the per-inst density params. Called once per
  // non-virtual timing-driven iteration, after fixPointers() has restored
  // host-side consistency; the repair callbacks create, destroy (swap-remove,
  // permuting indices), and resize instances, which invalidates every
  // construction-time view and CSR here. The object identity is preserved so
  // backends holding a DeviceState* stay valid. Leaves coords invalidated;
  // the next ensureCoordsFresh() reloads coords and pin locations.
  void rebuild(const std::vector<GCell>& gCellStor,
               const std::vector<GPin>& gPinStor,
               const std::vector<GNet>& gNetStor);

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
  // the timing-driven boundary (TimingBase::executeTimingDriven), not inside
  // the Nesterov inner loop. Called via
  // NesterovBaseCommon::refreshDeviceNetWeights after every virtual TD
  // iteration; non-virtual iterations reload weights through rebuild().
  void refreshNetWeights(const std::vector<GNet>& gNetStor);

  // Re-push per-inst density params (half_dx, half_dy, density_scale) after
  // they change. Called from NesterovBase::updateDensitySize (routability
  // inflation, TD area changes); non-virtual TD iterations reload them
  // through rebuild() instead. The inst-density views are allocated at
  // construction, so this is safe to call any time after the ctor. Static
  // during the main Nesterov loop.
  void refreshDensityParams(const std::vector<GCell>& gCellStor);

  // Counts (for backends to size their own per-net / per-pin buffers).
  int numInsts() const;
  int numPins() const;
  int numNets() const;

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
  //     device pin coords (HPWL, WA gradient). No-op while coords are fresh
  //     (NB scatter ran and no host mutation invalidated them — the flag is
  //     sticky, NOT one-shot, so repeated consumers in one iteration don't
  //     clobber device coords with stale host data). Otherwise syncs from
  //     host and updates pin locations.
  //   - markCoordsFresh() — called by NesterovBase::commitCoordsToDeviceState
  //     after scatterToDeviceState + updatePinLocations.
  //   - invalidateCoords() — called by every host-side gCellStor coord
  //     mutator (updateGCellCenterLocation / updateGCellDensityCenterLocation
  //     and the db callbacks moveGCell / resizeGCell / fixPointers) so the
  //     next ensureCoordsFresh() re-syncs.
  void ensureCoordsFresh(const std::vector<GCell>& gCellStor);
  void markCoordsFresh() { coords_fresh_ = true; }
  void invalidateCoords() { coords_fresh_ = false; }

  // Accessor for Kokkos-aware backend translation units. Consumers must
  // also #include "deviceState_kokkos.h" to use the returned reference.
  KokkosDeviceState& kokkos() { return *kokkos_; }
  const KokkosDeviceState& kokkos() const { return *kokkos_; }

 private:
  // Shared by the constructor and rebuild(): (re)allocate the inst/pin/net
  // views, rebuild both CSRs, and push static attributes + initial coords.
  void buildTopology(const std::vector<GCell>& gCellStor,
                     const std::vector<GPin>& gPinStor,
                     const std::vector<GNet>& gNetStor);
  // (Re)allocate + fill the per-inst density views (half size, scale,
  // gradient). Sized num_insts_, so a rebuild() that changed the instance
  // count must re-run this. Called once from the constructor (which then sets
  // inst_density_ready_) and again from rebuild().
  void rebuildInstDensityViews(const std::vector<GCell>& gCellStor);

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

  // True once rebuildInstDensityViews has allocated the per-inst density
  // views (done in the ctor). rebuild() re-runs them only when set.
  bool inst_density_ready_ = false;
};

// Lock the "must construct via the GPU ctor" invariant at compile time so a
// future refactor that re-enables default/copy/move construction also fails
// to build instead of silently regressing the null-deleter footgun.
static_assert(!std::is_default_constructible_v<DeviceState>);
static_assert(!std::is_copy_constructible_v<DeviceState>);
static_assert(!std::is_move_constructible_v<DeviceState>);

}  // namespace gpl
