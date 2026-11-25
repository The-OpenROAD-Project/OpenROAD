// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "StaCheck.h"

#include <cstdlib>
#include <string>
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

constexpr int kSlackDigits = 4;

// Better than b by a cost that weighs the total negative slack like the
// worst slack, and never below the start on either.
bool betterThan(const StaSlack& a, const StaSlack& b, const StaSlack& start)
{
  const double total_scale
      = start.worst < -kStaSlackEps && start.total < -kStaSlackEps
            ? static_cast<double>(start.worst) / start.total
            : 0.0;
  const auto cost = [total_scale](const StaSlack& slack) {
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
    if (inst->getMaster()->isSequential()) {
      ++count;
    }
  }
  return count;
}

std::unordered_set<std::string> StaCheck::unplacedCells() const
{
  std::unordered_set<std::string> unplaced;
  for (odb::dbInst* inst : block_->getInsts()) {
    if (!inst->isPlaced()) {
      unplaced.insert(inst->getName());
    }
  }
  return unplaced;
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

void StaCheck::run(const int max_moves)
{
  constexpr int kRounds = 16;
  constexpr int kRoundMoves = 16;
  constexpr const char* kHeader
      = " Round | Proposed | Trials | Kept |         WNS |           TNS | "
        "Registers";
  placer_->prepare();
  const StaSlack before = staSlack(sta_, *guard_);
  const int registers_before = registerCount();
  int total = 0;
  int left = max_moves;
  int rounds_kept = 0;
  PlaceStats placed;
  bool whole = true;
  bool table = false;
  for (int number = 1; rounds_kept < kRounds; ++number) {
    int limit = left;
    if (!whole && (limit <= 0 || kRoundMoves < limit)) {
      limit = kRoundMoves;
    }
    const Round round = runRound(limit);
    if (round.proposed > 0) {
      if (!table) {
        logger_->report(kHeader);
        logger_->report("{:->75}", "");
        reportRow("start", "", "", "", before, registers_before);
        table = true;
      }
      reportRow(std::to_string(number),
                std::to_string(round.proposed),
                std::to_string(round.trials),
                std::to_string(round.kept),
                round.slack,
                registerCount());
    }
    const bool was_whole = whole;
    whole = false;
    if (round.kept == 0) {
      // Nothing of the whole search kept: on to the short rounds, unless a
      // short round would propose the same slides.
      if (was_whole && round.largest > kRoundMoves) {
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
                registerCount(),
                placed.placed,
                placed.resized);
}

StaCheck::Trial StaCheck::runTrial(const int limit, const StaSlack& before)
{
  const std::unordered_set<std::string> unplaced = unplacedCells();
  Trial trial{.slides = 0, .largest = 0, .slack = before, .placed = {}};
  odb::dbDatabase::beginEco(block_);
  try {
    const SearchResult result = search_(limit);
    trial.slides = std::abs(result.slides);
    trial.largest = result.largest;
    if (result.slides > 0) {
      trial.placed = placer_->placeNewCells(block_, unplaced, *guard_);
      trial.slack = staSlack(sta_, *guard_);
      ++trials_;
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
}

void StaCheck::keepTrial(const Trial& trial, Round& round)
{
  odb::dbDatabase::commitEco(block_);
  round.kept = trial.slides;
  round.slack = trial.slack;
  round.placed = trial.placed;
}

int StaCheck::bestPrefix(const int slides, const StaSlack& before)
{
  constexpr int kMaxTrials = 12;
  int best_limit = 0;
  StaSlack best = before;
  int limit = slides / 2;
  for (int tried = 1; tried < kMaxTrials && limit > 0; ++tried) {
    const Trial trial = runTrial(limit, before);
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

StaCheck::Round StaCheck::runRound(const int max_moves)
{
  trials_ = 0;
  const StaSlack before = staSlack(sta_, *guard_);
  Round round;
  round.slack = before;
  const Trial full = runTrial(max_moves, before);
  round.proposed = full.slides;
  round.largest = full.largest;
  if (full.slides > 0 && betterThan(full.slack, before, before)) {
    keepTrial(full, round);
  } else {
    undoTrial();
    const int limit
        = full.slides > 0 ? bestPrefix(full.slides, before) : 0;
    if (limit > 0) {
      const Trial again = runTrial(limit, before);
      if (betterThan(again.slack, before, before)) {
        keepTrial(again, round);
      } else {
        undoTrial();
      }
    }
  }
  round.trials = trials_;
  return round;
}

}  // namespace ret
