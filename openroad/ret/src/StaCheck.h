// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

#include "RetimerPlacer.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "ret/Retimer.h"

namespace sta {
class dbSta;
}  // namespace sta
namespace est {
class IncrementalParasiticsGuard;
}  // namespace est
namespace utl {
class Logger;
}

namespace ret {

// The cells created since clear() that still exist, and the number of
// registers in the block.  odb calls it for every cell created or deleted,
// also by an undo, so neither is found by reading the block again.
class NewCells : public odb::dbBlockCallBackObj
{
 public:
  explicit NewCells(odb::dbBlock* block);

  void inDbInstCreate(odb::dbInst* inst) override;
  void inDbInstDestroy(odb::dbInst* inst) override;

  void clear() { created_.clear(); }
  std::vector<odb::dbInst*> inBlockOrder() const;
  int registerCount() const { return registers_; }

 private:
  std::unordered_set<odb::dbInst*> created_;
  int registers_ = 0;
};

// STA, not the search's delay model, decides what is kept: the slides are
// applied, the new cells placed, and the design timed.  They are kept when
// the setup slack improves with neither the worst nor the total negative
// slack below the start, for setup and for hold; else ever shorter prefixes
// are tried, each half the last.
class StaCheck
{
 public:
  // The gates a search with a slide limit (<= 0: none) slides across.
  using Propose = std::function<std::vector<odb::dbInst*>(int max_moves)>;
  // Applies slides; max_moves caps a search the unmapping repeats.
  using Apply = std::function<int(const std::vector<odb::dbInst*>& slides,
                                  int max_moves)>;
  // After a trial is undone, or before a kept one is committed.
  using Closed = std::function<void(bool kept)>;

  StaCheck(sta::dbSta* sta,
           utl::Logger* logger,
           RetimerPlacer* placer,
           odb::dbBlock* block,
           est::IncrementalParasiticsGuard* guard,
           Propose propose,
           Apply apply,
           Closed closed);

  // Rounds repeat from what was kept until one keeps nothing.
  void run(const RetimeOptions& options);

 private:
  struct Round
  {
    StaSlack slack;
    int kept = 0;
    int proposed = 0;
    int trials = 0;
    PlaceStats placed;
  };
  struct Trial
  {
    int slides = 0;
    bool applied = false;
    StaSlack slack;  // the slack before when the slides were not applied
    PlaceStats placed;
  };

  Round runRound(int limit);
  static std::vector<odb::dbInst*> prefix(
      const std::vector<odb::dbInst*>& slides,
      int limit);
  // Leaves the edit open.
  Trial runTrial(const std::vector<odb::dbInst*>& slides,
                 int limit,
                 const StaSlack& before);
  void undoTrial();
  void keepTrial(const Trial& trial, Round& round);
  void reportStart(const StaSlack& slack, int registers) const;
  void reportRound(int number, const Round& round) const;
  void reportRow(const std::string& round,
                 const std::string& proposed,
                 const std::string& trials,
                 const std::string& kept,
                 const StaSlack& slack,
                 int registers) const;

  sta::dbSta* sta_ = nullptr;
  utl::Logger* logger_ = nullptr;
  RetimerPlacer* placer_ = nullptr;
  odb::dbBlock* block_ = nullptr;
  est::IncrementalParasiticsGuard* guard_ = nullptr;
  Propose propose_;
  Apply apply_;
  Closed closed_;
  NewCells new_cells_;
};

}  // namespace ret
