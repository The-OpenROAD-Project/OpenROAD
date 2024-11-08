///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include "SACoreSoftMacro.h"

#include "Mpl2Observer.h"
#include "utl/Logger.h"

namespace mpl2 {
using utl::MPL;

//////////////////////////////////////////////////////////////////
// Class SACoreSoftMacro
// constructors
SACoreSoftMacro::SACoreSoftMacro(
    Cluster* root,
    const Rect& outline,
    const std::vector<SoftMacro>& macros,
    // weight for different penalty
    float area_weight,
    float outline_weight,
    float wirelength_weight,
    float guidance_weight,
    float fence_weight,  // each blockage will be modeled by a macro with fences
    float boundary_weight,
    float macro_blockage_weight,
    float notch_weight,
    // notch threshold
    float notch_h_threshold,
    float notch_v_threshold,
    // probability of each action
    float pos_swap_prob,
    float neg_swap_prob,
    float double_swap_prob,
    float exchange_prob,
    float resize_prob,
    // Fast SA hyperparameter
    float init_prob,
    int max_num_step,
    int num_perturb_per_step,
    unsigned seed,
    Mpl2Observer* graphics,
    utl::Logger* logger)
    : SimulatedAnnealingCore<SoftMacro>(outline,
                                        macros,
                                        area_weight,
                                        outline_weight,
                                        wirelength_weight,
                                        guidance_weight,
                                        fence_weight,
                                        pos_swap_prob,
                                        neg_swap_prob,
                                        double_swap_prob,
                                        exchange_prob,
                                        init_prob,
                                        max_num_step,
                                        num_perturb_per_step,
                                        seed,
                                        graphics,
                                        logger),
      root_(root)
{
  boundary_weight_ = boundary_weight;
  macro_blockage_weight_ = macro_blockage_weight;
  original_notch_weight_ = notch_weight;
  resize_prob_ = resize_prob;
  notch_h_th_ = notch_h_threshold;
  notch_v_th_ = notch_v_threshold;
  adjust_h_th_ = notch_h_th_;
  adjust_v_th_ = notch_v_th_;
  logger_ = logger;
}

void SACoreSoftMacro::run()
{
  if (graphics_) {
    graphics_->startSA();
  }

  fastSA();

  if (centralization_on_) {
    attemptCentralization(calNormCost());
  }

  if (graphics_) {
    graphics_->endSA(calNormCost());
  }
}

// acessors functions
float SACoreSoftMacro::getBoundaryPenalty() const
{
  return boundary_penalty_;
}

float SACoreSoftMacro::getNormBoundaryPenalty() const
{
  return norm_boundary_penalty_;
}

float SACoreSoftMacro::getMacroBlockagePenalty() const
{
  return macro_blockage_penalty_;
}

float SACoreSoftMacro::getNormMacroBlockagePenalty() const
{
  return norm_macro_blockage_penalty_;
}

float SACoreSoftMacro::getNotchPenalty() const
{
  return notch_penalty_;
}

float SACoreSoftMacro::getAreaPenalty() const
{
  const float outline_area = outline_.getWidth() * outline_.getHeight();
  return (width_ * height_) / outline_area;
}

float SACoreSoftMacro::getNormNotchPenalty() const
{
  return norm_notch_penalty_;
}

// Operations
float SACoreSoftMacro::calNormCost() const
{
  float cost = 0.0;  // Initialize cost
  if (norm_area_penalty_ > 0.0) {
    cost += area_weight_ * getAreaPenalty();
  }
  if (norm_outline_penalty_ > 0.0) {
    cost += outline_weight_ * outline_penalty_ / norm_outline_penalty_;
  }
  if (norm_wirelength_ > 0.0) {
    cost += wirelength_weight_ * wirelength_ / norm_wirelength_;
  }
  if (norm_guidance_penalty_ > 0.0) {
    cost += guidance_weight_ * guidance_penalty_ / norm_guidance_penalty_;
  }
  if (norm_fence_penalty_ > 0.0) {
    cost += fence_weight_ * fence_penalty_ / norm_fence_penalty_;
  }
  if (norm_boundary_penalty_ > 0.0) {
    cost += boundary_weight_ * boundary_penalty_ / norm_boundary_penalty_;
  }
  if (norm_macro_blockage_penalty_ > 0.0) {
    cost += macro_blockage_weight_ * macro_blockage_penalty_
            / norm_macro_blockage_penalty_;
  }
  if (norm_notch_penalty_ > 0.0) {
    cost += notch_weight_ * notch_penalty_ / norm_notch_penalty_;
  }
  return cost;
}

void SACoreSoftMacro::calPenalty()
{
  calOutlinePenalty();
  calWirelength();
  calGuidancePenalty();
  calFencePenalty();
  calBoundaryPenalty();
  calMacroBlockagePenalty();
  calNotchPenalty();
  if (graphics_) {
    graphics_->setAreaPenalty({area_weight_, getAreaPenalty()});
    graphics_->penaltyCalculated(calNormCost());
  }
}

void SACoreSoftMacro::perturb()
{
  if (macros_.empty()) {
    return;
  }

  // Keep back up
  pre_pos_seq_ = pos_seq_;
  pre_neg_seq_ = neg_seq_;
  pre_width_ = width_;
  pre_height_ = height_;
  pre_outline_penalty_ = outline_penalty_;
  pre_wirelength_ = wirelength_;
  pre_guidance_penalty_ = guidance_penalty_;
  pre_fence_penalty_ = fence_penalty_;
  pre_boundary_penalty_ = boundary_penalty_;
  pre_macro_blockage_penalty_ = macro_blockage_penalty_;
  pre_notch_penalty_ = notch_penalty_;

  // generate random number (0 - 1) to determine actions
  const float op = distribution_(generator_);
  const float action_prob_1 = pos_swap_prob_;
  const float action_prob_2 = action_prob_1 + neg_swap_prob_;
  const float action_prob_3 = action_prob_2 + double_swap_prob_;
  const float action_prob_4 = action_prob_3 + exchange_prob_;
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
  } else if (op <= action_prob_4) {
    action_id_ = 4;
    exchangeMacros();  // exchange two macros in the sequence pair
  } else {
    action_id_ = 5;
    pre_macros_ = macros_;
    resizeOneCluster();
  }

  // update the macro locations based on Sequence Pair
  packFloorplan();
  // Update all the penalties
  calPenalty();
}

void SACoreSoftMacro::restore()
{
  if (macros_.empty()) {
    return;
  }

  // To reduce the runtime, here we do not call PackFloorplan
  // again. So when we need to generate the final floorplan out,
  // we need to call PackFloorplan again at the end of SA process
  if (action_id_ == 5) {
    macros_[macro_id_] = pre_macros_[macro_id_];
  } else if (action_id_ == 1) {
    pos_seq_ = pre_pos_seq_;
  } else if (action_id_ == 2) {
    neg_seq_ = pre_neg_seq_;
  } else {
    pos_seq_ = pre_pos_seq_;
    neg_seq_ = pre_neg_seq_;
  }

  width_ = pre_width_;
  height_ = pre_height_;
  outline_penalty_ = pre_outline_penalty_;
  wirelength_ = pre_wirelength_;
  guidance_penalty_ = pre_guidance_penalty_;
  fence_penalty_ = pre_fence_penalty_;
  boundary_penalty_ = pre_boundary_penalty_;
  macro_blockage_penalty_ = pre_macro_blockage_penalty_;
  notch_penalty_ = pre_notch_penalty_;
}

void SACoreSoftMacro::initialize()
{
  initSequencePair();

  std::vector<float> outline_penalty_list;
  std::vector<float> wirelength_list;
  std::vector<float> guidance_penalty_list;
  std::vector<float> fence_penalty_list;
  std::vector<float> boundary_penalty_list;
  std::vector<float> macro_blockage_penalty_list;
  std::vector<float> notch_penalty_list;
  std::vector<float> area_penalty_list;
  std::vector<float> width_list;
  std::vector<float> height_list;

  // We don't want to stop in the normalization factor setup
  Mpl2Observer* save_graphics = graphics_;
  graphics_ = nullptr;

  for (int i = 0; i < num_perturb_per_step_; i++) {
    perturb();
    // store current penalties
    width_list.push_back(width_);
    height_list.push_back(height_);
    area_penalty_list.push_back(width_ * height_ / outline_.getWidth()
                                / outline_.getHeight());
    outline_penalty_list.push_back(outline_penalty_);
    wirelength_list.push_back(wirelength_);
    guidance_penalty_list.push_back(guidance_penalty_);
    fence_penalty_list.push_back(fence_penalty_);
    boundary_penalty_list.push_back(boundary_penalty_);
    macro_blockage_penalty_list.push_back(macro_blockage_penalty_);
    notch_penalty_list.push_back(notch_penalty_);
  }
  graphics_ = save_graphics;

  norm_area_penalty_ = calAverage(area_penalty_list);
  norm_outline_penalty_ = calAverage(outline_penalty_list);
  norm_wirelength_ = calAverage(wirelength_list);
  norm_guidance_penalty_ = calAverage(guidance_penalty_list);
  norm_fence_penalty_ = calAverage(fence_penalty_list);
  norm_boundary_penalty_ = calAverage(boundary_penalty_list);
  norm_macro_blockage_penalty_ = calAverage(macro_blockage_penalty_list);
  norm_notch_penalty_ = calAverage(notch_penalty_list);

  // Reset penalites if lower than threshold

  if (norm_area_penalty_ <= 1e-4) {
    norm_area_penalty_ = 1.0;
  }

  if (norm_outline_penalty_ <= 1e-4) {
    norm_outline_penalty_ = 1.0;
  }

  if (norm_wirelength_ <= 1e-4) {
    norm_wirelength_ = 1.0;
  }

  if (norm_guidance_penalty_ <= 1e-4) {
    norm_guidance_penalty_ = 1.0;
  }

  if (norm_fence_penalty_ <= 1e-4) {
    norm_fence_penalty_ = 1.0;
  }

  if (norm_macro_blockage_penalty_ <= 1e-4) {
    norm_macro_blockage_penalty_ = 1.0;
  }

  if (norm_boundary_penalty_ <= 1e-4) {
    norm_boundary_penalty_ = 1.0;
  }

  if (norm_notch_penalty_ <= 1e-4) {
    norm_notch_penalty_ = 1.0;
  }

  // Calculate initial temperature
  std::vector<float> cost_list;
  for (int i = 0; i < outline_penalty_list.size(); i++) {
    width_ = width_list[i];
    height_ = height_list[i];
    outline_penalty_ = outline_penalty_list[i];
    wirelength_ = wirelength_list[i];
    guidance_penalty_ = guidance_penalty_list[i];
    fence_penalty_ = fence_penalty_list[i];
    boundary_penalty_ = boundary_penalty_list[i];
    macro_blockage_penalty_ = macro_blockage_penalty_list[i];
    notch_penalty_ = notch_penalty_list[i];
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

// Independently of the current parent's level, we always compute
// the boundary penalty based on the boundaries of the root (core).
// The macros we're trying to place i.e. the ones from the sequence pair,
// have their lower left corner based on the parent's outline.
void SACoreSoftMacro::calBoundaryPenalty()
{
  boundary_penalty_ = 0.0;

  if (boundary_weight_ <= 0.0) {
    return;
  }

  int tot_num_macros = 0;
  for (const auto& macro_id : pos_seq_) {
    tot_num_macros += macros_[macro_id].getNumMacro();
  }

  if (tot_num_macros <= 0) {
    return;
  }

  float global_lx = 0.0f, global_ly = 0.0f;
  float global_ux = 0.0f, global_uy = 0.0f;
  float x_dist_from_root = 0.0f, y_dist_from_root = 0.0f;

  for (const auto& macro_id : pos_seq_) {
    if (macros_[macro_id].getNumMacro() > 0) {
      global_lx = macros_[macro_id].getX() + outline_.xMin() - root_->getX();
      global_ly = macros_[macro_id].getY() + outline_.yMin() - root_->getY();
      global_ux = global_lx + macros_[macro_id].getWidth();
      global_uy = global_ly + macros_[macro_id].getHeight();

      x_dist_from_root
          = std::min(global_lx, std::abs(root_->getWidth() - global_ux));
      y_dist_from_root
          = std::min(global_ly, std::abs(root_->getHeight() - global_uy));

      boundary_penalty_ += std::min(x_dist_from_root, y_dist_from_root)
                           * macros_[macro_id].getNumMacro();
    }
  }
  // normalization
  boundary_penalty_ = boundary_penalty_ / tot_num_macros;
  if (graphics_) {
    graphics_->setBoundaryPenalty(
        {boundary_weight_, boundary_penalty_ / norm_boundary_penalty_});
  }
}

// Penalty for overlapping between clusters with macros and macro blockages.
// There may be situations in which we cannot guarantee that there will be
// no overlap, so we consider:
// 1) Number of macros to prioritize clusters with more macros.
// 2) The macro area percentage over the cluster's total area so that
//    mixed clusters with large std cell area have less penalty.

void SACoreSoftMacro::calMacroBlockagePenalty()
{
  macro_blockage_penalty_ = 0.0;
  if (blockages_.empty() || macro_blockage_weight_ <= 0.0) {
    return;
  }

  int tot_num_macros = 0;
  for (const auto& macro_id : pos_seq_) {
    tot_num_macros += macros_[macro_id].getNumMacro();
  }
  if (tot_num_macros <= 0) {
    return;
  }

  for (auto& blockage : blockages_) {
    for (const auto& macro_id : pos_seq_) {
      if (macros_[macro_id].getNumMacro() > 0) {
        const float soft_macro_x_min = macros_[macro_id].getX();
        const float soft_macro_x_max
            = soft_macro_x_min + macros_[macro_id].getWidth();
        const float soft_macro_y_min = macros_[macro_id].getY();
        const float soft_macro_y_max
            = soft_macro_y_min + macros_[macro_id].getHeight();

        const float overlap_width
            = std::min(blockage.xMax(), soft_macro_x_max)
              - std::max(blockage.xMin(), soft_macro_x_min);
        const float overlap_height
            = std::min(blockage.yMax(), soft_macro_y_max)
              - std::max(blockage.yMin(), soft_macro_y_min);

        // If any of the dimensions is negative, then there's no overlap.
        if (overlap_width < 0 || overlap_height < 0) {
          continue;
        }

        Cluster* cluster = macros_[macro_id].getCluster();
        float macro_dominance = cluster->getMacroArea() / cluster->getArea();

        macro_blockage_penalty_ += overlap_width * overlap_height
                                   * macros_[macro_id].getNumMacro()
                                   * macro_dominance;
      }
    }
  }
  // normalization
  macro_blockage_penalty_ = macro_blockage_penalty_ / tot_num_macros;
  if (graphics_) {
    graphics_->setMacroBlockagePenalty(
        {macro_blockage_weight_,
         macro_blockage_penalty_ / norm_macro_blockage_penalty_});
  }
}

// Align macro clusters to reduce notch
void SACoreSoftMacro::alignMacroClusters()
{
  if (width_ > outline_.getWidth() || height_ > outline_.getHeight()) {
    return;
  }
  // update threshold value
  adjust_h_th_ = notch_h_th_;
  adjust_v_th_ = notch_v_th_;
  for (auto& macro_id : pos_seq_) {
    if (macros_[macro_id].isMacroCluster()) {
      adjust_h_th_ = std::min(
          adjust_h_th_, macros_[macro_id].getWidth() * (1 - acc_tolerance_));
      adjust_v_th_ = std::min(
          adjust_v_th_, macros_[macro_id].getHeight() * (1 - acc_tolerance_));
    }
  }
  const float ratio = 0.1;
  adjust_h_th_ = std::min(adjust_h_th_, outline_.getHeight() * ratio);
  adjust_v_th_ = std::min(adjust_v_th_, outline_.getWidth() * ratio);

  // Align macro clusters to boundaries
  for (auto& macro_id : pos_seq_) {
    if (macros_[macro_id].isMacroCluster()) {
      const float lx = macros_[macro_id].getX();
      const float ly = macros_[macro_id].getY();
      const float ux = lx + macros_[macro_id].getWidth();
      const float uy = ly + macros_[macro_id].getHeight();
      // align to left / right boundaries
      if (lx <= adjust_h_th_) {
        macros_[macro_id].setX(0.0);
      } else if (outline_.getWidth() - ux <= adjust_h_th_) {
        macros_[macro_id].setX(outline_.getWidth()
                               - macros_[macro_id].getWidth());
      }
      // align to top / bottom boundaries
      if (ly <= adjust_v_th_) {
        macros_[macro_id].setY(0.0);
      } else if (outline_.getHeight() - uy <= adjust_v_th_) {
        macros_[macro_id].setY(outline_.getHeight()
                               - macros_[macro_id].getHeight());
      }
    }
  }
}

// If there is no HardMacroCluster, we do not consider the notch penalty
void SACoreSoftMacro::calNotchPenalty()
{
  // Initialization
  notch_penalty_ = 0.0;
  if (notch_weight_ <= 0.0) {
    return;
  }
  // If the floorplan cannot fit into the outline
  // We think the entire floorplan is a "huge" notch
  if (width_ > outline_.getWidth() * 1.001
      || height_ > outline_.getHeight() * 1.001) {
    notch_penalty_ += outline_.getWidth() * outline_.getHeight()
                      / (outline_.getWidth() * outline_.getHeight());
    return;
  }

  pre_macros_ = macros_;
  // align macro clusters to reduce notches
  alignMacroClusters();
  // Fill dead space
  fillDeadSpace();
  // Create grids based on location of MixedCluster and HardMacroCluster
  std::set<float> x_point;
  std::set<float> y_point;
  std::vector<bool> macro_mask;
  for (auto& macro : macros_) {
    if (macro.getArea() <= 0.0
        || (!macro.isMacroCluster() && !macro.isMixedCluster())) {
      macro_mask.push_back(false);
      continue;
    }
    x_point.insert(macro.getX());
    x_point.insert(macro.getX() + macro.getWidth());
    y_point.insert(macro.getY());
    y_point.insert(macro.getY() + macro.getHeight());
    macro_mask.push_back(true);
  }
  x_point.insert(0.0);
  y_point.insert(0.0);
  x_point.insert(outline_.getWidth());
  y_point.insert(outline_.getHeight());
  // create grid
  std::vector<float> x_grid(x_point.begin(), x_point.end());
  std::vector<float> y_grid(y_point.begin(), y_point.end());
  // create grid in a row-based manner
  std::vector<std::vector<int>> grids;  // store the macro id
  const int num_x = x_grid.size() - 1;
  const int num_y = y_grid.size() - 1;
  for (int j = 0; j < num_y; j++) {
    std::vector<int> macro_ids(num_x, -1);
    grids.push_back(macro_ids);
  }
  // detect the notch region around each MixedCluster and HardMacroCluster
  for (int macro_id = 0; macro_id < macros_.size(); macro_id++) {
    if (!macro_mask[macro_id]) {
      continue;
    }
    int x_start = 0;
    int x_end = 0;
    calSegmentLoc(macros_[macro_id].getX(),
                  macros_[macro_id].getX() + macros_[macro_id].getWidth(),
                  x_start,
                  x_end,
                  x_grid);
    int y_start = 0;
    int y_end = 0;
    calSegmentLoc(macros_[macro_id].getY(),
                  macros_[macro_id].getY() + macros_[macro_id].getHeight(),
                  y_start,
                  y_end,
                  y_grid);
    for (int j = y_start; j < y_end; j++) {
      for (int i = x_start; i < x_end; i++) {
        grids[j][i] = macro_id;
      }
    }
  }
  // check surroundings of each HardMacroCluster and MixecCluster
  for (int macro_id = 0; macro_id < macros_.size(); macro_id++) {
    if (!macro_mask[macro_id]) {
      continue;
    }
    int x_start = 0;
    int x_end = 0;
    calSegmentLoc(macros_[macro_id].getX(),
                  macros_[macro_id].getX() + macros_[macro_id].getWidth(),
                  x_start,
                  x_end,
                  x_grid);
    int y_start = 0;
    int y_end = 0;
    calSegmentLoc(macros_[macro_id].getY(),
                  macros_[macro_id].getY() + macros_[macro_id].getHeight(),
                  y_start,
                  y_end,
                  y_grid);
    int x_start_new = x_start;
    int x_end_new = x_end;
    int y_start_new = y_start;
    int y_end_new = y_end;
    // check left first
    for (int i = x_start - 1; i >= 0; i--) {
      bool flag = true;
      for (int j = y_start; j < y_end; j++) {
        if (grids[j][i] != -1) {
          flag = false;  // we cannot extend the current cluster
          break;
        }           // end if
      }             // end y
      if (!flag) {  // extension done
        break;
      }
      x_start_new--;  // extend left
      for (int j = y_start; j < y_end; j++) {
        grids[j][i] = macro_id;
      }
    }  // end left
    // check top second
    for (int j = y_end; j < num_y; j++) {
      bool flag = true;
      for (int i = x_start; i < x_end; i++) {
        if (grids[j][i] != -1) {
          flag = false;  // we cannot extend the current cluster
          break;
        }           // end if
      }             // end y
      if (!flag) {  // extension done
        break;
      }
      y_end_new++;  // extend top
      for (int i = x_start; i < x_end; i++) {
        grids[j][i] = macro_id;
      }
    }  // end top
    // check right third
    for (int i = x_end; i < num_x; i++) {
      bool flag = true;
      for (int j = y_start; j < y_end; j++) {
        if (grids[j][i] != -1) {
          flag = false;  // we cannot extend the current cluster
          break;
        }           // end if
      }             // end y
      if (!flag) {  // extension done
        break;
      }
      x_end_new++;  // extend right
      for (int j = y_start; j < y_end; j++) {
        grids[j][i] = macro_id;
      }
    }  // end right
    // check down second
    for (int j = y_start - 1; j >= 0; j--) {
      bool flag = true;
      for (int i = x_start; i < x_end; i++) {
        if (grids[j][i] != -1) {
          flag = false;  // we cannot extend the current cluster
          break;
        }           // end if
      }             // end y
      if (!flag) {  // extension done
        break;
      }
      y_start_new--;  // extend down
      for (int i = x_start; i < x_end; i++) {
        grids[j][i] = macro_id;
      }
    }  // end down
    // check the notch area
    if ((x_grid[x_start] - x_grid[x_start_new]) <= notch_h_th_) {
      notch_penalty_ += (x_grid[x_start] - x_grid[x_start_new])
                        * macros_[macro_id].getHeight();
    }
    if ((x_grid[x_end_new] - x_grid[x_end]) <= notch_h_th_) {
      notch_penalty_ += (x_grid[x_end_new] - x_grid[x_end])
                        * macros_[macro_id].getHeight();
    }
    if ((y_grid[y_start] - y_grid[y_start_new]) <= notch_v_th_) {
      notch_penalty_ += (y_grid[y_start] - y_grid[y_start_new])
                        * macros_[macro_id].getWidth();
    }
    if ((y_grid[y_end_new] - y_grid[y_end]) <= notch_v_th_) {
      notch_penalty_
          += (y_grid[y_end_new] - y_grid[y_end]) * macros_[macro_id].getWidth();
    }
  }
  macros_ = pre_macros_;
  // normalization
  notch_penalty_
      = notch_penalty_ / (outline_.getWidth() * outline_.getHeight());
  if (graphics_) {
    graphics_->setNotchPenalty(
        {notch_weight_, notch_penalty_ / norm_notch_penalty_});
  }
}

void SACoreSoftMacro::resizeOneCluster()
{
  const int idx = static_cast<int>(
      std::floor(distribution_(generator_) * pos_seq_.size()));
  macro_id_ = idx;
  SoftMacro& src_macro = macros_[idx];
  if (src_macro.isMacroCluster()) {
    src_macro.resizeRandomly(distribution_, generator_);
    return;
  }

  const float lx = src_macro.getX();
  const float ly = src_macro.getY();
  const float ux = lx + src_macro.getWidth();
  const float uy = ly + src_macro.getHeight();
  // if the macro is outside of the outline, we randomly resize the macro
  if (ux >= outline_.getWidth() || uy >= outline_.getHeight()) {
    src_macro.resizeRandomly(distribution_, generator_);
    return;
  }

  if (distribution_(generator_) < 0.4) {
    src_macro.resizeRandomly(distribution_, generator_);
    return;
  }

  const float option = distribution_(generator_);
  if (option <= 0.25) {
    // Change the width of soft block to Rb = e.x2 - b.x1
    float e_x2 = outline_.getWidth();
    for (const auto& macro : macros_) {
      const float cur_x2 = macro.getX() + macro.getWidth();
      if (cur_x2 > ux && cur_x2 < e_x2) {
        e_x2 = cur_x2;
      }
    }
    src_macro.setWidth(e_x2 - lx);
  } else if (option <= 0.5) {
    float d_x2 = lx;
    for (const auto& macro : macros_) {
      const float cur_x2 = macro.getX() + macro.getWidth();
      if (cur_x2 < ux && cur_x2 > d_x2) {
        d_x2 = cur_x2;
      }
    }
    if (d_x2 <= lx) {
      return;
    }
    src_macro.setWidth(d_x2 - lx);
  } else if (option <= 0.75) {
    // change the height of soft block to Tb = a.y2 - b.y1
    float a_y2 = outline_.getHeight();
    for (const auto& macro : macros_) {
      const float cur_y2 = macro.getY() + macro.getHeight();
      if (cur_y2 > uy && cur_y2 < a_y2) {
        a_y2 = cur_y2;
      }
    }
    src_macro.setHeight(a_y2 - ly);
  } else {
    // Change the height of soft block to Bb = c.y2 - b.y1
    float c_y2 = ly;
    for (const auto& macro : macros_) {
      const float cur_y2 = macro.getY() + macro.getHeight();
      if (cur_y2 < uy && cur_y2 > c_y2) {
        c_y2 = cur_y2;
      }
    }
    if (c_y2 <= ly) {
      return;
    }
    src_macro.setHeight(c_y2 - ly);
  }
}

void SACoreSoftMacro::shrink()
{
  for (auto& macro_id : pos_seq_) {
    macros_[macro_id].shrinkArea(shrink_factor_);
  }
}

void SACoreSoftMacro::printResults() const
{
  debugPrint(
      logger_, MPL, "hierarchical_macro_placement", 2, "SACoreSoftMacro");
  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             2,
             "number of macros : {}",
             macros_.size());
  for (const auto& macro : macros_) {
    debugPrint(logger_,
               MPL,
               "hierarchical_macro_placement",
               2,
               "lx = {}, ly = {}, width = {}, height = {}, name = {}",
               macro.getX(),
               macro.getY(),
               macro.getWidth(),
               macro.getHeight(),
               macro.getName());
  }
  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             2,
             "width = {}, outline_width = {}",
             width_,
             outline_.getWidth());
  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             2,
             "height = {}, outline_height = {}",
             height_,
             outline_.getHeight());
  debugPrint(
      logger_,
      MPL,
      "hierarchical_macro_placement",
      2,
      "outline_weight = {}, outline_penalty  = {}, norm_outline_penalty = {}",
      outline_weight_,
      outline_penalty_,
      norm_outline_penalty_);
  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             2,
             "wirelength_weight = {}, wirelength  = {}, norm_wirelength = {}",
             wirelength_weight_,
             wirelength_,
             norm_wirelength_);
  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             2,
             "guidance_weight = {}, guidance_penalty  = {}, "
             "norm_guidance_penalty = {}",
             guidance_weight_,
             guidance_penalty_,
             norm_guidance_penalty_);
  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             2,
             "fence_weight = {}, fence_penalty  = {}, norm_fence_penalty = {}",
             fence_weight_,
             fence_penalty_,
             norm_fence_penalty_);
  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             2,
             "macro_blockage_weight = {}, macro_blockage_penalty  = {}, "
             "norm_macro_blockage_penalty = {}",
             macro_blockage_weight_,
             macro_blockage_penalty_,
             norm_macro_blockage_penalty_);
  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             2,
             "notch_weight = {}, notch_penalty  = {}, norm_notch_penalty = {}",
             notch_weight_,
             notch_penalty_,
             norm_notch_penalty_);
  debugPrint(logger_,
             MPL,
             "hierarchical_macro_placement",
             2,
             "final cost = {}",
             getNormCost());
}

// fill the dead space by adjust the size of MixedCluster
void SACoreSoftMacro::fillDeadSpace()
{
  // if the floorplan is invalid, do nothing
  if (width_ > outline_.getWidth() * (1.0 + 0.001)
      || height_ > outline_.getHeight() * (1.0 + 0.001)) {
    return;
  }

  // adjust the location of MixedCluster
  // Step1 : Divide the entire floorplan into grids
  std::set<float> x_point;
  std::set<float> y_point;
  for (auto& macro_id : pos_seq_) {
    if (macros_[macro_id].getArea() <= 0.0) {
      continue;
    }
    x_point.insert(macros_[macro_id].getX());
    x_point.insert(macros_[macro_id].getX() + macros_[macro_id].getWidth());
    y_point.insert(macros_[macro_id].getY());
    y_point.insert(macros_[macro_id].getY() + macros_[macro_id].getHeight());
  }
  x_point.insert(0.0);
  y_point.insert(0.0);
  x_point.insert(outline_.getWidth());
  y_point.insert(outline_.getHeight());
  // create grid
  std::vector<float> x_grid(x_point.begin(), x_point.end());
  std::vector<float> y_grid(y_point.begin(), y_point.end());
  // create grid in a row-based manner
  std::vector<std::vector<int>> grids;  // store the macro id
  const int num_x = x_grid.size() - 1;
  const int num_y = y_grid.size() - 1;
  for (int j = 0; j < num_y; j++) {
    std::vector<int> macro_ids(num_x, -1);
    grids.push_back(macro_ids);
  }

  for (int macro_id = 0; macro_id < pos_seq_.size(); macro_id++) {
    if (macros_[macro_id].getArea() <= 0.0) {
      continue;
    }
    int x_start = 0;
    int x_end = 0;
    calSegmentLoc(macros_[macro_id].getX(),
                  macros_[macro_id].getX() + macros_[macro_id].getWidth(),
                  x_start,
                  x_end,
                  x_grid);
    int y_start = 0;
    int y_end = 0;
    calSegmentLoc(macros_[macro_id].getY(),
                  macros_[macro_id].getY() + macros_[macro_id].getHeight(),
                  y_start,
                  y_end,
                  y_grid);
    for (int j = y_start; j < y_end; j++) {
      for (int i = x_start; i < x_end; i++) {
        grids[j][i] = macro_id;
      }
    }
  }
  // propagate from the MixedCluster and then StdCellCluster
  for (int order = 0; order <= 1; order++) {
    for (int macro_id = 0; macro_id < pos_seq_.size(); macro_id++) {
      if (macros_[macro_id].getArea() <= 0.0) {
        continue;
      }
      const bool forward_flag = (order == 0)
                                    ? macros_[macro_id].isMixedCluster()
                                    : macros_[macro_id].isStdCellCluster();
      if (!forward_flag) {
        continue;
      }
      int x_start = 0;
      int x_end = 0;
      calSegmentLoc(macros_[macro_id].getX(),
                    macros_[macro_id].getX() + macros_[macro_id].getWidth(),
                    x_start,
                    x_end,
                    x_grid);
      int y_start = 0;
      int y_end = 0;
      calSegmentLoc(macros_[macro_id].getY(),
                    macros_[macro_id].getY() + macros_[macro_id].getHeight(),
                    y_start,
                    y_end,
                    y_grid);
      int x_start_new = x_start;
      int x_end_new = x_end;
      int y_start_new = y_start;
      int y_end_new = y_end;
      // propagate left first
      for (int i = x_start - 1; i >= 0; i--) {
        bool flag = true;
        for (int j = y_start; j < y_end; j++) {
          if (grids[j][i] != -1) {
            flag = false;  // we cannot extend the current cluster
            break;
          }           // end if
        }             // end y
        if (!flag) {  // extension done
          break;
        }
        x_start_new--;  // extend left
        for (int j = y_start; j < y_end; j++) {
          grids[j][i] = macro_id;
        }
      }  // end left
      x_start = x_start_new;
      // propagate top second
      for (int j = y_end; j < num_y; j++) {
        bool flag = true;
        for (int i = x_start; i < x_end; i++) {
          if (grids[j][i] != -1) {
            flag = false;  // we cannot extend the current cluster
            break;
          }           // end if
        }             // end y
        if (!flag) {  // extension done
          break;
        }
        y_end_new++;  // extend top
        for (int i = x_start; i < x_end; i++) {
          grids[j][i] = macro_id;
        }
      }  // end top
      y_end = y_end_new;
      // propagate right third
      for (int i = x_end; i < num_x; i++) {
        bool flag = true;
        for (int j = y_start; j < y_end; j++) {
          if (grids[j][i] != -1) {
            flag = false;  // we cannot extend the current cluster
            break;
          }           // end if
        }             // end y
        if (!flag) {  // extension done
          break;
        }
        x_end_new++;  // extend right
        for (int j = y_start; j < y_end; j++) {
          grids[j][i] = macro_id;
        }
      }  // end right
      x_end = x_end_new;
      // propagate down second
      for (int j = y_start - 1; j >= 0; j--) {
        bool flag = true;
        for (int i = x_start; i < x_end; i++) {
          if (grids[j][i] != -1) {
            flag = false;  // we cannot extend the current cluster
            break;
          }           // end if
        }             // end y
        if (!flag) {  // extension done
          break;
        }
        y_start_new--;  // extend down
        for (int i = x_start; i < x_end; i++) {
          grids[j][i] = macro_id;
        }
      }  // end down
      y_start = y_start_new;
      // update the location of cluster
      macros_[macro_id].setLocationF(x_grid[x_start], y_grid[y_start]);
      macros_[macro_id].setShapeF(x_grid[x_end] - x_grid[x_start],
                                  y_grid[y_end] - y_grid[y_start]);
    }
  }
}

// A utility function for FillDeadSpace.
// It's used for calculate the start point and end point for a segment in a grid
void SACoreSoftMacro::calSegmentLoc(float seg_start,
                                    float seg_end,
                                    int& start_id,
                                    int& end_id,
                                    std::vector<float>& grid)
{
  start_id = -1;
  end_id = -1;
  for (int i = 0; i < grid.size() - 1; i++) {
    if ((grid[i] <= seg_start) && (grid[i + 1] > seg_start)) {
      start_id = i;
    }
    if ((grid[i] <= seg_end) && (grid[i + 1] > seg_end)) {
      end_id = i;
    }
  }
  if (end_id == -1) {
    end_id = grid.size() - 1;
  }
}

// The blockages here are only those that overlap with the annealing outline.
void SACoreSoftMacro::addBlockages(const std::vector<Rect>& blockages)
{
  blockages_.insert(blockages_.end(), blockages.begin(), blockages.end());
}

void SACoreSoftMacro::attemptCentralization(const float pre_cost)
{
  if (outline_penalty_ > 0) {
    return;
  }

  // In order to revert the centralization, we cache the current location
  // of the clusters to avoid floating-point evilness when creating the
  // x,y grid to fill the dead space by expanding mixed clusters.
  std::map<int, std::pair<float, float>> clusters_locations;

  for (int& id : pos_seq_) {
    clusters_locations[id] = {macros_[id].getX(), macros_[id].getY()};
  }

  std::pair<float, float> offset((outline_.getWidth() - width_) / 2,
                                 (outline_.getHeight() - height_) / 2);
  moveFloorplan(offset);

  // revert centralization
  if (calNormCost() > pre_cost) {
    centralization_was_reverted_ = true;

    for (int& id : pos_seq_) {
      macros_[id].setX(clusters_locations[id].first);
      macros_[id].setY(clusters_locations[id].second);
    }

    if (graphics_) {
      graphics_->saStep(macros_);
    }

    calPenalty();
  }
}

void SACoreSoftMacro::moveFloorplan(const std::pair<float, float>& offset)
{
  for (auto& id : pos_seq_) {
    macros_[id].setX(macros_[id].getX() + offset.first);
    macros_[id].setY(macros_[id].getY() + offset.second);
  }

  if (graphics_) {
    graphics_->saStep(macros_);
  }

  calPenalty();
}

}  // namespace mpl2
