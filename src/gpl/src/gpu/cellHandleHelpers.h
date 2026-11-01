// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Small shared helpers for GPU gradient backends.
//
// Both GpuWirelengthGradientBackend and GpuDensityGradientBackend gather
// per-inst gradients from a host-mirror View, but the input vector mixes
// NesterovBaseCommon cells (indexed into the device buffer) with
// NesterovBase-local filler cells (not in DeviceState — backend-specific
// fallback). mapNbcGrads centralizes the dispatch so each backend only
// defines the two leaf lookups (NBC lookup + filler fallback).
//
// Header is Kokkos-free on purpose: callers wrap their Kokkos host-mirror
// reads in a plain callable before passing it in, so this header is safe
// to include from any TU.

#pragma once

#include <cstddef>
#include <vector>

#include "nesterovBase.h"
#include "point.h"

namespace gpl {

// For each GCellHandle, write a FloatPoint to out[i]:
//   - NesterovBaseCommon cell: nbcLookup(storage_index)
//   - Filler (NesterovBase-local): fillerFallback(gCells[i])
//
// out must already be sized to gCells.size() (mirrors the caller contract
// in WirelengthGradient::getCellGradients / DensityGradient::getCellGradients).
template <typename NbcLookup, typename FillerFallback>
inline void mapNbcGrads(const std::vector<GCellHandle>& gCells,
                        NbcLookup nbcLookup,
                        FillerFallback fillerFallback,
                        std::vector<FloatPoint>& out)
{
  for (std::size_t i = 0; i < gCells.size(); ++i) {
    if (!gCells[i].isNesterovBaseCommon()) {
      out[i] = fillerFallback(gCells[i]);
      continue;
    }
    out[i] = nbcLookup(gCells[i].getStorageIndex());
  }
}

}  // namespace gpl
