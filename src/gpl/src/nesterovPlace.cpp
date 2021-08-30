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

#include "placerBase.h"
#include "nesterovBase.h"
#include "nesterovPlace.h"
#include "odb/db.h"
#include "routeBase.h"
#include "timingBase.h"
#include "utl/Logger.h"
#include <iostream>
#include <sstream>
#include <iomanip>
using namespace std;

#include "plot.h"
#include "graphics.h"

namespace gpl {

using utl::GPL;

static float
getDistance(const vector<FloatPoint>& a, const vector<FloatPoint>& b);

static float
getSecondNorm(const vector<FloatPoint>& a);

#ifdef ENABLE_CIMG_LIB
static std::string
getZeroFillStr(int iterNum);
#endif

NesterovPlaceVars::NesterovPlaceVars()
{
  reset();
}

void
NesterovPlaceVars::reset() {
  maxNesterovIter = 5000;
  maxBackTrack = 10;
  initDensityPenalty = 0.00008;
  initWireLengthCoef = 0.25;
  targetOverflow = 0.1;
  minPhiCoef = 0.95;
  maxPhiCoef = 1.05;
  minPreconditioner = 1.0;
  initialPrevCoordiUpdateCoef = 100;
  referenceHpwl = 446000000;
  routabilityCheckOverflow = 0.20;
  timingDrivenMode = true;
  routabilityDrivenMode = true;
  debug = false;
  debug_pause_iterations = 10;
  debug_update_iterations = 10;
  debug_draw_bins = true;
}

NesterovPlace::NesterovPlace() 
  : pb_(nullptr), nb_(nullptr), log_(nullptr), rb_(nullptr), tb_(nullptr), 
  npVars_(), 
  wireLengthGradSum_(0), 
  densityGradSum_(0),
  stepLength_(0),
  densityPenalty_(0),
  baseWireLengthCoef_(0), 
  wireLengthCoefX_(0), 
  wireLengthCoefY_(0),
  sumPhi_(0),
  sumOverflow_(0),
  prevHpwl_(0),
  isDiverged_(false),
  isRoutabilityNeed_(true),
  divergeCode_(0) {}

NesterovPlace::NesterovPlace(
    NesterovPlaceVars npVars,
    std::shared_ptr<PlacerBase> pb, 
    std::shared_ptr<NesterovBase> nb,
    std::shared_ptr<RouteBase> rb,
    std::shared_ptr<TimingBase> tb,
    utl::Logger* log) 
: NesterovPlace() {
  npVars_ = npVars;
  pb_ = pb;
  nb_ = nb;
  rb_ = rb;
  tb_ = tb;
  log_ = log;
  if (npVars.debug && Graphics::guiActive()) {
    graphics_ = make_unique<Graphics>(log_, this, pb, nb, npVars_.debug_draw_bins);
  }
  init();
}

NesterovPlace::~NesterovPlace() {
  reset();
}

#ifdef ENABLE_CIMG_LIB
static PlotEnv pe;
#endif

void NesterovPlace::init() {
  const int gCellSize = nb_->gCells().size();
  curSLPCoordi_.resize(gCellSize, FloatPoint());
  curSLPWireLengthGrads_.resize(gCellSize, FloatPoint());
  curSLPDensityGrads_.resize(gCellSize, FloatPoint());
  curSLPSumGrads_.resize(gCellSize, FloatPoint());

  nextSLPCoordi_.resize(gCellSize, FloatPoint());
  nextSLPWireLengthGrads_.resize(gCellSize, FloatPoint());
  nextSLPDensityGrads_.resize(gCellSize, FloatPoint());
  nextSLPSumGrads_.resize(gCellSize, FloatPoint());
  
  prevSLPCoordi_.resize(gCellSize, FloatPoint());
  prevSLPWireLengthGrads_.resize(gCellSize, FloatPoint());
  prevSLPDensityGrads_.resize(gCellSize, FloatPoint());
  prevSLPSumGrads_.resize(gCellSize, FloatPoint());

  curCoordi_.resize(gCellSize, FloatPoint());
  nextCoordi_.resize(gCellSize, FloatPoint());

  initCoordi_.resize(gCellSize, FloatPoint());

  for(auto& gCell : nb_->gCells()) {
    nb_->updateDensityCoordiLayoutInside( gCell );
    int idx = &gCell - &nb_->gCells()[0];
    curSLPCoordi_[idx] 
      = prevSLPCoordi_[idx] 
      = curCoordi_[idx] 
      = initCoordi_[idx]
      = FloatPoint(gCell->dCx(), gCell->dCy()); 
  }

  // bin 
  nb_->updateGCellDensityCenterLocation(curSLPCoordi_);
  
  prevHpwl_ 
    = nb_->getHpwl();

  debugPrint(log_, GPL, "replace", 3, "npinit: InitialHPWL: {}", prevHpwl_);

  // FFT update
  nb_->updateDensityForceBin();

  baseWireLengthCoef_ 
    = npVars_.initWireLengthCoef 
    / (static_cast<float>(nb_->binSizeX() + nb_->binSizeY()) * 0.5);

  debugPrint(log_, GPL, "replace", 3, "npinit: BaseWireLengthCoef: {:g}", baseWireLengthCoef_);
  
  sumOverflow_ = 
    static_cast<float>(nb_->overflowArea()) 
        / static_cast<float>(nb_->nesterovInstsArea());

  debugPrint(log_, GPL, "replace", 3, "npinit: InitSumOverflow: {:g}", sumOverflow_);

  updateWireLengthCoef(sumOverflow_);

  debugPrint(log_, GPL, "replace", 3, "npinit: WireLengthCoef: {:g}", wireLengthCoefX_);

  // WL update
  nb_->updateWireLengthForceWA(wireLengthCoefX_, wireLengthCoefY_);
 
  // fill in curSLPSumGrads_, curSLPWireLengthGrads_, curSLPDensityGrads_ 
  updateGradients(
      curSLPSumGrads_, curSLPWireLengthGrads_,
      curSLPDensityGrads_);

  if( isDiverged_ ) {
    return;
  }

  // approximately fill in 
  // prevSLPCoordi_ to calculate lc vars
  updateInitialPrevSLPCoordi();

  // bin, FFT, wlen update with prevSLPCoordi.
  nb_->updateGCellDensityCenterLocation(prevSLPCoordi_);
  nb_->updateDensityForceBin();
  nb_->updateWireLengthForceWA(wireLengthCoefX_, wireLengthCoefY_);
  
  // update previSumGrads_, prevSLPWireLengthGrads_, prevSLPDensityGrads_
  updateGradients(
      prevSLPSumGrads_, prevSLPWireLengthGrads_,
      prevSLPDensityGrads_);
  
  if( isDiverged_ ) {
    return;
  }
  
  debugPrint(log_, GPL, "replace", 3, "npinit: WireLengthGradSum {:g}", wireLengthGradSum_);
  debugPrint(log_, GPL, "replace", 3, "npinit: DensityGradSum {:g}", densityGradSum_);

  densityPenalty_ 
    = (wireLengthGradSum_ / densityGradSum_ )
    * npVars_.initDensityPenalty; 
  
  debugPrint(log_, GPL, "replace", 3, "npinit: InitDensityPenalty {:g}", densityPenalty_);
  
  sumOverflow_ = 
    static_cast<float>(nb_->overflowArea()) 
        / static_cast<float>(nb_->nesterovInstsArea());
  
  debugPrint(log_, GPL, "replace", 3, "npinit: PrevSumOverflow {:g}", sumOverflow_);
  
  stepLength_  
    = getStepLength (prevSLPCoordi_, prevSLPSumGrads_, curSLPCoordi_, curSLPSumGrads_);

  debugPrint(log_, GPL, "replace", 3, "npinit: InitialStepLength {:g}", stepLength_);

  if( isnan(stepLength_) || isinf(stepLength_) ) {
    log_->error(GPL, 304, "RePlAce diverged at initial iteration. "
       "Re-run with a smaller init_density_penalty value.");
  }
}

// clear reset
void NesterovPlace::reset() {

  npVars_.reset();
  log_ = nullptr;

  curSLPCoordi_.clear();
  curSLPWireLengthGrads_.clear();
  curSLPDensityGrads_.clear();
  curSLPSumGrads_.clear();
  
  nextSLPCoordi_.clear();
  nextSLPWireLengthGrads_.clear();
  nextSLPDensityGrads_.clear();
  nextSLPSumGrads_.clear();
  
  prevSLPCoordi_.clear();
  prevSLPWireLengthGrads_.clear();
  prevSLPDensityGrads_.clear();
  prevSLPSumGrads_.clear();
  
  curCoordi_.clear();
  nextCoordi_.clear();

  densityPenaltyStor_.clear();
  
  curSLPCoordi_.shrink_to_fit();
  curSLPWireLengthGrads_.shrink_to_fit();
  curSLPDensityGrads_.shrink_to_fit();
  curSLPSumGrads_.shrink_to_fit();
  
  nextSLPCoordi_.shrink_to_fit();
  nextSLPWireLengthGrads_.shrink_to_fit();
  nextSLPDensityGrads_.shrink_to_fit();
  nextSLPSumGrads_.shrink_to_fit();
  
  prevSLPCoordi_.shrink_to_fit();
  prevSLPWireLengthGrads_.shrink_to_fit();
  prevSLPDensityGrads_.shrink_to_fit();
  prevSLPSumGrads_.shrink_to_fit();
  
  curCoordi_.shrink_to_fit();
  nextCoordi_.shrink_to_fit();

  densityPenaltyStor_.shrink_to_fit();

  wireLengthGradSum_ = 0;
  densityGradSum_ = 0;
  stepLength_ = 0;
  densityPenalty_ = 0;
  baseWireLengthCoef_ = 0;
  wireLengthCoefX_ = wireLengthCoefY_ = 0;
  prevHpwl_ = 0;
  isDiverged_ = false;
  isRoutabilityNeed_ = true;
  
  divergeMsg_ = "";
  divergeCode_ = 0;
}

// to execute following function,
// 
// nb_->updateGCellDensityCenterLocation(coordi); // bin update
// nb_->updateDensityForceBin(); // bin Force update
//  
// nb_->updateWireLengthForceWA(wireLengthCoefX_, wireLengthCoefY_); // WL update
//
void
NesterovPlace::updateGradients(
    std::vector<FloatPoint>& sumGrads,
    std::vector<FloatPoint>& wireLengthGrads,
    std::vector<FloatPoint>& densityGrads) {

  wireLengthGradSum_ = 0;
  densityGradSum_ = 0;

  float gradSum = 0;

  debugPrint(log_, GPL, "replace", 3, "updateGrad:  DensityPenalty: {:g}", densityPenalty_);

  for(size_t i=0; i<nb_->gCells().size(); i++) {
    GCell* gCell = nb_->gCells().at(i);
    wireLengthGrads[i] = nb_->getWireLengthGradientWA(
        gCell, wireLengthCoefX_, wireLengthCoefY_);
    densityGrads[i] = nb_->getDensityGradient(gCell); 

    // Different compiler has different results on the following formula.
    // e.g. wireLengthGradSum_ += fabs(~~.x) + fabs(~~.y);
    //
    // To prevent instability problem,
    // I partitioned the fabs(~~.x) + fabs(~~.y) as two terms.
    //
    wireLengthGradSum_ += fabs(wireLengthGrads[i].x);
    wireLengthGradSum_ += fabs(wireLengthGrads[i].y);
      
    densityGradSum_ += fabs(densityGrads[i].x);
    densityGradSum_ += fabs(densityGrads[i].y);

    sumGrads[i].x = wireLengthGrads[i].x + densityPenalty_ * densityGrads[i].x;
    sumGrads[i].y = wireLengthGrads[i].y + densityPenalty_ * densityGrads[i].y;

    FloatPoint wireLengthPreCondi 
      = nb_->getWireLengthPreconditioner(gCell);
    FloatPoint densityPrecondi
      = nb_->getDensityPreconditioner(gCell);

    FloatPoint sumPrecondi(
        wireLengthPreCondi.x + densityPenalty_ * densityPrecondi.x,
        wireLengthPreCondi.y + densityPenalty_ * densityPrecondi.y);

    if( sumPrecondi.x <= npVars_.minPreconditioner ) {
      sumPrecondi.x = npVars_.minPreconditioner;
    }

    if( sumPrecondi.y <= npVars_.minPreconditioner ) {
      sumPrecondi.y = npVars_.minPreconditioner; 
    }
    
    sumGrads[i].x /= sumPrecondi.x;
    sumGrads[i].y /= sumPrecondi.y; 

    gradSum += fabs(sumGrads[i].x) + fabs(sumGrads[i].y);
  }
  
  debugPrint(log_, GPL, "replace", 3, "updateGrad:  WireLengthGradSum: {:g}", wireLengthGradSum_);
  debugPrint(log_, GPL, "replace", 3, "updateGrad:  DensityGradSum: {:g}", densityGradSum_);
  debugPrint(log_, GPL, "replace", 3, "updateGrad:  GradSum: {:g}", gradSum);
  
  // divergence detection on 
  // Wirelength / density gradient calculation
  if( isnan(wireLengthGradSum_) || isinf(wireLengthGradSum_) ||
      isnan(densityGradSum_) || isinf(densityGradSum_) ) {
    isDiverged_ = true;
    divergeMsg_ = "RePlAce diverged at wire/density gradient Sum.";
    divergeCode_ = 306; 
  }
}

void
NesterovPlace::doNesterovPlace() {

  // if replace diverged in init() function, 
  // replace must be skipped.
  if( isDiverged_ ) {
    log_->error(GPL, divergeCode_, divergeMsg_);
    return;
  }

#ifdef ENABLE_CIMG_LIB  
  pe.setPlacerBase(pb_);
  pe.setNesterovBase(nb_);
  pe.setLogger(log_);
  pe.Init();
  if (PlotEnv::isPlotEnabled()) {
      pe.SaveCellPlotAsJPEG("Nesterov - BeforeStart", true,
         "cell_0");
      pe.SaveBinPlotAsJPEG("Nesterov - BeforeStart",
         "bin_0");
      pe.SaveArrowPlotAsJPEG("Nesterov - BeforeStart",
         "arrow_0");
  }
#endif

  if (graphics_) {
    graphics_->cellPlot(true);
  }

  // backTracking variable.
  float curA = 1.0;

  // divergence detection
  float minSumOverflow = 1e30;
  float hpwlWithMinSumOverflow = 1e30;

  // dynamic adjustment of max_phi_coef
  bool isMaxPhiCoefChanged = false;

  // snapshot saving detection 
  bool isSnapshotSaved = false;

  // snapshot info
  vector<FloatPoint> snapshotCoordi;
  vector<FloatPoint> snapshotSLPCoordi;
  vector<FloatPoint> snapshotSLPSumGrads;
  float snapshotA = 0;
  float snapshotDensityPenalty = 0;
  float snapshotStepLength = 0;
  float snapshotWlCoefX = 0, snapshotWlCoefY = 0;

  bool isDivergeTriedRevert = false;


  // Core Nesterov Loop
  for(int i=0; i<npVars_.maxNesterovIter; i++) {
    debugPrint(log_, GPL, "replace", 3, "np:  Iter: {}", i+1);
    
    float prevA = curA;

    // here, prevA is a_(k), curA is a_(k+1)
    // See, the ePlace-MS paper's Algorithm 1
    //
    curA = (1.0 + sqrt(4.0 * prevA * prevA + 1.0)) * 0.5;

    // coeff is (a_k - 1) / ( a_(k+1) ) in paper.
    float coeff = (prevA - 1.0)/curA;
    
    debugPrint(log_, GPL, "replace", 3, "np:  PreviousA: {:g}", prevA);
    debugPrint(log_, GPL, "replace", 3, "np:  CurrentA: {:g}", curA);
    debugPrint(log_, GPL, "replace", 3, "np:  Coefficient: {:g}", coeff);
    debugPrint(log_, GPL, "replace", 3, "np:  StepLength: {:g}", stepLength_);

    // Back-Tracking loop
    int numBackTrak = 0;
    for(numBackTrak = 0; numBackTrak < npVars_.maxBackTrack; numBackTrak++) {
      
      // fill in nextCoordinates with given stepLength_
      for(size_t k=0; k<nb_->gCells().size(); k++) {
        FloatPoint nextCoordi(
          curSLPCoordi_[k].x + stepLength_ * curSLPSumGrads_[k].x,
          curSLPCoordi_[k].y + stepLength_ * curSLPSumGrads_[k].y );

        FloatPoint nextSLPCoordi(
          nextCoordi.x + coeff * (nextCoordi.x - curCoordi_[k].x),
          nextCoordi.y + coeff * (nextCoordi.y - curCoordi_[k].y));

        GCell* curGCell = nb_->gCells()[k];

        nextCoordi_[k] 
          = FloatPoint( 
              nb_->getDensityCoordiLayoutInsideX( 
                curGCell, nextCoordi.x),
              nb_->getDensityCoordiLayoutInsideY(
                curGCell, nextCoordi.y));
        
        nextSLPCoordi_[k]
          = FloatPoint(
              nb_->getDensityCoordiLayoutInsideX(
                curGCell, nextSLPCoordi.x),
              nb_->getDensityCoordiLayoutInsideY(
                curGCell, nextSLPCoordi.y));
      }
 

      nb_->updateGCellDensityCenterLocation(nextSLPCoordi_);
      nb_->updateDensityForceBin();
      nb_->updateWireLengthForceWA(wireLengthCoefX_, wireLengthCoefY_);

      updateGradients(nextSLPSumGrads_, nextSLPWireLengthGrads_, nextSLPDensityGrads_ );

      // NaN or inf is detected in WireLength/Density Coef 
      if( isDiverged_ ) {
        break;
      }
  
      float newStepLength  
        = getStepLength (curSLPCoordi_, curSLPSumGrads_, nextSLPCoordi_, nextSLPSumGrads_);
     
      debugPrint(log_, GPL, "replace", 3, "np:  NewStepLength: {:g}", newStepLength);

      if( isnan(newStepLength) || isinf(newStepLength) ) {
        isDiverged_ = true;
        divergeMsg_ = "RePlAce diverged at newStepLength.";
        divergeCode_ = 305;
        break;
      }

      if( newStepLength > stepLength_ * 0.95) {
        stepLength_ = newStepLength;
        break;
      }
      else {
        stepLength_ = newStepLength;
      } 
    }

    debugPrint(log_, GPL, "replace", 3, "np:  NumBackTrak: {}", numBackTrak+1);

    // dynamic adjustment for
    // better convergence with
    // large designs 
    if( !isMaxPhiCoefChanged && sumOverflow_ 
        < 0.35f ) {
      isMaxPhiCoefChanged = true;
      npVars_.maxPhiCoef *= 0.99;
    }

    if( npVars_.maxBackTrack == numBackTrak ) {
      debugPrint(log_, GPL, "replace", 3, "Backtracking limit reached so a small step will be taken");
    }

    if( isDiverged_ ) {
      break;
    }

    updateNextIter(); 


    // For JPEG Saving
    // debug

    if (graphics_) {
      bool update = (i == 0 || (i+1) % npVars_.debug_update_iterations == 0);
      if (update) {
        bool pause = (i == 0 || (i+1) % npVars_.debug_pause_iterations == 0);
        graphics_->cellPlot(pause);
      }
    }

    if( i == 0 || (i+1) % 10 == 0 ) {
      log_->report("[NesterovSolve] Iter: {} overflow: {:g} HPWL: {}",
          i+1, sumOverflow_, prevHpwl_);

#ifdef ENABLE_CIMG_LIB
      if (PlotEnv::isPlotEnabled()) {
        pe.SaveCellPlotAsJPEG(string("Nesterov - Iter: " + std::to_string(i+1)), true,
            string("cell_") +
            getZeroFillStr(i+1));
        pe.SaveBinPlotAsJPEG(string("Nesterov - Iter: " + std::to_string(i+1)),
            string("bin_") +
            getZeroFillStr(i+1));
        pe.SaveArrowPlotAsJPEG(string("Nesterov - Iter: " + std::to_string(i+1)),
            string("arrow_") +
            getZeroFillStr(i+1));
      }
#endif
    }

    if( minSumOverflow > sumOverflow_ ) {
      minSumOverflow = sumOverflow_;
      hpwlWithMinSumOverflow = prevHpwl_; 
    }

    // timing driven feature
    // do reweight on timing-critical nets. 
    if( npVars_.timingDrivenMode 
        && tb_->isTimingUpdateIter(sumOverflow_) ){
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
    //
    if( sumOverflow_ < 0.3f 
        && sumOverflow_ - minSumOverflow >= 0.02f
        && hpwlWithMinSumOverflow * 1.2f < prevHpwl_ ) {
      divergeMsg_ = "RePlAce divergence detected. ";
      divergeMsg_ += "Re-run with a smaller max_phi_cof value.";
      divergeCode_ = 307;
      isDiverged_ = true;

      // revert back to the original rb solutions
      // one more opportunity
      if( !isDivergeTriedRevert 
          && rb_->numCall() >= 1 ) {

        // get back to the working rc size
        rb_->revertGCellSizeToMinRc();

        // revert back the current density penality
        curCoordi_ = snapshotCoordi;
        curSLPCoordi_ = snapshotSLPCoordi;
        curSLPSumGrads_ = snapshotSLPSumGrads;
        curA = snapshotA;
        densityPenalty_ 
          = snapshotDensityPenalty;
        stepLength_ = snapshotStepLength;
        wireLengthCoefX_ = snapshotWlCoefX;
        wireLengthCoefY_ = snapshotWlCoefY;

        nb_->updateGCellDensityCenterLocation(curCoordi_);
        nb_->updateDensityForceBin();
        nb_->updateWireLengthForceWA(wireLengthCoefX_, wireLengthCoefY_);

        isDiverged_ = false;
        divergeCode_ = 0;
        divergeMsg_ = "";
        isDivergeTriedRevert = true;

        // turn off the RD forcely
        isRoutabilityNeed_ = false;
      } 
      else {
        // no way to revert
        break;
      }
    }
  
    // save snapshots for routability-driven  
    if( !isSnapshotSaved 
        && npVars_.routabilityDrivenMode 
        && 0.6 >= sumOverflow_ ) {
      snapshotCoordi = curCoordi_; 
      snapshotSLPCoordi = curSLPCoordi_;
      snapshotSLPSumGrads = curSLPSumGrads_;
      snapshotA = curA;
      snapshotDensityPenalty = densityPenalty_;
      snapshotStepLength = stepLength_;
      snapshotWlCoefX = wireLengthCoefX_;
      snapshotWlCoefY = wireLengthCoefY_;

      isSnapshotSaved = true;
      log_->report("[NesterovSolve] Snapshot saved at iter = {}", i);
    }

    // check routability using GR
    if( npVars_.routabilityDrivenMode 
        && isRoutabilityNeed_ 
        && npVars_.routabilityCheckOverflow >= sumOverflow_ ) {

      // recover the densityPenalty values
      // if further routability-driven is needed 
      std::pair<bool, bool> result = rb_->routability();
      isRoutabilityNeed_ = result.first;
      bool isRevertInitNeeded = result.second;

      // if routability is needed
      if( isRoutabilityNeed_ || isRevertInitNeeded ) {
        // cutFillerCoordinates();

        // revert back the current density penality
        curCoordi_ = snapshotCoordi;
        curSLPCoordi_ = snapshotSLPCoordi;
        curSLPSumGrads_ = snapshotSLPSumGrads;
        curA = snapshotA;
        densityPenalty_ 
          = snapshotDensityPenalty;
        stepLength_ = snapshotStepLength;
        wireLengthCoefX_ = snapshotWlCoefX;
        wireLengthCoefY_ = snapshotWlCoefY;

        nb_->updateGCellDensityCenterLocation(curCoordi_);
        nb_->updateDensityForceBin();
        nb_->updateWireLengthForceWA(wireLengthCoefX_, wireLengthCoefY_);
  
        // reset the divergence detect conditions 
        minSumOverflow = 1e30;
        hpwlWithMinSumOverflow = 1e30; 
        log_->report("[NesterovSolve] Revert back to snapshot coordi");
      }
    }

    // minimum iteration is 50
    if( i > 50 && sumOverflow_ <= npVars_.targetOverflow) {
      log_->report("[NesterovSolve] Finished with Overflow: {:.6f}", sumOverflow_);
      break;
    }
  }
 
  // in all case including diverge, 
  // db should be updated. 
  updateDb();

  if( isDiverged_ ) { 
    log_->error(GPL, divergeCode_, divergeMsg_);
  }

  if (graphics_) {
    graphics_->status("End placement");
    graphics_->cellPlot(true);
  }
}

void
NesterovPlace::updateWireLengthCoef(float overflow) {
  if( overflow > 1.0 ) {
    wireLengthCoefX_ = wireLengthCoefY_ = 0.1;
  }
  else if( overflow < 0.1 ) {
    wireLengthCoefX_ = wireLengthCoefY_ = 10.0;
  }
  else {
    wireLengthCoefX_ = wireLengthCoefY_ 
      = 1.0 / pow(10.0, (overflow-0.1)*20 / 9.0 - 1.0);
  }

  wireLengthCoefX_ *= baseWireLengthCoef_;
  wireLengthCoefY_ *= baseWireLengthCoef_;
  debugPrint(log_, GPL, "replace", 3, "updateWireLengthCoef:  NewWireLengthCoef: {:g}", wireLengthCoefX_);
}

void
NesterovPlace::updateInitialPrevSLPCoordi() {
  for(size_t i=0; i<nb_->gCells().size(); i++) {
    GCell* curGCell = nb_->gCells()[i];


    float prevCoordiX 
      = curSLPCoordi_[i].x - npVars_.initialPrevCoordiUpdateCoef
      * curSLPSumGrads_[i].x;
  
    float prevCoordiY
      = curSLPCoordi_[i].y - npVars_.initialPrevCoordiUpdateCoef
      * curSLPSumGrads_[i].y;
    
    FloatPoint newCoordi( 
      nb_->getDensityCoordiLayoutInsideX( curGCell, prevCoordiX),
      nb_->getDensityCoordiLayoutInsideY( curGCell, prevCoordiY) );

    prevSLPCoordi_[i] = newCoordi;
  } 
}

void
NesterovPlace::updateNextIter() {
  // swap vector pointers
  std::swap(prevSLPCoordi_, curSLPCoordi_);
  std::swap(prevSLPWireLengthGrads_, curSLPWireLengthGrads_);
  std::swap(prevSLPDensityGrads_, curSLPDensityGrads_);
  std::swap(prevSLPSumGrads_, curSLPSumGrads_);
  
  std::swap(curSLPCoordi_, nextSLPCoordi_);
  std::swap(curSLPWireLengthGrads_, nextSLPWireLengthGrads_);
  std::swap(curSLPDensityGrads_, nextSLPDensityGrads_);
  std::swap(curSLPSumGrads_, nextSLPSumGrads_);

  std::swap(curCoordi_, nextCoordi_);

  sumOverflow_ = 
      static_cast<float>(nb_->overflowArea()) 
        / static_cast<float>(nb_->nesterovInstsArea());

  debugPrint(log_, GPL, "replace", 3, "updateNextIter:  Gradient: {:g}", getSecondNorm(curSLPSumGrads_));
  debugPrint(log_, GPL, "replace", 3, "updateNextIter:  Phi: {:g}", nb_->sumPhi());
  debugPrint(log_, GPL, "replace", 3, "updateNextIter:  Overflow: {:g}", sumOverflow_);

  updateWireLengthCoef(sumOverflow_);
  int64_t hpwl = nb_->getHpwl();
  
  debugPrint(log_, GPL, "replace", 3, "updateNextIter:  PreviousHPWL: {}", prevHpwl_);
  debugPrint(log_, GPL, "replace", 3, "updateNextIter:  NewHPWL: {}", hpwl);
  

  float phiCoef = getPhiCoef( 
      static_cast<float>(hpwl - prevHpwl_) 
      / npVars_.referenceHpwl );
  
  prevHpwl_ = hpwl;
  densityPenalty_ *= phiCoef;
  
  debugPrint(log_, GPL, "replace", 3, "updateNextIter:  PhiCoef: {:g}", phiCoef);

  // for routability densityPenalty recovery
  if( rb_->numCall() == 0 ) {
    densityPenaltyStor_.push_back( densityPenalty_ );
  }
}

float
NesterovPlace::getStepLength(
    const std::vector<FloatPoint>& prevSLPCoordi_,
    const std::vector<FloatPoint>& prevSLPSumGrads_,
    const std::vector<FloatPoint>& curSLPCoordi_,
    const std::vector<FloatPoint>& curSLPSumGrads_ ) {

  float coordiDistance 
    = getDistance(prevSLPCoordi_, curSLPCoordi_);
  float gradDistance 
    = getDistance(prevSLPSumGrads_, curSLPSumGrads_);

  debugPrint(log_, GPL, "replace", 3, "getStepLength:  CoordinateDistance: {:g}", coordiDistance);
  debugPrint(log_, GPL, "replace", 3, "getStepLength:  GradientDistance: {:g}", gradDistance);

  return coordiDistance / gradDistance;
}

float
NesterovPlace::getPhiCoef(float scaledDiffHpwl) const {
  debugPrint(log_, GPL, "replace", 3, "getPhiCoef:  InputScaleDiffHPWL: {:g}", scaledDiffHpwl);


  float retCoef 
    = (scaledDiffHpwl < 0)? 
    npVars_.maxPhiCoef: 
    npVars_.maxPhiCoef * pow( npVars_.maxPhiCoef, scaledDiffHpwl * -1.0 );
  retCoef = std::max(npVars_.minPhiCoef, retCoef);
  return retCoef;
}

void
NesterovPlace::updateDb() {
  nb_->updateDbGCells();
}

void
NesterovPlace::cutFillerCoordinates() {
  curSLPCoordi_.resize( nb_->fillerCnt() );
  curSLPWireLengthGrads_.resize( nb_->fillerCnt() );
  curSLPDensityGrads_.resize( nb_->fillerCnt() );
  curSLPSumGrads_.resize( nb_->fillerCnt() );

  nextSLPCoordi_.resize( nb_->fillerCnt() );
  nextSLPWireLengthGrads_.resize( nb_->fillerCnt() );
  nextSLPDensityGrads_.resize( nb_->fillerCnt() );
  nextSLPSumGrads_.resize( nb_->fillerCnt() );

  prevSLPCoordi_.resize( nb_->fillerCnt() );
  prevSLPWireLengthGrads_.resize( nb_->fillerCnt() );
  prevSLPDensityGrads_.resize( nb_->fillerCnt() );
  prevSLPSumGrads_.resize( nb_->fillerCnt() );

  curCoordi_.resize(nb_->fillerCnt());
  nextCoordi_.resize(nb_->fillerCnt());
}


static float
getDistance(const vector<FloatPoint>& a, const vector<FloatPoint>& b) {
  float sumDistance = 0.0f;
  for(size_t i=0; i<a.size(); i++) {
    sumDistance += (a[i].x - b[i].x) * (a[i].x - b[i].x);
    sumDistance += (a[i].y - b[i].y) * (a[i].y - b[i].y);
  }

  return sqrt( sumDistance / (2.0 * a.size()) );
}

static float
getSecondNorm(const vector<FloatPoint>& a) {
  float norm = 0;
  for(auto& coordi : a) {
    norm += coordi.x * coordi.x + coordi.y * coordi.y;
  }
  return sqrt( norm / (2.0*a.size()) ); 
}

#ifdef ENABLE_CIMG_LIB
static std::string
getZeroFillStr(int iterNum) {
  std::ostringstream str;
  str << std::setw(4) << std::setfill('0') << iterNum;
  return str.str();
}
#endif

}
