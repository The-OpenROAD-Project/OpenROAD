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

void SACoreHardMacro::initialize()
{
  initSequencePair();

  std::vector<float> area_penalty_list;
  std::vector<float> outline_penalty_list;
  std::vector<float> wirelength_list;
  std::vector<float> guidance_penalty_list;
  std::vector<float> fence_penalty_list;
  std::vector<float> width_list;
  std::vector<float> height_list;

  // We don't want to stop in the normalization factor setup
  MplObserver* save_graphics = graphics_;
  graphics_ = nullptr;

  for (int i = 0; i < num_perturb_per_step_; i++) {
    saveState();
    perturb();
    if (!invalid_states_allowed_ && !isValid()) {
      restoreState();
      continue;
    }

    // store current penalties
    width_list.push_back(width_);
    height_list.push_back(height_);
    area_penalty_list.push_back(penalties_[kArea].value);
    outline_penalty_list.push_back(penalties_[kOutline].value);
    wirelength_list.push_back(penalties_[kWirelength].value);
    guidance_penalty_list.push_back(penalties_[kGuidance].value);
    fence_penalty_list.push_back(penalties_[kFence].value);
  }
  graphics_ = save_graphics;

  penalties_[kArea].normalization_factor = calAverage(area_penalty_list);
  penalties_[kOutline].normalization_factor = calAverage(outline_penalty_list);
  penalties_[kWirelength].normalization_factor = calAverage(wirelength_list);
  penalties_[kGuidance].normalization_factor
      = calAverage(guidance_penalty_list);
  penalties_[kFence].normalization_factor = calAverage(fence_penalty_list);

  if (penalties_[kArea].normalization_factor <= 1e-4) {
    penalties_[kArea].normalization_factor = 1.0;
  }

  if (penalties_[kOutline].normalization_factor <= 1e-4) {
    penalties_[kOutline].normalization_factor = 1.0;
  }

  if (penalties_[kWirelength].normalization_factor <= 1e-4) {
    penalties_[kWirelength].normalization_factor = 1.0;
  }

  if (penalties_[kGuidance].normalization_factor <= 1e-4) {
    penalties_[kGuidance].normalization_factor = 1.0;
  }

  if (penalties_[kFence].normalization_factor <= 1e-4) {
    penalties_[kFence].normalization_factor = 1.0;
  }

  // Calculate initial temperature
  std::vector<float> cost_list;
  for (int i = 0; i < outline_penalty_list.size(); i++) {
    width_ = width_list[i];
    height_ = height_list[i];
    penalties_[kArea].value = area_penalty_list[i];
    penalties_[kOutline].value = outline_penalty_list[i];
    penalties_[kWirelength].value = wirelength_list[i];
    penalties_[kGuidance].value = guidance_penalty_list[i];
    penalties_[kFence].value = fence_penalty_list[i];
    cost_list.push_back(calNormCost());
  }
  float delta_cost = 0.0;
  for (int i = 1; i < cost_list.size(); i++) {
    delta_cost += std::abs(cost_list[i] - cost_list[i - 1]);
  }
  if (cost_list.size() > 1 && delta_cost > 0.0) {
    init_temperature_
        = (-1.0) * (delta_cost / (cost_list.size() - 1)) / std::log(init_prob_);
  } else {
    init_temperature_ = 1.0;
  }
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
