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

#include <algorithm>
#include <cmath>
#include <fstream>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "odb/dbTypes.h"
#include "utl/Logger.h"
#include "db_sta/dbReadVerilog.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "sta/Bfs.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/Sta.hh"
#include "object.h"
#include "SimulatedAnnealingCore.h"

namespace mpl {

//////////////////////////////////////////////////////////////////
// Class SACoreSoftMacro
// constructors
SACoreSoftMacro::SACoreSoftMacro(float outline_width, 
                float outline_height, // boundary constraints
                const std::vector<SoftMacro>& macros, 
                // weight for different penalty
                float area_weight,
                float outline_weight,
                float wirelength_weight,
                float guidance_weight,
                float fence_weight, // each blockage will be modeled by a macro with fences
                float boundary_weight,
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
                float init_prob, int max_num_step, int num_perturb_per_step,
                int k, int c, unsigned seed) 
  : SimulatedAnnealingCore<SoftMacro>(outline_width, outline_height, macros,
                              area_weight, outline_weight, wirelength_weight, 
                              guidance_weight, fence_weight,
                              pos_swap_prob, neg_swap_prob, double_swap_prob, exchange_prob,
                              init_prob, max_num_step, num_perturb_per_step, k, c, seed) 
{ 
  boundary_weight_ = boundary_weight;
  original_notch_weight_    = notch_weight;
  resize_prob_     = resize_prob;
  notch_h_th_      = notch_h_threshold;
  notch_v_th_      = notch_v_threshold;
  adjust_h_th_     = notch_h_th_;
  adjust_v_th_     = notch_v_th_;
}

// acessors functions
float SACoreSoftMacro::GetBoundaryPenalty() const 
{
  return boundary_penalty_;
}

float SACoreSoftMacro::GetNormBoundaryPenalty() const 
{
  return norm_boundary_penalty_;
}

float SACoreSoftMacro::GetNotchPenalty() const 
{
  return notch_penalty_;
}

float SACoreSoftMacro::GetNormNotchPenalty() const 
{
  return norm_notch_penalty_;
}


// Operations
float SACoreSoftMacro::CalNormCost() 
{
  float cost = 0.0; // Initialize cost
  if (norm_area_penalty_ > 0.0)
    cost += area_weight_ * (width_ * height_) / norm_area_penalty_;
  if (norm_outline_penalty_ > 0.0)
    cost += outline_weight_ * outline_penalty_ / norm_outline_penalty_;
  if (norm_wirelength_ > 0.0)
    cost += wirelength_weight_ * wirelength_ / norm_wirelength_;
  if (norm_guidance_penalty_ > 0.0)
    cost += guidance_weight_ * guidance_penalty_ / norm_guidance_penalty_;
  if (norm_fence_penalty_ > 0.0)
    cost += fence_weight_ * fence_penalty_ / norm_fence_penalty_;
  if (norm_boundary_penalty_ > 0.0)
    cost += boundary_weight_ * boundary_penalty_ / norm_boundary_penalty_;
  if (norm_notch_penalty_ > 0.0)
    cost += notch_weight_ * notch_penalty_ / norm_notch_penalty_;
  return cost;
}


void SACoreSoftMacro::CalPenalty()
{
  CalOutlinePenalty();
  CalWirelength();
  CalGuidancePenalty();
  CalFencePenalty();
  CalBoundaryPenalty();
  CalNotchPenalty();
}


void SACoreSoftMacro::Perturb()
{
  if (macros_.size() == 0)
    return;

  // Keep back up
  pre_pos_seq_ = pos_seq_;
  pre_neg_seq_ = neg_seq_;
  pre_width_ = width_;
  pre_height_ = height_;
  pre_outline_penalty_  = outline_penalty_;
  pre_wirelength_       = wirelength_;
  pre_guidance_penalty_ = guidance_penalty_;
  pre_fence_penalty_    = fence_penalty_;
  pre_boundary_penalty_ = boundary_penalty_;
  pre_notch_penalty_    = notch_penalty_;   

  // generate random number (0 - 1) to determine actions
  const float op = (distribution_) (generator_);
  const float action_prob_1 = pos_swap_prob_;
  const float action_prob_2 = action_prob_1 + neg_swap_prob_;
  const float action_prob_3 = action_prob_2 + double_swap_prob_;
  const float action_prob_4 = action_prob_3 + exchange_prob_;
  const float action_prob_5 = action_prob_4 + resize_prob_;
  if (op <= action_prob_1) {
    action_id_ = 1;
    SingleSeqSwap(true); // Swap two macros in pos_seq_
  } else if (op <= action_prob_2) {
    action_id_ = 2;
    SingleSeqSwap(false); // Swap two macros in neg_seq_;
  } else if (op <= action_prob_3) {
    action_id_ = 3;
    DoubleSeqSwap(); // Swap two macros in pos_seq_ and 
                     // other two macros in neg_seq_
  } else if (op <= action_prob_4) {
    action_id_ = 4;
    ExchangeMacros();  // exchange two macros in the sequence pair
  } else {
    action_id_= 5;
    pre_macros_ = macros_;
    Resize();      // Flip one macro
  }
  
  // update the macro locations based on Sequence Pair
  PackFloorplan();
  // Update all the penalties
  CalPenalty();
}

void SACoreSoftMacro::Restore()
{
  if (macros_.size() == 0)
    return;
 
  // To reduce the runtime, here we do not call PackFloorplan
  // again. So when we need to generate the final floorplan out,
  // we need to call PackFloorplan again at the end of SA process
  if (action_id_ == 5)
    macros_[macro_id_] = pre_macros_[macro_id_];
  else if (action_id_ == 1)
    pos_seq_ = pre_pos_seq_;
  else if (action_id_ == 2)
    neg_seq_ = pre_neg_seq_;
  else {
    pos_seq_ = pre_pos_seq_;
    neg_seq_ = pre_neg_seq_;
  }
 
  width_  = pre_width_;
  height_ = pre_height_;
  outline_penalty_  = pre_outline_penalty_;
  wirelength_       = pre_wirelength_;
  guidance_penalty_ = pre_guidance_penalty_;
  fence_penalty_    = pre_fence_penalty_;
  boundary_penalty_ = pre_boundary_penalty_;
  notch_penalty_    = pre_notch_penalty_;
}

void SACoreSoftMacro::Initialize()
{  
  std::vector<float> outline_penalty_list;
  std::vector<float> wirelength_list;
  std::vector<float> guidance_penalty_list;
  std::vector<float> fence_penalty_list;
  std::vector<float> boundary_penalty_list;
  std::vector<float> notch_penalty_list;
  std::vector<float> area_penalty_list;
  std::vector<float> width_list;
  std::vector<float> height_list;
  for (int i = 0; i < num_perturb_per_step_; i++) {
    Perturb();
    // store current penalties 
    width_list.push_back(width_);
    height_list.push_back(height_);
    area_penalty_list.push_back(width_ * height_);
    outline_penalty_list.push_back(outline_penalty_);
    wirelength_list.push_back(wirelength_);
    guidance_penalty_list.push_back(guidance_penalty_);
    fence_penalty_list.push_back(fence_penalty_);
    boundary_penalty_list.push_back(boundary_penalty_);
    notch_penalty_list.push_back(notch_penalty_);
  }
 
  norm_area_penalty_     = CalAverage(area_penalty_list);
  norm_outline_penalty_  = CalAverage(outline_penalty_list);
  norm_wirelength_       = CalAverage(wirelength_list);
  norm_guidance_penalty_ = CalAverage(guidance_penalty_list);
  norm_fence_penalty_    = CalAverage(fence_penalty_list);
  norm_boundary_penalty_ = CalAverage(boundary_penalty_list);
  //norm_notch_penalty_    = CalAverage(notch_penalty_list);
  norm_notch_penalty_    = outline_width_ * outline_height_;

  // Calculate initial temperature
  std::vector<float> cost_list;
  for (int i = 0; i < outline_penalty_list.size(); i++) {
    width_            = width_list[i];
    height_           = height_list[i];
    outline_penalty_  = outline_penalty_list[i];
    wirelength_       = wirelength_list[i];
    guidance_penalty_ = guidance_penalty_list[i];
    fence_penalty_    = fence_penalty_list[i];
    boundary_penalty_ = boundary_penalty_list[i];
    notch_penalty_    = notch_penalty_list[i];
    cost_list.push_back(CalNormCost());
  }
  float delta_cost = 0.0;
  for (int i = 1; i < cost_list.size(); i++)
    delta_cost += std::abs(cost_list[i] - cost_list[i - 1]);
  init_T_ = (-1.0) * (delta_cost / (cost_list.size() - 1)) / log(init_prob_);
}

// We only push hard macro clusters to boundaries
// Note that we do not push MixedCluster into boundaries
void SACoreSoftMacro::CalBoundaryPenalty()
{
  // Initialization
  boundary_penalty_ = 0.0;
  if (boundary_weight_ <= 0.0)
    return;
  
  for (const auto& macro : macros_) {
    if (macro.IsMacroCluster() == true) {
      const float lx = macro.GetX();
      const float ly = macro.GetY();
      const float ux = lx + macro.GetWidth();
      const float uy = ly + macro.GetHeight();
      const float x_dist = std::min(lx, std::abs(outline_width_ - ux));
      const float y_dist = std::min(ly, std::abs(outline_height_ - uy));
      boundary_penalty_ += std::pow(std::min(x_dist, y_dist) * macro.GetNumMacro(), 2);
    }
  }
}


// Align macro clusters to reduce notch
void SACoreSoftMacro::AlignMacroClusters()
{
  if (width_ > outline_width_ || height_ > outline_height_)
    return;
  // update threshold value  
  adjust_h_th_ = notch_h_th_;
  adjust_v_th_ = notch_v_th_;
  for (auto& macro : macros_) {
    if (macro.IsMacroCluster() == true) {
      adjust_h_th_ = std::min(adjust_h_th_, macro.GetWidth());
      adjust_v_th_ = std::min(adjust_v_th_, macro.GetHeight());
    }
  }
  // Align macro clusters to boundaries
  for (auto& macro : macros_) {
    if (macro.IsMacroCluster() == true) {
      const float lx = macro.GetX();
      const float ly = macro.GetY();
      const float ux = lx + macro.GetWidth();
      const float uy = ly + macro.GetHeight();
      // align to left / right boundaries
      if (lx <= adjust_h_th_)
        macro.SetX(0.0);
      else if (outline_width_ - ux <= adjust_h_th_)
        macro.SetX(outline_width_ - macro.GetWidth());
      // align to top / bottom boundaries
      if (ly <= adjust_v_th_)
        macro.SetY(0.0);
      else if (outline_height_ - uy <= adjust_v_th_)
        macro.SetY(outline_height_ - macro.GetHeight());
    }
  }
}

// If there is no HardMacroCluster, we do not consider the notch penalty
void SACoreSoftMacro::CalNotchPenalty()
{
  // Initialization
  notch_penalty_ = 0.0;
  if (notch_weight_ <= 0.0)
    return;
  // If the floorplan cannot fit into the outline
  // We think the entire floorplan is a "huge" notch
  if (width_ > outline_width_ || height_ > outline_height_) {
    notch_penalty_ += outline_width_ * outline_height_;
    return;
  }

  pre_macros_ = macros_;
  // Fill dead space
  FillDeadSpace();
  // align macro clusters to reduce notches
  AlignMacroClusters();
  // Create grids based on location of MixedCluster and HardMacroCluster
  std::set<float> x_point;
  std::set<float> y_point;
  std::vector<bool> macro_mask;
  for (auto& macro : macros_) {
    if (macro.GetArea() <= 0.0 ||
       (macro.IsMacroCluster() == false && macro.IsMixedCluster() == false)) {
      macro_mask.push_back(false);
      continue;
    }
    x_point.insert(macro.GetX());
    x_point.insert(macro.GetX() + macro.GetWidth());
    y_point.insert(macro.GetY());
    y_point.insert(macro.GetY() + macro.GetHeight());
    macro_mask.push_back(true);
  }
  x_point.insert(0.0);
  y_point.insert(0.0);
  x_point.insert(outline_width_);
  y_point.insert(outline_height_);
  // create grid
  std::vector<float> x_grid(x_point.begin(), x_point.end());
  std::vector<float> y_grid(y_point.begin(), y_point.end());
  // create grid in a row-based manner
  std::vector<std::vector<int> > grids; // store the macro id
  const int num_x = x_grid.size() - 1;
  const int num_y = y_grid.size() - 1;
  for (int j = 0; j < num_y; j++) {
    std::vector<int> macro_ids(num_x, -1);
    grids.push_back(macro_ids);
  }
  // detect the notch region around each MixedCluster and HardMacroCluster
  for (int macro_id = 0; macro_id < macros_.size(); macro_id++) {
    if (macro_mask[macro_id] == false)
      continue;
    int x_start = 0;
    int x_end = 0;
    CalSegmentLoc(macros_[macro_id].GetX(), 
                  macros_[macro_id].GetX() + macros_[macro_id].GetWidth(),
                  x_start, x_end, x_grid);
    int y_start = 0;
    int y_end   = 0;
    CalSegmentLoc(macros_[macro_id].GetY(), 
                  macros_[macro_id].GetY() + macros_[macro_id].GetHeight(),
                  y_start, y_end, y_grid);
    for (int j = y_start; j < y_end; j++)
      for (int i = x_start; i < x_end; i++)
        grids[j][i] = macro_id;
  }
  // check surroundings of each HardMacroCluster and MixecCluster
  for (int macro_id = 0; macro_id < macros_.size(); macro_id++) {
    if (macro_mask[macro_id] == false)
      continue;
    int x_start = 0;
    int x_end = 0;
    CalSegmentLoc(macros_[macro_id].GetX(), 
                  macros_[macro_id].GetX() + macros_[macro_id].GetWidth(),
                  x_start, x_end, x_grid);
    int y_start = 0;
    int y_end   = 0;
    CalSegmentLoc(macros_[macro_id].GetY(), 
                  macros_[macro_id].GetY() + macros_[macro_id].GetHeight(),
                  y_start, y_end, y_grid);
    int x_start_new = x_start;
    int x_end_new   = x_end;
    int y_start_new = y_start;
    int y_end_new   = y_end;
    // check left first
    for (int i = x_start - 1; i >= 0; i--) {
      bool flag = true;
      for (int j = y_start; j < y_end; j++) {
        if (grids[j][i] != -1) {
          flag = false; // we cannot extend the current cluster   
          break;
        } // end if
      } // end y
      if (flag == false) { // extension done
        break; 
      } else {
        x_start_new--; // extend left
        for (int j = y_start; j < y_end; j++) 
          grids[j][i] = macro_id;
      } // end update
    } // end left 
    // check top second
    for (int j = y_end; j < num_y; j++) {
      bool flag = true;
      for (int i = x_start; i < x_end; i++) {
        if (grids[j][i] != -1) {
          flag = false; // we cannot extend the current cluster   
          break;
        } // end if
      } // end y
      if (flag == false) { // extension done
        break; 
      } else {
        y_end_new++; // extend top
        for (int i = x_start; i < x_end; i++) 
          grids[j][i] = macro_id;
      } // end update
    } // end top 
    // check right third
    for (int i = x_end; i < num_x; i++) {
      bool flag = true;
      for (int j = y_start; j < y_end; j++) {
        if (grids[j][i] != -1) {
          flag = false; // we cannot extend the current cluster   
          break;
        } // end if
      } // end y
      if (flag == false) { // extension done
        break; 
      } else {
        x_end_new++; // extend right
        for (int j = y_start; j < y_end; j++) 
          grids[j][i] = macro_id;
      } // end update
    } // end right
    // check down second
    for (int j = y_start - 1; j >= 0; j--) {
      bool flag = true;
      for (int i = x_start; i < x_end; i++) {
        if (grids[j][i] != -1) {
          flag = false; // we cannot extend the current cluster   
          break;
        } // end if
      } // end y
      if (flag == false) { // extension done
        break; 
      } else {
        y_start_new--; // extend down
        for (int i = x_start; i < x_end; i++) 
          grids[j][i] = macro_id;
      } // end update
    } // end down 
    // check the notch area
    if ((x_grid[x_start] - x_grid[x_start_new]) <= notch_h_th_)
      notch_penalty_ += (x_grid[x_start] - x_grid[x_start_new]) * 
                        macros_[macro_id].GetHeight();    
    if ((x_grid[x_end_new] - x_grid[x_end]) <= notch_h_th_)
      notch_penalty_ += (x_grid[x_end_new] - x_grid[x_end]) * 
                        macros_[macro_id].GetHeight();   
    if ((y_grid[y_start] - y_grid[y_start_new]) <= notch_v_th_)
      notch_penalty_ += (y_grid[y_start] - y_grid[y_start_new]) * 
                        macros_[macro_id].GetWidth();
    if ((y_grid[y_end_new] - y_grid[y_end]) <= notch_v_th_)
      notch_penalty_ += (y_grid[y_end_new] - y_grid[y_end]) * 
                        macros_[macro_id].GetWidth();   
  } 
  macros_ = pre_macros_;
}


void SACoreSoftMacro::Resize()
{
  int idx = static_cast<int>(std::floor((distribution_)(generator_) * macros_.size()));
  macro_id_ = idx;
  SoftMacro& src_macro = macros_[idx];
  if (src_macro.IsMacroCluster() == true) {
    src_macro.ResizeRandomly(distribution_, generator_);
    return;
  }
  
  const float lx = src_macro.GetX();
  const float ly = src_macro.GetY();
  const float ux = lx + src_macro.GetWidth();
  const float uy = ly + src_macro.GetHeight();
  // if the macro is outside of the outline, we randomly resize the macro
  if (lx >= outline_width_ || ly >= outline_height_) {
    src_macro.ResizeRandomly(distribution_, generator_);
    return;
  } 

  float option = (distribution_) (generator_);
  if (option <= 0.25) {
    // Change the width of soft block to Rb = e.x2 - b.x1
    float e_x2 = outline_width_;
    for (const auto& macro : macros_) {
      float cur_x2 = macro.GetX() + macro.GetWidth();
      if (cur_x2 > ux && cur_x2 < e_x2)
        e_x2 = cur_x2;
    }
    src_macro.SetWidth(e_x2 - lx);
  } else if (option <= 0.5) {
    float d_x2 = lx;
    for (const auto& macro : macros_) {
      float cur_x2 = macro.GetX() + macro.GetWidth();
      if (cur_x2 < ux && cur_x2 > d_x2)
        d_x2 = cur_x2;
    }
    if (d_x2 <= lx)
      return;
    else    
      src_macro.SetWidth(d_x2 - lx);
  } else if (option <= 0.75) {
    // change the height of soft block to Tb = a.y2 - b.y1
    float a_y2 = outline_height_;
    for (const auto& macro : macros_) {
      float cur_y2 = macro.GetY() + macro.GetHeight();
      if (cur_y2 > uy && cur_y2 < a_y2)
        a_y2 = cur_y2;
    }
    src_macro.SetHeight(a_y2 - ly);
  } else {
    // Change the height of soft block to Bb = c.y2 - b.y1
    float c_y2 = ly;
    for (const auto& macro : macros_) {
      float cur_y2 = macro.GetY() + macro.GetHeight();
      if (cur_y2 < uy && cur_y2 > c_y2)
        c_y2 = cur_y2;
    }
    if (c_y2 <= ly)
      return;
    else
      src_macro.SetHeight(c_y2 - ly);
  }
}


void SACoreSoftMacro::Shrink()
{
  for (auto& macro : macros_)
    macro.ShrinkArea(shrink_factor_);
}

void SACoreSoftMacro::PrintResults() const
{
  std::cout << "outline_penalty : "
            << outline_penalty_ / norm_outline_penalty_ << std::endl;
  std::cout << "wirelength : "
            << wirelength_ / norm_wirelength_ << std::endl;
  std::cout << "guidance_penalty : "
            << guidance_penalty_ / norm_guidance_penalty_ << std::endl;
  std::cout << "fence_penalty : "
            << fence_penalty_ / norm_fence_penalty_ << std::endl;
  std::cout << "boundary_penalty : " 
            << boundary_penalty_ / norm_boundary_penalty_ << std::endl;
  std::cout << "notch_penalty : " << notch_penalty_ / norm_notch_penalty_ << std::endl;
}


// fill the dead space by adjust the size of MixedCluster
void SACoreSoftMacro::FillDeadSpace()
{ 
  // if the floorplan is invalid, do nothing
  if (width_ > outline_width_ || height_ > outline_height_)
    return;

  // adjust the location of MixedCluster
  // Step1 : Divide the entire floorplan into grids
  std::set<float> x_point;
  std::set<float> y_point;
  for (auto& macro : macros_) {
    if (macro.GetArea() <= 0.0)
      continue;
    x_point.insert(macro.GetX());
    x_point.insert(macro.GetX() + macro.GetWidth());
    y_point.insert(macro.GetY());
    y_point.insert(macro.GetY() + macro.GetHeight());
  }
  x_point.insert(0.0);
  y_point.insert(0.0);
  x_point.insert(outline_width_);
  y_point.insert(outline_height_);
  // create grid
  std::vector<float> x_grid(x_point.begin(), x_point.end());
  std::vector<float> y_grid(y_point.begin(), y_point.end());
  // create grid in a row-based manner
  std::vector<std::vector<int> > grids; // store the macro id
  const int num_x = x_grid.size() - 1;
  const int num_y = y_grid.size() - 1;
  for (int j = 0; j < num_y; j++) {
    std::vector<int> macro_ids(num_x, -1);
    grids.push_back(macro_ids);
  }

  for (int macro_id = 0; macro_id < macros_.size(); macro_id++) {
    if (macros_[macro_id].GetArea() <= 0.0)
      continue;
    int x_start = 0;
    int x_end = 0;
    CalSegmentLoc(macros_[macro_id].GetX(), 
                  macros_[macro_id].GetX() + macros_[macro_id].GetWidth(),
                  x_start, x_end, x_grid);
    int y_start = 0;
    int y_end   = 0;
    CalSegmentLoc(macros_[macro_id].GetY(), 
                  macros_[macro_id].GetY() + macros_[macro_id].GetHeight(),
                  y_start, y_end, y_grid);
    for (int j = y_start; j < y_end; j++)
      for (int i = x_start; i < x_end; i++)
        grids[j][i] = macro_id;
  }
  // propagate from the MixedCluster and then StdCellCluster
  for (int order = 0; order <= 1; order++) {
    for (int macro_id = 0; macro_id < macros_.size(); macro_id++) {
      if (macros_[macro_id].GetArea() <= 0.0)
        continue;
      const bool forward_flag = (order == 0) ?
                                 macros_[macro_id].IsMixedCluster() :
                                 macros_[macro_id].IsStdCellCluster();
      if (forward_flag == false)
        continue;
      int x_start = 0;
      int x_end = 0;
      CalSegmentLoc(macros_[macro_id].GetX(), 
                    macros_[macro_id].GetX() + macros_[macro_id].GetWidth(),
                    x_start, x_end, x_grid);
      int y_start = 0;
      int y_end   = 0;
      CalSegmentLoc(macros_[macro_id].GetY(), 
                    macros_[macro_id].GetY() + macros_[macro_id].GetHeight(),
                    y_start, y_end, y_grid);
      int x_start_new = x_start;
      int x_end_new   = x_end;
      int y_start_new = y_start;
      int y_end_new   = y_end;
      // propagate left first
      for (int i = x_start - 1; i >= 0; i--) {
        bool flag = true;
        for (int j = y_start; j < y_end; j++) {
          if (grids[j][i] != -1) {
            flag = false; // we cannot extend the current cluster   
            break;
          } // end if
        } // end y
        if (flag == false) { // extension done
          break; 
        } else {
          x_start_new--; // extend left
          for (int j = y_start; j < y_end; j++) 
            grids[j][i] = macro_id;
        } // end update
      } // end left 
      x_start = x_start_new;
      // propagate top second
      for (int j = y_end; j < num_y; j++) {
        bool flag = true;
        for (int i = x_start; i < x_end; i++) {
          if (grids[j][i] != -1) {
            flag = false; // we cannot extend the current cluster   
            break;
          } // end if
        } // end y
        if (flag == false) { // extension done
          break; 
        } else {
          y_end_new++; // extend top
          for (int i = x_start; i < x_end; i++) 
            grids[j][i] = macro_id;
        } // end update
      } // end top
      y_end = y_end_new;
      // propagate right third
      for (int i = x_end; i < num_x; i++) {
        bool flag = true;
        for (int j = y_start; j < y_end; j++) {
          if (grids[j][i] != -1) {
            flag = false; // we cannot extend the current cluster   
            break;
          } // end if
        } // end y
        if (flag == false) { // extension done
          break; 
        } else {
          x_end_new++; // extend right
          for (int j = y_start; j < y_end; j++) 
            grids[j][i] = macro_id;
        } // end update
      } // end right
      x_end = x_end_new;
      // propagate down second
      for (int j = y_start - 1; j >= 0; j--) {
        bool flag = true;
        for (int i = x_start; i < x_end; i++) {
          if (grids[j][i] != -1) {
            flag = false; // we cannot extend the current cluster   
            break;
          } // end if
        } // end y
        if (flag == false) { // extension done
          break; 
        } else {
          y_start_new--; // extend down
          for (int i = x_start; i < x_end; i++) 
            grids[j][i] = macro_id;
        } // end update
      } // end down 
      y_start = y_start_new;
      // update the location of cluster
      macros_[macro_id].SetLocationF(x_grid[x_start], y_grid[y_start]);
      macros_[macro_id].SetShapeF(x_grid[x_end] - x_grid[x_start], 
                                  y_grid[y_end] - y_grid[y_start]);
    }
  }
}

// A utility function for FillDeadSpace.
// It's used for calculate the start point and end point for a segment in a grid
void SACoreSoftMacro::CalSegmentLoc(float seg_start, float seg_end,
                                    int& start_id,   int& end_id,
                                    std::vector<float>& grid)
{
  start_id = -1;
  end_id = -1;
  for (int i = 0; i < grid.size() - 1; i++) {
    if ((grid[i] <= seg_start) && (grid[i+1] > seg_start))
      start_id = i;
    if ((grid[i] <= seg_end) && (grid[i+1] > seg_end))
      end_id = i;
  } 
  if (end_id == -1)
    end_id = grid.size() - 1;
}


}  // namespace mpl
