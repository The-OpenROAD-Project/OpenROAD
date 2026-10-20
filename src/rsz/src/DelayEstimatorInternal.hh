// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include "OptimizerTypes.hh"

namespace sta {
class LibertyCell;
}  // namespace sta

// Helpers DelayEstimator.cc shares with the buffer-tree estimator in
// DelayEstimatorBufferTree.cc.  Not an API for other files.
namespace rsz::estimator_detail {

inline constexpr float kMinSlewRatioDenominator = 1.0e-15f;
inline constexpr float kElmoreSlewFactor = 1.39f;

// Delay and output slew of `cell` on the selected arc.
bool lookupArcDelayAndSlew(const SelectedArc& arc,
                           float input_slew,
                           float load_cap,
                           const sta::LibertyCell* cell,
                           float& delay,
                           float& output_slew,
                           ArcMatchMode match_mode = ArcMatchMode::kExact,
                           bool* match_relaxed_out = nullptr);
// Delay and output slew of the inserted buffer on the target transition.
bool lookupBufferDelayAndSlew(const DelayStageState& target_stage,
                              const sta::LibertyCell* buffer_cell,
                              float input_slew,
                              float load_cap,
                              float& delay,
                              float& output_slew);
// Calibrated output slew of `stage` with `cell` driving load_cap.
float estimateOutputSlew(const DelayStageState& stage,
                         const sta::LibertyCell* cell,
                         float path_input_slew,
                         float load_cap,
                         float selected_output_slew,
                         ArcMatchMode merge_arc_mode = ArcMatchMode::kExact,
                         bool* slew_relaxed_out = nullptr);
// Receiver input slew for a new driver output slew, keeping the current
// wire degradation ratio.
float estimateReceiverInputSlew(const DelayStageState& driver_stage,
                                const DelayStageState& receiver_stage,
                                float candidate_driver_output_slew);
float wireElmoreDelay(const WireRC& wire, float load_cap);
DelayEstimate makeEstimate(float current_delay, float candidate_delay);

}  // namespace rsz::estimator_detail
