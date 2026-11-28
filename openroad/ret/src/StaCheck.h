// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <functional>
#include <string>
#include <vector>

#include "RetimerPlacer.h"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"

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

class Discards;

// The cells created since clear() that still exist.  odb reports every cell
// created or deleted, also by an undo, so the block is not read again.
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
  odb::PtrSet<odb::dbInst> created_;
  int registers_ = 0;
};

// Of the worst scene, and of each scene.  Hold: of the violations only, so
// zero when hold is met.
struct StaSlack
{
  float worst = 0.0f;
  float total = 0.0f;
  float hold_worst = 0.0f;
  float hold_total = 0.0f;
  std::vector<StaSlack> scenes;
};

// No scene is worse than at the start of the round or of the command, and
// the worst or the total setup slack is better than at the round's start.
bool keepsTrial(const StaSlack& trial,
                const StaSlack& round_start,
                const StaSlack& start);

// STA, not the search's delay model, decides what is kept.  When keepsTrial()
// refuses the slides, ever shorter prefixes are tried, each half the last.
class StaCheck
{
 public:
  using Propose = std::function<std::vector<odb::dbInst*>(int max_moves)>;
  // Returns the number of slides, negative when they were not applied.
  using Apply = std::function<int(const std::vector<odb::dbInst*>& slides,
                                  int max_moves)>;
  using Undone = std::function<void()>;

  StaCheck(sta::dbSta* sta,
           utl::Logger* logger,
           RetimerPlacer* placer,
           odb::dbBlock* block,
           est::IncrementalParasiticsGuard* guard,
           Discards* discards,
           Propose propose,
           Apply apply,
           Undone undone);

  // max_moves: slides over all rounds; <= 0: no cap.
  void run(int max_moves);

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
    StaSlack slack;  // the round's start when the slides were not applied
    PlaceStats placed;
  };
  struct Totals
  {
    int slides = 0;
    int rounds = 0;
    PlaceStats placed;
  };

  // start: the slack before the first round.
  Round runRound(int max_moves, const StaSlack& start);
  // Leaves the edit open.
  Trial runTrial(const std::vector<odb::dbInst*>& slides,
                 int max_moves,
                 const StaSlack& round_start);
  void undoTrial();
  void keepTrial(const Trial& trial, Round& round);
  StaSlack slack() const;
  // Without the discarded ones.
  int registerCount() const;
  void warnOtherScene(const StaSlack& slack) const;
  void reportStart(const StaSlack& slack, int registers) const;
  void reportRound(int number, const Round& round) const;
  void reportRow(const std::string& round,
                 const std::string& proposed,
                 const std::string& trials,
                 const std::string& kept,
                 const StaSlack& slack,
                 int registers) const;
  void reportRule() const;
  void reportTotals(const Totals& totals) const;
  std::string slackText(float slack) const;

  sta::dbSta* sta_ = nullptr;
  utl::Logger* logger_ = nullptr;
  RetimerPlacer* placer_ = nullptr;
  odb::dbBlock* block_ = nullptr;
  est::IncrementalParasiticsGuard* guard_ = nullptr;
  Discards* discards_ = nullptr;
  Propose propose_;
  Apply apply_;
  Undone undone_;
  NewCells new_cells_;
  // A driver of each net with two.  STA's incremental delay calculation
  // times such a net only from the driver it picked.
  std::vector<odb::dbITerm*> shared_drivers_;
};

}  // namespace ret
