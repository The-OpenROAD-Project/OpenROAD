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

#include "grt/GlobalRouter.h"
#include "nesterovBase.h"
#include "placerBase.h"
#include "rsz/Resizer.hh"
#include "sta/Fuzzy.hh"
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

}  // namespace gpl
