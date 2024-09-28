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

#pragma once

#include <map>
#include <random>
#include <vector>

#include "Mpl2Observer.h"

namespace utl {
class Logger;
}

namespace mpl2 {

struct BundledNet;
struct Rect;
class Graphics;

struct SACoreWeights
{
  float area = 0.0f;
  float outline = 0.0f;
  float wirelength = 0.0f;
  float guidance = 0.0f;
  float fence = 0.0f;
};

// Class SimulatedAnnealingCore is a base class
// It will have two derived classes:
// 1) SACoreHardMacro : SA for hard macros.  It will be called by ShapeEngine
// and PinAlignEngine 2) SACoreSoftMacro : SA for soft macros.  It will be
// called by MacroPlaceEngine
template <class T>
class SimulatedAnnealingCore
{
 public:
  SimulatedAnnealingCore(
      const Rect& outline,           // boundary constraints
      const std::vector<T>& macros,  // macros (T = HardMacro or T = SoftMacro)
      // weight for different penalty
      float area_weight,
      float outline_weight,
      float wirelength_weight,
      float guidance_weight,
      float fence_weight,  // each blockage will be modeled by a macro
                           // with fences probability of each action
      float pos_swap_prob,
      float neg_swap_prob,
      float double_swap_prob,
      float exchange_prob,
      // Fast SA hyperparameter
      float init_prob,
      int max_num_step,
      int num_perturb_per_step,
      unsigned seed,
      Mpl2Observer* graphics,
      utl::Logger* logger);

  virtual ~SimulatedAnnealingCore() = default;

  void setNumberOfMacrosToPlace(int macros_to_place)
  {
    macros_to_place_ = macros_to_place;
  };
  void setCentralizationAttemptOn(bool centralization_on)
  {
    centralization_on_ = centralization_on;
  };
  bool centralizationWasReverted() { return centralization_was_reverted_; }

  void setNets(const std::vector<BundledNet>& nets);
  // Fence corresponds to each macro (macro_id, fence)
  void setFences(const std::map<int, Rect>& fences);
  // Guidance corresponds to each macro (macro_id, guide)
  void setGuides(const std::map<int, Rect>& guides);
  void setInitialSequencePair(const SequencePair& sequence_pair);

  bool isValid() const;
  bool isValid(const Rect& outline) const;
  void writeCostFile(const std::string& file_name) const;
  float getNormCost() const;
  float getWidth() const;
  float getHeight() const;
  float getOutlinePenalty() const;
  float getNormOutlinePenalty() const;
  float getWirelength() const;
  float getNormWirelength() const;
  float getGuidancePenalty() const;
  float getNormGuidancePenalty() const;
  float getFencePenalty() const;
  float getNormFencePenalty() const;
  void getMacros(std::vector<T>& macros) const;

  // Initialize the SA worker
  virtual void initialize() = 0;
  // Run FastSA algorithm
  void fastSA();
  virtual void fillDeadSpace() = 0;

 protected:
  void initSequencePair();
  void attemptCentralization(float pre_cost);
  void moveFloorplan(const std::pair<float, float>& offset);

  virtual float calNormCost() const = 0;
  virtual void calPenalty() = 0;
  void calOutlinePenalty();
  void calWirelength();
  void calGuidancePenalty();
  void calFencePenalty();

  // operations
  void packFloorplan();
  virtual void perturb() = 0;
  virtual void restore() = 0;
  // actions used
  void singleSeqSwap(bool pos);
  void doubleSeqSwap();
  void exchangeMacros();
  void generateRandomIndices(int& index1, int& index2);

  virtual void shrink() = 0;  // Shrink the size of macros

  // utilities
  static float calAverage(std::vector<float>& value_list);

  /////////////////////////////////////////////
  // private member variables
  /////////////////////////////////////////////
  // boundary constraints
  Rect outline_;

  // Number of macros that will actually be part of the sequence pair
  int macros_to_place_ = 0;

  // nets, fences, guides, blockages
  std::vector<BundledNet> nets_;
  std::map<int, Rect> fences_;
  std::map<int, Rect> guides_;

  // weight for different penalty
  float area_weight_ = 0.0;
  float outline_weight_ = 0.0;
  float wirelength_weight_ = 0.0;
  float guidance_weight_ = 0.0;
  float fence_weight_ = 0.0;

  float original_notch_weight_ = 0.0;
  float notch_weight_ = 0.0;

  // Fast SA hyperparameter
  float init_prob_ = 0.0;
  float init_temperature_ = 1.0;
  int max_num_step_ = 0;
  int num_perturb_per_step_ = 0;

  // shrink_factor for dynamic weight
  const float shrink_factor_ = 0.8;
  const float shrink_freq_ = 0.1;

  // seed for reproduciabilty
  std::mt19937 generator_;
  std::uniform_real_distribution<float> distribution_;

  // current solution
  std::vector<int> pos_seq_;
  std::vector<int> neg_seq_;
  std::vector<T> macros_;  // here the macros can be HardMacro or SoftMacro

  // previous solution
  std::vector<int> pre_pos_seq_;
  std::vector<int> pre_neg_seq_;
  std::vector<T> pre_macros_;  // here the macros can be HardMacro or SoftMacro
  int macro_id_ = -1;          // the macro changed in the perturb
  int action_id_ = -1;         // the action_id of current step

  // metrics
  float width_ = 0.0;
  float height_ = 0.0;
  float pre_width_ = 0.0;
  float pre_height_ = 0.0;

  float outline_penalty_ = 0.0;
  float wirelength_ = 0.0;
  float guidance_penalty_ = 0.0;
  float fence_penalty_ = 0.0;

  float pre_outline_penalty_ = 0.0;
  float pre_wirelength_ = 0.0;
  float pre_guidance_penalty_ = 0.0;
  float pre_fence_penalty_ = 0.0;

  float norm_outline_penalty_ = 0.0;
  float norm_wirelength_ = 0.0;
  float norm_guidance_penalty_ = 0.0;
  float norm_fence_penalty_ = 0.0;
  float norm_area_penalty_ = 0.0;

  // probability of each action
  float pos_swap_prob_ = 0.0;
  float neg_swap_prob_ = 0.0;
  float double_swap_prob_ = 0.0;
  float exchange_prob_ = 0.0;

  utl::Logger* logger_ = nullptr;
  Mpl2Observer* graphics_ = nullptr;

  std::vector<float> cost_list_;  // store the cost in the list
  std::vector<float> T_list_;     // store the temperature
  // we define accuracy to determine whether the floorplan is valid
  // because the error introduced by the type conversion
  static constexpr float acc_tolerance_ = 0.001;

  bool has_initial_sequence_pair_ = false;
  bool centralization_on_ = false;
  bool centralization_was_reverted_ = false;
};

// SACore wrapper function
// T can be SACoreHardMacro or SACoreSoftMacro
template <class T>
void runSA(T* sa_core)
{
  sa_core->initialize();
  sa_core->fastSA();
}

}  // namespace mpl2
