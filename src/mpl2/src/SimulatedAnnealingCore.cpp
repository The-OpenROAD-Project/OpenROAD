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

#include "SimulatedAnnealingCore.h"

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

//////////////////////////////////////////////////////////////////
// Class SimulatedAnnealingCore
template <class T>
SimulatedAnnealingCore<T>::SimulatedAnnealingCore(float outline_width, 
                     float outline_height, // boundary constraints
                     const std::vector<T>& macros, // macros (T = HardMacro or T = SoftMacro)
                     // weight for different penalty
                     float area_weight,
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

  area_weight_       = area_weight;
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

  for (unsigned int i = 0; i < macros.size(); i++) {
     pos_seq_.push_back(i);
     neg_seq_.push_back(i);
     pre_pos_seq_.push_back(i);
     pre_neg_seq_.push_back(i);
  }
}

// access functions
template <class T>
void SimulatedAnnealingCore<T>::SetNets(const std::vector<BundledNet>& nets)
{
  nets_ = nets;
}

template <class T>
void SimulatedAnnealingCore<T>::SetFences(const std::map<int, Rect>& fences)
{
  fences_ = fences;
}

template <class T>
void SimulatedAnnealingCore<T>::SetGuides(const std::map<int, Rect>& guides)
{
  guides_ = guides;
}

template <class T>
bool SimulatedAnnealingCore<T>::IsValid() const
{
  std::cout << "width_ :  " << width_ << "   "
            << "outline_width_ :  " << outline_width_ << std::endl;
  std::cout << "height_ : " << height_ << "   "
            << "outline_height_ : " << outline_height_ << std::endl;
  return (width_ <= outline_width_ * (1.0 + acc_tolerance_)) && 
         (height_ <= outline_height_ * (1.0 + acc_tolerance_));
}

template <class T>
float SimulatedAnnealingCore<T>::GetNormCost()
{
  return CalNormCost();
}

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
void SimulatedAnnealingCore<T>::GetMacros(std::vector<T>& macros) const
{
  macros =  macros_;
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
void SimulatedAnnealingCore<T>::Shrink() 
{
  return; // this function will be redefined in the derived class
}

template <class T>
void SimulatedAnnealingCore<T>::FillDeadSpace() 
{
  return; // this function will be redefined in the derived class
}


template <class T>
void SimulatedAnnealingCore<T>::CalOutlinePenalty() 
{
  const float max_width = std::max(outline_width_, width_);
  const float max_height = std::max(outline_height_, height_);
  outline_penalty_ = max_width * max_height - outline_width_ * outline_height_;
}
 

template <class T>
void SimulatedAnnealingCore<T>::CalWirelength() 
{
  // Initialization
  wirelength_ = 0.0;
  if (wirelength_weight_ <= 0.0)
    return;
  
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
  if (fence_weight_ <= 0.0)
    return;
  
  for (const auto& [id , bbox] : fences_) {
    const float lx = macros_[id].GetX();
    const float ly = macros_[id].GetY();
    const float ux = lx + macros_[id].GetWidth();
    const float uy = ly + macros_[id].GetHeight();
    const float width  = std::max(ux, bbox.xMax()) - std::min(lx, bbox.xMin()) 
                         - (bbox.xMax() - bbox.xMin());
    const float height = std::max(uy, bbox.yMax()) - std::min(ly, bbox.yMin()) 
                         - (bbox.yMax() - bbox.yMin());
    fence_penalty_ += width * width + height * height;
  } 
}

template <class T>
void SimulatedAnnealingCore<T>::CalGuidancePenalty() 
{
  // Initialization
  guidance_penalty_ = 0.0;
  if (guidance_weight_ <= 0.0)
    return;

  for (const auto& [id , bbox] : guides_) {
    const float macro_lx = macros_[id].GetX();
    const float macro_ly = macros_[id].GetY();
    const float macro_ux = macro_lx + macros_[id].GetWidth();
    const float macro_uy = macro_ly + macros_[id].GetHeight();
    // center to center distance
    const float width  = (macro_ux - macro_lx) + (bbox.xMax() - bbox.xMin());
    const float height = (macro_uy - macro_ly) + (bbox.yMax() - bbox.yMin());
    float x_dist = std::abs((macro_ux + macro_lx) / 2.0 - (bbox.xMax() + bbox.xMin()) / 2.0);
    float y_dist = std::abs((macro_uy + macro_ly) / 2.0 - (bbox.yMax() + bbox.yMin()) / 2.0);
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
    // add the continue syntax to handle fixed terminals
    if (macros_[b].GetWidth() <= 0 || macros_[b].GetHeight() <= 0)
      continue;
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
    const int b = pos_seq[i]; // macro_id
    // add continue syntax to handle fixed terminals
    if (macros_[b].GetHeight() <= 0 || macros_[b].GetWidth() <= 0.0)
      continue;
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
  notch_weight_ = 0.0;
  // const for restart
  int num_restart = 1;
  const int max_num_restart = 2;
  const int shrink_freq = int(max_num_step_ * shrink_freq_); 
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
      t = init_T_ / (step * c_ * k_);
    else
      t = init_T_ / (step);
    // increase step
    step++;
    // check if restart condition
    if ((num_restart <= max_num_restart) && 
        (step == std::floor(max_num_step_ / max_num_restart)  && 
        (outline_penalty_ > 0.0))) {
      Shrink();
      PackFloorplan();
      CalPenalty();
      pre_cost = CalNormCost();
      num_restart++;
      step = 1;
      t = init_T_;
    } // end if
    // only consider the last step to optimize notch weight
    if (step == max_num_step_) {
      notch_weight_ = original_notch_weight_;
      PackFloorplan();
      CalPenalty();
      pre_cost = CalNormCost();
    }
    /*
    if (step % shrink_freq == 0) {
      Shrink();
      PackFloorplan();
      CalPenalty();
      pre_cost = CalNormCost();
    }
    */
  } // end while
  // update the final results
  PackFloorplan();
  //FillDeadSpace();
  CalPenalty();

  std::ofstream file;
  file.open("floorplan.txt");
  for (auto& macro : macros_)
    file << macro.GetName() << "    "
         << macro.GetX()    << "    "
         << macro.GetY()    << "    "
         << macro.GetWidth() << "   "
         << macro.GetHeight() << std::endl;
  file.close();
}

template class SimulatedAnnealingCore<SoftMacro>;
template class SimulatedAnnealingCore<HardMacro>;


}  // namespace mpl
