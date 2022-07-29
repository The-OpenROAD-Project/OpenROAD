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

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <string>
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

// Class SimulatedAnnealingCore is a base class
// It will have two derived classes:
// 1) SACoreHardMacro : SA for hard macros.  It will be called by ShapeEngine and PinAlignEngine
// 2) SACoreSoftMacro : SA for soft macros.  It will be called by MacroPlaceEngine
template <class T> 
class SimulatedAnnealingCore {
  public:
    SimulatedAnnealingCore() {   };
    SimulatedAnnealingCore(float outline_width, float outline_height, // boundary constraints
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
                    int k, int c, unsigned seed = 0);

    void SetNets(const std::vector<BundledNet>& nets);
    // Fence corresponds to each macro (macro_id, fence)
    void SetFences(const std::map<int, Rect>& fences);
    // Guidance corresponds to each macro (macro_id, guide)
    void SetGuides(const std::map<int, Rect>& guides);
    

    bool IsValid() const;
    float GetNormCost(); // This is not a const function

    float GetWidth() const;
    float GetHeight() const;
    float GetOutlinePenalty() const;
    float GetNormOutlinePenalty() const;
    float GetWirelength() const;
    float GetNormWirelength() const;
    float GetGuidancePenalty() const;
    float GetNormGuidancePenalty() const;
    float GetFencePenalty() const;
    float GetNormFencePenalty() const;
    void GetMacros(std::vector<T>& macros) const;

    // Initialize the SA worker
    virtual void Initialize(); 
    // Run FastSA algorithm
    void FastSA();
    virtual void FillDeadSpace();


  protected:
    virtual float CalNormCost();
    virtual void CalPenalty();
    void CalOutlinePenalty();
    void CalWirelength();
    void CalGuidancePenalty();
    void CalFencePenalty();

    // operations
    void PackFloorplan();
    virtual void Perturb();
    virtual void Restore();
    // actions used
    void SingleSeqSwap(bool pos);
    void DoubleSeqSwap();
    void ExchangeMacros();
    
    virtual void Shrink();  // Shrink the size of macros

    // utilities
    float CalAverage(std::vector<float>& value_list);

    /////////////////////////////////////////////
    // private member variables
    /////////////////////////////////////////////
    // boundary constraints
    float outline_width_ = 0.0;
    float outline_height_ = 0.0;
   
    // nets, fences, guides, blockages
    std::vector<BundledNet> nets_;
    std::map<int, Rect> fences_;
    std::map<int, Rect> guides_;

    // weight for different penalty
    float area_weight_       = 0.0;
    float outline_weight_    = 0.0; 
    float wirelength_weight_ = 0.0;
    float guidance_weight_   = 0.0;
    float fence_weight_      = 0.0;
    
    float original_notch_weight_ = 0.0;
    float notch_weight_ = 0.0;


    // Fast SA hyperparameter
    float init_prob_         = 0.0;
    float init_T_            = 1.0;
    int max_num_step_          = 0;
    int num_perturb_per_step_  = 0;
    // if step < k_, T = init_T_ / (c_ * step_);
    // else T = init_T_ / step
    int k_ = 0; 
    int c_ = 0;

    // shrink_factor for dynamic weight
    float shrink_factor_ = 0.8;
    float shrink_freq_ = 0.1;
    
    // seed for reproduciabilty
    std::mt19937 generator_;
    std::uniform_real_distribution<float> distribution_;

    // current solution
    std::vector<int> pos_seq_;
    std::vector<int> neg_seq_;
    std::vector<T>   macros_; // here the macros can be HardMacro or SoftMacro
    
    // previous solution
    std::vector<int> pre_pos_seq_;
    std::vector<int> pre_neg_seq_;
    std::vector<T>   pre_macros_; // here the macros can be HardMacro or SoftMacro
    int macro_id_ = -1;  // the macro changed in the perturb
    int action_id_ = -1; // the action_id of current step


    // we define accuracy to determine whether the floorplan is valid
    // because the error introduced by the type conversion
    float acc_tolerance_ = 0.01;

    // metrics 
    float width_ = 0.0;
    float height_ = 0.0;
    float pre_width_ = 0.0;
    float pre_height_ = 0.0;

    float outline_penalty_  = 0.0;
    float wirelength_       = 0.0;
    float guidance_penalty_ = 0.0;
    float fence_penalty_    = 0.0;
  
    float pre_outline_penalty_  = 0.0;
    float pre_wirelength_       = 0.0;
    float pre_guidance_penalty_ = 0.0;
    float pre_fence_penalty_    = 0.0;
  
    float norm_outline_penalty_  = 0.0;
    float norm_wirelength_       = 0.0;
    float norm_guidance_penalty_ = 0.0;
    float norm_fence_penalty_    = 0.0;
    float norm_area_penalty_     = 0.0;

    // probability of each action
    float pos_swap_prob_    = 0.0;
    float neg_swap_prob_    = 0.0;
    float double_swap_prob_ = 0.0;
    float exchange_prob_    = 0.0;
};

// SACore wrapper function
// T can be SACoreHardMacro or SACoreSoftMacro
template <class T> 
void RunSA(T* sa_core) 
{
  sa_core->Initialize();
  sa_core->FastSA();
}



} // namespace mpl



