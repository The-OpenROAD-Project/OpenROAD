// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
#pragma once

namespace drt {

// A stubborn-tiles worker is one of nine cost variants of its tile, and in DRC
// ripup mode a worker that ends with more markers than it started with is not
// written back (FlexDRWorker::end). So a variant whose markers have grown far
// past its initial count stops routing instead of running on (#11992): it
// counts its markers every kMarkerGrowthCheckPeriod routed nets and stops once
// they exceed markerGrowthLimit(initial markers).
inline constexpr int kMarkerGrowthCheckPeriod = 50;

inline int markerGrowthLimit(int init_markers)
{
  return 10 * init_markers + 20;
}

inline bool isMarkerGrowthCheck(int num_routed)
{
  return num_routed > 0 && num_routed % kMarkerGrowthCheckPeriod == 0;
}

}  // namespace drt
