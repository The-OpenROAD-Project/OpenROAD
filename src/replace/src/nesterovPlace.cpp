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
#include "opendb/db.h"
#include "routeBase.h"
#include "logger.h"
#include <iostream>
using namespace std;

#include "plot.h"

namespace replace {

static float
getDistance(vector<FloatPoint>& a, vector<FloatPoint>& b);

static float
getSecondNorm(vector<FloatPoint>& a);

NesterovPlaceVars::NesterovPlaceVars()
  : maxNesterovIter(5000), 
  maxBackTrack(10),
  initDensityPenalty(0.00008),
  initWireLengthCoef(0.25),
  targetOverflow(0.1),
  minPhiCoef(0.95),
  maxPhiCoef(1.05),
  minPreconditioner(1.0),
  initialPrevCoordiUpdateCoef(100),
  referenceHpwl(446000000),
  routabilityCheckOverflow(0.20),
  timingDrivenMode(true),
  routabilityDrivenMode(true) {}


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
}

NesterovPlace::NesterovPlace() 
  : pb_(nullptr), nb_(nullptr), rb_(nullptr), log_(nullptr), npVars_(), 
  wireLengthGradSum_(0), 
  densityGradSum_(0),
  stepLength_(0),
  densityPenalty_(0),
  baseWireLengthCoef_(0), 
  wireLengthCoefX_(0), 
  wireLengthCoefY_(0),
  prevHpwl_(0),
  isDiverged_(false),
  isRoutabilityNeed_(true) {}

NesterovPlace::NesterovPlace(
    NesterovPlaceVars npVars,
    std::shared_ptr<PlacerBase> pb, 
    std::shared_ptr<NesterovBase> nb,
    std::shared_ptr<RouteBase> rb,
    std::shared_ptr<Logger> log) 
: NesterovPlace() {
  npVars_ = npVars;
  pb_ = pb;
  nb_ = nb;
  rb_ = rb;
  log_ = log;
  init();
}

NesterovPlace::~NesterovPlace() {
  reset();
}

#ifdef ENABLE_CIMG_LIB
static PlotEnv pe;
#endif

void NesterovPlace::init() {
  log_->procBegin("NesterovInit", 3);

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

  log_->infoInt64("InitialHPWL", prevHpwl_, 3);

  // FFT update
  nb_->updateDensityForceBin();

  baseWireLengthCoef_ 
    = npVars_.initWireLengthCoef 
    / (static_cast<float>(nb_->binSizeX() + nb_->binSizeY()) * 0.5);

  log_->infoFloatSignificant("BaseWireLengthCoef", baseWireLengthCoef_, 3);
  
  sumOverflow_ = 
    static_cast<float>(nb_->overflowArea()) 
        / static_cast<float>(nb_->nesterovInstsArea());

  log_->infoFloatSignificant("InitSumOverflow", sumOverflow_, 3);

  updateWireLengthCoef(sumOverflow_);

  log_->infoFloatSignificant("WireLengthCoef", wireLengthCoefX_, 3);

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
  
  log_->infoFloatSignificant("WireLengthGradSum", wireLengthGradSum_, 3);
  log_->infoFloatSignificant("DensityGradSum", densityGradSum_, 3);

  densityPenalty_ 
    = (wireLengthGradSum_ / densityGradSum_ )
    * npVars_.initDensityPenalty; 
  
  log_->infoFloatSignificant("InitDensityPenalty", densityPenalty_, 3);
  
  sumOverflow_ = 
    static_cast<float>(nb_->overflowArea()) 
        / static_cast<float>(nb_->nesterovInstsArea());
  
  log_->infoFloatSignificant("PrevSumOverflow", sumOverflow_, 3);
  
  stepLength_  
    = getStepLength (prevSLPCoordi_, prevSLPSumGrads_, curSLPCoordi_, curSLPSumGrads_);


  log_->infoFloatSignificant("InitialStepLength", stepLength_, 3);
  log_->procEnd("NesterovInit", 3);

  if( isnan(stepLength_) ) {
    string msg = "RePlAce diverged at initial iteration.\n";
    msg += "        Please tune the parameters again";
    log_->errorQuit(msg, 5);
    isDiverged_ = true;
  }
}

// clear reset
void NesterovPlace::reset() {

  npVars_.reset();

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

  log_->infoFloatSignificant("  DensityPenalty", densityPenalty_, 3);

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
  
  log_->infoFloatSignificant("  WireLengthGradSum", wireLengthGradSum_, 3);
  log_->infoFloatSignificant("  DensityGradSum", densityGradSum_, 3);
  log_->infoFloatSignificant("  GradSum", gradSum, 3);

  // divergence detection on 
  // Wirelength / density gradient calculation
  if( isnan(wireLengthGradSum_) || isinf(wireLengthGradSum_) ||
      isnan(densityGradSum_) || isinf(densityGradSum_) ) {
    isDiverged_ = true;
  }
}

void
NesterovPlace::doNesterovPlace() {

  // if replace diverged in init() function, 
  // replace must be skipped.
  if( isDiverged_ ) {
    string msg = "RePlAce diverged. Please tune the parameters again";
    log_->error(msg, 2);
    return;
  }

#ifdef ENABLE_CIMG_LIB  
  pe.setPlacerBase(pb_);
  pe.setNesterovBase(nb_);
  pe.Init();
      
  pe.SaveCellPlotAsJPEG("Nesterov - BeforeStart", true,
     "./plot/cell/cell_0");
  pe.SaveBinPlotAsJPEG("Nesterov - BeforeStart",
     "./plot/bin/bin_0");
  pe.SaveArrowPlotAsJPEG("Nesterov - BeforeStart",
     "./plot/arrow/arrow_0");
#endif


  // backTracking variable.
  float curA = 1.0;

  // divergence detection
  float minSumOverflow = 1e30;
  float hpwlWithMinSumOverflow = 1e30;

  // dynamic adjustment of max_phi_coef
  bool isMaxPhiCoefChanged = false;

  // diverge error handling
  string divergeMsg = "";
  int divergeCode = 0;

  // snapshot saving detection 
  bool isSnapshotSaved = false;

  // snapshot info
  vector<FloatPoint> snapshotCoordi;
  float snapshotA = 0;
  float snapshotDensityPenalty = 0;

  bool isDivergeTriedRevert = false;


  // Core Nesterov Loop
  for(int i=0; i<npVars_.maxNesterovIter; i++) {
    log_->infoInt("Iter", i+1, 3);
    
    float prevA = curA;

    // here, prevA is a_(k), curA is a_(k+1)
    // See, the papers' Algorithm 4 section
    //
    curA = (1.0 + sqrt(4.0 * prevA * prevA + 1.0)) * 0.5;

    // coeff is (a_k -1) / ( a_(k+1)) in paper.
    float coeff = (prevA - 1.0)/curA;
    
    log_->infoFloatSignificant("  PreviousA", prevA, 3);
    log_->infoFloatSignificant("  CurrentA", curA, 3);
    log_->infoFloatSignificant("  Coefficient", coeff, 3);
    log_->infoFloatSignificant("  StepLength", stepLength_, 3);

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
     
      log_->infoFloatSignificant("  NewStepLength", newStepLength, 3);

      if( isnan(newStepLength) ) {
        divergeMsg = "RePlAce divergence detected. \n";
        divergeMsg += "        Please tune the parameters again";
        divergeCode = 6;
        isDiverged_ = true;
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

    log_->infoInt("  NumBackTrak", numBackTrak+1, 3);

    // dynamic adjustment for
    // better convergence with
    // large designs 
    if( !isMaxPhiCoefChanged && sumOverflow_ 
        < 0.35f ) {
      isMaxPhiCoefChanged = true;
      npVars_.maxPhiCoef *= 0.99;
    }

    // usually, maxBackTrack should be 1~3
    // 10 is the case when
    // all of cells are not moved at all.
    if( npVars_.maxBackTrack == numBackTrak ) {
      divergeMsg = "RePlAce divergence detected. \n";
      divergeMsg += "        Please decrease init_density_penalty value";
      divergeCode = 3;
      isDiverged_ = true;
    } 

    if( isDiverged_ ) {
      break;
    }

    updateNextIter(); 


    // For JPEG Saving
    // debug

    if( i == 0 || (i+1) % 10 == 0 ) {
      cout << "[NesterovSolve] Iter: " << i+1 
        << " overflow: " << sumOverflow_ << " HPWL: " << prevHpwl_ << endl; 
#ifdef ENABLE_CIMG_LIB
      pe.SaveCellPlotAsJPEG(string("Nesterov - Iter: " + std::to_string(i+1)), true,
          string("./plot/cell/cell_") +
          std::to_string (i+1));
      pe.SaveBinPlotAsJPEG(string("Nesterov - Iter: " + std::to_string(i+1)),
          string("./plot/bin/bin_") +
          std::to_string(i+1));
      pe.SaveArrowPlotAsJPEG(string("Nesterov - Iter: " + std::to_string(i+1)),
          string("./plot/arrow/arrow_") +
          std::to_string(i+1));
#endif
    }

    if( minSumOverflow > sumOverflow_ ) {
      minSumOverflow = sumOverflow_;
      hpwlWithMinSumOverflow = prevHpwl_; 
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
      divergeMsg = "RePlAce divergence detected. \n";
      divergeMsg += "        Please decrease max_phi_cof value";
      divergeCode = 4;
      isDiverged_ = true;

      // revert back to the original rb solutions
      // one more opportunity
      if( !isDivergeTriedRevert 
          && rb_->numCall() >= 1 ) {

        // get back to the working rc size
        rb_->revertGCellSizeToMinRc();

        // revert back the current density penality
        curA = snapshotA;
        nb_->updateGCellDensityCenterLocation(snapshotCoordi);
        init();
        densityPenalty_ 
          = snapshotDensityPenalty;
        isDiverged_ = false;
        isDivergeTriedRevert = true;

        // turn off the RD forcely
        isRoutabilityNeed_ = false;
      } 
      else {
        // no way to revert
        break;
      }
    }
    
    if( !isSnapshotSaved 
        && npVars_.routabilityDrivenMode 
        && 0.6 >= sumOverflow_ ) {
      snapshotCoordi = curCoordi_; 
      snapshotDensityPenalty = densityPenalty_;
      snapshotA = curA;

      isSnapshotSaved = true;
      cout << "[NesterovSolve] Snapshot saved at iter = " + to_string(i) << endl;
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
        curA = snapshotA;
        nb_->updateGCellDensityCenterLocation(snapshotCoordi);
        init();
        densityPenalty_ 
          = snapshotDensityPenalty;
  
        // reset the divergence detect conditions 
        minSumOverflow = 1e30;
        hpwlWithMinSumOverflow = 1e30; 
      }
    }

    // minimum iteration is 50
    if( i > 50 && sumOverflow_ <= npVars_.targetOverflow) {
      cout << "[NesterovSolve] Finished with Overflow: " << sumOverflow_ << endl;
      break;
    }
  }
 
  // in all case including diverge, 
  // db should be updated. 
  updateDb();

  if( isDiverged_ ) { 
    log_->errorQuit(divergeMsg, divergeCode);
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
  log_->infoFloatSignificant("  NewWireLengthCoef", wireLengthCoefX_, 3);
}

void
NesterovPlace::updateInitialPrevSLPCoordi() {
  for(size_t i=0; i<nb_->gCells().size(); i++) {
    GCell* curGCell = nb_->gCells()[i];


    float prevCoordiX 
      = curSLPCoordi_[i].x + npVars_.initialPrevCoordiUpdateCoef 
      * curSLPSumGrads_[i].x;
  
    float prevCoordiY
      = curSLPCoordi_[i].y + npVars_.initialPrevCoordiUpdateCoef
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

  log_->infoFloatSignificant("  Gradient", getSecondNorm(curSLPSumGrads_), 3);
  log_->infoFloatSignificant("  Phi", nb_->sumPhi(), 3);
  log_->infoFloatSignificant("  Overflow", sumOverflow_, 3);

  updateWireLengthCoef(sumOverflow_);
  int64_t hpwl = nb_->getHpwl();
  
  log_->infoInt64("  PreviousHPWL", prevHpwl_, 3);
  log_->infoInt64("  NewHPWL", hpwl, 3);
  

  float phiCoef = getPhiCoef( 
      static_cast<float>(hpwl - prevHpwl_) 
      / npVars_.referenceHpwl );
  
  prevHpwl_ = hpwl;
  densityPenalty_ *= phiCoef;
  
  log_->infoFloatSignificant("  PhiCoef", phiCoef, 3);

  // for routability densityPenalty recovery
  if( rb_->numCall() == 0 ) {
    densityPenaltyStor_.push_back( densityPenalty_ );
  }
}

float
NesterovPlace::getStepLength(
    std::vector<FloatPoint>& prevSLPCoordi_,
    std::vector<FloatPoint>& prevSLPSumGrads_,
    std::vector<FloatPoint>& curSLPCoordi_,
    std::vector<FloatPoint>& curSLPSumGrads_ ) {

  float coordiDistance 
    = getDistance(prevSLPCoordi_, curSLPCoordi_);
  float gradDistance 
    = getDistance(prevSLPSumGrads_, curSLPSumGrads_);

  log_->infoFloatSignificant("  CoordinateDistance", coordiDistance, 3);
  log_->infoFloatSignificant("  GradientDistance", gradDistance, 3);

  return coordiDistance / gradDistance;
}

float
NesterovPlace::getPhiCoef(float scaledDiffHpwl) {
  log_->infoFloatSignificant("  InputScaleDiffHPWL", scaledDiffHpwl, 3);

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
getDistance(vector<FloatPoint>& a, vector<FloatPoint>& b) {
  float sumDistance = 0.0f;
  for(size_t i=0; i<a.size(); i++) {
    sumDistance += (a[i].x - b[i].x) * (a[i].x - b[i].x);
    sumDistance += (a[i].y - b[i].y) * (a[i].y - b[i].y);
  }

  return sqrt( sumDistance / (2.0 * a.size()) );
}

static float
getSecondNorm(vector<FloatPoint>& a) {
  float norm = 0;
  for(auto& coordi : a) {
    norm += coordi.x * coordi.x + coordi.y * coordi.y;
  }
  return sqrt( norm / (2.0*a.size()) ); 
}


}
