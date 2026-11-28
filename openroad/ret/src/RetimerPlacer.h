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

struct PlaceStats
{
  int placed = 0;
  int resized = 0;  // new registers that changed cell
};

// Places the cells retime created at the middle of the placed cells they
// connect to and chooses the cell of each new register.
class RetimerPlacer
{
 public:
  RetimerPlacer(sta::dbSta* sta,
                est::EstimateParasitics* estimate_parasitics,
                rsz::Resizer* resizer);

  // Once per retime call: the libraries and the don't-use cells can change.
  void prepare();
  // A cell with no placed cell on its nets stays unplaced.
  PlaceStats placeNewCells(const std::vector<odb::dbInst*>& cells,
                           est::IncrementalParasiticsGuard& guard);
  std::vector<odb::dbMaster*> masterAndEquivalents(odb::dbMaster* master);

 private:
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
  };

  // nullopt when no cell it connects to is placed.
  std::optional<odb::Rect> neighborBox(odb::dbInst* inst) const;
  void centerAt(odb::dbInst* inst, const odb::Point& center);

  // Returns how many registers changed cell.
  int chooseCells(const std::vector<odb::dbInst*>& registers,
                  est::IncrementalParasiticsGuard& guard);
  std::vector<CellChoice> cellChoices(
      const std::vector<odb::dbInst*>& registers);
  // Every register tries its candidate of this rank in one STA update.
  void tryRank(std::vector<CellChoice>& choices,
               size_t rank,
               est::IncrementalParasiticsGuard& guard);
  static int swapToBest(const std::vector<CellChoice>& choices);
  PinSlack pinSlack(odb::dbInst* inst) const;
  static bool betterSlack(const PinSlack& a, const PinSlack& b);

  // The other cells of the same function and site, smallest first.
  const std::vector<odb::dbMaster*>& equivalentMasters(odb::dbMaster* master);
  std::vector<odb::dbMaster*> findEquivalentMasters(
      odb::dbMaster* master) const;

  sta::dbSta* sta_ = nullptr;
  est::EstimateParasitics* estimate_parasitics_ = nullptr;
  rsz::Resizer* resizer_ = nullptr;
  std::unordered_map<odb::dbMaster*, std::vector<odb::dbMaster*>>
      equivalent_masters_;
};

}  // namespace ret
