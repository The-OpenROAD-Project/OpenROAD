// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <vector>

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "odb/geom.h"
#include "sta/NetworkClass.hh"

namespace odb {
class dbITerm;
}

namespace sta {
class LibertyCell;
}

namespace rsz {

// Candidate that duplicates a driver instance (gate cloning) and reassigns
// a selected load subset to the clone's output.
//
// apply() creates the clone, copies all input connections, rewires the
// selected load pins to the clone output, and updates parasitic estimates.
// The clone placement (clone_loc_) is pre-computed by the generator as the
// centroid of the moved loads.  Single-threaded only; mutates OpenDB.
class CloneCandidate : public MoveCandidate
{
 public:
  // === Construction =========================================================
  CloneCandidate(Resizer& resizer,
                 const Target& target,
                 sta::Pin* drvr_pin,
                 sta::Instance* drvr_inst,
                 sta::Instance* parent,
                 sta::LibertyCell* original_cell,
                 sta::LibertyCell* clone_cell,
                 const odb::Point& clone_loc,
                 std::vector<sta::Pin*> moved_loads);

  // === MoveCandidate API ====================================================
  MoveResult apply() override;
  MoveType type() const override { return MoveType::kClone; }

 private:
  // === Clone application helpers ===========================================
  bool connectCloneInputs();
  bool connectCloneOutput();
  bool moveLoads();
  bool rewireCloneTopology();
  MoveResult applyClone();

  // === Candidate state ======================================================
  sta::Pin* drvr_pin_{nullptr};
  sta::Instance* drvr_inst_{nullptr};
  sta::Instance* clone_inst_{nullptr};
  sta::Instance* parent_{nullptr};
  sta::LibertyCell* original_cell_{nullptr};
  sta::LibertyCell* clone_cell_{nullptr};
  sta::Pin* clone_output_pin_{nullptr};
  odb::dbITerm* clone_output_iterm_{nullptr};
  sta::Net* out_net_{nullptr};
  odb::Point clone_loc_;
  std::vector<sta::Pin*> moved_loads_;
};

}  // namespace rsz
