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
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "odb/db.h"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/MinMax.hh"
#include "sta/Scene.hh"
#include "utl/Logger.h"

namespace ret {

namespace {

constexpr int kSlackDigits = 4;

bool notWorse(const StaSlack& a, const StaSlack& start)
{
  return a.worst >= start.worst - kStaSlackEps
         && a.total >= start.total - kStaSlackEps
         && a.hold_worst >= start.hold_worst - kStaSlackEps
         && a.hold_total >= start.hold_total - kStaSlackEps;
}

std::vector<odb::dbITerm*> sharedNetDrivers(odb::dbBlock* block)
{
  std::vector<odb::dbITerm*> drivers;
  for (odb::dbNet* net : block->getNets()) {
    if (net->getSigType().isSupply() || !hasSecondDriver(net)) {
      continue;
    }
    odb::dbITerm* driver = net->getFirstOutput();
    if (driver != nullptr) {
      drivers.push_back(driver);
    }
  }
  return drivers;
}

StaSlack sceneSlack(sta::dbSta* sta, const sta::Scene* scene)
{
  const sta::MinMax* setup = sta::MinMax::max();
  const sta::MinMax* hold = sta::MinMax::min();
  sta::Slack worst;
  sta::Slack hold_worst;
  sta::Vertex* vertex;
  sta->worstSlack(scene, setup, worst, vertex);
  sta->worstSlack(scene, hold, hold_worst, vertex);
  StaSlack slack;
  slack.worst = sta::delayAsFloat(worst);
  slack.total = sta::delayAsFloat(sta->totalNegativeSlack(scene, setup));
  slack.hold_worst = std::min(sta::delayAsFloat(hold_worst), 0.0f);
  slack.hold_total = sta::delayAsFloat(sta->totalNegativeSlack(scene, hold));
  return slack;
}

std::vector<odb::dbInst*> prefix(const std::vector<odb::dbInst*>& slides,
                                 const int count)
{
  const size_t length = std::min(slides.size(), static_cast<size_t>(count));
  return {slides.begin(), slides.begin() + length};
}

}  // namespace

bool keepsTrial(const StaSlack& trial,
                const StaSlack& round_start,
                const StaSlack& start)
{
  for (size_t i = 0; i < trial.scenes.size(); ++i) {
    if (!notWorse(trial.scenes[i], round_start.scenes[i])
        || !notWorse(trial.scenes[i], start.scenes[i])) {
      return false;
    }
  }
  return trial.worst > round_start.worst + kStaSlackEps
         || trial.total > round_start.total + kStaSlackEps;
}

NewCells::NewCells(odb::dbBlock* block)
{
  registers_ = static_cast<int>(std::ranges::count_if(
      block->getInsts(),
      [](odb::dbInst* inst) { return inst->getMaster()->isSequential(); }));
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
  return {created_.begin(), created_.end()};
}

StaCheck::StaCheck(sta::dbSta* sta,
                   utl::Logger* logger,
                   RetimerPlacer* placer,
                   odb::dbBlock* block,
                   est::IncrementalParasiticsGuard* guard,
                   Discards* discards,
                   Propose propose,
                   Apply apply,
                   Undone undone)
    : sta_(sta),
      logger_(logger),
      placer_(placer),
      block_(block),
      guard_(guard),
      discards_(discards),
      propose_(std::move(propose)),
      apply_(std::move(apply)),
      undone_(std::move(undone)),
      new_cells_(block)
{
}

void StaCheck::warnOtherScene(const StaSlack& slack) const
{
  const sta::SceneSeq& scenes = sta_->scenes();
  const sta::Scene* current = sta_->cmdScene();
  const sta::Scene* failing = nullptr;
  for (size_t i = 0; i < scenes.size(); ++i) {
    const bool fails = slack.scenes[i].worst < -kStaSlackEps;
    if (fails && scenes[i] == current) {
      return;
    }
    if (fails && failing == nullptr) {
      failing = scenes[i];
    }
  }
  if (failing != nullptr) {
    logger_->warn(utl::RET,
                  50,
                  "Scene {} fails timing, but the search reads the current "
                  "scene, {}, which meets it; make the failing one current "
                  "with set_scene.",
                  failing->name(),
                  current->name());
  }
}

void StaCheck::reportStart(const StaSlack& slack, const int registers) const
{
  logger_->report(
      " Round | Proposed | Trials | Kept |         WNS |           TNS | "
      "Registers");
  reportRule();
  reportRow("start", "", "", "", slack, registers);
}

void StaCheck::reportRound(const int number, const Round& round) const
{
  reportRow(std::to_string(number),
            std::to_string(round.proposed),
            std::to_string(round.trials),
            std::to_string(round.kept),
            round.slack,
            registerCount());
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
                  slackText(slack.worst),
                  slackText(slack.total),
                  registers);
}

void StaCheck::reportRule() const
{
  logger_->report("{:->75}", "");
}

void StaCheck::reportTotals(const Totals& totals) const
{
  const StaSlack final_slack = slack();
  logger_->info(utl::RET,
                53,
                "Retime: slides {}, rounds {}, WNS {}, TNS {}, registers {}, "
                "placed {}, resized {}.",
                totals.slides,
                totals.rounds,
                slackText(final_slack.worst),
                slackText(final_slack.total),
                registerCount(),
                totals.placed.placed,
                totals.placed.resized);
}

std::string StaCheck::slackText(const float slack) const
{
  return sta::delayAsString(slack, kSlackDigits, sta_);
}

void StaCheck::run(const int max_moves)
{
  placer_->prepare();
  shared_drivers_ = sharedNetDrivers(block_);
  const StaSlack start = slack();
  warnOtherScene(start);
  const int registers_at_start = registerCount();
  Totals totals;
  int moves_left = max_moves;
  bool table = false;
  for (int number = 1;; ++number) {
    const Round round = runRound(moves_left, start);
    if (round.proposed > 0) {
      if (!table) {
        reportStart(start, registers_at_start);
        table = true;
      }
      reportRound(number, round);
    }
    if (round.kept == 0) {
      break;
    }
    ++totals.rounds;
    totals.slides += round.kept;
    totals.placed.placed += round.placed.placed;
    totals.placed.resized += round.placed.resized;
    if (max_moves > 0) {
      moves_left -= round.kept;
      if (moves_left <= 0) {
        break;
      }
    }
  }
  if (table) {
    reportRule();
  }
  reportTotals(totals);
}

StaCheck::Round StaCheck::runRound(const int max_moves, const StaSlack& start)
{
  Round round;
  round.slack = slack();
  const StaSlack round_start = round.slack;
  const std::vector<odb::dbInst*> slides = propose_(max_moves);
  int length = static_cast<int>(slides.size());
  // The first trial keeps the caller's cap, which may be none; a shorter
  // one is capped at its own length.
  int cap = max_moves;
  while (length > 0) {
    const Trial trial = runTrial(prefix(slides, length), cap, round_start);
    round.proposed = std::max(round.proposed, trial.slides);
    round.trials += trial.applied ? 1 : 0;
    if (keepsTrial(trial.slack, round_start, start)) {
      keepTrial(trial, round);
      break;
    }
    undoTrial();
    length = cap = length / 2;
  }
  return round;
}

StaCheck::Trial StaCheck::runTrial(const std::vector<odb::dbInst*>& slides,
                                   const int max_moves,
                                   const StaSlack& round_start)
{
  Trial trial;
  trial.slack = round_start;
  odb::dbDatabase::beginEco(block_);
  new_cells_.clear();
  try {
    const int applied = apply_(slides, max_moves);
    trial.slides = std::abs(applied);
    trial.applied = applied > 0;
    if (trial.applied) {
      trial.placed = placer_->placeNewCells(new_cells_.inBlockOrder(), *guard_);
      trial.slack = slack();
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
  discards_->undo();
  undone_();
}

void StaCheck::keepTrial(const Trial& trial, Round& round)
{
  discards_->keep();
  odb::dbDatabase::commitEco(block_);
  round.kept = trial.slides;
  round.slack = trial.slack;
  round.placed = trial.placed;
}

StaSlack StaCheck::slack() const
{
  guard_->update();
  sta::dbNetwork* network = sta_->getDbNetwork();
  for (odb::dbITerm* driver : shared_drivers_) {
    sta_->delaysInvalidFrom(network->dbToSta(driver));
  }
  StaSlack all;
  all.worst = sta::INF;
  for (const sta::Scene* scene : sta_->scenes()) {
    const StaSlack of_scene = sceneSlack(sta_, scene);
    all.worst = std::min(all.worst, of_scene.worst);
    all.total = std::min(all.total, of_scene.total);
    all.hold_worst = std::min(all.hold_worst, of_scene.hold_worst);
    all.hold_total = std::min(all.hold_total, of_scene.hold_total);
    all.scenes.push_back(of_scene);
  }
  return all;
}

int StaCheck::registerCount() const
{
  return new_cells_.registerCount() - discards_->registerCount();
}

}  // namespace ret
