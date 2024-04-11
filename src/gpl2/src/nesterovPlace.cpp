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

#include "nesterovPlace.h"

#include <iomanip>
#include <iostream>
#include <sstream>

#include "odb/db.h"
#include "placerBase.h"
#include "gpuRouteBase.h"
#include "gpuTimingBase.h"
#include "utl/Logger.h"

namespace gpl2 {

using namespace std;
using utl::GPL2;

/////////////////////////////////////////////////////////////////
// Class NesterovPlace
NesterovPlace::NesterovPlace()
  : pbc_(nullptr),
    log_(nullptr),
    rb_(nullptr),
    tb_(nullptr),
    npVars_(),
    baseWireLengthCoef_(0),
    wireLengthCoefX_(0),
    wireLengthCoefY_(0),
    prevHpwl_(0),
    isDiverged_(false),
    isRoutabilityNeed_(true),
    divergeCode_(0),
    recursionCntWlCoef_(0),
    recursionCntInitSLPCoef_(0)
{
}


NesterovPlace::NesterovPlace(const NesterovPlaceVars& npVars,
                             bool DREAMPlaceFlag,
                             const std::shared_ptr<PlacerBaseCommon>& pbc,
                             std::vector<std::shared_ptr<PlacerBase> >& pbVec,
                             std::shared_ptr<GpuRouteBase> rb,
                             std::shared_ptr<GpuTimingBase> tb,
                             utl::Logger* log)
  : NesterovPlace()
{
  npVars_ = npVars;
  pbc_ = pbc;
  pbVec_ = pbVec;
  rb_ = rb;
  tb_ = tb;
  log_ = log;
  DREAMPlaceFlag_ = DREAMPlaceFlag;

  init();
}

NesterovPlace::~NesterovPlace()
{
  reset();
}

void NesterovPlace::init()
{ 
  std::cout << "NesterovPlace initialization starts" << std::endl;
  // Create CUDA kernels for PlacerBaseCommon and PlacerBase
  // TODO: move PlacerBaseCommon CUDA kernels to initial placement
    
  auto startTimestamp = std::chrono::high_resolution_clock::now();


  pbc_->initCUDAKernel();
  std::cout << "[INFO] DREAMPlaceFlag = " << DREAMPlaceFlag_ << std::endl;
  if (DREAMPlaceFlag_ == true) {
    pbc_->enableDREAMPlaceFlag();
  }
  
  for (auto& pb : pbVec_) {
    pb->initCUDAKernel();
  }

  //pbc_->printInstInfo();

  auto endTimestamp = std::chrono::high_resolution_clock::now();
  double endTime = std::chrono::duration_cast<std::chrono::nanoseconds>(
        endTimestamp- startTimestamp).count();
  std::cout << "Initialization time = " << endTime * 1e-9 << std::endl;

  // for_each nesterovbase call init
  totalSumOverflow_ = 0;
  float totalBaseWireLengthCoeff = 0;
  for (auto& pb : pbVec_) {
    pb->setNpVars(npVars_);
    pb->initDensity1(); // update the density cooridinates of instances
    totalSumOverflow_ += pb->getSumOverflow();
    std::cout << "totalSumOverflow_ = " << totalSumOverflow_ << std::endl;
    totalBaseWireLengthCoeff += pb->getBaseWireLengthCoef();
    std::cout << "totalBaseWireLengthCoeff = " << totalBaseWireLengthCoeff << std::endl;
  }

  averageOverflow_ = totalSumOverflow_ / pbVec_.size();
  baseWireLengthCoef_ = totalBaseWireLengthCoeff / pbVec_.size();
  std::cout << "averageOverflow_ = " << averageOverflow_ << std::endl;
  std::cout << "baseWireLengthCoef_ = " << baseWireLengthCoef_ << std::endl;
  
  updateWireLengthCoef(averageOverflow_);

  // updated the WA wirelength and its gradient
  pbc_->updatePinLocation();
  pbc_->updateWireLengthForce(wireLengthCoefX_, wireLengthCoefY_);

  for (auto& pb : pbVec_) {
    // fill in curSLPSumGrads_, curSLPWirelengthGrads_, curSLPDensityGrads_
    updateCurGradient(pb); 
    // approximately fill in prevSLPCoordi_ to calculate lc vars
    pb->updateInitialPrevSLPCoordi();
    // bin, FFT, when update with prevSLPCoordi
    pb->updateDensityCenterPrevSLP();
    pb->updateDensityForceBin(); // update the density force on each instance
  }

  // update the WA gradient again
  pbc_->updatePinLocation();
  pbc_->updateWireLengthForce(wireLengthCoefX_, wireLengthCoefY_);  

  for (auto& pb : pbVec_) {
    // update prevSumGrads_, prevWirelengthGrads_, prevDensityGrads_
    updatePrevGradient(pb);
  }

  for (auto& pb : pbVec_) {
    float stepL = pb->initDensity2();
    if ((isnan(stepL) || isinf(stepL)) 
        && recursionCntInitSLPCoef_ < npVars_.maxRecursionInitSLPCoef) {
      npVars_.initialPrevCoordiUpdateCoef *= 10;
      std::string msg = "steplength = 0 detected. Rerunning Nesterov::init() ";
      msg += "with initPrevSLPCoef ";
      msg += std::to_string(npVars_.initialPrevCoordiUpdateCoef);
      log_->report(msg);
      recursionCntInitSLPCoef_++;
      init();
      break;
    }

    if (isnan(stepL) || isinf(stepL)) {
      std::string msg = "RePlAce diverged at initial iteration with steplength being " + std::to_string(stepL) + ". ";
      msg += "Re-run with a smaller init_density_penalty value."; 
      log_->error(GPL2, 304, msg);
    } 
  }

  std::cout << "NesterovPlace initialization ends" << std::endl;
}



// clear reset
void NesterovPlace::reset()
{
  npVars_.reset();
  log_ = nullptr;

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



// If replace diverged in init() function,
// the doNesterovPlace() function must be skipped.
// Please check the init() function for details.
bool NesterovPlace::doNesterovPlace(int start_iter)
{
  bool convergeFlag = true;
  
  // if replace diverged in init() function,
  // replace must be skipped.
  if (isDiverged_) {
    log_->error(GPL2, divergeCode_, divergeMsg_);
    return false; // diverge 
  }

  auto start_timestamp_global = std::chrono::high_resolution_clock::now();

  bool isDivergeTriedRevert = false;
  // backTracking variable.
  float curA = 1.0; // a_(0)

  for (auto& pb : pbVec_) {
    pb->setIter(start_iter);
    pb->setMaxPhiCoefChanged(false);
    pb->resetMinSumOverflow();
  }

  // Core Nesterov Loop
  int iter = start_iter;

  //for (; iter < npVars_.maxNesterovIter; iter++) {
  for(; iter <= 2000; iter++) {
    pbc_->updateVirtualWeightFactor(iter);
    
    //if (pbc_->numPlaceInsts() < 10000 && iter % 100 == 0) {
    //  pbc_->plotDBCluster(iter);
    //} 
    
    float prevA = curA;
    // here, prevA is a_(k), curA is a_(k+1)
    // See, the ePlace-MS paper's Algorithm 1
    //
    curA = (1.0 + sqrt(4.0 * prevA * prevA + 1.0)) * 0.5;    
    // coeff is (a_k - 1) / ( a_(k+1) ) in paper.
    float coeff = (prevA - 1.0) / curA;
    
    // Back-Tracking loop
    int numBackTrak = 0;
    for (numBackTrak = 0; numBackTrak < npVars_.maxBackTrack; numBackTrak++) {
      // the backtracking method is applied to enhance the prediction accuracy
      // via preventing potential steplength overestimation
      // fill in nextCoordinates with given stepLength_
      for (auto& pb : pbVec_) {
        pb->nesterovUpdateCoordinates(coeff);
      }

      // wirelength is global
      pbc_->updatePinLocation();
      pbc_->updateWireLengthForce(wireLengthCoefX_, wireLengthCoefY_);

      // density is local
      int numDiverge = 0;
      for (auto& pb : pbVec_) {
        updateNextGradient(pb);
        numDiverge += pb->isDiverged();
      }


      if (debugFlag) {
        std::cout << "numDiverege = " << numDiverge << "  "
                  << "isDiverged = " << isDiverged_ << std::endl;
      }

      // Nan or inf is detected in WireLength/Density Coef
      if (numDiverge > 0 || isDiverged_) {
        isDiverged_ = true;
        divergeMsg_ = "RePlAce diverged at wire/density gradient Sum.";
        divergeCode_ = 306;
        convergeFlag = false;
        break;
      }

      // Update the stepLength_ for next iteration
      int stepLengthLimitOK = 0;
      numDiverge = 0;
      for (auto& pb : pbVec_) {
        stepLengthLimitOK += pb->nesterovUpdateStepLength();
        numDiverge += pb->isDiverged();
      }

      if (numDiverge > 0) {
        isDiverged_ = true;
        divergeMsg_ = "RePlAce diverged at newStepLength.";
        divergeCode_ = 305;
        convergeFlag = false;
        break;
      }
      
      if (stepLengthLimitOK != pbVec_.size()) {
        // stepLength_ is NOT OK
        convergeFlag = false;
        break;
      }     
    }

    // Adjust Phi dynamically for larger designs
    for (auto& pb : pbVec_) {
      pb->nesterovAdjustPhi();
    }

    // check if diverged
    if (isDiverged_) {
      convergeFlag = false;
      break;
    }

    // update the wirelength coefficient (1 / gamma)
    // perform the next iteration update
    // Trick here:  basically we have three vectors:  v_(k-1), v_(k), v_(k+1)
    // In the next step, we need to change the vector pointers as following:
    // v_(k-1) = v_(k), v_(k) = v_(k+1)
    // The updateNextIter() function will do this.
    // All other parts of the Nesterove Update is in the aboveb steplength backtracking loop.
    updateNextIter(iter);

    // leave the timing-driven feature here
    // TODO:  do reweight on timing-critical nets

    // leave the routability-driven feature here
    // TODO:  check routability using GR

    // check each for converge and if all are converged then stop
    int numConverge = 0;
    for (auto& pb : pbVec_) {
      numConverge += pb->checkConvergence();
    }
    
    if (numConverge == pbVec_.size()) {
      convergeFlag = true;
      std::cout << "NestrovSolve converged" << std::endl;
      break;
    }
  }

  // in all case including diverge,
  // db should be updated.
  updateDb();

  std::cout << std::string(80, '*') << std::endl;
  std::cout << "NesterovPlace runtime analysis" << std::endl;
  std::cout << "Wirelength runtime : " << pbc_->wireLengthRuntime() << " sec" << std::endl;
  for (auto& pb : pbVec_) {
    std::cout << "Density runtime : " << pb->densityTime() << " sec" <<  std::endl;
    std::cout << "FFT runtime = " << pb->fftTime() << " sec" << std::endl;
    std::cout << "updateGradientRuntime1 = " << pb->updateGradientRuntime1() << " sec" << std::endl;
    std::cout << "updateGradientRuntime2 = " << pb->updateGradientRuntime2() << " sec" << std::endl;
    std::cout << "updateGradientRuntime3 = " << pb->updateGradientRuntime3() << " sec" << std::endl;
    std::cout << "updateGradientRuntime4 = " << pb->updateGradientRuntime4() << " sec" << std::endl;
    std::cout << "updateGradientRuntime = " << pb->updateGradientRuntime() << " sec" << std::endl;
  }

  auto end_timestamp_global = std::chrono::high_resolution_clock::now();
  double total_global_time
          = std::chrono::duration_cast<std::chrono::nanoseconds>(
                end_timestamp_global - start_timestamp_global)
                .count();
  total_global_time *= 1e-9;
  std::cout << "[Time Info] The runtime for non-linear optimization is " 
                << total_global_time << std::endl;

  //
  //if (isDiverged_) {
  //  log_->error(GPL2, divergeCode_, divergeMsg_);
  //}

  //if (convergeFlag == true && iter == npVars_.maxNesterovIter) {
  //  convergeFlag = false;
  //}

  return convergeFlag;
}


// The wirelength coeffocient is updated
// X and Y direction are always the same
// The coefficent is defined as the 1 / gamma
// We set the gamma to a smaller value when the overflow is large
// We set the gamma to a lower value when the overflow is small
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
}


// update next iter
// Compared to RePlAce, we abdondon the "_"
void NesterovPlace::updateNextIter(const int iter) 
{
  totalSumOverflow_ = 0;

  // update the density overflow
  for (auto& pb : pbVec_) {
    pb->updateNextIter(iter);
    totalSumOverflow_ += pb->getSumOverflow();
  }

  // For coefficient update, using the average regions's overflow
  averageOverflow_ = totalSumOverflow_ / pbVec_.size();

  // update the wirelength coefficient
  updateWireLengthCoef(averageOverflow_);
}

// update the OpenDB database
void NesterovPlace::updateDb()
{
  // In the original implementation,
  // nbc_->updateDbGCells();
  // pbc_->updateDB();  
  pbc_->updateDB();
  //pbc_->updateDBCluster();
}


// Gradient related functions
// update the gradient
void NesterovPlace::updatePrevGradient(std::shared_ptr<PlacerBase> pb)
{
  pb->updatePrevGradient();
  float wireLengthGradSum = pb->getWireLengthGradSum();
  float densityGradSum = pb->getDensityGradSum();

  if (debugFlag) {
    std::cout << "updatePrevGradient" << std::endl;
    std::cout << "wireLengthGradSum = " << wireLengthGradSum << std::endl;
    std::cout << "densityGradSum = " << densityGradSum << std::endl;
  }

  if (wireLengthGradSum == 0 
      && recursionCntWlCoef_ < npVars_.maxRecursionWlCoef) {
    wireLengthCoefX_ *= 0.5;
    wireLengthCoefY_ *= 0.5;
    baseWireLengthCoef_ *= 0.5;
    std::string msg = "sum (WL gradient) = 0 detected.";
    msg += "trying again with wlCoef: ";
    msg += std::to_string(wireLengthCoefX_) + " " + std::to_string(wireLengthCoefY_);
    log_->report(msg);

    // update WL forces
    pbc_->updateWireLengthForce(wireLengthCoefX_, wireLengthCoefY_);

    // recursive call again with smaller wirelength coefficient
    recursionCntWlCoef_++;
    updatePrevGradient(pb);
    return;
  }  

  if (debugFlag) {
    std::cout << "after updatePrevGradient" << std::endl;
    std::cout << "wireLengthGradSum = " << wireLengthGradSum << std::endl;
    std::cout << "densityGradSum = " << densityGradSum << std::endl;
  }

  // divergence detection on
  // Wirelength / density gradient calculation
  if (isnan(wireLengthGradSum) || isinf(wireLengthGradSum)
      || isnan(densityGradSum) || isinf(densityGradSum)) {
    isDiverged_ = true;
    divergeMsg_ = "RePlAce diverged at wire/density gradient Sum.";
    divergeCode_ = 306;
  }
}


void NesterovPlace::updateCurGradient(std::shared_ptr<PlacerBase> pb)
{
  pb->updateCurGradient();

  float wireLengthGradSum = pb->getWireLengthGradSum();
  float densityGradSum = pb->getDensityGradSum();

  if (debugFlag) {
    std::cout << "after updateCurGradient" << std::endl;
    std::cout << "wireLengthGradSum = " << wireLengthGradSum << std::endl;
    std::cout << "densityGradSum = " << densityGradSum << std::endl;
  }

  if (wireLengthGradSum == 0 
      && recursionCntWlCoef_ < npVars_.maxRecursionWlCoef) {
    wireLengthCoefX_ *= 0.5;
    wireLengthCoefY_ *= 0.5;
    baseWireLengthCoef_ *= 0.5;
    std::string msg = "sum (WL gradient) = 0 detected.";
    msg += "trying again with wlCoef: ";
    msg += std::to_string(wireLengthCoefX_) + " " + std::to_string(wireLengthCoefY_);
    log_->report(msg);

    // update WL forces
    pbc_->updateWireLengthForce(wireLengthCoefX_, wireLengthCoefY_);

    // recursive call again with smaller wirelength coefficient
    recursionCntWlCoef_++;
    updateCurGradient(pb);
    return;
  }  

  // divergence detection on
  // Wirelength / density gradient calculation
  if (isnan(wireLengthGradSum) || isinf(wireLengthGradSum)
      || isnan(densityGradSum) || isinf(densityGradSum)) {
    isDiverged_ = true;
    divergeMsg_ = "RePlAce diverged at wire/density gradient Sum.";
    divergeCode_ = 306;
  }
}


void NesterovPlace::updateNextGradient(std::shared_ptr<PlacerBase> pb)
{
  pb->updateNextGradient();
  float wireLengthGradSum = pb->getWireLengthGradSum();
  float densityGradSum = pb->getDensityGradSum();

  if (wireLengthGradSum == 0 
      && recursionCntWlCoef_ < npVars_.maxRecursionWlCoef) {
    wireLengthCoefX_ *= 0.5;
    wireLengthCoefY_ *= 0.5;
    baseWireLengthCoef_ *= 0.5;
    std::string msg = "sum (WL gradient) = 0 detected.";
    msg += "trying again with wlCoef: ";
    msg += std::to_string(wireLengthCoefX_) + " " + std::to_string(wireLengthCoefY_);
    log_->report(msg);

    // update WL forces
    pbc_->updateWireLengthForce(wireLengthCoefX_, wireLengthCoefY_);

    // recursive call again with smaller wirelength coefficient
    recursionCntWlCoef_++;
    updateNextGradient(pb);
    return;
  }  


  if (debugFlag) {
    std::cout << "after updateNextGradient" << std::endl;
    std::cout << "wireLengthGradSum = " << wireLengthGradSum << std::endl;
    std::cout << "densityGradSum = " << densityGradSum << std::endl;
  }

  // divergence detection on
  // Wirelength / density gradient calculation
  if (isnan(wireLengthGradSum) || isinf(wireLengthGradSum)
      || isnan(densityGradSum) || isinf(densityGradSum)) {
    isDiverged_ = true;
    divergeMsg_ = "RePlAce diverged at wire/density gradient Sum.";
    divergeCode_ = 306;
  }
}

}  // namespace gpl2
