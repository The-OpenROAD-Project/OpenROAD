// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "SACoreHardMacro.h"

#include <cmath>
#include <vector>

#include "MplObserver.h"
#include "SimulatedAnnealingCore.h"
#include "clusterEngine.h"
#include "mpl-util.h"
#include "object.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace mpl {
using utl::MPL;

//////////////////////////////////////////////////////////////////
// Class SACoreHardMacro
// constructors
SACoreHardMacro::SACoreHardMacro(PhysicalHierarchy* tree,
                                 const odb::Rect& outline,
                                 const std::vector<HardMacro>& macros,
                                 const SACoreWeights& core_weights,
                                 // probability of each action
                                 float pos_swap_prob,
                                 float neg_swap_prob,
                                 float double_swap_prob,
                                 float exchange_prob,
                                 // Fast SA hyperparameter
                                 float init_prob,
                                 int max_num_step,
                                 int num_perturb_per_step,
                                 unsigned seed,
                                 MplObserver* graphics,
                                 utl::Logger* logger,
                                 odb::dbBlock* block)
    : SimulatedAnnealingCore<HardMacro>(tree,
                                        outline,
                                        macros,
                                        core_weights,
                                        pos_swap_prob,
                                        neg_swap_prob,
                                        double_swap_prob,
                                        exchange_prob,
                                        init_prob,
                                        max_num_step,
                                        num_perturb_per_step,
                                        seed,
                                        graphics,
                                        logger,
                                        block)
{
}

void SACoreHardMacro::run()
{
  if (graphics_) {
    graphics_->startSA("hard", max_num_step_, num_perturb_per_step_);
  }

  fastSA();

  if (graphics_) {
    graphics_->endSA(calNormCost());
  }
}

float SACoreHardMacro::calNormCost() const
{
  return sumNormalizedPenalties();
}

void SACoreHardMacro::calPenalty()
{
  calAreaPenalty();
  calOutlinePenalty();
  calWirelength();
  calGuidancePenalty();
  calFencePenalty();
  if (graphics_) {
    graphics_->penaltyCalculated(calNormCost());
  }
}

void SACoreHardMacro::perturb()
{
  if (macros_.empty()) {
    return;
  }

  // generate random number (0 - 1) to determine actions
  const float op = distribution_(generator_);
  const float action_prob_1 = pos_swap_prob_;
  const float action_prob_2 = action_prob_1 + neg_swap_prob_;
  const float action_prob_3 = action_prob_2 + double_swap_prob_;

  if (op <= action_prob_1) {
    action_id_ = 1;
    singleSeqSwap(true);  // Swap two macros in pos_seq_
  } else if (op <= action_prob_2) {
    action_id_ = 2;
    singleSeqSwap(false);  // Swap two macros in neg_seq_;
  } else if (op <= action_prob_3) {
    action_id_ = 3;
    doubleSeqSwap();  // Swap two macros in pos_seq_ and
                      // other two macros in neg_seq_
  } else {
    action_id_ = 4;
    exchangeMacros();  // exchange two macros in the sequence pair
  }

  // update the macro locations based on Sequence Pair
  packFloorplan();
  // Update all the penalties
  calPenalty();
}

void SACoreHardMacro::saveState()
{
  if (macros_.empty()) {
    return;
  }

  pre_macros_ = macros_;
  pre_pos_seq_ = pos_seq_;
  pre_neg_seq_ = neg_seq_;

  pre_width_ = width_;
  pre_height_ = height_;

  savePenalties();
}

void SACoreHardMacro::restoreState()
{
  if (macros_.empty()) {
    return;
  }

  // To reduce the runtime, here we do not call PackFloorplan
  // again. So when we need to generate the final floorplan out,
  // we need to call PackFloorplan again at the end of SA process
  if (action_id_ == 1) {
    pos_seq_ = pre_pos_seq_;
  } else if (action_id_ == 2) {
    neg_seq_ = pre_neg_seq_;
  } else if (action_id_ == 3 || action_id_ == 4) {
    pos_seq_ = pre_pos_seq_;
    neg_seq_ = pre_neg_seq_;
  }

  macros_ = pre_macros_;

  width_ = pre_width_;
  height_ = pre_height_;

  restorePenalties();
}

void SACoreHardMacro::setWeights(const SACoreWeights& weights)
{
  core_weights_ = weights;

  // calNormCost() sums penalties_[i].weight, so keep it in sync.
  penalties_[kArea].weight = core_weights_.area;
  penalties_[kOutline].weight = core_weights_.outline;
  penalties_[kWirelength].weight = core_weights_.wirelength;
  penalties_[kGuidance].weight = core_weights_.guidance;
  penalties_[kFence].weight = core_weights_.fence;
}

void SACoreHardMacro::printResults() const
{
  reportCoreWeights();
  reportTotalCost();
  if (logger_->debugCheck(MPL, "hierarchical_macro_placement", 2)) {
    reportLocations();
  }
}

}  // namespace mpl
