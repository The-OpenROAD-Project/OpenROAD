// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "odb/geom.h"
#include "sta/NetworkClass.hh"

namespace sta {
class LibertyCell;
}  // namespace sta

namespace rsz {

// Candidate that inserts one buffer on the driver net and rewires a selected
// load subset to the buffer output.
//
// Unlike BufferCandidate (which uses the Rebuffer engine), SplitLoad
// performs a simpler insertion: it creates one buffer at drvr_loc_, reconnects
// the load pins in load_pins_ to the buffer output, and then optionally
// resizes the inserted buffer.  Useful for high-fanout nets where the full
// Rebuffer tree-search is too expensive.  Single-threaded only.
class SplitLoadCandidate : public MoveCandidate
{
 public:
  // === Construction =========================================================
  SplitLoadCandidate(Resizer& resizer,
                     const Target& target,
                     sta::Net* drvr_net,
                     sta::LibertyCell* buffer_cell,
                     const odb::Point& drvr_loc,
                     std::unique_ptr<sta::PinSet> load_pins);
  ~SplitLoadCandidate() override;

  // === MoveCandidate API ====================================================
  MoveResult apply() override;
  MoveType type() const override { return MoveType::kSplitLoad; }

 private:
  // === Split-buffer application helpers ====================================
  int resizeInsertedBuffer(sta::Instance* buffer) const;
  void invalidateAffectedParasitics(sta::Instance* buffer) const;
  MoveResult applySplitBuffer();

  // === Candidate state ======================================================
  sta::Net* drvr_net_{nullptr};
  sta::LibertyCell* buffer_cell_{nullptr};
  odb::Point drvr_loc_;
  std::unique_ptr<sta::PinSet> load_pins_;
};

}  // namespace rsz
