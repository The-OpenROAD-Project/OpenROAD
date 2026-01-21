// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "RecoverPowerMore.hh"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "grt/GlobalRouter.h"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/Clock.hh"
#include "sta/Corner.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/PowerClass.hh"
#include "sta/Sdc.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"

namespace rsz {

using std::string;
using std::vector;

using utl::RSZ;

namespace {

constexpr int kDigits = 3;

Net* mutableNet(const Net* net)
{
  // OpenSTA/Network APIs used by the limit checks are not const-correct.
  // They take Net* but do not mutate the net, so casting away const is safe.
  return const_cast<Net*>(net);
}

class AllowCongestionGuard
{
 public:
  explicit AllowCongestionGuard(est::EstimateParasitics* estimate_parasitics)
  {
    if (estimate_parasitics == nullptr) {
      return;
    }
    grt::GlobalRouter* grouter = estimate_parasitics->getGlobalRouter();
    if (grouter == nullptr) {
      return;
    }
    grouter_ = grouter;
    allow_congestion_prev_ = grouter_->getAllowCongestion();
    grouter_->setAllowCongestion(true);
  }

  ~AllowCongestionGuard()
  {
    if (grouter_ != nullptr) {
      grouter_->setAllowCongestion(allow_congestion_prev_);
    }
  }

 private:
  grt::GlobalRouter* grouter_ = nullptr;
  bool allow_congestion_prev_ = false;
};

class SkipDirtyOnInstSwapMasterGuard
{
 public:
  explicit SkipDirtyOnInstSwapMasterGuard(
      est::EstimateParasitics* estimate_parasitics)
  {
    if (estimate_parasitics == nullptr) {
      return;
    }
    grt::GlobalRouter* grouter = estimate_parasitics->getGlobalRouter();
    if (grouter == nullptr) {
      return;
    }
    grouter_ = grouter;
    prev_ = grouter_->getSkipDirtyOnInstSwapMaster();
    grouter_->setSkipDirtyOnInstSwapMaster(true);
  }

  ~SkipDirtyOnInstSwapMasterGuard()
  {
    if (grouter_ != nullptr) {
      grouter_->setSkipDirtyOnInstSwapMaster(prev_);
    }
  }

 private:
  grt::GlobalRouter* grouter_ = nullptr;
  bool prev_ = false;
};

}  // namespace

RecoverPowerMore::RecoverPowerMore(Resizer* resizer) : resizer_(resizer)
{
}

void RecoverPowerMore::init()
{
  logger_ = resizer_->logger_;
  dbStaState::init(resizer_->sta_);
  db_network_ = resizer_->db_network_;
  estimate_parasitics_ = resizer_->estimate_parasitics_;
}

bool RecoverPowerMore::recoverPower(const float recover_power_percent,
                                    const bool verbose)
{
  init();

  if (recover_power_percent <= 0.0f) {
    return false;
  }

  resize_count_ = 0;
  buffer_remove_count_ = 0;
  action_history_.clear();
  resizer_->buffer_moved_into_core_ = false;

  // Ensure clock network exists for sta_->isClock().
  sta_->ensureClkNetwork();

  est::IncrementalParasiticsGuard guard(estimate_parasitics_);
  AllowCongestionGuard allow_congestion(estimate_parasitics_);
  SkipDirtyOnInstSwapMasterGuard skip_dirty_swap(estimate_parasitics_);

  // Make sure STA is consistent with any pending incremental routing/parasitic
  // updates before we measure the timing budget baseline.
  if (auto* gr = estimate_parasitics_->getGlobalRouter()) {
    gr->discardDirtyNets();
  }
  estimate_parasitics_->updateParasitics(false);
  sta_->delaysInvalid();
  // Use a full timing update to make the baseline (and derived floor) stable.
  sta_->updateTiming(true);
  sta_->findRequireds();

  // Baseline timing + DRV limits: do not regress.
  Slack worst_slack_before;
  Vertex* worst_vertex;
  sta_->worstSlack(max_, worst_slack_before, worst_vertex);
  (void) worst_vertex;

  const float clock_period = minClockPeriod();

  Slack worst_hold_before;
  Vertex* worst_hold_vertex;
  sta_->worstSlack(min_, worst_hold_before, worst_hold_vertex);
  (void) worst_hold_vertex;

  // Limit performance degradation from recover_power by bounding the increase
  // in effective clock period (target clock period - WNS).
  const Slack wns_floor = computeWnsFloor(worst_slack_before, clock_period);
  wns_floor_ = wns_floor;
  const Slack hold_floor = (worst_hold_before >= 0.0) ? 0.0 : worst_hold_before;
  hold_floor_ = hold_floor;

  sta::Unit* time_unit = sta_->units()->timeUnit();
  logger_->metric("recover_power__timing__setup__ws_before",
                  time_unit->staToUser(worst_slack_before));
  logger_->metric("recover_power__timing__setup__ws_floor",
                  time_unit->staToUser(wns_floor));

  initial_design_area_ = resizer_->computeDesignArea();
  initial_max_slew_violations_
      = sta_->checkSlewLimits(nullptr, true, nullptr, max_).size();
  initial_max_cap_violations_
      = sta_->checkCapacitanceLimits(nullptr, true, nullptr, max_).size();
  initial_max_fanout_violations_
      = sta_->checkFanoutLimits(nullptr, true, max_).size();
  curr_max_slew_violations_ = initial_max_slew_violations_;
  curr_max_cap_violations_ = initial_max_cap_violations_;
  curr_max_fanout_violations_ = initial_max_fanout_violations_;

  corner_ = selectPowerCorner();
  initial_power_total_ = designTotalPower(corner_);

  logger_->metric("recover_power__power__total_before", initial_power_total_);
  if (clock_period > 0.0f && std::isfinite(worst_slack_before)) {
    const float eff_period_before = clock_period - worst_slack_before;
    if (eff_period_before > 0.0f) {
      logger_->metric("recover_power__timing__fmax_before",
                      1.0 / eff_period_before);
    }
  }

  int iteration = 0;
  printProgress(0, 0, true, false);

  auto run_passes = [&](auto keep_candidate,
                        auto optimize_instance,
                        const int max_passes) {
    for (int pass = 0; pass < max_passes; ++pass) {
      Slack current_wns;
      Vertex* current_wns_vertex;
      sta_->worstSlack(max_, current_wns, current_wns_vertex);
      (void) current_wns_vertex;

      std::vector<CandidateInstance> candidates
          = collectCandidates(current_wns);
      candidates.erase(std::remove_if(candidates.begin(),
                                      candidates.end(),
                                      [&](const CandidateInstance& cand) {
                                        return !keep_candidate(cand);
                                      }),
                       candidates.end());
      if (candidates.empty()) {
        break;
      }

      // Rank by power-first so we spend the limited ECP budget on the highest
      // impact cells. Then break ties by headroom/area.
      std::sort(candidates.begin(),
                candidates.end(),
                [](const CandidateInstance& a, const CandidateInstance& b) {
                  if (a.power != b.power) {
                    return a.power > b.power;
                  }
                  if (a.headroom != b.headroom) {
                    return a.headroom > b.headroom;
                  }
                  return a.area > b.area;
                });

      int max_inst_count = static_cast<int>(
          std::ceil(candidates.size() * recover_power_percent));
      max_inst_count = std::clamp(max_inst_count, 1, (int) candidates.size());

      // Print progress roughly every ~1% (bounded).
      print_interval_ = std::clamp(
          max_inst_count / 100, min_print_interval_, max_print_interval_);

      int accepted_in_last_pass = 0;
      int consecutive_rejects = 0;
      // Stop burning runtime on long tails of rejected candidates. Candidates
      // are processed in descending score order, so after enough consecutive
      // rejects further candidates are unlikely to be acceptable.
      const int max_consecutive_rejects
          = std::clamp(max_inst_count / 20, 200, 5000);
      for (int i = 0; i < max_inst_count; ++i) {
        ++iteration;
        const CandidateInstance& cand = candidates[i];
        if (optimize_instance(cand.inst)) {
          ++accepted_in_last_pass;
          consecutive_rejects = 0;
        } else {
          consecutive_rejects++;
        }

        if (verbose || (iteration % print_interval_ == 0)) {
          printProgress(iteration, max_inst_count, false, false);
        }

        if (consecutive_rejects >= max_consecutive_rejects) {
          break;
        }
      }

      if (accepted_in_last_pass == 0) {
        break;
      }
    }
  };

  // Reserve part of the ECP budget for buffer removals, which can deliver large
  // power wins per change on some designs. Do an initial swap pass with a
  // tighter floor, then buffer removals using the full floor, then a final swap
  // pass to spend any remaining budget.
  Slack swap_floor = wns_floor;
  if (clock_period > 0.0f && std::isfinite(worst_slack_before)) {
    const float eff_period_before = clock_period - worst_slack_before;
    if (eff_period_before > 0.0f && std::isfinite(eff_period_before)) {
      const Slack slack_budget_total = static_cast<Slack>(
          max_eff_period_degrade_frac_ * eff_period_before);
      const Slack swap_budget = static_cast<Slack>(0.5f * slack_budget_total);
      const Slack swap_guard = std::min<Slack>(
          timing_floor_guard_cap_,
          static_cast<Slack>(timing_floor_guard_frac_ * swap_budget));
      swap_floor = worst_slack_before - swap_budget + swap_guard;
      swap_floor = std::min(swap_floor, worst_slack_before);
    }
  }

  wns_floor_ = swap_floor;
  run_passes(
      [&](const CandidateInstance& cand) {
        LibertyCell* cell = network_->libertyCell(cand.inst);
        return cell != nullptr && !cell->isBuffer();
      },
      [&](sta::Instance* inst) {
        return optimizeInstanceSwaps(inst,
                                     swap_floor,
                                     hold_floor,
                                     initial_max_slew_violations_,
                                     initial_max_cap_violations_,
                                     initial_max_fanout_violations_,
                                     verbose);
      },
      /*max_passes=*/3);

  wns_floor_ = wns_floor;
  run_passes(
      [&](const CandidateInstance& cand) {
        LibertyCell* cell = network_->libertyCell(cand.inst);
        return cell != nullptr && cell->isBuffer() && !cand.drives_clock;
      },
      [&](sta::Instance* inst) {
        return optimizeInstanceBufferRemoval(inst,
                                             wns_floor,
                                             hold_floor,
                                             initial_max_slew_violations_,
                                             initial_max_cap_violations_,
                                             initial_max_fanout_violations_,
                                             verbose);
      },
      max_passes_);

  // Final swap phase with the full floor (uses any remaining budget).
  run_passes(
      [&](const CandidateInstance& cand) {
        LibertyCell* cell = network_->libertyCell(cand.inst);
        return cell != nullptr && !cell->isBuffer();
      },
      [&](sta::Instance* inst) {
        return optimizeInstanceSwaps(inst,
                                     wns_floor,
                                     hold_floor,
                                     initial_max_slew_violations_,
                                     initial_max_cap_violations_,
                                     initial_max_fanout_violations_,
                                     verbose);
      },
      max_passes_);

  Slack final_wns;
  Vertex* final_wns_vertex;
  sta_->worstSlack(max_, final_wns, final_wns_vertex);
  (void) final_wns_vertex;

  Slack final_hold;
  Vertex* final_hold_vertex;
  sta_->worstSlack(min_, final_hold, final_hold_vertex);
  (void) final_hold_vertex;

  if (final_wns < wns_floor || final_hold < hold_floor) {
    backoffToTimingFloor(wns_floor, hold_floor, verbose);
  }

  // RecoverPowerMore uses journal checkpoints for buffer removals so backoff
  // can undo them safely. Commit any remaining saved ECOs now that we're within
  // the timing/DRV budget.
  resizer_->journalCommitSaved();

  // Resync DRV violation counts for final reporting/sanity.
  curr_max_slew_violations_
      = sta_->checkSlewLimits(nullptr, true, nullptr, max_).size();
  curr_max_cap_violations_
      = sta_->checkCapacitanceLimits(nullptr, true, nullptr, max_).size();
  curr_max_fanout_violations_
      = sta_->checkFanoutLimits(nullptr, true, max_).size();
  if (curr_max_slew_violations_ > initial_max_slew_violations_
      || curr_max_cap_violations_ > initial_max_cap_violations_
      || curr_max_fanout_violations_ > initial_max_fanout_violations_) {
    logger_->warn(RSZ,
                  145,
                  "Power recovery increased DRV violations (slew {}/{} cap "
                  "{}/{} fanout {}/{}).",
                  curr_max_slew_violations_,
                  initial_max_slew_violations_,
                  curr_max_cap_violations_,
                  initial_max_cap_violations_,
                  curr_max_fanout_violations_,
                  initial_max_fanout_violations_);
  }

  printProgress(iteration, iteration, true, true);

  Slack worst_slack_after;
  sta_->worstSlack(max_, worst_slack_after, worst_vertex);
  logger_->metric("recover_power__timing__setup__ws_after",
                  time_unit->staToUser(worst_slack_after));
  const float power_after = designTotalPower(corner_);
  logger_->metric("recover_power__power__total_after", power_after);
  if (clock_period > 0.0f && std::isfinite(worst_slack_after)) {
    const float eff_period_after = clock_period - worst_slack_after;
    if (eff_period_after > 0.0f) {
      logger_->metric("recover_power__timing__fmax_after",
                      1.0 / eff_period_after);
    }
  }

  if (resize_count_ > 0 || buffer_remove_count_ > 0) {
    logger_->info(
        RSZ,
        147,
        "Applied {} cell swaps and removed {} buffers for power recovery.",
        resize_count_,
        buffer_remove_count_);
  }

  return resize_count_ > 0 || buffer_remove_count_ > 0;
}

std::vector<const Net*> RecoverPowerMore::instanceSignalNets(
    sta::Instance* inst) const
{
  std::vector<const Net*> nets;
  if (inst == nullptr) {
    return nets;
  }

  std::unordered_set<const Net*> seen;
  std::unique_ptr<sta::InstancePinIterator> pin_iter(
      network_->pinIterator(inst));
  while (pin_iter->hasNext()) {
    Pin* pin = pin_iter->next();
    if (pin == nullptr) {
      continue;
    }
    Net* net = network_->net(pin);
    if (net == nullptr) {
      continue;
    }
    const Net* net_const = network_->highestConnectedNet(net);
    if (net_const == nullptr || network_->isPower(net_const)
        || network_->isGround(net_const)) {
      continue;
    }
    if (seen.insert(net_const).second) {
      nets.push_back(net_const);
    }
  }

  return nets;
}

std::vector<const Net*> RecoverPowerMore::slewCheckNetCone(
    const std::vector<const Net*>& seed_nets) const
{
  std::vector<const Net*> cone;
  if (seed_nets.empty()) {
    return cone;
  }

  std::unordered_set<const Net*> visited;
  std::vector<const Net*> frontier = seed_nets;
  for (int depth = 0; depth <= slew_check_depth_; ++depth) {
    std::vector<const Net*> next_frontier;

    for (const Net* net : frontier) {
      if (net == nullptr) {
        continue;
      }
      net = network_->highestConnectedNet(mutableNet(net));
      if (net == nullptr || network_->isPower(net) || network_->isGround(net)) {
        continue;
      }
      if (!visited.insert(net).second) {
        continue;
      }

      cone.push_back(net);

      // Slew propagation through timing arcs is not relevant for clock nets
      // and expanding large clock nets is expensive.
      if (sta_->isClock(net)) {
        continue;
      }
      if (depth == slew_check_depth_) {
        continue;
      }

      std::unique_ptr<sta::NetConnectedPinIterator> net_pin_iter(
          network_->connectedPinIterator(net));
      while (net_pin_iter->hasNext()) {
        const Pin* pin = net_pin_iter->next();
        if (pin == nullptr) {
          continue;
        }
        if (network_->isDriver(pin) || network_->isTopLevelPort(pin)) {
          continue;
        }

        sta::Instance* load_inst = network_->instance(pin);
        if (load_inst == nullptr) {
          continue;
        }

        std::unique_ptr<sta::InstancePinIterator> inst_pin_iter(
            network_->pinIterator(load_inst));
        while (inst_pin_iter->hasNext()) {
          Pin* inst_pin = inst_pin_iter->next();
          if (inst_pin == nullptr || !network_->isDriver(inst_pin)) {
            continue;
          }
          const Net* out_net = network_->net(inst_pin);
          if (out_net == nullptr) {
            continue;
          }
          out_net = network_->highestConnectedNet(mutableNet(out_net));
          if (out_net == nullptr || network_->isPower(out_net)
              || network_->isGround(out_net)) {
            continue;
          }
          if (visited.find(out_net) == visited.end()) {
            next_frontier.push_back(out_net);
          }
        }
      }
    }

    frontier.swap(next_frontier);
    if (frontier.empty()) {
      break;
    }
  }

  return cone;
}

size_t RecoverPowerMore::countSlewViolations(
    const std::vector<const Net*>& nets) const
{
  size_t count = 0;
  for (const Net* net : nets) {
    if (net != nullptr) {
      count
          += sta_->checkSlewLimits(mutableNet(net), true, nullptr, max_).size();
    }
  }
  return count;
}

size_t RecoverPowerMore::countCapViolations(
    const std::vector<const Net*>& nets) const
{
  size_t count = 0;
  for (const Net* net : nets) {
    if (net != nullptr) {
      count
          += sta_->checkCapacitanceLimits(mutableNet(net), true, nullptr, max_)
                 .size();
    }
  }
  return count;
}

size_t RecoverPowerMore::countFanoutViolations(
    const std::vector<const Net*>& nets) const
{
  size_t count = 0;
  for (const Net* net : nets) {
    if (net != nullptr) {
      count += sta_->checkFanoutLimits(mutableNet(net), true, max_).size();
    }
  }
  return count;
}

std::vector<RecoverPowerMore::CandidateInstance>
RecoverPowerMore::collectCandidates(const Slack current_setup_wns) const
{
  std::vector<CandidateInstance> candidates;

  std::unique_ptr<sta::LeafInstanceIterator> inst_iter(
      network_->leafInstanceIterator());
  while (inst_iter->hasNext()) {
    sta::Instance* inst = inst_iter->next();
    if (resizer_->dontTouch(inst) || !resizer_->isLogicStdCell(inst)) {
      continue;
    }

    LibertyCell* cell = network_->libertyCell(inst);
    if (cell == nullptr) {
      continue;
    }

    const bool drives_clock = instanceDrivesClock(inst);
    const bool allow_buffer_removal = cell->isBuffer() && !drives_clock;
    Slack slack = instanceWorstSlack(inst);
    if (!std::isfinite(slack)) {
      if (!drives_clock && !allow_buffer_removal) {
        continue;
      }
      // Clock network instances often have no meaningful data slack at their
      // output pins. Use the current global WNS as a proxy for candidate
      // ordering but rely on full STA checks after each swap for correctness.
      slack = current_setup_wns;
    }

    const Slack headroom = slack - wns_floor_;
    if (!drives_clock && !allow_buffer_removal) {
      // For designs with negative WNS, use headroom relative to the current
      // worst slack so power recovery can still operate on paths that are not
      // the current worst.
      if (!(headroom > setup_slack_margin_
            && slack < setup_slack_max_margin_)) {
        continue;
      }
    } else {
      // Still exclude unconstrained cases where slack can be extremely large.
      if (!(slack < setup_slack_max_margin_)) {
        continue;
      }
    }

    const odb::dbMaster* master = db_network_->staToDb(cell);
    if (master == nullptr) {
      continue;
    }

    float power = 0.0f;
    if (corner_ != nullptr) {
      sta::PowerResult inst_power = sta_->power(inst, corner_);
      power = inst_power.total();
    }

    candidates.push_back(
        {inst, slack, headroom, power, master->getArea(), drives_clock});
  }

  return candidates;
}

Slack RecoverPowerMore::instanceWorstSlack(sta::Instance* inst) const
{
  Slack worst_slack = std::numeric_limits<Slack>::infinity();
  bool found = false;

  std::unique_ptr<sta::InstancePinIterator> pin_iter(
      network_->pinIterator(inst));
  while (pin_iter->hasNext()) {
    Pin* pin = pin_iter->next();
    if (!network_->isDriver(pin)) {
      continue;
    }
    if (sta_->isClock(pin)) {
      continue;
    }

    Vertex* vertex = graph_->pinDrvrVertex(pin);
    if (vertex == nullptr || vertex->isConstant()
        || vertex->isDisabledConstraint()) {
      continue;
    }

    const Slack slack = sta_->vertexSlack(vertex, max_);
    worst_slack = std::min(worst_slack, slack);
    found = true;
  }

  return found ? worst_slack : std::numeric_limits<Slack>::infinity();
}

bool RecoverPowerMore::instanceDrivesClock(sta::Instance* inst) const
{
  std::unique_ptr<sta::InstancePinIterator> pin_iter(
      network_->pinIterator(inst));
  while (pin_iter->hasNext()) {
    Pin* pin = pin_iter->next();
    if (!network_->isDriver(pin)) {
      continue;
    }
    if (sta_->isClock(pin)) {
      return true;
    }
  }
  return false;
}

float RecoverPowerMore::minClockPeriod() const
{
  auto* clocks = sdc_->clocks();
  if (clocks == nullptr || clocks->empty()) {
    return 0.0f;
  }

  float min_period = std::numeric_limits<float>::infinity();
  for (const sta::Clock* clock : *clocks) {
    min_period = std::min(min_period, clock->period());
  }
  return std::isfinite(min_period) ? min_period : 0.0f;
}

Slack RecoverPowerMore::computeWnsFloor(const Slack worst_setup_before,
                                        const float clock_period) const
{
  if (!(clock_period > 0.0f) || !std::isfinite(worst_setup_before)) {
    return (worst_setup_before >= 0.0) ? 0.0 : worst_setup_before;
  }

  const float eff_period_before = clock_period - worst_setup_before;
  if (!(eff_period_before > 0.0f) || !std::isfinite(eff_period_before)) {
    return (worst_setup_before >= 0.0) ? 0.0 : worst_setup_before;
  }

  const Slack slack_budget
      = static_cast<Slack>(max_eff_period_degrade_frac_ * eff_period_before);
  const Slack slack_guard = std::min<Slack>(
      timing_floor_guard_cap_,
      static_cast<Slack>(timing_floor_guard_frac_ * slack_budget));

  Slack floor = worst_setup_before - slack_budget + slack_guard;
  return std::min(floor, worst_setup_before);
}

const Corner* RecoverPowerMore::selectPowerCorner() const
{
  const Corner* corner = sta_->cmdCorner();
  if (corner != nullptr) {
    return corner;
  }

  sta::Corners* corners = sta_->corners();
  if (corners != nullptr && corners->count() > 0) {
    return corners->corners()[0];
  }

  return nullptr;
}

float RecoverPowerMore::designTotalPower(const Corner* corner) const
{
  if (corner == nullptr) {
    return -1.0f;
  }

  sta::PowerResult total;
  sta::PowerResult sequential;
  sta::PowerResult combinational;
  sta::PowerResult clock;
  sta::PowerResult macro;
  sta::PowerResult pad;
  sta_->power(corner, total, sequential, combinational, clock, macro, pad);
  return total.total();
}

std::vector<LibertyCell*> RecoverPowerMore::nextSmallerCells(
    LibertyCell* cell) const
{
  std::vector<LibertyCell*> candidates;
  if (cell == nullptr) {
    return candidates;
  }

  const odb::dbMaster* curr_master = db_network_->staToDb(cell);
  if (curr_master == nullptr) {
    return candidates;
  }
  const int curr_w = curr_master->getWidth();
  const int curr_h = curr_master->getHeight();
  const int64_t curr_area = curr_master->getArea();

  LibertyCellSeq swappable_cells = resizer_->getSwappableCells(cell);
  if (swappable_cells.size() <= 1) {
    return candidates;
  }

  // Find the closest smaller area.
  int64_t target_area = -1;
  for (LibertyCell* cand : swappable_cells) {
    if (cand == cell) {
      continue;
    }
    const odb::dbMaster* cand_master = db_network_->staToDb(cand);
    if (cand_master == nullptr) {
      continue;
    }
    if (cand_master->getHeight() != curr_h
        || cand_master->getWidth() > curr_w) {
      continue;
    }
    const int64_t cand_area = cand_master->getArea();
    if (cand_area < curr_area) {
      target_area = std::max(target_area, cand_area);
    }
  }
  if (target_area < 0) {
    return candidates;
  }

  for (LibertyCell* cand : swappable_cells) {
    if (cand == cell) {
      continue;
    }
    const odb::dbMaster* cand_master = db_network_->staToDb(cand);
    if (cand_master == nullptr) {
      continue;
    }
    if (cand_master->getArea() != target_area) {
      continue;
    }
    if (cand_master->getHeight() != curr_h
        || cand_master->getWidth() > curr_w) {
      continue;
    }

    candidates.push_back(cand);
  }

  // Prefer weaker (higher resistance) and lower leakage candidates first;
  // the full STA check will decide which are actually acceptable.
  std::ranges::stable_sort(candidates, [this](LibertyCell* a, LibertyCell* b) {
    float ra = std::max(0.0f, resizer_->cellDriveResistance(a));
    float rb = std::max(0.0f, resizer_->cellDriveResistance(b));
    const float la = resizer_->cellLeakage(a).value_or(
        std::numeric_limits<float>::infinity());
    const float lb = resizer_->cellLeakage(b).value_or(
        std::numeric_limits<float>::infinity());
    if (ra != rb) {
      return ra > rb;
    }
    if (la != lb) {
      return la < lb;
    }
    return std::strcmp(a->name(), b->name()) < 0;
  });

  return candidates;
}

LibertyCell* RecoverPowerMore::nextLowerLeakageVtCell(LibertyCell* cell) const
{
  if (cell == nullptr) {
    return nullptr;
  }

  LibertyCellSeq vt_equiv_cells = resizer_->getVTEquivCells(cell);
  if (vt_equiv_cells.size() < 2) {
    return nullptr;
  }

  int idx = -1;
  for (int i = 0; i < (int) vt_equiv_cells.size(); ++i) {
    if (vt_equiv_cells[i] == cell) {
      idx = i;
      break;
    }
  }
  if (idx <= 0) {
    return nullptr;
  }

  // Try the closest lower-leakage VT first (least timing impact).
  for (int i = idx - 1; i >= 0; --i) {
    LibertyCell* cand = vt_equiv_cells[i];
    if (cand != nullptr && cand != cell && meetsSizeCriteria(cell, cand)) {
      return cand;
    }
  }
  return nullptr;
}

bool RecoverPowerMore::trySwapCell(sta::Instance* inst,
                                   LibertyCell* replacement,
                                   Slack wns_floor,
                                   Slack hold_floor,
                                   size_t max_slew_vio,
                                   size_t max_cap_vio,
                                   size_t max_fanout_vio,
                                   bool verbose)
{
  LibertyCell* curr_cell = network_->libertyCell(inst);
  if (curr_cell == nullptr || replacement == nullptr
      || replacement == curr_cell) {
    return false;
  }
  if (!meetsSizeCriteria(curr_cell, replacement)
      || resizer_->dontUse(replacement)) {
    return false;
  }

  const std::vector<const Net*> seed_nets = instanceSignalNets(inst);
  const std::vector<const Net*> slew_cone = slewCheckNetCone(seed_nets);
  const size_t pre_slew = countSlewViolations(slew_cone);
  const size_t pre_cap = countCapViolations(seed_nets);
  const size_t pre_fanout = countFanoutViolations(seed_nets);

  if (!resizer_->replaceCell(inst, replacement, /* journal */ false)) {
    return false;
  }

  sta_->findRequireds();

  // Fast rejection without perturbing routing/parasitics. Most candidates fail
  // timing and can be rejected based on existing parasitics.
  Slack wns_quick;
  Vertex* wns_quick_vertex;
  sta_->worstSlack(max_, wns_quick, wns_quick_vertex);
  (void) wns_quick_vertex;

  Slack hold_quick;
  Vertex* hold_quick_vertex;
  sta_->worstSlack(min_, hold_quick, hold_quick_vertex);
  (void) hold_quick_vertex;

  if (wns_quick < wns_floor || hold_quick < hold_floor) {
    resizer_->replaceCell(inst, curr_cell, /* journal */ false);
    if (auto* gr = estimate_parasitics_->getGlobalRouter()) {
      gr->discardDirtyNets();
    }
    estimate_parasitics_->updateParasitics(false);
    sta_->updateTiming(false);
    sta_->findRequireds();
    return false;
  }

  // Parasitics must be updated after an inst master swap because pin offsets
  // can change (and incremental groute callbacks can mark additional nets).
  // Do this before the definitive timing/DRV acceptance checks so the timing
  // budget is enforced against the same state that will be kept.
  if (auto* gr = estimate_parasitics_->getGlobalRouter()) {
    gr->discardDirtyNets();
  }
  estimate_parasitics_->updateParasitics(false);
  sta_->updateTiming(false);
  sta_->findRequireds();

  const Slack inst_slack_after = instanceWorstSlack(inst);

  Slack wns_after;
  Vertex* wns_vertex;
  sta_->worstSlack(max_, wns_after, wns_vertex);
  (void) wns_vertex;

  Slack hold_after;
  Vertex* hold_vertex;
  sta_->worstSlack(min_, hold_after, hold_vertex);
  (void) hold_vertex;

  const bool timing_ok = wns_after >= wns_floor && hold_after >= hold_floor;

  const size_t post_slew = countSlewViolations(slew_cone);
  const size_t post_cap = countCapViolations(seed_nets);
  const size_t post_fanout = countFanoutViolations(seed_nets);

  const int64_t new_slew
      = static_cast<int64_t>(curr_max_slew_violations_)
        + (static_cast<int64_t>(post_slew) - static_cast<int64_t>(pre_slew));
  const int64_t new_cap
      = static_cast<int64_t>(curr_max_cap_violations_)
        + (static_cast<int64_t>(post_cap) - static_cast<int64_t>(pre_cap));
  const int64_t new_fanout = static_cast<int64_t>(curr_max_fanout_violations_)
                             + (static_cast<int64_t>(post_fanout)
                                - static_cast<int64_t>(pre_fanout));

  const bool drv_ok = new_slew >= 0 && new_cap >= 0 && new_fanout >= 0
                      && new_slew <= static_cast<int64_t>(max_slew_vio)
                      && new_cap <= static_cast<int64_t>(max_cap_vio)
                      && new_fanout <= static_cast<int64_t>(max_fanout_vio);

  if (!timing_ok || !drv_ok) {
    if (verbose) {
      debugPrint(logger_,
                 RSZ,
                 "recover_power_more",
                 2,
                 "REJECT swap {} {} -> {} (inst_slack={} wns={} floor={})",
                 network_->pathName(inst),
                 curr_cell->name(),
                 replacement->name(),
                 delayAsString(inst_slack_after, sta_, kDigits),
                 delayAsString(wns_after, sta_, kDigits),
                 delayAsString(wns_floor, sta_, kDigits));
    }
    resizer_->replaceCell(inst, curr_cell, /* journal */ false);
    if (auto* gr = estimate_parasitics_->getGlobalRouter()) {
      gr->discardDirtyNets();
    }
    estimate_parasitics_->updateParasitics(false);
    sta_->updateTiming(false);
    sta_->findRequireds();
    return false;
  }

  if (verbose) {
    debugPrint(logger_,
               RSZ,
               "recover_power_more",
               2,
               "ACCEPT swap {} {} -> {} (inst_slack={} wns={})",
               network_->pathName(inst),
               curr_cell->name(),
               replacement->name(),
               delayAsString(inst_slack_after, sta_, kDigits),
               delayAsString(wns_after, sta_, kDigits));
  }

  action_history_.push_back(
      {ActionType::kSwap, network_->pathName(inst), curr_cell, replacement});
  curr_max_slew_violations_ = static_cast<size_t>(new_slew);
  curr_max_cap_violations_ = static_cast<size_t>(new_cap);
  curr_max_fanout_violations_ = static_cast<size_t>(new_fanout);
  resize_count_++;
  return true;
}

bool RecoverPowerMore::tryRemoveBuffer(sta::Instance* inst,
                                       Slack wns_floor,
                                       Slack hold_floor,
                                       size_t max_slew_vio,
                                       size_t max_cap_vio,
                                       size_t max_fanout_vio,
                                       bool verbose)
{
  LibertyCell* curr_cell = network_->libertyCell(inst);
  if (curr_cell == nullptr || !curr_cell->isBuffer()) {
    return false;
  }

  const std::string inst_name = network_->pathName(inst);
  const std::string cell_name = curr_cell->name();

  const Net* removed_net = nullptr;
  {
    LibertyPort *in_port, *out_port;
    curr_cell->bufferPorts(in_port, out_port);
    Pin* in_pin = db_network_->findPin(inst, in_port);
    Pin* out_pin = db_network_->findPin(inst, out_port);
    const Net* in_net = (in_pin != nullptr) ? network_->net(in_pin) : nullptr;
    const Net* out_net
        = (out_pin != nullptr) ? network_->net(out_pin) : nullptr;
    if (in_net != nullptr) {
      in_net = network_->highestConnectedNet(mutableNet(in_net));
    }
    if (out_net != nullptr) {
      out_net = network_->highestConnectedNet(mutableNet(out_net));
    }
    // Mirror UnbufferMove::removeBuffer() survivor selection to avoid
    // dereferencing a net that is destroyed by mergeNet().
    removed_net = out_net;
    if (in_net != nullptr && out_net != nullptr && !db_network_->hasPort(in_net)
        && db_network_->hasPort(out_net)) {
      removed_net = in_net;
    }
  }

  const std::vector<const Net*> seed_nets = instanceSignalNets(inst);
  const std::vector<const Net*> slew_cone = slewCheckNetCone(seed_nets);
  const size_t pre_slew = countSlewViolations(slew_cone);
  const size_t pre_cap = countCapViolations(seed_nets);
  const size_t pre_fanout = countFanoutViolations(seed_nets);

  resizer_->journalBegin();

  if (!resizer_->removeBuffer(inst, /* honor dont touch/fixed */ true)) {
    resizer_->journalEnd();
    return false;
  }

  if (auto* gr = estimate_parasitics_->getGlobalRouter()) {
    gr->discardDirtyNets();
  }
  estimate_parasitics_->updateParasitics(false);
  sta_->updateTiming(false);
  sta_->findRequireds();

  Slack wns_after;
  Vertex* wns_vertex;
  sta_->worstSlack(max_, wns_after, wns_vertex);
  (void) wns_vertex;

  Slack hold_after;
  Vertex* hold_vertex;
  sta_->worstSlack(min_, hold_after, hold_vertex);
  (void) hold_vertex;

  const bool timing_ok = wns_after >= wns_floor && hold_after >= hold_floor;

  auto filter_removed = [removed_net](const std::vector<const Net*>& nets) {
    std::vector<const Net*> filtered;
    filtered.reserve(nets.size());
    for (const Net* net : nets) {
      if (net != removed_net) {
        filtered.push_back(net);
      }
    }
    return filtered;
  };

  const std::vector<const Net*> seed_nets_post = filter_removed(seed_nets);
  const std::vector<const Net*> slew_cone_post = filter_removed(slew_cone);

  const size_t post_slew = countSlewViolations(slew_cone_post);
  const size_t post_cap = countCapViolations(seed_nets_post);
  const size_t post_fanout = countFanoutViolations(seed_nets_post);

  const int64_t new_slew
      = static_cast<int64_t>(curr_max_slew_violations_)
        + (static_cast<int64_t>(post_slew) - static_cast<int64_t>(pre_slew));
  const int64_t new_cap
      = static_cast<int64_t>(curr_max_cap_violations_)
        + (static_cast<int64_t>(post_cap) - static_cast<int64_t>(pre_cap));
  const int64_t new_fanout = static_cast<int64_t>(curr_max_fanout_violations_)
                             + (static_cast<int64_t>(post_fanout)
                                - static_cast<int64_t>(pre_fanout));

  const bool drv_ok = new_slew >= 0 && new_cap >= 0 && new_fanout >= 0
                      && new_slew <= static_cast<int64_t>(max_slew_vio)
                      && new_cap <= static_cast<int64_t>(max_cap_vio)
                      && new_fanout <= static_cast<int64_t>(max_fanout_vio);

  if (!timing_ok || !drv_ok) {
    if (verbose) {
      debugPrint(logger_,
                 RSZ,
                 "recover_power_more",
                 2,
                 "REJECT remove_buffer {} ({}) (wns={} floor={} hold={})",
                 inst_name,
                 cell_name,
                 delayAsString(wns_after, sta_, kDigits),
                 delayAsString(wns_floor, sta_, kDigits),
                 delayAsString(hold_after, sta_, kDigits));
    }
    resizer_->journalRestore();
    init();  // journalRestore reinitializes the resizer.
    return false;
  }

  if (verbose) {
    debugPrint(logger_,
               RSZ,
               "recover_power_more",
               2,
               "ACCEPT remove_buffer {} ({}) (wns={} hold={})",
               inst_name,
               cell_name,
               delayAsString(wns_after, sta_, kDigits),
               delayAsString(hold_after, sta_, kDigits));
  }

  action_history_.push_back(
      {ActionType::kRemoveBuffer, inst_name, nullptr, curr_cell});
  resizer_->journalEndSave(false);
  curr_max_slew_violations_ = static_cast<size_t>(new_slew);
  curr_max_cap_violations_ = static_cast<size_t>(new_cap);
  curr_max_fanout_violations_ = static_cast<size_t>(new_fanout);
  buffer_remove_count_++;
  return true;
}

bool RecoverPowerMore::optimizeInstanceSwaps(sta::Instance* inst,
                                             Slack wns_floor,
                                             Slack hold_floor,
                                             size_t max_slew_vio,
                                             size_t max_cap_vio,
                                             size_t max_fanout_vio,
                                             bool verbose)
{
  if (inst == nullptr || resizer_->dontTouch(inst)) {
    return false;
  }

  LibertyCell* curr_cell = network_->libertyCell(inst);
  if (curr_cell == nullptr || curr_cell->isBuffer()) {
    return false;
  }

  bool changed = false;
  int swaps = 0;

  // Prefer area reduction first (dynamic power), then use remaining slack for
  // VT swaps (leakage).
  while (swaps < max_swaps_per_instance_) {
    LibertyCell* curr_cell = network_->libertyCell(inst);
    const auto candidates = nextSmallerCells(curr_cell);
    if (candidates.empty()) {
      break;
    }
    bool accepted = false;
    for (LibertyCell* cand : candidates) {
      if (trySwapCell(inst,
                      cand,
                      wns_floor,
                      hold_floor,
                      max_slew_vio,
                      max_cap_vio,
                      max_fanout_vio,
                      verbose)) {
        accepted = true;
        break;
      }
    }
    if (!accepted) {
      // If none of the "next smaller" candidates are acceptable, smaller ones
      // will be even harder.
      break;
    }
    changed = true;
    swaps++;
  }

  while (swaps < max_swaps_per_instance_) {
    LibertyCell* curr_cell = network_->libertyCell(inst);
    LibertyCell* cand = nextLowerLeakageVtCell(curr_cell);
    if (cand == nullptr) {
      break;
    }
    if (!trySwapCell(inst,
                     cand,
                     wns_floor,
                     hold_floor,
                     max_slew_vio,
                     max_cap_vio,
                     max_fanout_vio,
                     verbose)) {
      // If swapping to a lower leakage VT isn't acceptable, going further in
      // the same direction will be worse.
      break;
    }
    changed = true;
    swaps++;
  }

  return changed;
}

bool RecoverPowerMore::optimizeInstanceBufferRemoval(sta::Instance* inst,
                                                     Slack wns_floor,
                                                     Slack hold_floor,
                                                     size_t max_slew_vio,
                                                     size_t max_cap_vio,
                                                     size_t max_fanout_vio,
                                                     bool verbose)
{
  if (inst == nullptr || resizer_->dontTouch(inst)) {
    return false;
  }

  LibertyCell* curr_cell = network_->libertyCell(inst);
  if (curr_cell == nullptr || !curr_cell->isBuffer()) {
    return false;
  }

  const bool drives_clock = instanceDrivesClock(inst);
  if (drives_clock) {
    return false;
  }

  return tryRemoveBuffer(inst,
                         wns_floor,
                         hold_floor,
                         max_slew_vio,
                         max_cap_vio,
                         max_fanout_vio,
                         verbose);
}

void RecoverPowerMore::backoffToTimingFloor(const Slack wns_floor,
                                            const Slack hold_floor,
                                            const bool verbose)
{
  Slack wns;
  Vertex* wns_vertex;
  sta_->worstSlack(max_, wns, wns_vertex);
  (void) wns_vertex;

  Slack hold;
  Vertex* hold_vertex;
  sta_->worstSlack(min_, hold, hold_vertex);
  (void) hold_vertex;

  int undone_actions = 0;
  int undone_swaps = 0;
  int undone_buffers = 0;
  while ((wns < wns_floor || hold < hold_floor) && !action_history_.empty()) {
    const ActionRecord action = action_history_.back();
    action_history_.pop_back();

    bool did_undo = false;
    if (action.type == ActionType::kSwap && action.prev_cell != nullptr) {
      sta::Instance* inst = network_->findInstance(action.inst_name.c_str());
      if (inst == nullptr) {
        continue;
      }
      // Undo the swap directly (avoid ECO restore, which can perturb routing).
      if (!resizer_->replaceCell(inst, action.prev_cell, /* journal */ false)) {
        continue;
      }
      if (auto* gr = estimate_parasitics_->getGlobalRouter()) {
        gr->discardDirtyNets();
      }
      estimate_parasitics_->updateParasitics(false);
      sta_->updateTiming(false);
      sta_->findRequireds();
      did_undo = true;
    } else if (action.type == ActionType::kRemoveBuffer) {
      // Buffer removals are applied via ECO journal checkpoints. Undo the last
      // saved ECO so we can back off without leaving the routing/parasitics
      // state inconsistent.
      resizer_->journalRestore();
      init();  // journalRestore reinitializes the resizer.
      if (auto* gr = estimate_parasitics_->getGlobalRouter()) {
        gr->discardDirtyNets();
      }
      estimate_parasitics_->updateParasitics(false);
      sta_->updateTiming(false);
      sta_->findRequireds();
      did_undo = true;
    }

    if (!did_undo) {
      continue;
    }

    if (action.type == ActionType::kSwap) {
      if (resize_count_ > 0) {
        resize_count_--;
      }
      undone_swaps++;
    } else {
      if (buffer_remove_count_ > 0) {
        buffer_remove_count_--;
      }
      undone_buffers++;
    }
    undone_actions++;

    sta::Vertex* wns_after_vertex;
    sta_->worstSlack(max_, wns, wns_after_vertex);
    (void) wns_after_vertex;

    sta::Vertex* hold_after_vertex;
    sta_->worstSlack(min_, hold, hold_after_vertex);
    (void) hold_after_vertex;

    if (verbose) {
      const char* kind
          = (action.type == ActionType::kSwap) ? "swap" : "remove_buffer";
      const std::string from_name
          = action.prev_cell != nullptr ? action.prev_cell->name() : "";
      const std::string to_name
          = action.new_cell != nullptr ? action.new_cell->name() : "";
      debugPrint(logger_,
                 RSZ,
                 "recover_power_more",
                 1,
                 "Backoff undo {} {} {}->{} (wns={} floor={})",
                 kind,
                 action.inst_name,
                 from_name,
                 to_name,
                 delayAsString(wns, sta_, kDigits),
                 delayAsString(wns_floor, sta_, kDigits));
    }
  }

  if (undone_actions > 0) {
    logger_->warn(
        RSZ,
        170,
        "Power recovery exceeded timing budget; backed off {} actions "
        "({} swaps, {} buffer removals) (wns {} floor {}, hold {} "
        "floor {}).",
        undone_actions,
        undone_swaps,
        undone_buffers,
        delayAsString(wns, sta_, kDigits),
        delayAsString(wns_floor, sta_, kDigits),
        delayAsString(hold, sta_, kDigits),
        delayAsString(hold_floor, sta_, kDigits));
  }

  if (wns < wns_floor || hold < hold_floor) {
    logger_->warn(RSZ,
                  171,
                  "Power recovery timing budget still violated after backoff "
                  "(wns {} floor {}, hold {} floor {}).",
                  delayAsString(wns, sta_, kDigits),
                  delayAsString(wns_floor, sta_, kDigits),
                  delayAsString(hold, sta_, kDigits),
                  delayAsString(hold_floor, sta_, kDigits));
  }
}

bool RecoverPowerMore::meetsSizeCriteria(const LibertyCell* cell,
                                         const LibertyCell* candidate) const
{
  const odb::dbMaster* candidate_cell = db_network_->staToDb(candidate);
  const odb::dbMaster* curr_cell = db_network_->staToDb(cell);
  if (candidate_cell == nullptr || curr_cell == nullptr) {
    return false;
  }

  return candidate_cell->getWidth() <= curr_cell->getWidth()
         && candidate_cell->getHeight() == curr_cell->getHeight();
}

int RecoverPowerMore::fanout(Vertex* vertex) const
{
  int fanout = 0;
  sta::VertexOutEdgeIterator edge_iter(vertex, graph_);
  while (edge_iter.hasNext()) {
    edge_iter.next();
    fanout++;
  }
  return fanout;
}

void RecoverPowerMore::printProgress(int iteration,
                                     int total_iterations,
                                     bool force,
                                     bool end) const
{
  const bool start = iteration == 0;

  if (start && !end) {
    logger_->report(
        "Iteration |   Area    | Swaps/Rm |   WNS    | Power (W) | "
        "Slew/Cap/Fo");
    logger_->report(
        "----------------------------------------------------------------------"
        "-");
  }

  if (force || end
      || (print_interval_ > 0 && iteration % print_interval_ == 0)) {
    Slack wns;
    Vertex* worst_vertex;
    sta_->worstSlack(max_, wns, worst_vertex);

    const double design_area = resizer_->computeDesignArea();
    const double area_growth = design_area - initial_design_area_;
    double area_growth_percent = std::numeric_limits<double>::infinity();
    if (std::abs(initial_design_area_) > 0.0) {
      area_growth_percent = area_growth / initial_design_area_ * 100.0;
    }

    const float power_total = designTotalPower(corner_);
    const size_t slew_v = curr_max_slew_violations_;
    const size_t cap_v = curr_max_cap_violations_;
    const size_t fanout_v = curr_max_fanout_violations_;

    std::string itr_field = fmt::format("{}", iteration);
    if (end) {
      itr_field = "final";
    }

    const std::string wns_str = delayAsString(wns, sta_, kDigits);
    const std::string pow_str
        = (power_total >= 0.0f) ? fmt::format("{:.6g}", power_total) : "N/A";
    const std::string ops_str
        = fmt::format("{}/{}", resize_count_, buffer_remove_count_);

    logger_->report(
        "{: >9s} | {: >+8.1f}% | {: >7s} | {: >8s} | {: >9s} | {}/{}/{}",
        itr_field,
        area_growth_percent,
        ops_str,
        wns_str,
        pow_str,
        slew_v,
        cap_v,
        fanout_v);

    if ((start || end) && initial_power_total_ >= 0.0f && power_total >= 0.0f) {
      const float pct = (initial_power_total_ != 0.0f)
                            ? (power_total - initial_power_total_)
                                  / initial_power_total_ * 100.0f
                            : 0.0f;
      logger_->report("  Power delta vs start: {:.3f}%", pct);
    }

    (void) total_iterations;
    (void) worst_vertex;
  }

  if (end) {
    logger_->report(
        "----------------------------------------------------------------------"
        "-");
  }
}

}  // namespace rsz
