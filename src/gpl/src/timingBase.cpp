// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#include "timingBase.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "nesterovBase.h"
#include "placerBase.h"
#include "rsz/Resizer.hh"
#include "sta/Fuzzy.hh"
#include "sta/NetworkClass.hh"
#include "utl/Logger.h"

namespace gpl {

using utl::GPL;

namespace {
bool envVarTruthy(const char* name)
{
  const char* raw = std::getenv(name);
  if (raw == nullptr || *raw == '\0') {
    return false;
  }

  std::string value(raw);
  const size_t start = value.find_first_not_of(" \t\n\r");
  if (start == std::string::npos) {
    return false;
  }
  const size_t end = value.find_last_not_of(" \t\n\r");
  value = value.substr(start, end - start + 1);
  std::transform(
      value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
      });
  return value == "1" || value == "true" || value == "yes" || value == "on";
}

bool useOrfsNewOpenroad()
{
  return envVarTruthy("ORFS_ENABLE_NEW_OPENROAD");
}
}  // namespace

// TimingBase
TimingBase::TimingBase()
{
  if (useOrfsNewOpenroad()) {
    loadEnvOverrides();
  }
}

TimingBase::TimingBase(std::shared_ptr<NesterovBaseCommon> nbc,
                       rsz::Resizer* rs,
                       utl::Logger* log)
    : TimingBase()
{
  rs_ = rs;
  nbc_ = std::move(nbc);
  log_ = log;
  if (useOrfsNewOpenroad()) {
    loadEnvOverrides();
  }
}

void TimingBase::initTimingOverflowChk()
{
  timingOverflowChk_.clear();
  timingOverflowChk_.resize(timingNetWeightOverflow_.size(), false);
}

namespace {
std::optional<float> getEnvFloat(const char* name, utl::Logger* log)
{
  const char* raw = std::getenv(name);
  if (raw == nullptr || *raw == '\0') {
    return std::nullopt;
  }

  char* end = nullptr;
  const float value = std::strtof(raw, &end);
  if (end == raw || (end != nullptr && *end != '\0')) {
    if (log != nullptr) {
      log->warn(GPL, 124, "Ignoring {}='{}' (not a valid float).", name, raw);
    }
    return std::nullopt;
  }
  return value;
}
}  // namespace

void TimingBase::loadEnvOverrides()
{
  if (!useOrfsNewOpenroad()) {
    return;
  }

  if (auto env_max = getEnvFloat("GPL_WEIGHT_MAX", log_)) {
    if (*env_max > 0.0F) {
      net_weight_max_ = *env_max;
    } else if (log_ != nullptr) {
      log_->warn(
          GPL, 125, "Ignoring GPL_WEIGHT_MAX={} (must be > 0).", *env_max);
    }
  }

  if (auto env_exp = getEnvFloat("GPL_WEIGHT_EXP", log_)) {
    if (*env_exp > 0.0F) {
      net_weight_exponent_ = *env_exp;
    } else if (log_ != nullptr) {
      log_->warn(
          GPL, 126, "Ignoring GPL_WEIGHT_EXP={} (must be > 0).", *env_exp);
    }
  }

  const char* raw_use_zero_ref = std::getenv("GPL_WEIGHT_USE_ZERO_REF");
  if (raw_use_zero_ref != nullptr && *raw_use_zero_ref != '\0') {
    // Treat any non-zero value as enabled. Keep silent on parse issues to avoid
    // plumbing yet another message id just for an experimental knob.
    use_zero_slack_ref_ = (std::atoi(raw_use_zero_ref) != 0);
  }

  if (auto env_cov = getEnvFloat("GPL_WEIGHT_COVERAGE", log_)) {
    // Interpret as percent of the "worst nets" set to use as the cutoff.
    // Zero disables; values outside (0,100] are ignored.
    if (*env_cov > 0.0F && *env_cov <= 100.0F) {
      net_weight_coverage_percent_ = *env_cov;
    }
  }

  const char* raw_len_factor = std::getenv("GPL_WEIGHT_USE_LENGTH_FACTOR");
  if (raw_len_factor != nullptr && *raw_len_factor != '\0') {
    use_length_factor_ = (std::atoi(raw_len_factor) != 0);
  }

  if (auto env_len_alpha = getEnvFloat("GPL_WEIGHT_LENGTH_ALPHA", log_)) {
    // Clamp to [0,1] and stay silent on range violations (experimental knob).
    length_alpha_ = std::clamp(*env_len_alpha, 0.0F, 1.0F);
  }
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

bool TimingBase::executeTimingDriven(bool run_journal_restore)
{
  rs_->findResizeSlacks(run_journal_restore);

  if (!run_journal_restore) {
    nbc_->fixPointers();
  }

  // get worst resize nets
  sta::NetSeq worst_slack_nets = rs_->resizeWorstSlackNets();

  if (worst_slack_nets.empty()) {
    log_->warn(
        GPL,
        105,
        "Timing-driven: no net slacks found. Timing-driven mode disabled.");
    return false;
  }

  // min/max slack for worst nets
  const auto slack_min_opt = rs_->resizeNetSlack(worst_slack_nets[0]);
  const auto slack_max_opt
      = rs_->resizeNetSlack(worst_slack_nets[worst_slack_nets.size() - 1]);
  if (!slack_min_opt || !slack_max_opt) {
    log_->warn(
        GPL,
        111,
        "Timing-driven: missing net slack. Timing-driven mode disabled.");
    return false;
  }
  auto slack_min = *slack_min_opt;
  auto slack_max = *slack_max_opt;

  log_->info(GPL, 106, "Timing-driven: worst slack {:.3g}", slack_min);

  if (sta::fuzzyInf(slack_min)) {
    log_->warn(GPL,
               102,
               "Timing-driven: no slacks found. Timing-driven mode disabled.");
    return false;
  }

  if (!useOrfsNewOpenroad()) {
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
          if (slack_max == slack_min) {
            gNet->setTimingWeight(1.0);
          } else {
            // weight(min_slack) = net_weight_max_
            // weight(max_slack) = 1
            const float weight = 1
                                 + (net_weight_max_ - 1)
                                       * (slack_max - net_slack)
                                       / (slack_max - slack_min);
            gNet->setTimingWeight(weight);
          }
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

  int weighted_net_count = 0;
  using SlackT = decltype(slack_max);
  const SlackT slack_zero = 0.0;
  SlackT slack_ref = slack_max;
  if (use_zero_slack_ref_) {
    slack_ref = slack_zero;
    if (net_weight_coverage_percent_ > 0.0F && !worst_slack_nets.empty()) {
      const size_t worst_count = worst_slack_nets.size();
      size_t coverage_count = static_cast<size_t>(
          std::ceil(worst_count * net_weight_coverage_percent_ / 100.0F));
      coverage_count = std::clamp<size_t>(coverage_count, 1, worst_count);
      if (worst_count >= 2) {
        coverage_count = std::max<size_t>(coverage_count, 2);
      }
      auto cutoff_opt
          = rs_->resizeNetSlack(worst_slack_nets[coverage_count - 1]);
      if (cutoff_opt) {
        slack_ref = std::min(cutoff_opt.value(), slack_zero);
      }
    }
  }

  double length_norm = 0.0;
  size_t length_norm_count = 0;
  const float length_alpha = std::clamp(length_alpha_, 0.0F, 1.0F);
  if (use_length_factor_) {
    for (auto& gNet : nbc_->getGNets()) {
      if (gNet->getGPins().size() <= 1) {
        continue;
      }
      auto net_slack_opt = rs_->resizeNetSlack(gNet->getPbNet()->getDbNet());
      if (!net_slack_opt) {
        continue;
      }
      const SlackT net_slack = net_slack_opt.value();
      if (!(net_slack < slack_ref)) {
        continue;
      }
      const double dx = static_cast<double>(gNet->ux() - gNet->lx());
      const double dy = static_cast<double>(gNet->uy() - gNet->ly());
      length_norm += std::hypot(dx, dy);
      length_norm_count++;
    }
    if (length_norm_count > 0) {
      length_norm /= static_cast<double>(length_norm_count);
    } else {
      length_norm = 0.0;
    }
  }
  for (auto& gNet : nbc_->getGNets()) {
    // default weight
    gNet->setTimingWeight(1.0);
    if (gNet->getGPins().size() > 1) {
      auto net_slack_opt = rs_->resizeNetSlack(gNet->getPbNet()->getDbNet());
      if (!net_slack_opt) {
        continue;
      }
      auto net_slack = net_slack_opt.value();
      if (net_slack < slack_ref) {
        if (slack_ref == slack_min) {
          gNet->setTimingWeight(1.0);
        } else {
          // weight(min_slack) = net_weight_max_
          // weight(max_slack) = 1
          const float normalized_slack
              = std::clamp(static_cast<float>((slack_ref - net_slack)
                                              / (slack_ref - slack_min)),
                           0.0F,
                           1.0F);
          float scaled_slack = std::pow(normalized_slack, net_weight_exponent_);
          if (use_length_factor_ && length_norm > 0.0) {
            const double dx = static_cast<double>(gNet->ux() - gNet->lx());
            const double dy = static_cast<double>(gNet->uy() - gNet->ly());
            const double len_metric = std::hypot(dx, dy);
            const float len_ratio = std::clamp(
                static_cast<float>(len_metric / length_norm), 0.0F, 1.0F);
            const float length_factor
                = (1.0F - length_alpha) + length_alpha * len_ratio;
            scaled_slack *= length_factor;
          }
          const float weight = 1 + (net_weight_max_ - 1) * scaled_slack;
          gNet->setTimingWeight(weight);
        }
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
