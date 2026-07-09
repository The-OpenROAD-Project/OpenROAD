// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "OptimizePower.hh"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "rsz/Resizer.hh"
#include "sta/Graph.hh"
#include "sta/GraphClass.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/PowerClass.hh"
#include "sta/PortDirection.hh"
#include "sta/Scene.hh"
#include "utl/Logger.h"

namespace rsz {

using utl::RSZ;

OptimizePower::OptimizePower(Resizer* resizer) : resizer_(resizer)
{
}

void OptimizePower::init()
{
  logger_ = resizer_->logger_;
  dbStaState::init(resizer_->sta_);
  db_network_ = resizer_->db_network_;
  estimate_parasitics_ = resizer_->estimate_parasitics_;
}

// Worst (min) setup slack over all output-pin vertices of inst.
// Returns INF if the instance drives nothing timed.
sta::Slack OptimizePower::instanceWorstSlack(sta::Instance* inst) const
{
  sta::Slack worst = sta::INF;
  std::unique_ptr<sta::InstancePinIterator> pin_iter(
      network_->pinIterator(inst));
  while (pin_iter->hasNext()) {
    const sta::Pin* pin = pin_iter->next();
    if (!network_->direction(pin)->isAnyOutput()) {
      continue;
    }
    sta::Vertex* vertex = nullptr;
    sta::Vertex* bidirect_drvr_vertex = nullptr;
    resizer_->graph_->pinVertices(pin, vertex, bidirect_drvr_vertex);
    if (vertex != nullptr) {
      const sta::Slack slack = sta_->slack(vertex, max_);
      worst = std::min(worst, slack);
    }
    if (bidirect_drvr_vertex != nullptr) {
      const sta::Slack slack = sta_->slack(bidirect_drvr_vertex, max_);
      worst = std::min(worst, slack);
    }
  }
  return worst;
}

sta::LibertyCell* OptimizePower::lowestLeakageVariant(
    sta::Instance* inst,
    sta::LibertyCell* curr_cell) const
{
  // Respect set_dont_touch and only touch ordinary logic std cells.
  if (resizer_->dontTouch(inst) || !resizer_->isLogicStdCell(inst)) {
    return nullptr;
  }
  // getVTEquivCells: same footprint/area/site, different Vt, sorted ascending
  // by leakage; dontUse and non-link cells already filtered out.
  sta::LibertyCellSeq equiv_cells = resizer_->getVTEquivCells(curr_cell);
  if (equiv_cells.empty()) {
    return nullptr;
  }
  sta::LibertyCell* best = equiv_cells.front();  // lowest leakage
  if (best == curr_cell || resizer_->dontUse(best)) {
    return nullptr;
  }
  // Only worth swapping if it actually lowers leakage.
  const std::optional<float> curr_leak = resizer_->cellLeakage(curr_cell);
  const std::optional<float> best_leak = resizer_->cellLeakage(best);
  if (curr_leak.has_value() && best_leak.has_value()
      && best_leak.value() >= curr_leak.value()) {
    return nullptr;
  }
  return best;
}

bool OptimizePower::optimizePower(const float slack_margin, const bool verbose)
{
  init();
  swap_count_ = 0;

  if (resizer_->vtCategoryCount() < 2) {
    logger_->info(
        RSZ,
        508,
        "optimize_power: library has fewer than 2 Vt categories; no "
        "lower-leakage variants available. Design unchanged.");
    return false;
  }

  const sta::Scene* scene = sta_->cmdScene();

  // Incremental-parasitics scope: every updateParasitics() below (baseline,
  // per-swap, and final) must run inside this guard.
  est::IncrementalParasiticsGuard guard(estimate_parasitics_);

  // Baseline timing.
  estimate_parasitics_->updateParasitics();
  sta_->findRequireds();
  sta::Slack wns_before;
  sta::Vertex* worst_vertex = nullptr;
  sta_->worstSlack(max_, wns_before, worst_vertex);
  const sta::Slack tns_before = sta_->totalNegativeSlack(max_);

  // Baseline leakage power (engine-computed).
  sta::PowerResult total_before, seq, comb, clk, macro, pad;
  sta_->power(scene, total_before, seq, comb, clk, macro, pad);
  const float leakage_before = total_before.leakage();

  // Collect candidate instances: logic std cells whose worst slack is safely
  // positive (> margin) and that have a lower-leakage Vt variant.
  struct Candidate
  {
    sta::Instance* inst;
    sta::LibertyCell* curr_cell;
    sta::LibertyCell* best_cell;
    sta::Slack slack;
  };
  std::vector<Candidate> candidates;
  std::unique_ptr<sta::LeafInstanceIterator> inst_iter(
      network_->leafInstanceIterator());
  while (inst_iter->hasNext()) {
    sta::Instance* inst = inst_iter->next();
    sta::LibertyCell* curr_cell = network_->libertyCell(inst);
    if (curr_cell == nullptr) {
      continue;
    }
    sta::LibertyCell* best_cell = lowestLeakageVariant(inst, curr_cell);
    if (best_cell == nullptr) {
      continue;
    }
    const sta::Slack slack = instanceWorstSlack(inst);
    // Skip instances with no timed output or non-positive margin.
    if (slack == sta::INF || slack <= slack_margin) {
      continue;
    }
    candidates.push_back({inst, curr_cell, best_cell, slack});
  }

  // Try the cells with the most positive slack first (most headroom).
  std::ranges::sort(candidates,
                    [](const Candidate& a, const Candidate& b) {
                      return a.slack > b.slack;
                    });

  logger_->info(RSZ,
                509,
                "optimize_power: {} leakage-recovery candidates "
                "(slack > {:.3e}s).",
                candidates.size(),
                slack_margin);

  for (const Candidate& cand : candidates) {
    // Trial swap to the lowest-leakage variant.
    if (!resizer_->replaceCell(cand.inst, cand.best_cell, /*journal=*/false)) {
      continue;
    }
    estimate_parasitics_->updateParasitics();
    sta_->findRequireds();

    sta::Slack wns_after;
    sta_->worstSlack(max_, wns_after, worst_vertex);
    const sta::Slack tns_after = sta_->totalNegativeSlack(max_);
    const sta::Slack inst_slack_after = instanceWorstSlack(cand.inst);

    // Accept only if: the swapped cell still has slack >= margin, AND global
    // WNS and TNS did not degrade (timing not worse). Small tolerance guards
    // against numeric jitter.
    constexpr float kTol = 1e-15f;
    const bool inst_ok = inst_slack_after >= slack_margin - kTol;
    const bool wns_ok = wns_after >= wns_before - kTol;
    const bool tns_ok = tns_after >= tns_before - kTol;

    if (inst_ok && wns_ok && tns_ok) {
      swap_count_++;
      if (verbose) {
        logger_->report(
            "  swap {} {} -> {} (slack {:.3e} -> {:.3e})",
            network_->pathName(cand.inst),
            cand.curr_cell->name(),
            cand.best_cell->name(),
            cand.slack,
            inst_slack_after);
      }
    } else {
      // Revert: swap back to the original cell.
      resizer_->replaceCell(cand.inst, cand.curr_cell, /*journal=*/false);
      estimate_parasitics_->updateParasitics();
      sta_->findRequireds();
    }
  }

  // Final timing + leakage for the proof report.
  estimate_parasitics_->updateParasitics();
  sta_->findRequireds();
  sta::Slack wns_final;
  sta_->worstSlack(max_, wns_final, worst_vertex);
  const sta::Slack tns_final = sta_->totalNegativeSlack(max_);

  sta::PowerResult total_after;
  sta_->power(scene, total_after, seq, comb, clk, macro, pad);
  const float leakage_after = total_after.leakage();

  logger_->info(RSZ,
                511,
                "optimize_power: swapped {} cells. Leakage {:.4e} W -> "
                "{:.4e} W ({:+.2f}%). WNS {:.4e} -> {:.4e}, TNS {:.4e} -> "
                "{:.4e}.",
                swap_count_,
                leakage_before,
                leakage_after,
                leakage_before > 0.0
                    ? (leakage_after - leakage_before) / leakage_before * 100.0
                    : 0.0,
                wns_before,
                wns_final,
                tns_before,
                tns_final);

  return swap_count_ > 0;
}

}  // namespace rsz
