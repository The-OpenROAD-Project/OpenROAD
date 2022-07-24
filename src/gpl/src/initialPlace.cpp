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
  
#include <fstream>

#include "initialPlace.h"
#include "placerBase.h"

#include <Eigen/IterativeLinearSolvers>

#include "plot.h"
#include "graphics.h"

#include "utl/Logger.h"
#if CUDA
  #include "cudasplibs.h"
#endif
namespace gpl {
using namespace std;

using Eigen::BiCGSTAB;
using Eigen::IdentityPreconditioner;
using utl::GPL;

typedef Eigen::Triplet< float > T;

InitialPlaceVars::InitialPlaceVars() 
{
  reset();
}

void InitialPlaceVars::reset() {
  maxIter = 20;
  minDiffLength = 1500;
  maxSolverIter = 100;
  maxFanout = 200;
  netWeightScale = 800.0;
  debug = false;
}

InitialPlace::InitialPlace()
: ipVars_(), pb_(nullptr), log_(nullptr) {} 

InitialPlace::InitialPlace(InitialPlaceVars ipVars, 
    std::shared_ptr<PlacerBase> pb,
    utl::Logger* log)
: ipVars_(ipVars), pb_(pb), log_(log)
{
}

InitialPlace::~InitialPlace() {
  reset();
}

void InitialPlace::reset() {
  pb_ = nullptr;
  ipVars_.reset();
}

#ifdef ENABLE_CIMG_LIB
static PlotEnv pe;
#endif

void InitialPlace::doBicgstabPlace() {
  float errorX = 0.0f, errorY = 0.0f;

#ifdef ENABLE_CIMG_LIB
  pe.setPlacerBase(pb_);
  pe.setLogger(log_);
  pe.Init();
#endif

  std::unique_ptr<Graphics> graphics;
  if (ipVars_.debug && Graphics::guiActive()) {
    graphics = make_unique<Graphics>(log_, pb_);
  }

  placeInstsCenter();

  // set ExtId for idx reference // easy recovery
  setPlaceInstExtId();

  for(size_t iter=1; iter<=ipVars_.maxIter; iter++) {

    updatePinInfo();
    createSparseMatrix();
    #if CUDA

      // Set sparse matrices from dense matrices.
      cooRowIndexX.clear();
      cooColIndexX.clear();
      cooValX.clear();
      cooRowIndexY.clear();
      cooColIndexY.clear();
      cooValY.clear();
      for (size_t row = 0; row < placeInstForceMatrixX_.rows(); row ++){
        for (size_t col = 0; col < placeInstForceMatrixX_.cols(); col ++){
          if (placeInstForceMatrixX_.coeffRef(row, col) != 0){
            cooRowIndexX.push_back(row);
            cooColIndexX.push_back(col);
            cooValX.push_back(placeInstForceMatrixX_.coeffRef(row, col));
          } 
          if (placeInstForceMatrixY_.coeffRef(row, col) != 0){
            cooRowIndexY.push_back(row);
            cooColIndexY.push_back(col);
            cooValY.push_back(placeInstForceMatrixY_.coeffRef(row,col));
          } 
        }
      }

      cudasplibs SP1(cooRowIndexX, cooColIndexX, cooValX, fixedInstForceVecX_);
      SP1.cusolverSpQR(instLocVecX_);
      errorX = SP1.error_cal();
      SP1.release();

      cudasplibs SP2(cooRowIndexY, cooColIndexY, cooValY, fixedInstForceVecY_);
      SP2.cusolverSpQR(instLocVecY_);
      errorY = SP2.error_cal();
      SP2.release();
      log_->report("[InitialPlace]  Iter: {} CG residual: {:0.8f} HPWL: {}", iter, max(errorX, errorY), pb_->hpwl());
    
    #else
      // BiCGSTAB solver for initial place
      BiCGSTAB< SMatrix, IdentityPreconditioner > solver;
      solver.setMaxIterations(ipVars_.maxSolverIter);
      solver.compute(placeInstForceMatrixX_);
      instLocVecX_ = solver.solveWithGuess(fixedInstForceVecX_, instLocVecX_);
      errorX = solver.error();
      
      solver.compute(placeInstForceMatrixY_);
      instLocVecY_ = solver.solveWithGuess(fixedInstForceVecY_, instLocVecY_);
      errorY = solver.error();

      log_->report("[InitialPlace]  Iter: {} CG residual: {:0.8f} HPWL: {}",
        iter, max(errorX, errorY), pb_->hpwl());
    #endif
    updateCoordi();

#ifdef ENABLE_CIMG_LIB
    if (PlotEnv::isPlotEnabled()) pe.SaveCellPlotAsJPEG(
        string("InitPlace ") + to_string(i), false,
        string("ip_") + to_string(i));
#endif

    if (graphics) {
        graphics->cellPlot(true);
    }

    if (max(errorX, errorY) <= 1e-5 && iter >= 5 ) {
      break;
    }
  }
}

// starting point of initial place is center.
void InitialPlace::placeInstsCenter() {
  const int centerX = pb_->die().coreCx();
  const int centerY = pb_->die().coreCy();

  for(auto& inst: pb_->placeInsts()) {
    if (!inst->isLocked()) {
      inst->setCenterLocation(centerX, centerY);
    }
  }
}

void InitialPlace::setPlaceInstExtId() {
  // reset ExtId for all instances
  for(auto& inst : pb_->insts()) {
    inst->setExtId(INT_MAX);
  }
  // set index only with place-able instances
  for(auto& inst : pb_->placeInsts()) {
    inst->setExtId(&inst - &(pb_->placeInsts()[0]));
  }
}

void InitialPlace::updatePinInfo() {
  // reset all MinMax attributes
  for(auto& pin : pb_->pins()) {
    pin->unsetMinPinX();
    pin->unsetMinPinY();
    pin->unsetMaxPinX();
    pin->unsetMaxPinY();
  }

  for(auto& net : pb_->nets()) {
    Pin* pinMinX = nullptr, *pinMinY = nullptr;
    Pin* pinMaxX = nullptr, *pinMaxY = nullptr;  
    int lx = INT_MAX, ly = INT_MAX;
    int ux = INT_MIN, uy = INT_MIN;

    // Mark B2B info on Pin structures
    for(auto& pin : net->pins()) {
      if (lx > pin->cx() ) {
        if (pinMinX ) {
          pinMinX->unsetMinPinX();
        }
        lx = pin->cx();
        pinMinX = pin; 
        pinMinX->setMinPinX();
      } 
      
      if (ux < pin->cx() ) {
        if (pinMaxX ) {
          pinMaxX->unsetMaxPinX();
        }
        ux = pin->cx();
        pinMaxX = pin; 
        pinMaxX->setMaxPinX();
      } 

      if (ly > pin->cy() ) {
        if (pinMinY ) {
          pinMinY->unsetMinPinY();
        }
        ly = pin->cy();
        pinMinY = pin; 
        pinMinY->setMinPinY();
      } 
      
      if (uy < pin->cy() ) {
        if (pinMaxY ) {
          pinMaxY->unsetMaxPinY();
        }
        uy = pin->cy();
        pinMaxY = pin; 
        pinMaxY->setMaxPinY();
      } 
    }
  } 
}

// solve placeInstForceMatrixX_ * xcg_x_ = xcg_b_ and placeInstForceMatrixY_ * ycg_x_ = ycg_b_ eq.
void InitialPlace::createSparseMatrix() {
  const int placeCnt = pb_->placeInsts().size();
  instLocVecX_.resize( placeCnt );
  fixedInstForceVecX_.resize( placeCnt );
  instLocVecY_.resize( placeCnt );
  fixedInstForceVecY_.resize( placeCnt );

  placeInstForceMatrixX_.resize( placeCnt, placeCnt );
  placeInstForceMatrixY_.resize( placeCnt, placeCnt );


  // 
  // listX and listY is a temporary vector that have tuples, (idx1, idx2, val)
  //
  // listX finally becomes placeInstForceMatrixX_
  // listY finally becomes placeInstForceMatrixY_
  //
  // The triplet vector is recommended usages 
  // to fill in SparseMatrix from Eigen docs.
  //

  vector< T > listX, listY;
  listX.reserve(1000000);
  listY.reserve(1000000);

  // initialize vector
  for(auto& inst : pb_->placeInsts()) {
    int idx = inst->extId(); 
    
    instLocVecX_(idx) = inst->cx();
    instLocVecY_(idx) = inst->cy();

    fixedInstForceVecX_(idx) = fixedInstForceVecY_(idx) = 0;
  }

  // for each net
  for(auto& net : pb_->nets()) {

    // skip for small nets.
    if (net->pins().size() <= 1 ) {
      continue;
    }
 
    // escape long time cals on huge fanout.
    //
    if (net->pins().size() >= ipVars_.maxFanout) { 
      continue;
    }

    float netWeight = ipVars_.netWeightScale 
      / (net->pins().size() - 1);
    //cout << "net: " << net.net()->getConstName() << endl;

    // foreach two pins in single nets.
    auto& pins = net->pins();
    for(int pinIdx1 = 1; pinIdx1 < pins.size(); ++pinIdx1) {
      Pin* pin1 = pins[pinIdx1];
      for(int pinIdx2 = 0; pinIdx2 < pinIdx1; ++pinIdx2) {
        Pin* pin2 = pins[pinIdx2];

        // no need to fill in when instance is same
        if (pin1->instance() == pin2->instance() ) {
          continue;
        }

        // B2B modeling on min/maxX pins.
        if (pin1->isMinPinX() || pin1->isMaxPinX() ||
            pin2->isMinPinX() || pin2->isMaxPinX() ) {
          int diffX = abs(pin1->cx() - pin2->cx());
          float weightX = 0;
          if (diffX > ipVars_.minDiffLength ) {
            weightX = netWeight / diffX;
          }
          else {
            weightX = netWeight 
              / ipVars_.minDiffLength;
          }

          // both pin cames from instance
          if (pin1->isPlaceInstConnected() 
              && pin2->isPlaceInstConnected() ) {
            const int inst1 = pin1->instance()->extId();
            const int inst2 = pin2->instance()->extId();
            //cout << "inst: " << inst1 << " " << inst2 << endl;

            listX.push_back( T(inst1, inst1, weightX) );
            listX.push_back( T(inst2, inst2, weightX) );

            listX.push_back( T(inst1, inst2, -weightX) );
            listX.push_back( T(inst2, inst1, -weightX) );

            //cout << pin1->cx() << " " 
            //  << pin1->instance()->cx() << endl;
            fixedInstForceVecX_(inst1) += 
              -weightX * (
              (pin1->cx() - pin1->instance()->cx()) - 
              (pin2->cx() - pin2->instance()->cx()));

            fixedInstForceVecX_(inst2) +=
              -weightX * (
              (pin2->cx() - pin2->instance()->cx()) -
              (pin1->cx() - pin1->instance()->cx())); 
          }
          // pin1 from IO port / pin2 from Instance
          else if (!pin1->isPlaceInstConnected() 
              && pin2->isPlaceInstConnected() ) {
            const int inst2 = pin2->instance()->extId();
            //cout << "inst2: " << inst2 << endl;
            listX.push_back( T(inst2, inst2, weightX) );

            fixedInstForceVecX_(inst2) += weightX * 
              ( pin1->cx() - 
                ( pin2->cx() - pin2->instance()->cx()) );
          }
          // pin1 from Instance / pin2 from IO port
          else if (pin1->isPlaceInstConnected() 
              && !pin2->isPlaceInstConnected() ) {
            const int inst1 = pin1->instance()->extId();
            //cout << "inst1: " << inst1 << endl;
            listX.push_back( T(inst1, inst1, weightX) );


            fixedInstForceVecX_(inst1) += weightX *
              ( pin2->cx() -
                ( pin1->cx() - pin1->instance()->cx()) );
          }
        }
        
        // B2B modeling on min/maxY pins.
        if (pin1->isMinPinY() || pin1->isMaxPinY() ||
            pin2->isMinPinY() || pin2->isMaxPinY() ) {
          
          int diffY = abs(pin1->cy() - pin2->cy());
          float weightY = 0;
          if (diffY > ipVars_.minDiffLength ) {
            weightY = netWeight / diffY;
          }
          else {
            weightY = netWeight 
              / ipVars_.minDiffLength;
          }

          // both pin cames from instance
          if (pin1->isPlaceInstConnected() 
              && pin2->isPlaceInstConnected() ) {
            const int inst1 = pin1->instance()->extId();
            const int inst2 = pin2->instance()->extId();

            listY.push_back( T(inst1, inst1, weightY) );
            listY.push_back( T(inst2, inst2, weightY) );

            listY.push_back( T(inst1, inst2, -weightY) );
            listY.push_back( T(inst2, inst1, -weightY) );

            fixedInstForceVecY_(inst1) += 
              -weightY * (
              (pin1->cy() - pin1->instance()->cy()) - 
              (pin2->cy() - pin2->instance()->cy()));

            fixedInstForceVecY_(inst2) +=
              -weightY * (
              (pin2->cy() - pin2->instance()->cy()) -
              (pin1->cy() - pin1->instance()->cy())); 
          }
          // pin1 from IO port / pin2 from Instance
          else if (!pin1->isPlaceInstConnected() 
              && pin2->isPlaceInstConnected() ) {
            const int inst2 = pin2->instance()->extId();
            listY.push_back( T(inst2, inst2, weightY) );


            fixedInstForceVecY_(inst2) += weightY * 
              ( pin1->cy() - 
                ( pin2->cy() - pin2->instance()->cy()) );
          }
          // pin1 from Instance / pin2 from IO port
          else if (pin1->isPlaceInstConnected() 
              && !pin2->isPlaceInstConnected() ) {
            const int inst1 = pin1->instance()->extId();
            listY.push_back( T(inst1, inst1, weightY) );


            fixedInstForceVecY_(inst1) += weightY *
              ( pin2->cy() -
                ( pin1->cy() - pin1->instance()->cy()) );
          }
        }
      }
    }
  } 
  
  placeInstForceMatrixX_.setFromTriplets(listX.begin(), listX.end());
  placeInstForceMatrixY_.setFromTriplets(listY.begin(), listY.end());
}

void InitialPlace::updateCoordi() {
  for(auto& inst : pb_->placeInsts()) {
    int idx = inst->extId();
    if (!inst->isLocked()) {
      inst->dbSetCenterLocation( instLocVecX_(idx), instLocVecY_(idx) );
      inst->dbSetPlaced();
    }
  }
}

}
