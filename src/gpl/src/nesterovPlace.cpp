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

// Debug controls: npinit, updateGrad, np, updateNextIter

#include "nesterovPlace.h"

#include <iomanip>
#include <iostream>
#include <sstream>

#include "graphics.h"
#include "nesterovBase.h"
#include "odb/db.h"
#include "placerBase.h"
#include "routeBase.h"
#include "timingBase.h"
#include "utl/Logger.h"

namespace gpl {
using namespace std;

using namespace std;
using utl::GPL;

NesterovPlace::NesterovPlace()
    : pbc_(nullptr),
      nbc_(nullptr),
      log_(nullptr),
      rb_(nullptr),
      tb_(nullptr),
      npVars_(),
      wireLengthGradSum_(0),
      densityGradSum_(0),
      densityPenalty_(0),
      baseWireLengthCoef_(0),
      wireLengthCoefX_(0),
      wireLengthCoefY_(0),
      sumOverflow_(0),
      sumOverflowUnscaled_(0),
      prevHpwl_(0),
      isDiverged_(false),
      isRoutabilityNeed_(true),
      divergeCode_(0),
      recursionCntWlCoef_(0),
      recursionCntInitSLPCoef_(0)
{
}

NesterovPlace::NesterovPlace(const NesterovPlaceVars& npVars,
                              std::shared_ptr<PlacerBaseCommon> pbc,
                              std::shared_ptr<NesterovBaseCommon> nbc,
                              std::vector<std::shared_ptr<PlacerBase>>& pbVec,
                              std::vector<std::shared_ptr<NesterovBase>>& nbVec,
                              std::shared_ptr<RouteBase> rb,
                              std::shared_ptr<TimingBase> tb,
                              utl::Logger* log)
    : NesterovPlace()
{
  npVars_ = npVars;
  pbc_ = pbc;
  nbc_ = nbc;
  pbVec_ = pbVec;
  nbVec_ = nbVec;
  rb_ = rb;
  tb_ = tb;
  log_ = log;

  // TODO: FIX THIS
  // if (npVars.debug && Graphics::guiActive()) {
  //   graphics_ = make_unique<Graphics>(
  //       log_, this, pb, nb, npVars_.debug_draw_bins, npVars.debug_inst);
  // }
  init();
}

NesterovPlace::~NesterovPlace()
{
  reset();
}

void NesterovPlace::init()
{
  // foreach nesterovbase call init
  float totalSumOverflow = 0;
  float totalBaseWireLengthCoeff = 0;
  for(auto& nb : nbVec_){
    nb->setNpVars(npVars_);
    nb->initDensity1();
    totalSumOverflow += nb->getSumOverflow();
    totalBaseWireLengthCoeff += nb->getBaseWireLengthCoef();
  }

  baseWireLengthCoef_ = totalBaseWireLengthCoeff / nbVec_.size();
  
  updateWireLengthCoef(totalSumOverflow / nbVec_.size());

  debugPrint(
      log_, GPL, "npinit", 1, "npinit: WireLengthCoef: {:g}", wireLengthCoefX_);

  printf("npinit: WireLengthCoefX: %f, WireLengthCoefY: %f\n", wireLengthCoefX_, wireLengthCoefY_);

  nbc_->updateWireLengthForceWA(wireLengthCoefX_, wireLengthCoefY_);

  for(auto& nb : nbVec_){

    // fill in curSLPSumGrads_, curSLPWireLengthGrads_, curSLPDensityGrads_
    nb->updateCurGradient(wireLengthCoefX_, wireLengthCoefY_);

    // approximately fill in
    // prevSLPCoordi_ to calculate lc vars
    nb->updateInitialPrevSLPCoordi();

    // bin, FFT, wlen update with prevSLPCoordi.
    nb->updateDensityCenterPrevSLP();
    nb->updateDensityForceBin();
  }

  nbc_->updateWireLengthForceWA(wireLengthCoefX_, wireLengthCoefY_);


  for(auto& nb : nbVec_){
    // update previSumGrads_, prevSLPWireLengthGrads_, prevSLPDensityGrads_
    nb->updatePrevGradient(wireLengthCoefX_, wireLengthCoefY_);
  }


  for(auto& nb : nbVec_) {
    auto stepL = nb->initDensity2();
    if ((isnan(stepL) || isinf(stepL))
      && recursionCntInitSLPCoef_ < npVars_.maxRecursionInitSLPCoef) {
      npVars_.initialPrevCoordiUpdateCoef *= 10;
      debugPrint(log_,
                GPL,
                "npinit",
                1,
                "steplength = 0 detected. Rerunning Nesterov::init() "
                "with initPrevSLPCoef {:g}",
                npVars_.initialPrevCoordiUpdateCoef);
      recursionCntInitSLPCoef_++;
      init();
      break;
    }

    if (isnan(stepL) || isinf(stepL)) {
      log_->error(GPL,
                  304,
                  "RePlAce diverged at initial iteration with steplength being {}. "
                  "Re-run with a smaller init_density_penalty value.", stepL);
    }
  }
}

// clear reset
void NesterovPlace::reset()
{
  npVars_.reset();
  log_ = nullptr;

  densityPenaltyStor_.clear();


  densityPenaltyStor_.shrink_to_fit();

  wireLengthGradSum_ = 0;
  densityGradSum_ = 0;
  densityPenalty_ = 0;
  baseWireLengthCoef_ = 0;
  wireLengthCoefX_ = wireLengthCoefY_ = 0;
  prevHpwl_ = 0;
  isDiverged_ = false;
  isRoutabilityNeed_ = true;

  divergeMsg_ = "";
  divergeCode_ = 0;

  recursionCntWlCoef_ = 0;
  recursionCntInitSLPCoef_ = 0;
}


int NesterovPlace::doNesterovPlace(int start_iter)
{
  // if replace diverged in init() function,
  // replace must be skipped.
  if (isDiverged_) {
    log_->error(GPL, divergeCode_, divergeMsg_);
    return 0;
  }

  if (graphics_) {
    graphics_->cellPlot(true);
  }

  // backTracking variable.
  float curA = 1.0;

  // divergence detection
  // float minSumOverflow = 1e30;
  // float hpwlWithMinSumOverflow = 1e30;

  // dynamic adjustment of max_phi_coef
  // bool isMaxPhiCoefChanged = false;

  // snapshot saving detection
  // bool isSnapshotSaved = false;

  // snapshot info
  // vector<FloatPoint> snapshotCoordi;
  // vector<FloatPoint> snapshotSLPCoordi;
  // vector<FloatPoint> snapshotSLPSumGrads;
  // float snapshotA = 0;
  // float snapshotDensityPenalty = 0;
  // float snapshotStepLength = 0;
  // float snapshotWlCoefX = 0, snapshotWlCoefY = 0;

  // bool isDivergeTriedRevert = false;

  // Core Nesterov Loop
  int iter = start_iter;
  for (; iter < npVars_.maxNesterovIter; iter++) {
    debugPrint(log_, GPL, "np", 1, "Iter: {}", iter + 1);

    float prevA = curA;

    // here, prevA is a_(k), curA is a_(k+1)
    // See, the ePlace-MS paper's Algorithm 1
    //
    curA = (1.0 + sqrt(4.0 * prevA * prevA + 1.0)) * 0.5;

    // coeff is (a_k - 1) / ( a_(k+1) ) in paper.
    float coeff = (prevA - 1.0) / curA;

    debugPrint(log_, GPL, "np", 1, "PreviousA: {:g}", prevA);
    debugPrint(log_, GPL, "np", 1, "CurrentA: {:g}", curA);
    debugPrint(log_, GPL, "np", 1, "Coefficient: {:g}", coeff);

    // Back-Tracking loop
    int numBackTrak = 0;
    for (numBackTrak = 0; numBackTrak < npVars_.maxBackTrack; numBackTrak++) {
      
      // fill in nextCoordinates with given stepLength_
      for(auto& nb : nbVec_){
        nb->nesterovUpdateCoordinates(coeff);
      }
    
      nbc_->updateWireLengthForceWA(wireLengthCoefX_, wireLengthCoefY_);

      int numDiverge = 0;
      for(auto& nb : nbVec_){
        nb->updateNextGradient(wireLengthCoefX_, wireLengthCoefY_);
        numDiverge += nb->isDiverged();
      }

      // NaN or inf is detected in WireLength/Density Coef
      if (numDiverge > 0) {
        isDiverged_ = true;
        divergeMsg_ = "RePlAce diverged at wire/density gradient Sum.";
        divergeCode_ = 306;
        break;
      }

      int stepLengthLimitOK = 0;
      numDiverge = 0;
      for(auto& nb : nbVec_){
        stepLengthLimitOK += nb->nesterovUpdateStepLength();
        numDiverge += nb->isDiverged();
      }

      if (numDiverge > 0) {
        isDiverged_ = true;
        divergeMsg_ = "RePlAce diverged at newStepLength.";
        divergeCode_ = 305;
        break;
      }

      if(stepLengthLimitOK != nbVec_.size()){
        break;
      }
    }

    debugPrint(log_, GPL, "np", 1, "NumBackTrak: {}", numBackTrak + 1);

    // Adjust Phi dynamically for larger designs
    for(auto& nb : nbVec_){
      nb->nesterovAdjustPhi();
    }

    if (npVars_.maxBackTrack == numBackTrak) {
      debugPrint(log_,
                 GPL,
                 "np",
                 1,
                 "Backtracking limit reached so a small step will be taken");
    }

    if (isDiverged_) {
      break;
    }

    updateNextIter(iter);

    // TODO: FIX THIS
    // // For JPEG Saving
    // // debug

    // if (graphics_) {
    //   bool update
    //       = (iter == 0 || (iter + 1) % npVars_.debug_update_iterations == 0);
    //   if (update) {
    //     bool pause
    //         = (iter == 0 || (iter + 1) % npVars_.debug_pause_iterations == 0);
    //     graphics_->cellPlot(pause);
    //   }
    // }


    // timing driven feature
    // do reweight on timing-critical nets.
    if (npVars_.timingDrivenMode
        && tb_->isTimingNetWeightOverflow(sumOverflow_)) {
      // update db's instance location from current density coordinates
      updateDb();

      // Call resizer's estimateRC API to fill in PEX using placed locations,
      // Call sta's API to extract worst timing paths,
      // and update GNet's weights from worst timing paths.
      //
      // See timingBase.cpp in detail
      bool shouldTdProceed = tb_->updateGNetWeights(sumOverflow_);

      // problem occured
      // escape timing driven later
      if (!shouldTdProceed) {
        npVars_.timingDrivenMode = false;
      }
    }

    // diverge detection on
    // large max_phi_cof value + large design
    //
    // 1) happen overflow < 20%
    // 2) Hpwl is growing
    
    int numDiverge = 0;
    for(auto& nb : nbVec_){
      numDiverge += nb->checkDivergence();
    }

    if (numDiverge > 0) {
      divergeMsg_ = "RePlAce divergence detected. ";
      divergeMsg_ += "Re-run with a smaller max_phi_cof value.";
      divergeCode_ = 307;
      isDiverged_ = true;

      // // TODO: find a way to revert back 
      // // revert back to the original rb solutions
      // // one more opportunity
      // if (!isDivergeTriedRevert && rb_->numCall() >= 1) {
      //   // get back to the working rc size
      //   rb_->revertGCellSizeToMinRc();

      //   // revert back the current density penality
      //   curCoordi_ = snapshotCoordi;
      //   curSLPCoordi_ = snapshotSLPCoordi;
      //   curSLPSumGrads_ = snapshotSLPSumGrads;
      //   curA = snapshotA;
      //   densityPenalty_ = snapshotDensityPenalty;
      //   stepLength_ = snapshotStepLength;
      //   wireLengthCoefX_ = snapshotWlCoefX;
      //   wireLengthCoefY_ = snapshotWlCoefY;

      //   nb_->updateGCellDensityCenterLocation(curCoordi_);
      //   nb_->updateDensityForceBin();
      //   nb_->updateWireLengthForceWA(wireLengthCoefX_, wireLengthCoefY_);

      //   isDiverged_ = false;
      //   divergeCode_ = 0;
      //   divergeMsg_ = "";
      //   isDivergeTriedRevert = true;

      //   // turn off the RD forcely
      //   isRoutabilityNeed_ = false;
      // } else {
      //   // no way to revert
      //   break;
      // }

      // TODO: remove when fixed above!
      break;
    }


    // TODO: 
    // problem here is when to snapshot wirelength coeffecient
    // since we snapshot at different times depending on each region's overflow 
    // it's not straight forward which one to use 
    // for now will be using the last one
    for(auto& nb : nbVec_){
      nb->snapshot(wireLengthCoefX_, wireLengthCoefY_);
    }


    // // TODO: find a way to revert back for wire length coeffecients
    // // check routability using GR
    // if (npVars_.routabilityDrivenMode && isRoutabilityNeed_
    //     && npVars_.routabilityCheckOverflow >= sumOverflowUnscaled_) {
    //   // recover the densityPenalty values
    //   // if further routability-driven is needed
    //   std::pair<bool, bool> result = rb_->routability();
    //   isRoutabilityNeed_ = result.first;
    //   bool isRevertInitNeeded = result.second;

    //   // if routability is needed
    //   if (isRoutabilityNeed_ || isRevertInitNeeded) {
    //     // cutFillerCoordinates();

    //     // revert back the current density penality
    //     curCoordi_ = snapshotCoordi;
    //     curSLPCoordi_ = snapshotSLPCoordi;
    //     curSLPSumGrads_ = snapshotSLPSumGrads;
    //     curA = snapshotA;
    //     densityPenalty_ = snapshotDensityPenalty;
    //     stepLength_ = snapshotStepLength;
    //     wireLengthCoefX_ = snapshotWlCoefX;
    //     wireLengthCoefY_ = snapshotWlCoefY;

    //     nb_->updateGCellDensityCenterLocation(curCoordi_);
    //     nb_->updateDensityForceBin();
    //     nb_->updateWireLengthForceWA(wireLengthCoefX_, wireLengthCoefY_);

    //     // reset the divergence detect conditions
    //     minSumOverflow = 1e30;
    //     hpwlWithMinSumOverflow = 1e30;
    //     log_->report("[NesterovSolve] Revert back to snapshot coordi");
    //   }
    // }

    // check each for converge and if all are converged then stop
    int numConverge = 0;
    for(auto& nb : nbVec_){
      numConverge += nb->checkConvergence();
    }

    if(numConverge == nbVec_.size()){
      log_->report("[NesterovSolve] Finished, all regions converged");
      break;
    }
  }
  // in all case including diverge,
  // db should be updated.
  updateDb();

  if (isDiverged_) {
    log_->error(GPL, divergeCode_, divergeMsg_);
  }

  // if (graphics_) {
  //   graphics_->status("End placement");
  //   graphics_->cellPlot(true);
  // }

  return iter;
}

void NesterovPlace::updateWireLengthCoef(float overflow)
{
  if (overflow > 1.0) {
    wireLengthCoefX_ = wireLengthCoefY_ = 0.1;
  } else if (overflow < 0.1) {
    wireLengthCoefX_ = wireLengthCoefY_ = 10.0;
  } else {
    wireLengthCoefX_ = wireLengthCoefY_
        = 1.0 / pow(10.0, (overflow - 0.1) * 20 / 9.0 - 1.0);
  }

  wireLengthCoefX_ *= baseWireLengthCoef_;
  wireLengthCoefY_ *= baseWireLengthCoef_;
  debugPrint(log_, GPL, "np", 1, "NewWireLengthCoef: {:g}", wireLengthCoefX_);
}


void NesterovPlace::updateNextIter(const int iter)
{
  
  float total_sum_overflow = 0;

  for(auto& nb : nbVec_){
    nb->updateNextIter(iter);
    total_sum_overflow += nb->getSumOverflow();
  }

  // For coefficient, using average regions' overflow
  updateWireLengthCoef(total_sum_overflow / nbVec_.size());


  // updateWireLengthCoef(sumOverflow_);

  // for routability densityPenalty recovery
  if (rb_->numCall() == 0) {
    densityPenaltyStor_.push_back(densityPenalty_);
  }
}



void NesterovPlace::updateDb()
{
  nbc_->updateDbGCells();
}

}  // namespace gpl
