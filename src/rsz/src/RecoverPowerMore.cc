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
  resizer_->buffer_moved_into_core_ = false;

  // Ensure clock network exists for sta_->isClock().
  sta_->ensureClkNetwork();

  // Baseline timing + DRV limits: do not regress.
  Slack worst_slack_before;
  Vertex* worst_vertex;
  sta_->worstSlack(max_, worst_slack_before, worst_vertex);
  (void) worst_vertex;

  Slack worst_hold_before;
  Vertex* worst_hold_vertex;
  sta_->worstSlack(min_, worst_hold_before, worst_hold_vertex);
  (void) worst_hold_vertex;

  // Keep timing closed if it is closed (WNS >= 0), otherwise do not worsen
  // the current worst slack by default. For already-failing designs, allow a
  // small WNS degradation budget to trade performance for power.
  const Slack wns_floor
      = computeWnsFloor(worst_slack_before, recover_power_percent);
  wns_floor_ = wns_floor;
  const Slack hold_floor = (worst_hold_before >= 0.0) ? 0.0 : worst_hold_before;
  hold_floor_ = hold_floor;

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

  est::IncrementalParasiticsGuard guard(estimate_parasitics_);
  // Multi-pass loop: after each batch, recompute slacks and keep spending
  // available slack headroom on non-clock cells.
  int iteration = 0;
  int accepted_in_last_pass = 0;
  printProgress(0, 0, true, false);

  for (int pass = 0; pass < max_passes_; ++pass) {
    Slack current_wns;
    Vertex* current_wns_vertex;
    sta_->worstSlack(max_, current_wns, current_wns_vertex);
    (void) current_wns_vertex;

    const auto candidates = collectCandidates(current_wns);
    if (candidates.empty()) {
      break;
    }

    // Rank by "power × available headroom" so we focus effort on high-power,
    // non-critical instances. Then break ties by power/headroom/area.
    std::vector<CandidateInstance> sorted = candidates;
    std::ranges::sort(
        sorted, [](const CandidateInstance& a, const CandidateInstance& b) {
          const float a_score
              = a.power * static_cast<float>(std::max<Slack>(0.0, a.headroom));
          const float b_score
              = b.power * static_cast<float>(std::max<Slack>(0.0, b.headroom));
          if (a_score != b_score) {
            return a_score > b_score;
          }
          if (a.power != b.power) {
            return a.power > b.power;
          }
          if (a.headroom != b.headroom) {
            return a.headroom > b.headroom;
          }
          return a.area > b.area;
        });

    int max_inst_count
        = static_cast<int>(std::ceil(sorted.size() * recover_power_percent));
    max_inst_count = std::clamp(max_inst_count, 1, (int) sorted.size());

    // Print progress roughly every ~1% (bounded).
    print_interval_ = std::clamp(
        max_inst_count / 100, min_print_interval_, max_print_interval_);

    accepted_in_last_pass = 0;
    int consecutive_rejects = 0;
    // Stop burning runtime on long tails of rejected candidates. Candidates are
    // processed in descending score order, so after enough consecutive rejects
    // further candidates are unlikely to be acceptable.
    const int max_consecutive_rejects
        = std::clamp(max_inst_count / 10, 1000, 20000);
    for (int i = 0; i < max_inst_count; ++i) {
      ++iteration;
      const CandidateInstance& cand = sorted[i];
      if (optimizeInstance(cand.inst,
                           wns_floor,
                           hold_floor,
                           initial_max_slew_violations_,
                           initial_max_cap_violations_,
                           initial_max_fanout_violations_,
                           verbose)) {
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
                                        const float recover_power_percent) const
{
  if (worst_setup_before >= 0.0) {
    return 0.0;
  }

  const float min_period = minClockPeriod();
  if (!(min_period > 0.0f)) {
    return worst_setup_before;
  }

  const float percent = std::clamp(recover_power_percent, 0.0f, 1.0f);
  const Slack budget = static_cast<Slack>(max_wns_degrade_frac_of_period_
                                          * percent * min_period);
  return worst_setup_before - budget;
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

  resizer_->journalBegin();

  if (!resizer_->replaceCell(inst, replacement, /* journal */ true)) {
    resizer_->journalEnd();
    return false;
  }

  estimate_parasitics_->updateParasitics(true);
  sta_->findRequireds();

  // Local guard: don't turn a previously positive-slack instance into a
  // setup-violating one.
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
    resizer_->journalRestore();
    init();  // journalRestore reinitializes the resizer.
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

  resizer_->journalEnd();
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

  estimate_parasitics_->updateParasitics(true);
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

  resizer_->journalEnd();
  curr_max_slew_violations_ = static_cast<size_t>(new_slew);
  curr_max_cap_violations_ = static_cast<size_t>(new_cap);
  curr_max_fanout_violations_ = static_cast<size_t>(new_fanout);
  buffer_remove_count_++;
  return true;
}

bool RecoverPowerMore::optimizeInstance(sta::Instance* inst,
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
  if (curr_cell == nullptr) {
    return false;
  }

  const std::string inst_name = network_->pathName(inst);
  const bool drives_clock = instanceDrivesClock(inst);

  // Buffer removal can provide a large power reduction when the buffer is
  // redundant. Avoid clock networks where the buffer structure is deliberate.
  if (curr_cell->isBuffer() && !drives_clock) {
    if (tryRemoveBuffer(inst,
                        wns_floor,
                        hold_floor,
                        max_slew_vio,
                        max_cap_vio,
                        max_fanout_vio,
                        verbose)) {
      return true;
    }
    // Journal restore during buffer removal may invalidate instance pointers.
    inst = network_->findInstance(inst_name.c_str());
    if (inst == nullptr) {
      return false;
    }
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
