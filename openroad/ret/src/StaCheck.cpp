// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "StaCheck.h"

#include <unordered_set>
#include <utility>

#include "RetimerPlacer.h"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "odb/db.h"
#include "sta/Delay.hh"
#include "utl/Logger.h"

namespace ret {

namespace {

// Better than b by a cost that weighs the total negative slack like the
// worst slack, and never below the start on either.
bool betterThan(const StaSlack& a, const StaSlack& b, const StaSlack& start)
{
  const double total_scale
      = start.worst < -kStaSlackEps && start.total < -kStaSlackEps
            ? static_cast<double>(start.worst) / start.total
            : 0.0;
  auto cost = [total_scale](const StaSlack& slack) {
    return -static_cast<double>(slack.worst) - total_scale * slack.total;
  };
  return a.worst >= start.worst - kStaSlackEps
         && a.total >= start.total - kStaSlackEps
         && cost(a) < cost(b) - kStaSlackEps;
}

}  // namespace

StaCheck::StaCheck(sta::dbSta* sta,
                   utl::Logger* logger,
                   RetimerPlacer* placer,
                   odb::dbBlock* block,
                   est::IncrementalParasiticsGuard* guard,
                   Search search)
    : sta_(sta),
      logger_(logger),
      placer_(placer),
      block_(block),
      guard_(guard),
      search_(std::move(search))
{
}

int StaCheck::registerCount() const
{
  int count = 0;
  for (odb::dbInst* inst : block_->getInsts()) {
    count += inst->getMaster()->isSequential() ? 1 : 0;
  }
  return count;
}

std::unordered_set<odb::dbInst*> StaCheck::unplacedCells() const
{
  std::unordered_set<odb::dbInst*> unplaced;
  for (odb::dbInst* inst : block_->getInsts()) {
    if (!inst->isPlaced()) {
      unplaced.insert(inst);
    }
  }
  return unplaced;
}

int StaCheck::run(int max_moves)
{
  constexpr int kRounds = 16;
  constexpr int kRoundMoves = 16;
  placer_->prepare();
  const StaSlack before = staSlack(sta_, *guard_);
  const int registers_before = registerCount();
  logger_->info(utl::RET,
                49,
                "STA check: before retiming, worst slack {}, total negative "
                "slack {}.",
                sta::delayAsString(before.worst, 4, sta_),
                sta::delayAsString(before.total, 4, sta_));
  int total = 0;
  int left = max_moves;
  int rounds_kept = 0;
  PlaceStats placed;
  bool whole = true;
  for (int number = 1; rounds_kept < kRounds; ++number) {
    int limit = left;
    if (!whole && (limit <= 0 || kRoundMoves < limit)) {
      limit = kRoundMoves;
    }
    const Round round = runRound(number, limit);
    const bool was_whole = whole;
    whole = false;
    if (round.kept == 0) {
      // Nothing of the whole search kept: on to the short rounds.
      if (was_whole && round.proposed > 0) {
        continue;
      }
      break;
    }
    ++rounds_kept;
    total += round.kept;
    placed.placed += round.placed.placed;
    placed.resized += round.placed.resized;
    if (max_moves > 0) {
      left -= round.kept;
      if (left <= 0) {
        break;
      }
    }
  }
  const StaSlack after = staSlack(sta_, *guard_);
  logger_->info(utl::RET,
                53,
                "STA check: {} slides kept in {} rounds; worst slack {} -> {}, "
                "total negative slack {} -> {}; registers {} -> {}; {} new "
                "cells placed, {} new registers changed cell.",
                total,
                rounds_kept,
                sta::delayAsString(before.worst, 4, sta_),
                sta::delayAsString(after.worst, 4, sta_),
                sta::delayAsString(before.total, 4, sta_),
                sta::delayAsString(after.total, 4, sta_),
                registers_before,
                registerCount(),
                placed.placed,
                placed.resized);
  return total;
}

StaCheck::Trial StaCheck::runTrial(int limit, const StaSlack& before, int round)
{
  const std::unordered_set<odb::dbInst*> unplaced = unplacedCells();
  Trial trial{.slides = 0, .slack = before, .placed = {}};
  odb::dbDatabase::beginEco(block_);
  try {
    trial.slides = search_(limit);
    if (trial.slides > 0) {
      trial.placed = placer_->placeNewCells(block_, unplaced, *guard_);
      trial.slack = staSlack(sta_, *guard_);
    }
  } catch (...) {
    odb::dbDatabase::undoEco(block_);
    throw;
  }
  if (trial.slides > 0 && round > 0) {
    logger_->info(utl::RET,
                  50,
                  "STA check round {}: {} slides give worst slack {}, total "
                  "negative slack {}.",
                  round,
                  trial.slides,
                  sta::delayAsString(trial.slack.worst, 4, sta_),
                  sta::delayAsString(trial.slack.total, 4, sta_));
  }
  return trial;
}

void StaCheck::undoTrial()
{
  odb::dbDatabase::undoEco(block_);
  // The next search reads STA arcs: the restored nets need parasitics.
  guard_->update();
}

void StaCheck::keepTrial(const Trial& trial, int number, Round& round)
{
  odb::dbDatabase::commitEco(block_);
  logger_->info(utl::RET,
                51,
                "STA check round {}: {} slides kept.",
                number,
                trial.slides);
  round.kept = trial.slides;
  round.placed = trial.placed;
}

int StaCheck::bestPrefix(int slides, const StaSlack& before, int round)
{
  constexpr int kMaxTrials = 12;
  int best_limit = 0;
  StaSlack best = before;
  int limit = slides / 2;
  for (int tried = 1; tried < kMaxTrials && limit > 0; ++tried) {
    const Trial trial = runTrial(limit, before, round);
    undoTrial();
    if (trial.slides == 0) {
      break;
    }
    if (betterThan(trial.slack, best, before)) {
      best = trial.slack;
      best_limit = limit;
    }
    if (trial.slides == 1) {
      break;
    }
    limit = trial.slides / 2;
  }
  return best_limit;
}

StaCheck::Round StaCheck::runRound(int number, int max_moves)
{
  const StaSlack before = staSlack(sta_, *guard_);
  const Trial full = runTrial(max_moves, before, number);
  Round round{.kept = 0, .proposed = full.slides, .placed = {}};
  if (full.slides == 0) {
    undoTrial();
    logger_->info(utl::RET,
                  52,
                  "STA check round {}: the search proposes no slide.",
                  number);
    staSlack(sta_, *guard_);
    return round;
  }
  if (betterThan(full.slack, before, before)) {
    keepTrial(full, number, round);
    return round;
  }
  undoTrial();
  const int limit = bestPrefix(full.slides, before, number);
  if (limit > 0) {
    // Run again; kept only if STA calls it better than the start again.
    const Trial again = runTrial(limit, before, 0);
    if (betterThan(again.slack, before, before)) {
      keepTrial(again, number, round);
      return round;
    }
    undoTrial();
  }
  logger_->info(utl::RET,
                54,
                "STA check round {}: no number of slides is better than "
                "the start; nothing kept.",
                number);
  staSlack(sta_, *guard_);
  return round;
}

}  // namespace ret
