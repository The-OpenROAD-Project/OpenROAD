// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <functional>
#include <string>
#include <unordered_set>

#include "RetimerPlacer.h"
#include "odb/db.h"

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

// STA, not the search's delay model, decides what is kept: the slides are
// applied, the new cells placed and the design timed.  They are kept when
// the slack improves with neither the worst nor the total negative slack
// below the start; else ever shorter prefixes are tried, each half the last.
class StaCheck
{
 public:
  // Runs the search with a slide limit (<= 0: none) and applies it.  Returns
  // the number of slides.
  using Search = std::function<int(int max_moves)>;

  StaCheck(sta::dbSta* sta,
           utl::Logger* logger,
           RetimerPlacer* placer,
           odb::dbBlock* block,
           est::IncrementalParasiticsGuard* guard,
           Search search);

  // The first round is the whole search; each later round is a short search
  // from what was kept.
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
    StaSlack slack;
    PlaceStats placed;
  };

  Round runRound(int max_moves);
  // The slide limit of the prefix STA judges best; 0: none is better than
  // the start.
  int bestPrefix(int slides, const StaSlack& before);
  // Leaves the edit open.  `before`: the slack of a trial without slides.
  Trial runTrial(int limit, const StaSlack& before);
  void undoTrial();
  void keepTrial(const Trial& trial, Round& round);
  void reportRow(const std::string& round,
                 const std::string& proposed,
                 const std::string& trials,
                 const std::string& kept,
                 const StaSlack& slack,
                 int registers) const;
  std::unordered_set<odb::dbInst*> unplacedCells() const;
  int registerCount() const;

  sta::dbSta* sta_ = nullptr;
  utl::Logger* logger_ = nullptr;
  RetimerPlacer* placer_ = nullptr;
  odb::dbBlock* block_ = nullptr;
  est::IncrementalParasiticsGuard* guard_ = nullptr;
  Search search_;
  int trials_ = 0;
};

}  // namespace ret
