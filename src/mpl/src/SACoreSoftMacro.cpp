// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "SACoreSoftMacro.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <set>
#include <utility>
#include <vector>

#include "MplObserver.h"
#include "SimulatedAnnealingCore.h"
#include "boost/random/uniform_int_distribution.hpp"
#include "clusterEngine.h"
#include "mpl-util.h"
#include "object.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace mpl {
using utl::MPL;

//////////////////////////////////////////////////////////////////
// Class SACoreSoftMacro
// constructors
SACoreSoftMacro::SACoreSoftMacro(PhysicalHierarchy* tree,
                                 const odb::Rect& outline,
                                 const std::vector<SoftMacro>& macros,
                                 const SACoreWeights& core_weights,
                                 const SASoftWeights& soft_weights,
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
                                 MplObserver* graphics,
                                 utl::Logger* logger,
                                 odb::dbBlock* block)
    : SimulatedAnnealingCore<SoftMacro>(tree,
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
                                        block),
      root_(tree->root.get())
{
  boundary_weight_ = soft_weights.boundary;
  macro_blockage_weight_ = soft_weights.macro_blockage;
  notch_weight_ = soft_weights.notch;
  resize_prob_ = resize_prob;
  notch_h_th_ = notch_h_threshold;
  notch_v_th_ = notch_v_threshold;
  adjust_h_th_ = notch_h_th_;
  adjust_v_th_ = notch_v_th_;
  logger_ = logger;
}

void SACoreSoftMacro::findFixedMacros()
{
  for (const int macro_id : pos_seq_) {
    const SoftMacro& macro = macros_[macro_id];
    if (macro.isFixed()) {
      fixed_macros_.push_back(macro.getBBox());
    }
  }
}

void SACoreSoftMacro::run()
{
  if (graphics_) {
    graphics_->startSA("soft", max_num_step_, num_perturb_per_step_);
  }

  fastSA();

  if (enhancements_on_) {
    attemptCentralization(calNormCost());
    if (centralization_was_reverted_) {
      attemptMacroClusterAlignment();
    }
  }

  if (graphics_) {
    graphics_->endSA(calNormCost());
  }
}

bool SACoreSoftMacro::isValid() const
{
  if (!fixed_macros_.empty() && fixed_macros_penalty_ > 0.0f) {
    return false;
  }

  return resultFitsInOutline();
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

float SACoreSoftMacro::getNormNotchPenalty() const
{
  return norm_notch_penalty_;
}

// Operations
float SACoreSoftMacro::calNormCost() const
{
  float cost = 0.0;  // Initialize cost

  if (norm_area_penalty_ > 0.0) {
    cost += core_weights_.area * getAreaPenalty();
  }
  if (norm_outline_penalty_ > 0.0) {
    cost += core_weights_.outline * outline_penalty_ / norm_outline_penalty_;
  }
  if (norm_wirelength_ > 0.0) {
    cost += core_weights_.wirelength * wirelength_ / norm_wirelength_;
  }
  if (norm_guidance_penalty_ > 0.0) {
    cost += core_weights_.guidance * guidance_penalty_ / norm_guidance_penalty_;
  }
  if (norm_fence_penalty_ > 0.0) {
    cost += core_weights_.fence * fence_penalty_ / norm_fence_penalty_;
  }

  if (norm_boundary_penalty_ > 0.0) {
    cost += boundary_weight_ * boundary_penalty_ / norm_boundary_penalty_;
  }
  if (norm_macro_blockage_penalty_ > 0.0) {
    cost += macro_blockage_weight_ * macro_blockage_penalty_
            / norm_macro_blockage_penalty_;
  }
  if (norm_fixed_macros_penalty_ > 0.0) {
    cost += fixed_macros_weight_ * fixed_macros_penalty_
            / norm_fixed_macros_penalty_;
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
  calFixedMacrosPenalty();
  if (graphics_) {
    graphics_->setAreaPenalty(
        {"Area", core_weights_.area, getAreaPenalty(), norm_area_penalty_});
    graphics_->penaltyCalculated(calNormCost());
  }
}

void SACoreSoftMacro::perturb()
{
  if (macros_.empty()) {
    return;
  }

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
    resizeOneCluster();
  }

  // update the macro locations based on Sequence Pair
  packFloorplan();
  // Update all the penalties
  calPenalty();
}

void SACoreSoftMacro::saveState()
{
  if (macros_.empty()) {
    return;
  }

  pre_macros_ = macros_;
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
}

void SACoreSoftMacro::restoreState()
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
  findFixedMacros();

  std::vector<float> outline_penalty_list;
  std::vector<float> wirelength_list;
  std::vector<float> guidance_penalty_list;
  std::vector<float> fence_penalty_list;
  std::vector<float> boundary_penalty_list;
  std::vector<float> macro_blockage_penalty_list;
  std::vector<float> notch_penalty_list;
  std::vector<float> area_penalty_list;
  std::vector<float> fixed_macros_penalty_list;
  std::vector<int> width_list;
  std::vector<int> height_list;

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

    width_list.push_back(width_);
    height_list.push_back(height_);
    area_penalty_list.push_back(getAreaPenalty());
    outline_penalty_list.push_back(outline_penalty_);
    wirelength_list.push_back(wirelength_);
    guidance_penalty_list.push_back(guidance_penalty_);
    fence_penalty_list.push_back(fence_penalty_);
    boundary_penalty_list.push_back(boundary_penalty_);
    macro_blockage_penalty_list.push_back(macro_blockage_penalty_);
    notch_penalty_list.push_back(notch_penalty_);
    fixed_macros_penalty_list.push_back(fixed_macros_penalty_);
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
  norm_fixed_macros_penalty_ = calAverage(fixed_macros_penalty_list);

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

  if (norm_fixed_macros_penalty_ <= 1e-4) {
    norm_fixed_macros_penalty_ = 1.0;
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
    fixed_macros_penalty_ = fixed_macros_penalty_list[i];
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

  int number_of_movable_macros = 0;
  for (const auto& macro_id : pos_seq_) {
    const SoftMacro& soft_macro = macros_[macro_id];
    if (soft_macro.isFixed()) {
      continue;
    }

    number_of_movable_macros += soft_macro.getNumMacro();
  }

  if (number_of_movable_macros == 0) {
    return;
  }

  int global_lx = 0, global_ly = 0;
  int global_ux = 0, global_uy = 0;
  int x_dist_from_root = 0, y_dist_from_root = 0;

  for (const auto& macro_id : pos_seq_) {
    const SoftMacro& soft_macro = macros_[macro_id];
    if (soft_macro.isFixed()) {
      continue;
    }

    if (soft_macro.getNumMacro() > 0) {
      global_lx = soft_macro.getX() + outline_.xMin() - root_->getX();
      global_ly = soft_macro.getY() + outline_.yMin() - root_->getY();
      global_ux = global_lx + soft_macro.getWidth();
      global_uy = global_ly + soft_macro.getHeight();

      x_dist_from_root
          = std::min(global_lx, std::abs(root_->getWidth() - global_ux));
      y_dist_from_root
          = std::min(global_ly, std::abs(root_->getHeight() - global_uy));

      boundary_penalty_
          += block_->dbuToMicrons(x_dist_from_root + y_dist_from_root)
             * soft_macro.getNumMacro();
    }
  }
  // normalization
  boundary_penalty_ = boundary_penalty_ / number_of_movable_macros;
  if (graphics_) {
    graphics_->setBoundaryPenalty({"Boundary",
                                   boundary_weight_,
                                   boundary_penalty_,
                                   norm_boundary_penalty_});
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
      const SoftMacro& soft_macro = macros_[macro_id];
      if (soft_macro.getNumMacro() > 0) {
        odb::Rect overlap;
        blockage.intersection(soft_macro.getBBox(), overlap);

        // If any of the dimensions is negative, then there's no overlap.
        if (overlap.dx() < 0 || overlap.dy() < 0) {
          continue;
        }

        Cluster* cluster = soft_macro.getCluster();
        const float macro_dominance
            = cluster->getMacroArea() / static_cast<float>(cluster->getArea());

        macro_blockage_penalty_
            += overlap.area() * soft_macro.getNumMacro() * macro_dominance;
      }
    }
  }
  // normalization
  macro_blockage_penalty_ = macro_blockage_penalty_ / tot_num_macros;
  if (graphics_) {
    graphics_->setMacroBlockagePenalty({"Macro Blockage",
                                        macro_blockage_weight_,
                                        macro_blockage_penalty_,
                                        norm_macro_blockage_penalty_});
  }
}

void SACoreSoftMacro::calFixedMacrosPenalty()
{
  if (fixed_macros_.empty()) {
    return;
  }

  fixed_macros_penalty_ = 0.0f;

  for (const odb::Rect& fixed_macro : fixed_macros_) {
    for (const int macro_id : pos_seq_) {
      const SoftMacro& macro = macros_[macro_id];

      // Skip the current fixed macro itself and unneeded computation.
      if (macro.isFixed()) {
        continue;
      }

      odb::Rect overlap;
      fixed_macro.intersection(macro.getBBox(), overlap);

      // If any of the dimensions is negative, then there's no overlap.
      if (overlap.dx() < 0 || overlap.dy() < 0) {
        continue;
      }

      fixed_macros_penalty_ += block_->dbuAreaToMicrons(overlap.area());
    }
  }

  if (graphics_) {
    graphics_->setFixedMacrosPenalty({"Fixed Macros",
                                      fixed_macros_weight_,
                                      fixed_macros_penalty_,
                                      norm_fixed_macros_penalty_});
  }
}

// Align macro clusters to reduce notch
void SACoreSoftMacro::attemptMacroClusterAlignment()
{
  if (!isValid()) {
    return;
  }

  float pre_cost = calNormCost();
  // Cache current solution to allow reversal
  auto clusters_locations = getClustersLocations();

  // update threshold value
  adjust_h_th_ = notch_h_th_;
  adjust_v_th_ = notch_v_th_;
  for (auto& macro_id : pos_seq_) {
    if (macros_[macro_id].isMacroCluster()) {
      adjust_h_th_ = std::min(adjust_h_th_, macros_[macro_id].getWidth());
      adjust_v_th_ = std::min(adjust_v_th_, macros_[macro_id].getHeight());
    }
  }

  const int ratio = 10;
  adjust_h_th_ = std::min(adjust_h_th_, outline_.dy() / ratio);
  adjust_v_th_ = std::min(adjust_v_th_, outline_.dx() / ratio);

  // Align macro clusters to boundaries
  for (auto& macro_id : pos_seq_) {
    if (macros_[macro_id].isMacroCluster()) {
      const int lx = macros_[macro_id].getX();
      const int ly = macros_[macro_id].getY();
      const int ux = lx + macros_[macro_id].getWidth();
      const int uy = ly + macros_[macro_id].getHeight();
      // align to left / right boundaries
      if (lx < adjust_h_th_) {
        macros_[macro_id].setX(0);
      } else if (outline_.dx() - ux < adjust_h_th_) {
        macros_[macro_id].setX(outline_.dx() - macros_[macro_id].getWidth());
      }
      // align to top / bottom boundaries
      if (ly < adjust_v_th_) {
        macros_[macro_id].setY(0);
      } else if (outline_.dy() - uy < adjust_v_th_) {
        macros_[macro_id].setY(outline_.dy() - macros_[macro_id].getHeight());
      }
    }
  }

  calPenalty();

  // Revert macro alignemnt
  if (calNormCost() > pre_cost) {
    setClustersLocations(clusters_locations);

    if (graphics_) {
      graphics_->saStep(macros_);
    }

    calPenalty();
  }
}

void SACoreSoftMacro::fillCoordsLists(std::vector<int>& x_coords,
                                      std::vector<int>& y_coords)
{
  std::vector<int> x_point;
  std::vector<int> y_point;

  for (auto& macro : macros_) {
    if (macro.isStdCellCluster()) {
      continue;
    }
    x_point.push_back(macro.getX());
    x_point.push_back(macro.getX() + macro.getWidth());
    y_point.push_back(macro.getY());
    y_point.push_back(macro.getY() + macro.getHeight());
  }
  x_point.push_back(0);
  y_point.push_back(0);
  x_point.push_back(outline_.dx());
  y_point.push_back(outline_.dy());

  std::ranges::sort(x_point);
  std::ranges::sort(y_point);

  int epsilon = outline_.dx() / 100;
  for (int i = 0; i < x_point.size(); i++) {
    if (i + 1 < x_point.size()
        && std::abs(x_point[i + 1] - x_point[i]) <= epsilon) {
      continue;
    }
    x_coords.push_back(x_point[i]);
  }

  epsilon = outline_.dy() / 100;
  for (int i = 0; i < y_point.size(); i++) {
    if (i + 1 < y_point.size()
        && std::abs(y_point[i + 1] - y_point[i]) <= epsilon) {
      continue;
    }
    y_coords.push_back(y_point[i]);
  }
}

SACoreSoftMacro::Neighbors SACoreSoftMacro::findNeighbors(
    std::vector<std::vector<bool>>& grid,
    int start_row,
    int start_col,
    int end_row,
    int end_col)
{
  int num_y = grid.size();
  int num_x = grid.front().size();

  Neighbors neighbors;
  if (start_row > 0) {
    for (int i = start_col; i <= end_col; i++) {
      if (!grid[start_row - 1][i]) {
        neighbors.bottom = false;
        break;
      }
    }
  }
  if (end_row < num_y - 1) {
    for (int i = start_col; i <= end_col; i++) {
      if (!grid[end_row + 1][i]) {
        neighbors.top = false;
        break;
      }
    }
  }
  if (start_col > 0) {
    for (int i = start_row; i <= end_row; i++) {
      if (!grid[i][start_col - 1]) {
        neighbors.left = false;
        break;
      }
    }
  }
  if (end_col < num_x - 1) {
    for (int i = start_row; i <= end_row; i++) {
      if (!grid[i][end_col + 1]) {
        neighbors.right = false;
        break;
      }
    }
  }

  return neighbors;
}

bool SACoreSoftMacro::isSegmentEmpty(std::vector<std::vector<bool>>& grid,
                                     int start_row,
                                     int start_col,
                                     int end_row,
                                     int end_col)
{
  for (int i = start_row; i <= end_row; i++) {
    for (int j = start_col; j <= end_col; j++) {
      if (grid[i][j]) {
        return false;
      }
    }
  }

  return true;
}

float SACoreSoftMacro::calSingleNotchPenalty(int width, int height)
{
  return std::sqrt((static_cast<double>(width) * height) / outline_.area());
}

// If there is no HardMacroCluster, we do not consider the notch penalty
void SACoreSoftMacro::calNotchPenalty()
{
  if (graphics_) {
    graphics_->clearNotches();
  }

  if (notch_weight_ <= 0.0) {
    return;
  }

  // Initialization
  notch_penalty_ = 0.0;
  notch_h_th_ = outline_.dy() / 10;
  notch_v_th_ = outline_.dx() / 10;
  int width = 0;
  int height = 0;

  // If the floorplan is not valid
  // We think the entire floorplan is a "huge" notch
  if (!isValid()) {
    width = std::max(width_, outline_.dx());
    height = std::max(height_, outline_.dy());
    notch_penalty_ = calSingleNotchPenalty(width, height);

    if (graphics_) {
      graphics_->setNotchPenalty({.name = "Notch",
                                  .weight = notch_weight_,
                                  .value = notch_penalty_,
                                  .normalization_factor = norm_notch_penalty_});
    }
    return;
  }

  // Create grids based on location of MixedCluster and HardMacroCluster
  std::vector<int> x_coords;
  std::vector<int> y_coords;
  fillCoordsLists(x_coords, y_coords);

  int num_x = x_coords.size() - 1;
  int num_y = y_coords.size() - 1;

  std::vector<std::vector<bool>> grid(num_y, std::vector<bool>(num_x, false));
  for (auto& macro : macros_) {
    if (macro.isStdCellCluster()) {
      continue;
    }
    int x_start = getSegmentIndex(macro.getX(), x_coords);
    int x_end = getSegmentIndex(macro.getX() + macro.getWidth(), x_coords);
    int y_start = getSegmentIndex(macro.getY(), y_coords);
    int y_end = getSegmentIndex(macro.getY() + macro.getHeight(), y_coords);
    for (int row = y_start; row < y_end; row++) {
      for (int col = x_start; col < x_end; col++) {
        grid[row][col] = true;
      }
    }
  }

  for (int start_row = 0; start_row < num_y; start_row++) {
    for (int start_col = 0; start_col < num_x; start_col++) {
      if (grid[start_row][start_col]) {
        continue;
      }

      int end_row = start_row;
      int end_col = start_col;

      Neighbors current_neighbors
          = findNeighbors(grid, start_row, start_col, end_row, end_col);
      bool expand_rows = true;
      bool expand_cols = true;

      while (expand_rows || expand_cols) {
        if (expand_rows) {
          end_row += 1;
          if (end_row < num_y
              && isSegmentEmpty(grid, start_row, start_col, end_row, end_col)) {
            Neighbors expanded_neighbors
                = findNeighbors(grid, start_row, start_col, end_row, end_col);
            if (expanded_neighbors.total() > current_neighbors.total()
                || expanded_neighbors == current_neighbors) {
              current_neighbors = expanded_neighbors;
            } else {
              expand_rows = false;
              end_row -= 1;
            }
          } else {
            expand_rows = false;
            end_row -= 1;
          }
        }

        if (expand_cols) {
          end_col += 1;
          if (end_col < num_x
              && isSegmentEmpty(grid, start_row, start_col, end_row, end_col)) {
            Neighbors expanded_neighbors
                = findNeighbors(grid, start_row, start_col, end_row, end_col);
            if (expanded_neighbors.total() > current_neighbors.total()
                || expanded_neighbors == current_neighbors) {
              current_neighbors = expanded_neighbors;
            } else {
              expand_cols = false;
              end_col -= 1;
            }
          } else {
            expand_cols = false;
            end_col -= 1;
          }
        }
      }

      width = x_coords[end_col + 1] - x_coords[start_col];
      height = y_coords[end_row + 1] - y_coords[start_row];

      bool is_notch = false;
      if (current_neighbors.total() == 4) {
        is_notch = true;
      } else if (current_neighbors.top && current_neighbors.bottom) {
        if (height < notch_h_th_) {
          is_notch = true;
        }
      } else if (current_neighbors.left && current_neighbors.right) {
        if (width < notch_v_th_) {
          is_notch = true;
        }
      }

      if (is_notch) {
        notch_penalty_ += calSingleNotchPenalty(width, height);
        if (graphics_) {
          graphics_->addNotch(odb::Rect(x_coords[start_col],
                                        y_coords[start_row],
                                        x_coords[end_col + 1],
                                        y_coords[end_row + 1]));
        }
      }
    }
  }

  if (graphics_) {
    graphics_->setNotchPenalty(
        {"Notch", notch_weight_, notch_penalty_, norm_notch_penalty_});
  }
}

void SACoreSoftMacro::resizeOneCluster()
{
  if (pos_seq_.empty()) {
    logger_->error(
        utl::MPL,
        51,
        "Position sequence array is empty, please report this internal error");
  }

  boost::random::uniform_int_distribution<> index_distribution(
      0, pos_seq_.size() - 1);
  const int idx = index_distribution(generator_);

  macro_id_ = idx;
  SoftMacro& src_macro = macros_[idx];
  if (src_macro.isMacroCluster()) {
    src_macro.resizeRandomly(distribution_, generator_);
    return;
  }

  const int lx = src_macro.getX();
  const int ly = src_macro.getY();
  const int ux = lx + src_macro.getWidth();
  const int uy = ly + src_macro.getHeight();
  // if the macro is outside of the outline, we randomly resize the macro
  if (ux >= outline_.dx() || uy >= outline_.dy()) {
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
    int e_x2 = outline_.dx();
    for (const int macro_id : pos_seq_) {
      SoftMacro& macro = macros_[macro_id];
      const int cur_x2 = macro.getX() + macro.getWidth();
      if (cur_x2 > ux && cur_x2 < e_x2) {
        e_x2 = cur_x2;
      }
    }
    src_macro.setWidth(e_x2 - lx);
  } else if (option <= 0.5) {
    int d_x2 = lx;
    for (const int macro_id : pos_seq_) {
      SoftMacro& macro = macros_[macro_id];
      const int cur_x2 = macro.getX() + macro.getWidth();
      if (cur_x2 < ux && cur_x2 > d_x2) {
        d_x2 = cur_x2;
      }
    }
    if (d_x2 > lx) {
      src_macro.setWidth(d_x2 - lx);
    }
  } else if (option <= 0.75) {
    // change the height of soft block to Tb = a.y2 - b.y1
    int a_y2 = outline_.dy();
    for (const int macro_id : pos_seq_) {
      SoftMacro& macro = macros_[macro_id];
      const int cur_y2 = macro.getY() + macro.getHeight();
      if (cur_y2 > uy && cur_y2 < a_y2) {
        a_y2 = cur_y2;
      }
    }
    src_macro.setHeight(a_y2 - ly);
  } else {
    // Change the height of soft block to Bb = c.y2 - b.y1
    int c_y2 = ly;
    for (const int macro_id : pos_seq_) {
      SoftMacro& macro = macros_[macro_id];
      const int cur_y2 = macro.getY() + macro.getHeight();
      if (cur_y2 < uy && cur_y2 > c_y2) {
        c_y2 = cur_y2;
      }
    }
    if (c_y2 > ly) {
      src_macro.setHeight(c_y2 - ly);
    }
  }
}

void SACoreSoftMacro::printResults() const
{
  reportCoreWeights();
  report({"Boundary",
          boundary_weight_,
          boundary_penalty_,
          norm_boundary_penalty_});
  report({"Macro Blockage",
          macro_blockage_weight_,
          macro_blockage_penalty_,
          norm_macro_blockage_penalty_});
  report({"Notch", notch_weight_, notch_penalty_, norm_notch_penalty_});
  reportTotalCost();
  if (logger_->debugCheck(MPL, "hierarchical_macro_placement", 2)) {
    reportLocations();
  }
}

// fill the dead space by adjust the size of MixedCluster
void SACoreSoftMacro::fillDeadSpace()
{
  // if the floorplan is invalid, do nothing
  if (!isValid()) {
    return;
  }

  // adjust the location of MixedCluster
  // Step1 : Divide the entire floorplan into grids
  std::set<int> x_point;
  std::set<int> y_point;
  for (auto& macro_id : pos_seq_) {
    if (macros_[macro_id].getArea() == 0) {
      continue;
    }
    x_point.insert(macros_[macro_id].getX());
    x_point.insert(macros_[macro_id].getX() + macros_[macro_id].getWidth());
    y_point.insert(macros_[macro_id].getY());
    y_point.insert(macros_[macro_id].getY() + macros_[macro_id].getHeight());
  }
  x_point.insert(0);
  y_point.insert(0);
  x_point.insert(outline_.dx());
  y_point.insert(outline_.dy());
  // create grid
  std::vector<int> x_grid(x_point.begin(), x_point.end());
  std::vector<int> y_grid(y_point.begin(), y_point.end());
  // create grid in a row-based manner
  std::vector<std::vector<int>> grids;  // store the macro id
  const int num_x = x_grid.size() - 1;
  const int num_y = y_grid.size() - 1;
  for (int j = 0; j < num_y; j++) {
    std::vector<int> macro_ids(num_x, -1);
    grids.push_back(std::move(macro_ids));
  }

  for (int macro_id = 0; macro_id < pos_seq_.size(); macro_id++) {
    if (macros_[macro_id].getArea() == 0) {
      continue;
    }

    int x_start = getSegmentIndex(macros_[macro_id].getX(), x_grid);
    int x_end = getSegmentIndex(
        macros_[macro_id].getX() + macros_[macro_id].getWidth(), x_grid);
    int y_start = getSegmentIndex(macros_[macro_id].getY(), y_grid);
    int y_end = getSegmentIndex(
        macros_[macro_id].getY() + macros_[macro_id].getHeight(), y_grid);

    for (int j = y_start; j < y_end; j++) {
      for (int i = x_start; i < x_end; i++) {
        grids[j][i] = macro_id;
      }
    }
  }
  // propagate from the MixedCluster and then StdCellCluster
  for (int order = 0; order <= 1; order++) {
    for (int macro_id = 0; macro_id < pos_seq_.size(); macro_id++) {
      if (macros_[macro_id].getArea() == 0) {
        continue;
      }
      const bool forward_flag = (order == 0)
                                    ? macros_[macro_id].isMixedCluster()
                                    : macros_[macro_id].isStdCellCluster();
      if (!forward_flag) {
        continue;
      }

      int x_start = getSegmentIndex(macros_[macro_id].getX(), x_grid);
      int x_end = getSegmentIndex(
          macros_[macro_id].getX() + macros_[macro_id].getWidth(), x_grid);
      int y_start = getSegmentIndex(macros_[macro_id].getY(), y_grid);
      int y_end = getSegmentIndex(
          macros_[macro_id].getY() + macros_[macro_id].getHeight(), y_grid);

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
          }  // end if
        }  // end y
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
          }  // end if
        }  // end y
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
          }  // end if
        }  // end y
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
          }  // end if
        }  // end y
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

int SACoreSoftMacro::getSegmentIndex(const int segment,
                                     const std::vector<int>& coords)
{
  int index = std::distance(coords.begin(),
                            std::ranges::lower_bound(coords, segment));
  return index;
}

// The blockages here are only those that overlap with the annealing outline.
void SACoreSoftMacro::addBlockages(const std::vector<odb::Rect>& blockages)
{
  blockages_.insert(blockages_.end(), blockages.begin(), blockages.end());
}

std::vector<odb::Point> SACoreSoftMacro::getClustersLocations() const
{
  std::vector<odb::Point> clusters_locations(pos_seq_.size());
  for (int id : pos_seq_) {
    clusters_locations[id] = {macros_[id].getX(), macros_[id].getY()};
  }

  return clusters_locations;
}

void SACoreSoftMacro::setClustersLocations(
    const std::vector<odb::Point>& clusters_locations)
{
  if (clusters_locations.size() != pos_seq_.size()) {
    logger_->error(MPL,
                   52,
                   "setClustersLocation called with a different numbers of "
                   "clusters of that in the sequence pair");
  }

  for (int& id : pos_seq_) {
    macros_[id].setX(clusters_locations[id].x());
    macros_[id].setY(clusters_locations[id].y());
  }
}

void SACoreSoftMacro::attemptCentralization(const float pre_cost)
{
  if (outline_penalty_ > 0) {
    return;
  }

  // In order to revert the centralization, we cache the current location
  // of the clusters to avoid floating-point evilness when creating the
  // x,y grid to fill the dead space by expanding mixed clusters.
  auto clusters_locations = getClustersLocations();

  const odb::Point offset((outline_.dx() - width_) / 2,
                          (outline_.dy() - height_) / 2);
  moveFloorplan(offset);
  calPenalty();

  // revert centralization
  if (calNormCost() > pre_cost && !force_centralization_) {
    centralization_was_reverted_ = true;

    setClustersLocations(clusters_locations);

    if (graphics_) {
      graphics_->saStep(macros_);
    }

    calPenalty();
  }
}

void SACoreSoftMacro::moveFloorplan(const odb::Point& offset)
{
  for (auto& id : pos_seq_) {
    macros_[id].setX(macros_[id].getX() + offset.x());
    macros_[id].setY(macros_[id].getY() + offset.y());
  }

  if (graphics_) {
    graphics_->saStep(macros_);
  }
}

}  // namespace mpl
