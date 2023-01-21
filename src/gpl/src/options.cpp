///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018-2020, The Regents of the University of California
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

#include "gpl/Replace.h"

namespace gpl {

bool ReplaceOptions::getIncremental() const
{
  return incremental_;
}
bool ReplaceOptions::getDoNesterovPlace() const
{
  return do_nesterov_place_;
}

bool ReplaceOptions::getForceCpu() const
{
  return force_cpu_;
}

int ReplaceOptions::getInitialPlaceMaxIter() const
{
  return initialPlaceMaxIter_;
}

int ReplaceOptions::getInitialPlaceMinDiffLength() const
{
  return initialPlaceMinDiffLength_;
}

int ReplaceOptions::getInitialPlaceMaxSolverIter() const
{
  return initialPlaceMaxSolverIter_;
}

int ReplaceOptions::getInitialPlaceMaxFanout() const
{
  return initialPlaceMaxFanout_;
}

float ReplaceOptions::getInitialPlaceNetWeightScale() const
{
  return initialPlaceNetWeightScale_;
}

int ReplaceOptions::getNesterovPlaceMaxIter() const
{
  return nesterovPlaceMaxIter_;
}

int ReplaceOptions::getBinGridCntX() const
{
  return binGridCntX_;
}

int ReplaceOptions::getBinGridCntY() const
{
  return binGridCntY_;
}

float ReplaceOptions::getTargetDensity() const
{
  return density_;
}

bool ReplaceOptions::getUniformTargetDensityMode() const
{
  return uniformTargetDensityMode_;
}

float ReplaceOptions::getTargetOverflow() const
{
  return overflow_;
}

float ReplaceOptions::getInitDensityPenalityFactor() const
{
  return initDensityPenalityFactor_;
}

float ReplaceOptions::getInitWireLengthCoef() const
{
  return initWireLengthCoef_;
}

float ReplaceOptions::getMinPhiCoef() const
{
  return minPhiCoef_;
}

float ReplaceOptions::getMaxPhiCoef() const
{
  return maxPhiCoef_;
}

int ReplaceOptions::getPadLeft() const
{
  return padLeft_;
}

int ReplaceOptions::getPadRight() const
{
  return padRight_;
}

float ReplaceOptions::getRoutabilityCheckOverflow() const
{
  return routabilityCheckOverflow_;
}

float ReplaceOptions::getRoutabilityMaxDensity() const
{
  return routabilityMaxDensity_;
}

int ReplaceOptions::getRoutabilityMaxBloatIter() const
{
  return routabilityMaxBloatIter_;
}

int ReplaceOptions::getRoutabilityMaxInflationIter() const
{
  return routabilityMaxInflationIter_;
}

float ReplaceOptions::getRoutabilityTargetRcMetric() const
{
  return routabilityTargetRcMetric_;
}

float ReplaceOptions::getRoutabilityInflationRatioCoef() const
{
  return routabilityInflationRatioCoef_;
}

float ReplaceOptions::getRoutabilityMaxInflationRatio() const
{
  return routabilityMaxInflationRatio_;
}

float ReplaceOptions::getRoutabilityRcK1() const
{
  return routabilityRcK1_;
}

float ReplaceOptions::getRoutabilityRcK2() const
{
  return routabilityRcK2_;
}

float ReplaceOptions::getRoutabilityRcK3() const
{
  return routabilityRcK3_;
}

float ReplaceOptions::getRoutabilityRcK4() const
{
  return routabilityRcK4_;
}

float ReplaceOptions::getReferenceHpwl() const
{
  return referenceHpwl_;
}

const std::vector<int>& ReplaceOptions::getTimingNetWeightOverflows() const
{
  return timingNetWeightOverflows_;
}

float ReplaceOptions::getTimingNetWeightMax() const
{
  return timingNetWeightMax_;
}

bool ReplaceOptions::getGuiDebug() const
{
  return gui_debug_ ;
}

int ReplaceOptions::getGuiDebugPauseIterations() const
{
  return gui_debug_pause_iterations_;
}

int ReplaceOptions::getGuiDebugUpdateIterations() const
{
  return gui_debug_update_iterations_;
}

int ReplaceOptions::getGuiDebugDrawBins() const
{
  return gui_debug_draw_bins_;
}

int ReplaceOptions::getGuiDebugInitial() const
{
  return gui_debug_initial_;
}

odb::dbInst* ReplaceOptions::getGuiDebugInst() const
{
  return gui_debug_inst_;
}

bool ReplaceOptions::getSkipIoMode() const
{
  return skipIoMode_;
}

bool ReplaceOptions::getTimingDrivenMode() const
{
  return timingDrivenMode_;
}

bool ReplaceOptions::getRoutabilityDrivenMode() const
{
  return routabilityDrivenMode_;
}

// Setters

void ReplaceOptions::setIncremental(bool value)
{
  incremental_ = value;
}

void ReplaceOptions::setDoNesterovPlace(bool value)
{
  do_nesterov_place_ = value;
}

void ReplaceOptions::setForceCpu(bool value)
{
  force_cpu_ = value;
}

void ReplaceOptions::setInitialPlaceMaxIter(int iter)
{
  initialPlaceMaxIter_ = iter;
}

void ReplaceOptions::setInitialPlaceMinDiffLength(int length)
{
  initialPlaceMinDiffLength_ = length;
}

void ReplaceOptions::setInitialPlaceMaxSolverIter(int iter)
{
  initialPlaceMaxSolverIter_ = iter;
}

void ReplaceOptions::setInitialPlaceMaxFanout(int fanout)
{
  initialPlaceMaxFanout_ = fanout;
}

void ReplaceOptions::setInitialPlaceNetWeightScale(float scale)
{
  initialPlaceNetWeightScale_ = scale;
}

void ReplaceOptions::setNesterovPlaceMaxIter(int iter)
{
  nesterovPlaceMaxIter_ = iter;
}

void ReplaceOptions::setBinGridCntX(int binGridCntX)
{
  binGridCntX_ = binGridCntX;
}

void ReplaceOptions::setBinGridCntY(int binGridCntY)
{
  binGridCntY_ = binGridCntY;
}

void ReplaceOptions::setTargetDensity(float density)
{
  density_ = density;
}

void ReplaceOptions::setUniformTargetDensityMode(bool mode)
{
  uniformTargetDensityMode_ = mode;
}

void ReplaceOptions::setTargetOverflow(float overflow)
{
  overflow_ = overflow;
}

void ReplaceOptions::setInitDensityPenalityFactor(float penaltyFactor)
{
  initDensityPenalityFactor_ = penaltyFactor;
}

void ReplaceOptions::setInitWireLengthCoef(float coef)
{
  initWireLengthCoef_ = coef;
}

void ReplaceOptions::setMinPhiCoef(float minPhiCoef)
{
  minPhiCoef_ = minPhiCoef;
}

void ReplaceOptions::setMaxPhiCoef(float maxPhiCoef)
{
  maxPhiCoef_ = maxPhiCoef;
}

void ReplaceOptions::setPadLeft(int pad)
{
  padLeft_ = pad;
}

void ReplaceOptions::setPadRight(int pad)
{
  padRight_ = pad;
}

void ReplaceOptions::setRoutabilityCheckOverflow(float overflow)
{
  routabilityCheckOverflow_ = overflow;
}

void ReplaceOptions::setRoutabilityMaxDensity(float density)
{
  routabilityMaxDensity_ = density;
}

void ReplaceOptions::setRoutabilityMaxBloatIter(int iter)
{
  routabilityMaxBloatIter_ = iter;
}

void ReplaceOptions::setRoutabilityMaxInflationIter(int iter)
{
  routabilityMaxInflationIter_ = iter;
}

void ReplaceOptions::setRoutabilityTargetRcMetric(float rc)
{
  routabilityTargetRcMetric_ = rc;
}

void ReplaceOptions::setRoutabilityInflationRatioCoef(float coef)
{
  routabilityInflationRatioCoef_ = coef;
}

void ReplaceOptions::setRoutabilityMaxInflationRatio(float ratio)
{
  routabilityMaxInflationRatio_ = ratio;
}

void ReplaceOptions::setRoutabilityRcCoefficients(float k1,
                                                  float k2,
                                                  float k3,
                                                  float k4)
{
  routabilityRcK1_ = k1;
  routabilityRcK2_ = k2;
  routabilityRcK3_ = k3;
  routabilityRcK4_ = k4;
}

void ReplaceOptions::setReferenceHpwl(float refHpwl)
{
  referenceHpwl_ = refHpwl;
}

void ReplaceOptions::addTimingNetWeightOverflow(int overflow)
{
  if (hasDefaultTimingNetWeightOverflows_) {
    hasDefaultTimingNetWeightOverflows_ = false;
    timingNetWeightOverflows_.clear(); // remove the defaults
  }
  timingNetWeightOverflows_.push_back(overflow);
}

void ReplaceOptions::setTimingNetWeightMax(float max)
{
  timingNetWeightMax_ = max;
}

void ReplaceOptions::setDebug(int pause_iterations,
                              int update_iterations,
                              bool draw_bins,
                              bool initial,
                              odb::dbInst* inst)
{
  gui_debug_ = true;
  gui_debug_pause_iterations_ = pause_iterations;
  gui_debug_update_iterations_ = update_iterations;
  gui_debug_draw_bins_ = draw_bins;
  gui_debug_initial_ = initial;
  gui_debug_inst_ = inst;
}

void ReplaceOptions::setSkipIoMode(bool mode)
{
  skipIoMode_ = mode;
  if (mode) {
    initialPlaceMaxIter_ = 0;
    timingDrivenMode_ = false;
    routabilityDrivenMode_ = false;
  }
}

void ReplaceOptions::setTimingDrivenMode(bool mode)
{
  timingDrivenMode_ = mode;
}

void ReplaceOptions::setRoutabilityDrivenMode(bool mode)
{
  routabilityDrivenMode_ = mode;
}

}  // namespace gpl
