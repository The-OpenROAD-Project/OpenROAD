// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "StaCheck.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

#include "RegisterUtils.h"
#include "RetimerPlacer.h"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "odb/db.h"
#include "ret/Retimer.h"
#include "sta/Delay.hh"
#include "utl/Logger.h"

namespace ret {

namespace {

constexpr int kSlackDigits = 4;

// Neither the worst nor the total negative slack below the start, for setup
// and for hold, and one of the two setup slacks above it.
bool betterThan(const StaSlack& a, const StaSlack& start)
{
  return a.worst >= start.worst - kStaSlackEps
         && a.total >= start.total - kStaSlackEps
         && a.hold_worst >= start.hold_worst - kStaSlackEps
         && a.hold_total >= start.hold_total - kStaSlackEps
         && (a.worst > start.worst + kStaSlackEps
             || a.total > start.total + kStaSlackEps);
}

}  // namespace

NewCells::NewCells(odb::dbBlock* block)
{
  for (odb::dbInst* inst : block->getInsts()) {
    if (inst->getMaster()->isSequential()) {
      ++registers_;
    }
  }
  addOwner(block);
}

void NewCells::inDbInstCreate(odb::dbInst* inst)
{
  created_.insert(inst);
  if (inst->getMaster()->isSequential()) {
    ++registers_;
  }
}

void NewCells::inDbInstDestroy(odb::dbInst* inst)
{
  created_.erase(inst);
  if (inst->getMaster()->isSequential()) {
    --registers_;
  }
}

std::vector<odb::dbInst*> NewCells::inBlockOrder() const
{
  return inIdOrder(created_);
}

StaCheck::StaCheck(sta::dbSta* sta,
                   utl::Logger* logger,
                   RetimerPlacer* placer,
                   odb::dbBlock* block,
                   est::IncrementalParasiticsGuard* guard,
                   Propose propose,
                   Apply apply,
                   Closed closed)
    : sta_(sta),
      logger_(logger),
      placer_(placer),
      block_(block),
      guard_(guard),
      propose_(std::move(propose)),
      apply_(std::move(apply)),
      closed_(std::move(closed)),
      new_cells_(block)
{
}

void StaCheck::reportStart(const StaSlack& slack, const int registers) const
{
  logger_->report(
      " Round | Proposed | Trials | Kept |         WNS |           TNS | "
      "Registers");
  logger_->report("{:->75}", "");
  reportRow("start", "", "", "", slack, registers);
}

void StaCheck::reportRound(const int number, const Round& round) const
{
  reportRow(std::to_string(number),
            std::to_string(round.proposed),
            std::to_string(round.trials),
            std::to_string(round.kept),
            round.slack,
            new_cells_.registerCount());
}

void StaCheck::reportRow(const std::string& round,
                         const std::string& proposed,
                         const std::string& trials,
                         const std::string& kept,
                         const StaSlack& slack,
                         const int registers) const
{
  logger_->report("{:>6} | {:>8} | {:>6} | {:>4} | {:>11} | {:>13} | {:>9}",
                  round,
                  proposed,
                  trials,
                  kept,
                  sta::delayAsString(slack.worst, kSlackDigits, sta_),
                  sta::delayAsString(slack.total, kSlackDigits, sta_),
                  registers);
}

void StaCheck::run(const RetimeOptions& options)
{
  placer_->prepare();
  const StaSlack before = staSlack(sta_, *guard_);
  const int registers_before = new_cells_.registerCount();
  int total = 0;
  int left = options.max_moves;  // <= 0: no cap
  int rounds_kept = 0;
  PlaceStats placed;
  bool table = false;
  for (int number = 1;; ++number) {
    const Round round = runRound(left);
    if (round.proposed > 0) {
      if (!table) {
        reportStart(before, registers_before);
        table = true;
      }
      reportRound(number, round);
    }
    if (round.kept == 0) {
      break;
    }
    ++rounds_kept;
    total += round.kept;
    placed.placed += round.placed.placed;
    placed.resized += round.placed.resized;
    if (options.max_moves > 0) {
      left -= round.kept;
      if (left <= 0) {
        break;
      }
    }
  }
  if (table) {
    logger_->report("{:->75}", "");
  }
  const StaSlack after = staSlack(sta_, *guard_);
  logger_->info(utl::RET,
                53,
                "Retime: slides {}, rounds {}, WNS {}, TNS {}, registers {}, "
                "placed {}, resized {}.",
                total,
                rounds_kept,
                sta::delayAsString(after.worst, kSlackDigits, sta_),
                sta::delayAsString(after.total, kSlackDigits, sta_),
                new_cells_.registerCount(),
                placed.placed,
                placed.resized);
}

StaCheck::Trial StaCheck::runTrial(const std::vector<odb::dbInst*>& slides,
                                   const int limit,
                                   const StaSlack& before)
{
  Trial trial{.slides = 0, .applied = false, .slack = before, .placed = {}};
  odb::dbDatabase::beginEco(block_);
  new_cells_.clear();
  try {
    const int applied = apply_(slides, limit);
    trial.slides = std::abs(applied);
    trial.applied = applied > 0;
    if (trial.applied) {
      trial.placed = placer_->placeNewCells(new_cells_.inBlockOrder(), *guard_);
      trial.slack = staSlack(sta_, *guard_);
    }
  } catch (...) {
    odb::dbDatabase::undoEco(block_);
    throw;
  }
  return trial;
}

void StaCheck::undoTrial()
{
  odb::dbDatabase::undoEco(block_);
  // The next search reads STA arcs: the restored nets need parasitics.
  guard_->update();
  closed_(/*kept=*/false);
}

void StaCheck::keepTrial(const Trial& trial, Round& round)
{
  closed_(/*kept=*/true);
  odb::dbDatabase::commitEco(block_);
  round.kept = trial.slides;
  round.slack = trial.slack;
  round.placed = trial.placed;
}

std::vector<odb::dbInst*> StaCheck::prefix(
    const std::vector<odb::dbInst*>& slides,
    const int limit)
{
  const size_t count = std::min(slides.size(), static_cast<size_t>(limit));
  return {slides.begin(), slides.begin() + count};
}

StaCheck::Round StaCheck::runRound(const int limit)
{
  Round round;
  round.slack = staSlack(sta_, *guard_);
  const StaSlack before = round.slack;
  const std::vector<odb::dbInst*> slides = propose_(limit);
  // All the slides, then ever shorter prefixes until STA accepts one.
  int j = static_cast<int>(slides.size());
  int cap = limit;
  while (j > 0) {
    const Trial trial = runTrial(prefix(slides, j), cap, before);
    round.proposed = std::max(round.proposed, trial.slides);
    round.trials += trial.applied ? 1 : 0;
    if (betterThan(trial.slack, before)) {
      keepTrial(trial, round);
      break;
    }
    undoTrial();
    j = cap = j / 2;
  }
  return round;
}

}  // namespace ret
