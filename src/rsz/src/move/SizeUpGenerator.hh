// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"

namespace sta {
class Instance;
class LibertyCell;
class LibertyPort;
class MinMax;
class Pin;
class Scene;
}  // namespace sta

namespace rsz {

// Selects a stronger replacement cell for one path driver using legacy
// drive-strength heuristics (input-pin cap, previous-stage drive, load cap).
//
// Produces a single SizeUpCandidate per target.  The replacement must be in
// the same cell family (same footprint if match_cell_footprint is set) and
// must not introduce a max-capacitance violation.  Single-threaded only
// (uses live STA queries for cap and drive).
class SizeUpGenerator : public MoveGenerator
{
 public:
  // === Construction =========================================================
  explicit SizeUpGenerator(const GeneratorContext& context);

  // === MoveGenerator API ====================================================
  MoveType type() const override { return MoveType::kSizeUp; }
  std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target) override;

 private:
  // === Driver and stage context helpers ====================================
  bool resolveDriverContext(const Target& target,
                            sta::Pin*& drvr_pin,
                            sta::Instance*& inst,
                            sta::LibertyPort*& drvr_port) const;
  bool loadStageContext(const Target& target,
                        sta::Pin* drvr_pin,
                        const sta::Scene*& scene,
                        const sta::MinMax*& min_max,
                        float& load_cap,
                        sta::LibertyPort*& in_port,
                        float& prev_drive) const;
  sta::LibertyCell* selectReplacement(sta::LibertyPort* in_port,
                                      sta::LibertyPort* drvr_port,
                                      float load_cap,
                                      float prev_drive,
                                      const sta::Scene* scene,
                                      const sta::MinMax* min_max) const;

  sta::LibertyCell* upsizeCell(sta::LibertyPort* in_port,
                               sta::LibertyPort* drvr_port,
                               float load_cap,
                               float prev_drive,
                               const sta::Scene* scene,
                               const sta::MinMax* min_max) const;
};

}  // namespace rsz
