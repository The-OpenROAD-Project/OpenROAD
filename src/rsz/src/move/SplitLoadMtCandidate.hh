// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "DelayEstimator.hh"
#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "odb/geom.h"
#include "rsz/Resizer.hh"
#include "sta/NetworkClass.hh"

namespace sta {
class Instance;
class LibertyCell;
class Pin;
}  // namespace sta

namespace rsz {

// The split prepareInputs() chose for one target: the loads the new buffer
// takes over, the cell it is inserted as, the cell apply() resizes it to,
// and the loading change that scores the move.  The wire capacitances are
// estimated from pin bounding boxes.
struct SplitLoadMtInput
{
  sta::Net* drvr_net{nullptr};
  odb::Point drvr_loc;
  sta::PinSeq load_pins;
  sta::LibertyCell* buffer_cell{nullptr};
  sta::LibertyCell* resized_cell{nullptr};
  // Loads moved from the original driver to the new buffer.
  std::vector<SideLoad> moved_loads;
  // Pin capacitance of the moved loads.
  float moved_pin_cap{0.0f};
  // Wire capacitance the target net no longer drives.
  float target_wire_cap_saved{0.0f};
  // Wire capacitance of the new net from the new driver to the moved loads.
  float moved_wire_cap{0.0f};
  // The target driver drives the buffer input in place of the moved loads
  // and the wire to them.
  float target_load_delta{0.0f};
};

// MT-safe split-load move: one buffer at the driver takes the less critical
// half of the loads.  The target path gains from the lighter load.  The moved
// loads pay the buffer delay, charged like fanin side loads.
class SplitLoadMtCandidate : public MoveCandidate
{
 public:
  // === Construction =========================================================
  SplitLoadMtCandidate(Resizer& resizer,
                       const Target& target,
                       std::shared_ptr<const SplitLoadMtInput> input,
                       float score_scale);

  // === MoveCandidate API ====================================================
  Estimate estimate() override;
  MoveResult apply() override;
  MoveType type() const override { return MoveType::kSplitLoad; }
  MoveImpact impact() const override;
  std::vector<const sta::Pin*> guardedSidePins() const override;

 private:
  sta::Pin* bufferOutputPin(sta::Instance* buffer) const;

  // === Candidate state ======================================================
  std::shared_ptr<const SplitLoadMtInput> input_;
  float score_scale_{1.0f};
};

}  // namespace rsz
