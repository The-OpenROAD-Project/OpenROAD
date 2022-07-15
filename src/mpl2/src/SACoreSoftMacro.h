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
#include "SimulatedAnnealingCore.h"

namespace mpl {

// Class SimulatedAnnealingCore is a base class
// It will have two derived classes:
// 1) SACoreHardMacro : SA for hard macros.  It will be called by ShapeEngine and PinAlignEngine
// 2) SACoreSoftMacro : SA for soft macros.  It will be called by MacroPlaceEngine
class SACoreSoftMacro : public SimulatedAnnealingCore<SoftMacro> {
  public:
    SACoreSoftMacro() {   };
    SACoreSoftMacro(float outline_width, float outline_height, // boundary constraints
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
                    // action prob
                    float pos_swap_prob, 
                    float neg_swap_prob,
                    float double_swap_prob, 
                    float exchange_prob, 
                    float resize_prob,
                    // Fast SA hyperparameter
                    float init_prob, int max_num_step, int num_perturb_per_step,
                    int k, int c, unsigned seed = 0);
    // accessors
    float GetBoundaryPenalty()     const;
    float GetNormBoundaryPenalty() const;
    float GetNotchPenalty()        const;
    float GetNormNotchPenalty()    const;
    
    // Initialize the SA worker
    void Initialize(); 

  private:
    float CalNormCost();
    void CalPenalty();

    void Perturb();
    void Restore();
    // actions used
    void Resize();

    // notch threshold
    float notch_h_th_;
    float notch_v_th_;
  
    // additional penalties
    float boundary_weight_   = 0.0;
    float notch_weight_      = 0.0;
    
    float boundary_penalty_ = 0.0;
    float notch_penalty_    = 0.0;
    
    float pre_boundary_penalty_ = 0.0;
    float pre_notch_penalty_    = 0.0;

    float norm_boundary_penalty_ = 0.0;
    float norm_notch_penalty_    = 0.0;

    void CalBoundaryPenalty();
    void AlignMacroClusters();
    void CalNotchPenalty();
   
    // action prob
    float resize_prob_ = 0.0;
};

} // namespace mpl


