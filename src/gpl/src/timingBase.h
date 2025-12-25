// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "rsz/Resizer.hh"

namespace rsz {
class Resizer;
}

namespace utl {
class Logger;
}

namespace gpl {

class NesterovBaseCommon;
class GNet;

class TimingBase
{
 public:
  TimingBase();
  TimingBase(std::shared_ptr<NesterovBaseCommon> nbc,
             rsz::Resizer* rs,
             utl::Logger* log);

  // check whether overflow reached the timingOverflow
  bool isTimingNetWeightOverflow(float overflow);
  void addTimingNetWeightOverflow(int overflow);
  void setTimingNetWeightOverflows(std::vector<int>& overflows);
  void deleteTimingNetWeightOverflow(int overflow);
  void clearTimingNetWeightOverflow();
  size_t getTimingNetWeightOverflowSize() const;

  void setTimingNetWeightMax(float max);

  // updateNetWeight.
  // True: successfully reweighted gnets
  // False: no slacks found
  bool executeTimingDriven(bool run_journal_restore);

 private:
  void loadEnvOverrides();

  rsz::Resizer* rs_ = nullptr;
  utl::Logger* log_ = nullptr;
  std::shared_ptr<NesterovBaseCommon> nbc_;

  std::vector<int> timingNetWeightOverflow_;
  std::vector<int> timingOverflowChk_;
  float net_weight_max_ = 5;
  float net_weight_exponent_ = 1.0;
  // When enabled, normalize net criticality against 0 slack (violations)
  // instead of the best slack in the "worst nets" subset. This makes the
  // weighting less sensitive to slack distribution changes across STA
  // versions/corners, and avoids adding weight to positive-slack nets.
  bool use_zero_slack_ref_ = true;
  // Optional experimental knobs (off by default) to make the timing-driven
  // weighting less noisy under different STA semantics.
  float net_weight_coverage_percent_ = 0.0F;
  bool use_length_factor_ = false;
  float length_alpha_ = 0.5F;
  void initTimingOverflowChk();
};

}  // namespace gpl
