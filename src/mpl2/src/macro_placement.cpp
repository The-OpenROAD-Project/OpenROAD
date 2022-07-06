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

#include "macro_placement.h"

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

namespace mpl {
using std::abs;
using std::cout;
using std::endl;
using std::exp;
using std::fstream;
using std::getline;
using std::ios;
using std::log;
using std::max;
using std::pair;
using std::pow;
using std::sort;
using std::stof;
using std::string;
using std::swap;
using std::thread;
using std::to_string;
using std::unordered_map;
using std::vector;
using utl::Logger;
using utl::MPL;

//////////////////////////////////////////////////////////////////
// Class SimulatedAnnealingCore
template <class T>
SimulatedAnnealingCore<T>::SimulatedAnnealingCore(float outline_width, 
                     float outline_height, // boundary constraints
                     const std::vector<T>& macros, // macros (T = HardMacro or T = SoftMacro)
                     // weight for different penalty
                     float outline_weight,
                     float wirelength_weight,
                     float guidance_weight,
                     float fence_weight, // each blockage will be modeled by a macro with fences
                     // probability of each action 
                     float pos_swap_prob,
                     float neg_swap_prob,
                     float double_swap_prob,
                     float exchange_prob,
                     // Fast SA hyperparameter
                     float init_prob, int max_num_step, int num_perturb_per_step,
                     int k, int c, unsigned seed)
{
  outline_width_  = outline_width;
  outline_height_ = outline_height;

  outline_weight_    = outline_weight;
  wirelength_weight_ = wirelength_weight;
  guidance_weight_   = guidance_weight;
  fence_weight_      = fence_weight;
    
  pos_swap_prob_     = pos_swap_prob;
  neg_swap_prob_     = neg_swap_prob;
  double_swap_prob_  = double_swap_prob;
  exchange_prob_     = exchange_prob;
    
  init_prob_         = init_prob;
  max_num_step_      = max_num_step;
  num_perturb_per_step_ = num_perturb_per_step;
  k_ = k;
  c_ = c;
  
  // generate random
  std::mt19937 randGen(seed);
  generator_ = randGen;
  std::uniform_real_distribution<float> distribution(0.0, 1.0);
  distribution_ = distribution;
   
  // macros and nets
  macros_ = macros;
}

// access functions
template <class T>
float SimulatedAnnealingCore<T>::GetWidth() const
{
  return width_;
}

template <class T>
float SimulatedAnnealingCore<T>::GetHeight() const 
{
  return height_;
}
     
template<class T>
float SimulatedAnnealingCore<T>::GetOutlinePenalty() const 
{
  return outline_penalty_;
}
     
template<class T>
float SimulatedAnnealingCore<T>::GetNormOutlinePenalty() const 
{
  return norm_outline_penalty_;
}

template<class T>
float SimulatedAnnealingCore<T>::GetWirelength() const 
{
  return wirelength_;
}
     
template<class T>
float SimulatedAnnealingCore<T>::GetNormWirelength() const 
{
  return norm_wirelength_;
}

template<class T>
float SimulatedAnnealingCore<T>::GetGuidancePenalty() const 
{
  return guidance_penalty_;
}
     
template<class T>
float SimulatedAnnealingCore<T>::GetNormGuidancePenalty() const 
{
  return norm_guidance_penalty_;
}

template<class T>
float SimulatedAnnealingCore<T>::GetFencePenalty() const 
{
  return fence_penalty_;
}
     
template<class T>
float SimulatedAnnealingCore<T>::GetNormFencePenalty() const 
{
  return norm_fence_penalty_;
}

template <class T>
std::vector<T> SimulatedAnnealingCore<T>::GetMacros() const
{
  return macros_;
}


// Private functions
template <class T>
float SimulatedAnnealingCore<T>::CalNormCost()
{
  return 0.0; // this function will be redefined in the derived class
}
    
template <class T>
void SimulatedAnnealingCore<T>::CalPenalty() 
{
  return; // this function will be redefined in the derived class
}
        
    
template <class T>
void SimulatedAnnealingCore<T>::CalOutlinePenalty() 
{
  const float max_width = max(outline_width_, width_);
  const float max_height = max(outline_height_, height_);
  outline_penalty_ = max_width * max_height - outline_width_ * outline_height_;
}
 

template <class T>
void SimulatedAnnealingCore<T>::CalWirelength() 
{
  // Initialization
  wirelength_ = 0.0;
  for (const auto& net : nets_) {
    const float x1 = macros_[net.terminals.first].GetPinX();
    const float y1 = macros_[net.terminals.first].GetPinY();
    const float x2 = macros_[net.terminals.second].GetPinX();
    const float y2 = macros_[net.terminals.second].GetPinY();
    wirelength_ += net.weight * (std::abs(x2 - x1) + std::abs(y2 - y1));
  }
}


template <class T>
void SimulatedAnnealingCore<T>::CalFencePenalty() 
{
  // Initialization
  fence_penalty_ = 0.0;
  for (const auto& [id , bbox] : fences_) {
    const float lx = macros_[id].GetX();
    const float ly = macros_[id].GetY();
    const float ux = lx + macros_[id].GetWidth();
    const float uy = ly + macros_[id].GetHeight();
    const float width  = std::max(ux, bbox->xMax()) - std::min(lx, bbox->xMin()) 
                         - (bbox->xMax() - bbox->xMin());
    const float height = std::max(uy, bbox->yMax()) - std::min(ly, bbox->yMin()) 
                         - (bbox->yMax() - bbox->yMin());
    fence_penalty_ += width * height;
  } 
}

template <class T>
void SimulatedAnnealingCore<T>::CalGuidancePenalty() 
{
  // Initialization
  guidance_penalty_ = 0.0;
  for (const auto& [id , bbox] : guides_) {
    const float macro_lx = macros_[id].GetX();
    const float macro_ly = macros_[id].GetY();
    const float macro_ux = macro_lx + macros_[id].GetWidth();
    const float macro_uy = macro_ly + macros_[id].GetHeight();
    // center to center distance
    const float width  = (macro_ux - macro_lx) + (bbox->xMax() - bbox->xMin());
    const float height = (macro_uy - macro_ly) + (bbox->yMax() - bbox->yMin());
    float x_dist = std::abs((macro_ux + macro_lx) / 2.0 - (bbox->xMax() + bbox->xMin()) / 2.0);
    float y_dist = std::abs((macro_uy + macro_ly) / 2.0 - (bbox->yMax() + bbox->yMin()) / 2.0);
    x_dist = x_dist - width > 0.0 ? x_dist - width : 0.0;
    y_dist = y_dist - height > 0.0 ? y_dist - height : 0.0;
    if (x_dist >= 0.0 && y_dist >= 0.0)
      guidance_penalty_ += std::min(x_dist, y_dist);
  } 
}

 
// Determine the positions of macros based on sequence pair
template <class T>
void SimulatedAnnealingCore<T>::PackFloorplan() 
{
  for (auto& macro : macros_) {
    macro.SetX(0.0);
    macro.SetY(0.0);
  }
   
  // calculate X position
  // store the position of each macro in the pos_seq_ and neg_seq_
  std::vector<std::pair<int, int> > match(macros_.size());
  for (int i = 0; i < macros_.size(); i++) {
    match[pos_seq_[i]].first = i;
    match[neg_seq_[i]].second = i;
  }
  // Initialize current length
  std::vector<float> length(macros_.size(), 0.0);
  for (int i = 0; i < pos_seq_.size(); i++) {
    const int b = pos_seq_[i]; // macro_id
    const int p = match[b].second; // the position of current macro in neg_seq_
    macros_[b].SetX(length[p]);
    const float t = macros_[b].GetX() + macros_[b].GetWidth(); 
    for (int j = p; j < neg_seq_.size(); j++)
      if (t > length[j])
        length[j] = t;
      else
        break;
  }
  // update width_ of current floorplan
  width_ = length[macros_.size() - 1];
 
  // calulate Y position
  std::vector<int> pos_seq(pos_seq_.size());
  for (int i = 0; i < macros_.size(); i++)
    pos_seq[i] = pos_seq_[macros_.size() - 1 - i];
  // store the position of each macro in the pos_seq_ and neg_seq_
  for (int i = 0; i < macros_.size(); i++) {
    match[pos_seq[i]].first = i;
    match[neg_seq_[i]].second = i;
    length[i] = 0.0; // initialize the length
  }
  for (int i = 0; i < pos_seq_.size(); i++) {
    const int b = pos_seq_[i]; // macro_id
    const int p = match[b].second; // the position of current macro in neg_seq_
    macros_[b].SetY(length[p]);
    const float t = macros_[b].GetY() + macros_[b].GetHeight(); 
    for (int j = p; j < neg_seq_.size(); j++)
      if (t > length[j])
        length[j] = t;
      else
        break;
  }
  // update width_ of current floorplan
  height_ = length[macros_.size() - 1];
}


// Perturb
template <class T>
void SimulatedAnnealingCore<T>::Perturb() 
{
  return; // This function will be redefined in the derived classes
}

// Perturb
template <class T>
void SimulatedAnnealingCore<T>::Restore() 
{
  return; // This function will be redefined in the derived classes
}
 
// SingleSeqSwap
template <class T>
void SimulatedAnnealingCore<T>::SingleSeqSwap(bool pos) 
{
  if (macros_.size() <= 1)
    return;
    
  const int index1 = (int) (std::floor((distribution_) (generator_) * macros_.size()));
  int index2 = (int) (std::floor((distribution_) (generator_) * macros_.size()));
  while (index1 == index2) 
    index2 = (int) (std::floor((distribution_) (generator_) * macros_.size()));
   
  if (pos == true)
    std::swap(pos_seq_[index1], pos_seq_[index2]);
  else
    std::swap(neg_seq_[index1], neg_seq_[index2]);
}

// DoubleSeqSwap
template <class T>
void SimulatedAnnealingCore<T>::DoubleSeqSwap() 
{
  if (macros_.size() <= 1)
    return;
    
  const int index1 = (int) (std::floor((distribution_) (generator_) * macros_.size()));
  int index2 = (int) (std::floor((distribution_) (generator_) * macros_.size()));
  while (index1 == index2) 
    index2 = (int) (std::floor((distribution_) (generator_) * macros_.size()));
   
  std::swap(pos_seq_[index1], pos_seq_[index2]);
  std::swap(neg_seq_[index1], neg_seq_[index2]);
}

// ExchaneMacros
template <class T>
void SimulatedAnnealingCore<T>::ExchangeMacros() 
{
  if (macros_.size() <= 1)
    return;
  
  // swap positive seq
  int index1 = (int) (std::floor((distribution_) (generator_) * macros_.size()));
  int index2 = (int) (std::floor((distribution_) (generator_) * macros_.size()));
  while (index1 == index2) 
    index2 = (int) (std::floor((distribution_) (generator_) * macros_.size()));

  std::swap(pos_seq_[index1], pos_seq_[index2]);  
  int neg_index1 = -1;
  int neg_index2 = -1;
  // swap negative seq
  for (int i = 0; i < macros_.size(); i++) {
    if (pos_seq_[index1] == neg_seq_[i])
      neg_index1 = i;
    if (pos_seq_[index2] == neg_seq_[i])
      neg_index2 = i;
  }
  std::swap(neg_seq_[neg_index1], neg_seq_[neg_index2]);
}

template <class T>
float SimulatedAnnealingCore<T>::CalAverage(std::vector<float>& value_list) 
{
  if (value_list.size() == 0)
    return 0.0;

  float sum = 0.0;
  for (const auto& value : value_list)
    sum += value;
  return sum / value_list.size();
}

template <class T>
void SimulatedAnnealingCore<T>::Initialize() 
{
  return; // This function will be redefined in derived classes
}

template <class T>
void SimulatedAnnealingCore<T>::FastSA() 
{
  Perturb();  // Perturb from beginning
  // record the previous status
  float cost = CalNormCost();
  float pre_cost = cost;
  float delta_cost = 0.0;
  int step = 1;
  float t = init_T_;
  // const for restart
  int num_restart = 1;
  const int max_num_restart = 2;
  // SA process 
  while (step <= max_num_step_) {
    for (int i = 0; i < num_perturb_per_step_; i++) {
      Perturb();
      cost = CalNormCost();
      delta_cost = cost - pre_cost;
      const float num = distribution_(generator_);
      const float prob = (delta_cost > 0.0) ? exp((-1) * delta_cost / t) : 1;
      if (num < prob) 
        pre_cost = cost;   
      else 
        Restore();
    }
    // Update Temperature
    if (step <= k_)
      t = init_T_ / (step * c_);
    else
      t = init_T_ / step;
    // increase step
    step++;
    // check if restart condition
    if ((num_restart <= max_num_restart) && 
        (step == std::floor(max_num_step_ / max_num_restart)  && 
        (outline_penalty_ > 0.0))) {
      num_restart++;
      step = 1;
      t = init_T_;
    } // end if
  } // end while
  // update the final results
  PackFloorplan();
  CalPenalty();
}

//////////////////////////////////////////////////////////////////
// Class SACoreHardMacro
// constructors
SACoreHardMacro::SACoreHardMacro(float outline_width, 
                float outline_height, // boundary constraints
                const std::vector<HardMacro>& macros, 
                // weight for different penalty
                float outline_weight,
                float wirelength_weight,
                float guidance_weight,
                float fence_weight, // each blockage will be modeled by a macro with fences
                // probability of each action 
                float pos_swap_prob,
                float neg_swap_prob,
                float double_swap_prob,
                float exchange_prob,
                float flip_prob,
                // Fast SA hyperparameter
                float init_prob, int max_num_step, int num_perturb_per_step,
                int k, int c, unsigned seed) 
  : SimulatedAnnealingCore<HardMacro>(outline_width, outline_height, macros,
                              outline_weight, wirelength_weight, guidance_weight, fence_weight,
                              pos_swap_prob, neg_swap_prob, double_swap_prob, exchange_prob,
                              init_prob, max_num_step, num_perturb_per_step, k, c, seed) 
{ 
  flip_prob_ = flip_prob;  
}


float SACoreHardMacro::CalNormCost() 
{
  float cost = 0.0; // Initialize cost
  if (norm_outline_penalty_ > 0.0)
    cost += outline_weight_ * outline_penalty_ / norm_outline_penalty_;
  if (norm_wirelength_ > 0.0)
    cost += wirelength_weight_ * wirelength_ / norm_wirelength_;
  if (norm_guidance_penalty_ > 0.0)
    cost += guidance_weight_ * guidance_penalty_ / norm_guidance_penalty_;
  if (norm_fence_penalty_ > 0.0)
    cost += fence_weight_ * fence_penalty_ / norm_fence_penalty_;
  return cost;
}


void SACoreHardMacro::CalPenalty()
{
  CalOutlinePenalty();
  CalWirelength();
  CalGuidancePenalty();
  CalFencePenalty();
}

void SACoreHardMacro::FlipMacro()
{
  macro_id_ = static_cast<int>(std::floor(
              (distribution_)(generator_) * macros_.size()));
  const float prob = (distribution_) (generator_);
  if (prob <= 0.5)
    macros_[macro_id_].Flip(true);
  else
    macros_[macro_id_].Flip(false);
}

void SACoreHardMacro::Perturb()
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

  // generate random number (0 - 1) to determine actions
  const float op = (distribution_) (generator_);
  const float action_prob_1 = pos_swap_prob_;
  const float action_prob_2 = action_prob_1 + neg_swap_prob_;
  const float action_prob_3 = action_prob_2 + double_swap_prob_;
  const float action_prob_4 = action_prob_3 + exchange_prob_;
  const float action_prob_5 = action_prob_4 + flip_prob_;
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
    FlipMacro();      // Flip one macro
  }
  
  // update the macro locations based on Sequence Pair
  PackFloorplan();
  // Update all the penalties
  CalPenalty();
}

void SACoreHardMacro::Restore()
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
}

void SACoreHardMacro::Initialize()
{  
  std::vector<float> outline_penalty_list;
  std::vector<float> wirelength_list;
  std::vector<float> guidance_penalty_list;
  std::vector<float> fence_penalty_list;
 
  for (int i = 0; i < num_perturb_per_step_; i++) {
    Perturb();
    // store current penalties 
    outline_penalty_list.push_back(outline_penalty_);
    wirelength_list.push_back(wirelength_);
    guidance_penalty_list.push_back(guidance_penalty_);
    fence_penalty_list.push_back(fence_penalty_);
  }
 
  norm_outline_penalty_  = CalAverage(outline_penalty_list);
  norm_wirelength_       = CalAverage(wirelength_list);
  norm_guidance_penalty_ = CalAverage(guidance_penalty_list);
  norm_fence_penalty_    = CalAverage(fence_penalty_list);
  // Calculate initial temperature
  std::vector<float> cost_list;
  for (int i = 0; i < outline_penalty_list.size(); i++) {
    outline_penalty_  = outline_penalty_list[i];
    wirelength_       = wirelength_list[i];
    guidance_penalty_ = guidance_penalty_list[i];
    fence_penalty_    = fence_penalty_list[i];
    cost_list.push_back(CalNormCost());
  }
  float delta_cost = 0.0;
  for (int i = 1; i < cost_list.size(); i++)
    delta_cost += std::abs(cost_list[i] - cost_list[i - 1]);
  init_T_ = (-1.0) * (delta_cost / (cost_list.size() - 1)) / log(init_prob_);
}


//////////////////////////////////////////////////////////////////
// Class SACoreSoftMacro
// constructors
SACoreSoftMacro::SACoreSoftMacro(float outline_width, 
                float outline_height, // boundary constraints
                const std::vector<SoftMacro>& macros, 
                // weight for different penalty
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
                              outline_weight, wirelength_weight, guidance_weight, fence_weight,
                              pos_swap_prob, neg_swap_prob, double_swap_prob, exchange_prob,
                              init_prob, max_num_step, num_perturb_per_step, k, c, seed) 
{ 
  boundary_weight_ = boundary_weight;
  notch_weight_    = notch_weight;
  resize_prob_     = resize_prob;
  notch_h_th_      = notch_h_threshold;
  notch_v_th_      = notch_v_threshold;
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
  for (int i = 0; i < num_perturb_per_step_; i++) {
    Perturb();
    // store current penalties 
    outline_penalty_list.push_back(outline_penalty_);
    wirelength_list.push_back(wirelength_);
    guidance_penalty_list.push_back(guidance_penalty_);
    fence_penalty_list.push_back(fence_penalty_);
    boundary_penalty_list.push_back(boundary_penalty_);
    notch_penalty_list.push_back(notch_penalty_);
  }
 
  norm_outline_penalty_  = CalAverage(outline_penalty_list);
  norm_wirelength_       = CalAverage(wirelength_list);
  norm_guidance_penalty_ = CalAverage(guidance_penalty_list);
  norm_fence_penalty_    = CalAverage(fence_penalty_list);
  norm_boundary_penalty_ = CalAverage(boundary_penalty_list);
  norm_notch_penalty_    = CalAverage(notch_penalty_list);

  // Calculate initial temperature
  std::vector<float> cost_list;
  for (int i = 0; i < outline_penalty_list.size(); i++) {
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
  
  // Align macro clusters to boundaries
  for (auto& macro : macros_) {
    if (macro.IsMacroCluster() == true) {
      const float lx = macro.GetX();
      const float ly = macro.GetY();
      const float ux = lx + macro.GetWidth();
      const float uy = ly + macro.GetHeight();
      // align to left / right boundaries
      if (lx <= notch_h_th_)
        macro.SetX(0.0);
      else if (outline_width_ - ux <= notch_h_th_)
        macro.SetX(outline_width_ - macro.GetWidth());
      // align to top / bottom boundaries
      if (ly <= notch_v_th_)
        macro.SetY(0.0);
      else if (outline_height_ - uy <= notch_v_th_)
        macro.SetY(outline_height_ - macro.GetHeight());
    }
  }
  // Align macro clusters horizontally
  // Align macro clusters vertically
}


// If there is no HardMacroCluster, we do not consider the notch penalty
void SACoreSoftMacro::CalNotchPenalty()
{
  // Initialization
  notch_penalty_ = 0.0;
  bool hard_macro_cluster_flag = false;
  for (const auto& macro : macros_) {
    if (macro.IsMacroCluster() == true) {
      hard_macro_cluster_flag = true;
      break;
    }
  }
  
  // If there is no HardMacroCluster, we do not consider the notch penalty
  if (hard_macro_cluster_flag == false)
    return;
  
  // If the floorplan cannot fit into the outline
  if (width_ > outline_width_ || height_ > outline_height_) {
    const float area = std::max(width_, outline_width_) * max(height_, outline_height_);
    notch_penalty_ = std::sqrt(area / (outline_width_ * outline_height_));
    return;
  }
   
  // Calculate the notch penalty cost based on the area of notches
  // First align macro clusters to reduce notches
  AlignMacroClusters();
  // Then calculate notch penalty
  return;
}


void SACoreSoftMacro::Resize()
{
  int idx = static_cast<int>(std::floor((distribution_)(generator_) * macros_.size()));
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



}  // namespace mpl
