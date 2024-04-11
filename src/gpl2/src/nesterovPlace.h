///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018-2023, The Regents of the University of California
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

#include <memory>
#include <string>
#include <vector>

#include "placerBase.h"

namespace utl {
class Logger;
}

namespace odb {
class dbInst;
}

namespace gpl2 {

class PlacerBase;
class PlacerBaseCommon;
class GpuRouteBase;
class GpuTimingBase;

// Compared to the original implementation, 
// We do not have cutFillerCooridinates.
// And we do not have the densityPenaltyStor_.
// Because either the function or the variable is not used.
class NesterovPlace
{
  public:
    NesterovPlace();
    NesterovPlace(const NesterovPlaceVars& npVars,
                  bool DREAMPlaceFlag,
                  const std::shared_ptr<PlacerBaseCommon>& pbc,
                  std::vector<std::shared_ptr<PlacerBase> >& pbVec,
                  std::shared_ptr<GpuRouteBase> rb,
                  std::shared_ptr<GpuTimingBase> tb,
                  utl::Logger* log);
  
  ~NesterovPlace();

  // The main function of GpuNesterovPlace
  // return iteration count
  bool doNesterovPlace(int start_iter = 0);
  void updateWireLengthCoef(float overflow);
  void updateNextIter(const int iter);
  void updateDb(); // update database (OpenDB)

  float getWireLengthCoefX() const { return wireLengthCoefX_; }
  float getWireLengthCoefY() const { return wireLengthCoefY_; }

  // set the target overflow
  void setTargetOverflow(float targetOverflow) {
    npVars_.targetOverflow = targetOverflow;
  }

  // set the max iteration of nesterov optimizer
  void setMaxIters(int maxNesterovIter) {
    npVars_.maxNesterovIter = maxNesterovIter;
  }

  // gradient related functions
  void updatePrevGradient(std::shared_ptr<PlacerBase> pb);
  void updateCurGradient(std::shared_ptr<PlacerBase> pb);
  void updateNextGradient(std::shared_ptr<PlacerBase> pb);
  void enableDREAMPlaceFlag();

 private:
  std::shared_ptr<PlacerBaseCommon> pbc_;
  std::vector<std::shared_ptr<PlacerBase> > pbVec_;
  utl::Logger* log_;
  std::shared_ptr<GpuRouteBase> rb_;
  std::shared_ptr<GpuTimingBase> tb_;
  NesterovPlaceVars npVars_;

  bool DREAMPlaceFlag_ = false;

  // Without macro area scaling, the bin density at the the macro blocks becomes higher
  // than the target density. As a resul, the density force pushes the cells away from macros,
  // inducing under-filled whitespace around macros and wirelenth overhead
  // Basically, we should use the scaled value instead of the unscaled ones.
  // The unscaled ones is only for debugging and analysis purpose.
  float totalSumOverflow_;
  float averageOverflow_;

  // The experiments show that quality and convergence are sensitive to the smoothing parameter.
  // Our approach relaxes the smoothing parameter at early iterations, such that more cells are encouraged
  // to be globally moved out of the high-density regions. At later stages, when local movement dominates,
  // the parameter is reduced to make the smoothed wirelength W approach HPWL.
  // base_wcof
  float baseWireLengthCoef_;
  // wlen_cof
  float wireLengthCoefX_;
  float wireLengthCoefY_;

  // half-parameter-wire-length
  int64_t prevHpwl_;

  float isDiverged_;
  float isRoutabilityNeed_;

  std::string divergeMsg_;
  int divergeCode_;

  // check how many times of recursive calls when updating gradients 
  // each recursive call will have smaller wirelength coefficient  
  // This only happens when wirelengthGradSum is zero
  int recursionCntWlCoef_;
  // Check how many times the rerunning Nesterov::init() is called
  
  // This only happens when steplength = 0
  // This parameter will :  npVars_.initialPrevCoordiUpdateCoef *= 10;
  int recursionCntInitSLPCoef_;

  bool debugFlag = false;

  void init();
  void reset();
};
}  // namespace gpl
