// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <optional>
#include <unordered_map>
#include <vector>

#include "odb/db.h"
#include "odb/geom.h"

namespace sta {
class dbSta;
}  // namespace sta
namespace est {
class EstimateParasitics;
class IncrementalParasiticsGuard;
}  // namespace est
namespace rsz {
class Resizer;
}  // namespace rsz

namespace ret {

// Slack differences below this are ties.
inline constexpr float kStaSlackEps = 1e-13f;

// Setup: the worst slack and the total negative slack.  Hold: the same of
// the violations only, so both are zero when hold is met.  Of the worst
// scene, as STA reports them, and of each scene.
struct StaSlack
{
  float worst = 0.0f;
  float total = 0.0f;
  float hold_worst = 0.0f;
  float hold_total = 0.0f;
  std::vector<StaSlack> scenes;
};
// Brings the wire estimates up to date first.  shared_drivers: a driver of
// each net with two, timed again here: STA's incremental delay calculation
// times such a net only from the driver it picked, so a new slew at the
// input of another one would go unseen.
StaSlack staSlack(sta::dbSta* sta,
                  est::IncrementalParasiticsGuard& guard,
                  const std::vector<odb::dbITerm*>& shared_drivers);

struct PlaceStats
{
  int placed = 0;
  int resized = 0;  // new registers that changed cell
};

// Places the cells retime created at the middle of the placed cells they
// connect to and chooses the cell of each new register.  Detailed placement
// legalizes them later.
class RetimerPlacer
{
 public:
  RetimerPlacer(sta::dbSta* sta,
                est::EstimateParasitics* estimate_parasitics,
                rsz::Resizer* resizer);

  // Once before the first placeNewCells of a design.
  void prepare();
  // The libraries and the don't-use cells can change between retime calls.
  void forgetCells() { equivalent_masters_.clear(); }
  // A cell with no placed cell on its nets stays unplaced.
  PlaceStats placeNewCells(const std::vector<odb::dbInst*>& cells,
                           est::IncrementalParasiticsGuard& guard);
  std::vector<odb::dbMaster*> registerCells(odb::dbMaster* master);

 private:
  // The worst STA slack over a register's signal pins, then their sum.
  struct PinSlack
  {
    float worst = 0.0f;
    float sum = 0.0f;
  };
  struct CellChoice
  {
    odb::dbInst* inst;
    odb::dbMaster* original;
    std::vector<odb::dbMaster*> candidates;
    PinSlack best;
    odb::dbMaster* best_master;
    bool swapped;
  };

  // The box around the placed cells `inst` connects to, leaving out a
  // register's clock and control nets; nullopt when none is placed yet.
  std::optional<odb::Rect> neighborBox(odb::dbInst* inst) const;
  void moveTo(odb::dbInst* inst, int x, int y);

  // Returns how many registers changed cell.
  int chooseCells(const std::vector<odb::dbInst*>& registers,
                  est::IncrementalParasiticsGuard& guard);
  std::vector<CellChoice> cellChoices(
      const std::vector<odb::dbInst*>& registers);
  // Every register tries its candidate of this rank in one STA update.
  void tryRank(std::vector<CellChoice>& choices,
               size_t rank,
               est::IncrementalParasiticsGuard& guard);
  static int swapToBest(std::vector<CellChoice>& choices);
  PinSlack pinSlack(odb::dbInst* inst) const;
  static bool betterSlack(const PinSlack& a, const PinSlack& b);

  // The other cells of the same function and site, smallest first.
  const std::vector<odb::dbMaster*>& equivalentMasters(odb::dbMaster* master);

  sta::dbSta* sta_ = nullptr;
  est::EstimateParasitics* estimate_parasitics_ = nullptr;
  rsz::Resizer* resizer_ = nullptr;
  std::unordered_map<odb::dbMaster*, std::vector<odb::dbMaster*>>
      equivalent_masters_;
};

}  // namespace ret
