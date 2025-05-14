///////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2023, Google LLC
// Copyright (c) 2024, Antmicro
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
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

// We have two kinds of initial placement methods
// (i) for designs with preplaced macros, we call the original conjugate
// gradient based flat placement method.  The CG solver is implemented on the
// cpu side. (ii) for designs without preplaced macros, we call the nesterov
// based clustered placement method. The nesterov solver is implemented on the
// gpu side.

#include "initialPlace.h"

#include <chrono>
#include <thread>
#include <utility>

#include "placerBase.h"
#include "utl/Logger.h"

#include <Eigen/IterativeLinearSolvers>
#include <Eigen/SparseCore>
#include <memory>

namespace gpl2 {

using Eigen::BiCGSTAB;
using Eigen::IdentityPreconditioner;

typedef Eigen::Triplet<float> T;
typedef Eigen::SparseMatrix<float, Eigen::RowMajor> SMatrix;

struct ResidualError
{
  float x;  // The relative residual error for X
  float y;  // The relative residual error for Y
};

//////////////////////////////////////////////////////////////
// Class InitialPlaceVars
InitialPlaceVars::InitialPlaceVars()
{
  reset();
}

void InitialPlaceVars::reset()
{
  maxIter = 20;
  minDiffLength = 1500;
  maxSolverIter = 100;
  maxFanout = 200;
  netWeightScale = 800.0;
}

/////////////////////////////////////////////////////////////////
// Class InitialPlace
InitialPlace::InitialPlace() : pbc_(nullptr), log_(nullptr)
{
}

InitialPlace::InitialPlace(InitialPlaceVars ipVars,
                           std::shared_ptr<PlacerBaseCommon> pbc,
                           std::vector<std::shared_ptr<PlacerBase>>& pbVec,
                           utl::Logger* log)
    : ipVars_(ipVars), pbc_(std::move(pbc)), pbVec_(pbVec), log_(log)
{
}

InitialPlace::InitialPlace(InitialPlaceVars ipVars,
                           float haloWidth,
                           int numHops,
                           bool dataflowFlag,
                           sta::dbNetwork* network,
                           odb::dbDatabase* db,
                           utl::Logger* log)
    : ipVars_(ipVars),
      log_(log),
      network_(network),
      db_(db),
      haloWidth_(haloWidth),
      numHops_(numHops),
      dataflowFlag_(dataflowFlag)
{
}

InitialPlace::~InitialPlace()
{
  reset();
}

void InitialPlace::reset()
{
  pbc_ = nullptr;
  ipVars_.reset();
}

void InitialPlace::doInitialPlace()
{
  bool fixedMacroFlag = false;
  odb::dbBlock* block = db_->getChip()->getBlock();
  odb::dbSet<odb::dbInst> insts = block->getInsts();
  for (odb::dbInst* inst : insts) {
    auto type = inst->getMaster()->getType();
    if (type.isBlock()
        && (inst->getPlacementStatus() == odb::dbPlacementStatus::LOCKED
            || inst->getPlacementStatus() == odb::dbPlacementStatus::FIRM)) {
      fixedMacroFlag = true;
      break;
    }
  }

  fixedMacroFlag = true;
  // check if the design has fixed macros
  if (fixedMacroFlag == true) {
    log_->report("[InitialPlace]  Design has fixed macros.");
    log_->report("[InitialPlace]  Use CG-based initial placement.");
    doBicgstabPlace();
  } else {
    log_->report("[InitialPlace]  Design has no fixed macros.");
    log_->report("[InitialPlace]  Use Nesterov-based initial placement.");
    if (doClusterNesterovPlace() == false) {
      log_->report("[InitialPlace] Nesterov-based initial placement failed.");
      log_->report("[InitialPlace] Use CG-based initial placement.");
      doBicgstabPlace();
    }
  }
}

void InitialPlace::setPlacerBaseVars(PlacerBaseVars pbVars)
{
  pbVars_ = pbVars;
}

void InitialPlace::setNesterovBaseVars(NesterovBaseVars nbVars)
{
  nbVars_ = nbVars;
}

void InitialPlace::setNesterovPlaceVars(NesterovPlaceVars npVars)
{
  npVars_ = npVars;
}

//////////////////////////////////////////////////////////////////////////////
// Conjugate Gradient Solver for Initial Placement
//////////////////////////////////////////////////////////////////////////////

// CPU Solver from the original RePlAce
ResidualError cpuSparseSolve(int maxSolverIter,
                             int iter,
                             SMatrix& placeInstForceMatrixX,
                             Eigen::VectorXf& fixedInstForceVecX,
                             Eigen::VectorXf& instLocVecX,
                             SMatrix& placeInstForceMatrixY,
                             Eigen::VectorXf& fixedInstForceVecY,
                             Eigen::VectorXf& instLocVecY)
{
  ResidualError error;
  BiCGSTAB<SMatrix, IdentityPreconditioner> solver;
  solver.setMaxIterations(maxSolverIter);
  solver.compute(placeInstForceMatrixX);
  instLocVecX = solver.solveWithGuess(fixedInstForceVecX, instLocVecX);
  error.x = solver.error();

  solver.compute(placeInstForceMatrixY);
  instLocVecY = solver.solveWithGuess(fixedInstForceVecY, instLocVecY);
  error.y = solver.error();
  return error;
}

void InitialPlace::doBicgstabPlace()
{
  pbc_ = std::make_shared<PlacerBaseCommon>(network_,
                                            db_,
                                            pbVars_,
                                            log_,
                                            haloWidth_,
                                            0,
                                            0,
                                            false,
                                            false,
                                            false,
                                            false);
  pbVec_.push_back(std::make_shared<PlacerBase>(nbVars_, db_, pbc_, log_));
  for (const auto& pb : pbVec_) {
    pb->setNpVars(npVars_);
  }

  ResidualError error;
  placeInstsCenter();
  for (size_t iter = 1; iter <= ipVars_.maxIter; iter++) {
    updatePinInfo();
    createSparseMatrix();
    error = cpuSparseSolve(ipVars_.maxSolverIter,
                           iter,
                           placeInstForceMatrixX_,
                           fixedInstForceVecX_,
                           instLocVecX_,
                           placeInstForceMatrixY_,
                           fixedInstForceVecY_,
                           instLocVecY_);
    float error_max = std::max(error.x, error.y);
    log_->report(
        "[InitialPlace]  Iter: {} CG residual: {:0.8f}", iter, error_max);
    updateCoordi();

    if (error_max <= 1e-5 && iter >= 5) {
      break;
    }
  }
}

void InitialPlace::placeInstsCenter()
{
  const int centerX = pbc_->die().coreCx();
  const int centerY = pbc_->die().coreCy();

  // handle on the host side
  for (auto& inst : pbc_->placeInsts()) {
    auto group = inst->dbInst()->getGroup();
    if (group && group->getType() == odb::dbGroupType::POWER_DOMAIN) {
      auto domainRegion = group->getRegion();
      int domainXMin = std::numeric_limits<int>::max();
      int domainYMin = std::numeric_limits<int>::max();
      int domainXMax = std::numeric_limits<int>::min();
      int domainYMax = std::numeric_limits<int>::min();
      for (auto boundary : domainRegion->getBoundaries()) {
        domainXMin = std::min(domainXMin, boundary->xMin());
        domainYMin = std::min(domainYMin, boundary->yMin());
        domainXMax = std::max(domainXMax, boundary->xMax());
        domainYMax = std::max(domainYMax, boundary->yMax());
      }
      inst->setCenterLocation(domainXMax - (domainXMax - domainXMin) / 2,
                              domainYMax - (domainYMax - domainYMin) / 2);
    } else {
      inst->setCenterLocation(centerX, centerY);
    }
  }
}

// Same as the original CPU-based RePlAce
void InitialPlace::updatePinInfo()
{
  // reset all MinMax attributes
  for (auto& pin : pbc_->pins()) {
    pin->unsetMinPinX();
    pin->unsetMinPinY();
    pin->unsetMaxPinX();
    pin->unsetMaxPinY();
  }

  for (auto& net : pbc_->nets()) {
    Pin *pinMinX = nullptr, *pinMinY = nullptr;
    Pin *pinMaxX = nullptr, *pinMaxY = nullptr;
    int lx = INT_MAX, ly = INT_MAX;
    int ux = INT_MIN, uy = INT_MIN;

    // Mark B2B info on Pin structures
    for (auto& pin : net->pins()) {
      if (lx > pin->cx()) {
        if (pinMinX) {
          pinMinX->unsetMinPinX();
        }
        lx = pin->cx();
        pinMinX = pin;
        pinMinX->setMinPinX();
      }

      if (ux < pin->cx()) {
        if (pinMaxX) {
          pinMaxX->unsetMaxPinX();
        }
        ux = pin->cx();
        pinMaxX = pin;
        pinMaxX->setMaxPinX();
      }

      if (ly > pin->cy()) {
        if (pinMinY) {
          pinMinY->unsetMinPinY();
        }
        ly = pin->cy();
        pinMinY = pin;
        pinMinY->setMinPinY();
      }

      if (uy < pin->cy()) {
        if (pinMaxY) {
          pinMaxY->unsetMaxPinY();
        }
        uy = pin->cy();
        pinMaxY = pin;
        pinMaxY->setMaxPinY();
      }
    }
  }
}

void InitialPlace::createSparseMatrix()
{
  const int placeCnt = pbc_->placeInsts().size();
  instLocVecX_.resize(placeCnt);
  fixedInstForceVecX_.resize(placeCnt);
  instLocVecY_.resize(placeCnt);
  fixedInstForceVecY_.resize(placeCnt);

  placeInstForceMatrixX_.resize(placeCnt, placeCnt);
  placeInstForceMatrixY_.resize(placeCnt, placeCnt);

  //
  // listX and listY is a temporary vector that have tuples, (idx1, idx2, val)
  //
  // listX finally becomes placeInstForceMatrixX_
  // listY finally becomes placeInstForceMatrixY_
  //
  // The triplet vector is recommended usages
  // to fill in SparseMatrix from Eigen docs.
  //

  std::vector<T> listX, listY;
  listX.reserve(1000000);
  listY.reserve(1000000);

  // initialize vector
  for (auto& inst : pbc_->placeInsts()) {
    int idx = inst->instId();
    instLocVecX_(idx) = inst->cx();
    instLocVecY_(idx) = inst->cy();
    fixedInstForceVecX_(idx) = fixedInstForceVecY_(idx) = 0;
  }

  // for each net
  for (auto& net : pbc_->nets()) {
    // skip for small nets.
    if (net->pins().size() <= 1) {
      continue;
    }

    // escape long time cals on huge fanout.
    //
    if (net->pins().size() >= ipVars_.maxFanout) {
      continue;
    }

    float netWeight = ipVars_.netWeightScale / (net->pins().size() - 1);
    // foreach two pins in single nets.
    auto& pins = net->pins();
    for (int pinIdx1 = 1; pinIdx1 < pins.size(); ++pinIdx1) {
      Pin* pin1 = pins[pinIdx1];
      for (int pinIdx2 = 0; pinIdx2 < pinIdx1; ++pinIdx2) {
        Pin* pin2 = pins[pinIdx2];

        // no need to fill in when instance is same
        if (pin1->instance() == pin2->instance()) {
          continue;
        }

        // B2B modeling on min/maxX pins.
        if (pin1->isMinPinX() || pin1->isMaxPinX() || pin2->isMinPinX()
            || pin2->isMaxPinX()) {
          int diffX = abs(pin1->cx() - pin2->cx());
          float weightX = 0;
          if (diffX > ipVars_.minDiffLength) {
            weightX = netWeight / diffX;
          } else {
            weightX = netWeight / ipVars_.minDiffLength;
          }

          // both pin cames from instance
          if (pin1->isPlaceInstConnected() && pin2->isPlaceInstConnected()) {
            const int inst1 = pin1->instId();
            const int inst2 = pin2->instId();

            listX.push_back(T(inst1, inst1, weightX));
            listX.push_back(T(inst2, inst2, weightX));

            listX.push_back(T(inst1, inst2, -weightX));
            listX.push_back(T(inst2, inst1, -weightX));

            fixedInstForceVecX_(inst1)
                += -weightX
                   * ((pin1->cx() - pin1->instance()->cx())
                      - (pin2->cx() - pin2->instance()->cx()));

            fixedInstForceVecX_(inst2)
                += -weightX
                   * ((pin2->cx() - pin2->instance()->cx())
                      - (pin1->cx() - pin1->instance()->cx()));
          }
          // pin1 from IO port / pin2 from Instance
          else if (!pin1->isPlaceInstConnected()
                   && pin2->isPlaceInstConnected()) {
            const int inst2 = pin2->instId();
            listX.push_back(T(inst2, inst2, weightX));

            fixedInstForceVecX_(inst2)
                += weightX
                   * (pin1->cx() - (pin2->cx() - pin2->instance()->cx()));
          }
          // pin1 from Instance / pin2 from IO port
          else if (pin1->isPlaceInstConnected()
                   && !pin2->isPlaceInstConnected()) {
            const int inst1 = pin1->instId();
            listX.push_back(T(inst1, inst1, weightX));

            fixedInstForceVecX_(inst1)
                += weightX
                   * (pin2->cx() - (pin1->cx() - pin1->instance()->cx()));
          }
        }

        // B2B modeling on min/maxY pins.
        if (pin1->isMinPinY() || pin1->isMaxPinY() || pin2->isMinPinY()
            || pin2->isMaxPinY()) {
          int diffY = abs(pin1->cy() - pin2->cy());
          float weightY = 0;
          if (diffY > ipVars_.minDiffLength) {
            weightY = netWeight / diffY;
          } else {
            weightY = netWeight / ipVars_.minDiffLength;
          }

          // both pin cames from instance
          if (pin1->isPlaceInstConnected() && pin2->isPlaceInstConnected()) {
            const int inst1 = pin1->instId();
            const int inst2 = pin2->instId();

            listY.push_back(T(inst1, inst1, weightY));
            listY.push_back(T(inst2, inst2, weightY));

            listY.push_back(T(inst1, inst2, -weightY));
            listY.push_back(T(inst2, inst1, -weightY));

            fixedInstForceVecY_(inst1)
                += -weightY
                   * ((pin1->cy() - pin1->instance()->cy())
                      - (pin2->cy() - pin2->instance()->cy()));

            fixedInstForceVecY_(inst2)
                += -weightY
                   * ((pin2->cy() - pin2->instance()->cy())
                      - (pin1->cy() - pin1->instance()->cy()));
          }
          // pin1 from IO port / pin2 from Instance
          else if (!pin1->isPlaceInstConnected()
                   && pin2->isPlaceInstConnected()) {
            const int inst2 = pin2->instId();
            listY.push_back(T(inst2, inst2, weightY));

            fixedInstForceVecY_(inst2)
                += weightY
                   * (pin1->cy() - (pin2->cy() - pin2->instance()->cy()));
          }
          // pin1 from Instance / pin2 from IO port
          else if (pin1->isPlaceInstConnected()
                   && !pin2->isPlaceInstConnected()) {
            const int inst1 = pin1->instId();
            listY.push_back(T(inst1, inst1, weightY));

            fixedInstForceVecY_(inst1)
                += weightY
                   * (pin2->cy() - (pin1->cy() - pin1->instance()->cy()));
          }
        }
      }
    }
  }

  placeInstForceMatrixX_.setFromTriplets(listX.begin(), listX.end());
  placeInstForceMatrixY_.setFromTriplets(listY.begin(), listY.end());
}

void InitialPlace::updateCoordi()
{
  int instIdx = 0;
  for (auto& inst : pbc_->placeInsts()) {
    inst->setCenterLocation(instLocVecX_(instIdx), instLocVecY_(instIdx));
    inst->dbSetLocation();
    inst->dbSetPlaced();
    instIdx++;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Nesterov-based Initial Placement
//////////////////////////////////////////////////////////////////////////////////////////////////

bool InitialPlace::doClusterNesterovPlace()
{
  float bloatFactor = 1.0;
  bool convergeFlag = false;
  for (int i = 0; i < 10; i++) {
    if (doClusterNesterovPlaceIter(bloatFactor) == true) {
      convergeFlag = true;
      break;
    }
    // Wait for the GPU to free memory
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  return convergeFlag;
}

bool InitialPlace::doClusterNesterovPlaceIter(float& bloatFactor)
{
  // set the flag for clustered placement
  const int virtualIter = 0;        // We do not consider virtual connections
  const bool clusterFlag = true;    // We need to do cluster placement
  const bool datapathFlag = false;  // We do not need to consider datapath
  const bool clusterConstraintFlag = false;  // not applied here
  pbc_ = std::make_shared<PlacerBaseCommon>(network_,
                                            db_,
                                            pbVars_,
                                            log_,
                                            haloWidth_,
                                            virtualIter,
                                            numHops_,
                                            bloatFactor,
                                            clusterFlag,
                                            datapathFlag,
                                            clusterConstraintFlag);

  // put all instances to the center
  const int centerX = pbc_->die().coreCx();
  const int centerY = pbc_->die().coreCy();
  for (auto& inst : pbc_->placeInsts()) {
    inst->setCenterLocation(centerX, centerY);
  }

  pbVec_.push_back(std::make_shared<PlacerBase>(nbVars_, db_, pbc_, log_));
  for (const auto& pb : pbVec_) {
    pb->setNpVars(npVars_);
  }

  // TODO:  we do not have timing-driven or routability-driven mode
  rb_ = nullptr;
  tb_ = nullptr;

  std::unique_ptr<NesterovPlace> np(
      new NesterovPlace(npVars_, pbc_, pbVec_, rb_, tb_, log_));

  const int start_iter = 0;
  bool convergeFlag = np->doNesterovPlace(start_iter);

  bloatFactor
      = bloatFactor * npVars_.targetOverflow / pbVec_[0]->getSumOverflow();
  pbVec_.clear();

  return convergeFlag;
}

}  // namespace gpl2
