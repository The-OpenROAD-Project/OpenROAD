// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "odb/db.h"

namespace sta {
class dbSta;
}  // namespace sta
namespace est {
class EstimateParasitics;
class IncrementalParasiticsGuard;
}  // namespace est
namespace rsz {
class Resizer;
}

namespace ret {

// Slack differences below this are ties.
inline constexpr float kStaSlackEps = 1e-13f;

struct StaSlack
{
  float worst = 0.0f;
  float total = 0.0f;
};
// Brings the wire estimates up to date first.
StaSlack staSlack(sta::dbSta* sta, est::IncrementalParasiticsGuard& guard);

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
  // `skip`: the cells that were unplaced before this trial.
  PlaceStats placeNewCells(odb::dbBlock* block,
                           const std::unordered_set<odb::dbInst*>& skip,
                           est::IncrementalParasiticsGuard& guard);
  // The cells a register of this cell can be built from.
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
  struct TableInputs
  {
    float clock_slew = 0.0f;
    float data_slew = 0.0f;
    float load = 0.0f;
  };

  // The box around the placed cells `inst` connects to, leaving out a
  // register's clock and control nets; false when there is none yet.
  bool neighborBox(odb::dbInst* inst, odb::Rect& box) const;
  // Centers the cell on (x, y) and marks it placed.
  void moveTo(odb::dbInst* inst, int x, int y);

  // Returns how many registers changed cell.  Neighboring registers read
  // each other's trial cells, so the choice is kept only if STA does not
  // call the result worse.
  int chooseCells(const std::vector<odb::dbInst*>& registers,
                  est::IncrementalParasiticsGuard& guard);
  // The registers with negative slack, each with its candidates.
  std::vector<CellChoice> cellChoices(
      const std::vector<odb::dbInst*>& registers,
      est::IncrementalParasiticsGuard& guard);
  // Every register tries its candidate of this rank in one STA update.
  void tryRank(std::vector<CellChoice>& choices,
               size_t rank,
               est::IncrementalParasiticsGuard& guard);
  // Returns how many registers changed cell.
  static int swapToBest(std::vector<CellChoice>& choices);
  PinSlack pinSlack(odb::dbInst* inst, est::IncrementalParasiticsGuard& guard);
  static bool betterSlack(const PinSlack& a, const PinSlack& b);

  // The other cells of the same function and site, smallest first.
  const std::vector<odb::dbMaster*>& equivalentMasters(odb::dbMaster* master);
  // The few the liberty tables rank fastest for this register: clock-to-Q
  // at its load plus setup.
  std::vector<odb::dbMaster*> sizeCandidates(odb::dbInst* inst);
  TableInputs tableInputs(odb::dbInst* inst) const;

  sta::dbSta* sta_ = nullptr;
  est::EstimateParasitics* estimate_parasitics_ = nullptr;
  rsz::Resizer* resizer_ = nullptr;
  std::unordered_map<odb::dbMaster*, std::vector<odb::dbMaster*>>
      equivalent_masters_;
};

}  // namespace ret
