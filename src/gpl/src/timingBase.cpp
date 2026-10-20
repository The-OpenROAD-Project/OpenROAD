// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#include "timingBase.h"

#include <algorithm>
#include <boost/scope_exit.hpp>
#include <cmath>
#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "grt/GlobalRouter.h"
#include "nesterovBase.h"
#include "odb/db.h"
#include "placerBase.h"
#include "rsz/Resizer.hh"
#include "sta/Fuzzy.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "utl/Logger.h"

namespace gpl {

using utl::GPL;

size_t slackRampAnchorIndex(const size_t net_count, const float nets_percentage)
{
  if (nets_percentage <= 0.0f) {
    return 0;
  }
  // net_count covers nets_percentage % of the constrained nets, so dividing by
  // it expresses the anchor as a percentage of the whole design instead.
  const auto anchor
      = static_cast<size_t>(net_count * kSlackMinPercent / nets_percentage);
  return std::min(net_count - 1, anchor);
}

float computeTimingNetWeight(const float net_slack,
                             const float slack_min,
                             const float slack_max,
                             const float net_weight_max)
{
  if (slack_max <= slack_min) {
    return 1.0f;
  }
  const float criticality = std::clamp(
      (slack_max - net_slack) / (slack_max - slack_min), 0.0f, 1.0f);
  return 1.0f + (net_weight_max - 1.0f) * criticality;
}

// TimingBase
TimingBase::TimingBase() = default;

TimingBase::TimingBase(std::shared_ptr<NesterovBaseCommon> nbc,
                       grt::GlobalRouter* grt,
                       rsz::Resizer* rs,
                       utl::Logger* log)
    : TimingBase()
{
  grt_ = grt;
  rs_ = rs;
  nbc_ = std::move(nbc);
  log_ = log;
}

void TimingBase::initTimingOverflowChk()
{
  timingOverflowChk_.clear();
  timingOverflowChk_.resize(timingNetWeightOverflow_.size(), false);
}

bool TimingBase::isTimingNetWeightOverflow(float overflow)
{
  int intOverflow = std::round(overflow * 100);
  // exception case handling
  if (timingNetWeightOverflow_.empty()
      || intOverflow > timingNetWeightOverflow_[0]) {
    return false;
  }

  bool needTdRun = false;
  for (int i = 0; i < timingNetWeightOverflow_.size(); i++) {
    if (timingNetWeightOverflow_[i] > intOverflow) {
      if (!timingOverflowChk_[i]) {
        timingOverflowChk_[i] = true;
        needTdRun = true;
      }
      continue;
    }
    return needTdRun;
  }
  return needTdRun;
}

void TimingBase::addTimingNetWeightOverflow(int overflow)
{
  std::vector<int>::iterator it
      = std::ranges::find(timingNetWeightOverflow_, overflow);

  // only push overflow when the overflow is not in vector.
  if (it == timingNetWeightOverflow_.end()) {
    timingNetWeightOverflow_.push_back(overflow);
  }

  // do sort in reverse order
  std::ranges::sort(timingNetWeightOverflow_, std::greater<int>());
}

void TimingBase::setTimingNetWeightOverflows(const std::vector<int>& overflows)
{
  // sort by decreasing order
  auto sorted = overflows;
  std::ranges::sort(sorted, std::greater<int>());
  for (auto& overflow : sorted) {
    addTimingNetWeightOverflow(overflow);
  }
  initTimingOverflowChk();
}

void TimingBase::deleteTimingNetWeightOverflow(int overflow)
{
  std::vector<int>::iterator it
      = std::ranges::find(timingNetWeightOverflow_, overflow);
  // only erase overflow when the overflow is in vector.
  if (it != timingNetWeightOverflow_.end()) {
    timingNetWeightOverflow_.erase(it);
  }
}

void TimingBase::clearTimingNetWeightOverflow()
{
  timingNetWeightOverflow_.clear();
}

size_t TimingBase::getTimingNetWeightOverflowSize() const
{
  return timingNetWeightOverflow_.size();
}

void TimingBase::setTimingNetWeightMax(float max)
{
  net_weight_max_ = max;
}

void TimingBase::setTimingNetsPercentage(float percentage)
{
  nets_percentage_ = percentage;
}

bool TimingBase::executeTimingDriven(bool run_journal_restore,
                                     bool enable_repair_timing)
{
  if (incremental_sta_ != nullptr) {
    return executeIncrementalTimingDriven();
  }

  {
    BOOST_SCOPE_EXIT_ALL(&)
    {
      if (!run_journal_restore) {
        nbc_->fixPointers();
      }
    };

    rs_->findResizeSlacks(run_journal_restore,
                          (enable_repair_timing && repair_timing_),
                          repair_tns_end_percent_);
  }

  // get worst resize nets
  rs_->setWorstSlackNetsPercent(nets_percentage_);
  sta::NetSeq worst_slack_nets = rs_->resizeWorstSlackNets();

  if (worst_slack_nets.empty()) {
    log_->warn(
        GPL,
        105,
        "Timing-driven: no net slacks found. Timing-driven mode disabled.");
    return false;
  }

  // worst_slack_nets is sorted by increasing slack, so the ramp runs from the
  // anchor percentile up to the best slack still in the list.
  const size_t slack_min_index
      = slackRampAnchorIndex(worst_slack_nets.size(), nets_percentage_);
  const auto worst_slack
      = rs_->resizeNetSlack(worst_slack_nets.front()).value();
  const auto slack_min
      = rs_->resizeNetSlack(worst_slack_nets[slack_min_index]).value();
  const auto slack_max = rs_->resizeNetSlack(worst_slack_nets.back()).value();

  if (sta::fuzzyInf(worst_slack)) {
    log_->warn(GPL,
               102,
               "Timing-driven: no slacks found. Timing-driven mode disabled.");
    return false;
  }

  log_->info(GPL, 106, "Timing-driven: worst slack {}", worst_slack);
  debugPrint(log_,
             GPL,
             "timing",
             1,
             "Timing-driven: weight ramp {} to {}, anchored on net {} of {}.",
             slack_min,
             slack_max,
             slack_min_index + 1,
             worst_slack_nets.size());

  int weighted_net_count = 0;
  for (auto& gNet : nbc_->getGNets()) {
    // default weight
    gNet->setTimingWeight(1.0);
    if (gNet->getGPins().size() > 1) {
      auto net_slack_opt = rs_->resizeNetSlack(gNet->getPbNet()->getDbNet());
      if (!net_slack_opt) {
        continue;
      }
      auto net_slack = net_slack_opt.value();
      if (net_slack < slack_max) {
        gNet->setTimingWeight(computeTimingNetWeight(
            net_slack, slack_min, slack_max, net_weight_max_));
        weighted_net_count++;
      }
      debugPrint(log_,
                 GPL,
                 "timing",
                 1,
                 "net:{} slack:{} weight:{}",
                 gNet->getPbNet()->getDbNet()->getConstName(),
                 net_slack,
                 gNet->getTotalWeight());
    }
  }

  debugPrint(log_,
             GPL,
             "timing",
             1,
             "Timing-driven: weighted {} nets.",
             weighted_net_count);
  return true;
}

bool TimingBase::executeIncrementalTimingDriven()
{
  rs_->getEstimateParasitics()->estimateParasitics(
      est::ParasiticsSrc::kPlacement);
  std::vector<std::pair<GNet*, float>> nets;
  auto network = incremental_sta_->getDbNetwork();
  for (auto& net : nbc_->getGNets()) {
    net->setTimingWeight(1.0f);
    auto db_net = net->getPbNet()->getDbNet();
    if (db_net == nullptr || db_net->isSpecial()
        || db_net->getSigType() == odb::dbSigType::CLOCK
        || net->getGPins().size() < 2) {
      continue;
    }
    const auto sta_net = network->dbToSta(db_net);
    if (incremental_sta_->isClock(sta_net, incremental_sta_->cmdMode())) {
      continue;
    }
    const float slack = incremental_sta_->slack(sta_net, sta::MinMax::max());
    if (std::isfinite(slack) && !sta::fuzzyInf(slack)) {
      nets.emplace_back(net, slack);
    }
  }
  std::ranges::sort(nets, [](const auto& lhs, const auto& rhs) {
    if (lhs.second != rhs.second) {
      return lhs.second < rhs.second;
    }
    return lhs.first->getPbNet()->getDbNet()->getId()
           < rhs.first->getPbNet()->getDbNet()->getId();
  });
  const auto count
      = static_cast<size_t>(std::ceil(nets.size() * nets_percentage_ / 100.0));
  nets.resize(std::min(count, nets.size()));
  if (nets.empty()) {
    return false;
  }
  const float slack_min
      = nets[slackRampAnchorIndex(nets.size(), nets_percentage_)].second;
  const float slack_max = nets.back().second;
  for (const auto& [net, slack] : nets) {
    // Unlike the full-placement ramp, a single critical net still needs a
    // force: incremental edits often affect only a tiny constrained cone.
    net->setTimingWeight(
        slack_max <= slack_min
            ? net_weight_max_
            : computeTimingNetWeight(
                  slack, slack_min, slack_max, net_weight_max_));
  }
  return true;
}

bool IncrementalTimingMetrics::isValid() const
{
  return std::isfinite(worst_slack) && !sta::fuzzyInf(worst_slack)
         && std::isfinite(total_negative_slack)
         && !sta::fuzzyInf(total_negative_slack);
}

bool IncrementalTimingMetrics::degrades(
    const IncrementalTimingMetrics& baseline) const
{
  return !isValid() || !baseline.isValid()
         || sta::fuzzyLess(worst_slack, baseline.worst_slack)
         || sta::fuzzyLess(total_negative_slack, baseline.total_negative_slack);
}

IncrementalTimingGuard::IncrementalTimingGuard(
    sta::dbSta* sta,
    est::EstimateParasitics* parasitics,
    odb::dbBlock* block)
    : sta_(sta), parasitics_(parasitics), block_(block)
{
}

IncrementalTimingMetrics IncrementalTimingGuard::measure()
{
  parasitics_->estimateParasitics(est::ParasiticsSrc::kPlacement);
  return {sta_->worstSlack(sta::MinMax::max()),
          sta_->totalNegativeSlack(sta::MinMax::max())};
}

bool IncrementalTimingGuard::checkpoint()
{
  active_ = false;
  locations_.clear();
  // Preserve wirelength-only operation for designs without timing data.
  // worstSlack() requires Liberty, even when there are no timing constraints.
  if (sta_->getDbNetwork()->defaultLibertyLibrary() == nullptr) {
    return false;
  }
  if (sta::fuzzyInf(sta_->worstSlack(sta::MinMax::max()))) {
    return false;
  }
  for (const auto scene : sta_->scenes()) {
    const auto capacitance = parasitics_->wireSignalCapacitance(scene);
    const auto resistance = parasitics_->wireSignalResistance(scene);
    if (!std::isfinite(capacitance) || capacitance <= 0.0
        || !std::isfinite(resistance) || resistance <= 0.0) {
      return false;
    }
  }
  baseline_ = measure();
  if (!baseline_.isValid()) {
    return false;
  }
  for (auto inst : block_->getInsts()) {
    int x;
    int y;
    inst->getLocation(x, y);
    locations_.push_back({inst, x, y});
  }
  active_ = true;
  return true;
}

bool IncrementalTimingGuard::restoreIfWorse()
{
  if (!active_) {
    return false;
  }
  const auto candidate = measure();
  active_ = false;
  if (!candidate.degrades(baseline_)) {
    return false;
  }
  for (const auto& [inst, x, y] : locations_) {
    inst->setLocation(x, y);
  }
  // Rebuild RC after restoring coordinates so the caller never sees timing
  // cached for the rejected placement.
  measure();
  return true;
}

}  // namespace gpl
