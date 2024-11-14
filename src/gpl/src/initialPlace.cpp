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

#include "initialPlace.h"

#include <utility>

#include "placerBase.h"
#include "solver.h"

namespace gpl {

using T = Eigen::Triplet<float>;

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
  debug = false;
}

InitialPlace::InitialPlace(InitialPlaceVars ipVars,
                           std::shared_ptr<PlacerBaseCommon> pbc,
                           std::vector<std::shared_ptr<PlacerBase>>& pbVec,
                           utl::Logger* log)
    : ipVars_(ipVars), pbc_(std::move(pbc)), pbVec_(pbVec), log_(log)
{
}

void InitialPlace::doBicgstabPlace()
{
  ResidualError error;

  std::unique_ptr<Graphics> graphics;
  if (ipVars_.debug && Graphics::guiActive()) {
    graphics = std::make_unique<Graphics>(log_, pbc_, pbVec_);
  }

  placeInstsCenter();

  // set ExtId for idx reference // easy recovery
  setPlaceInstExtId();

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
                           instLocVecY_,
                           log_);
    float error_max = std::max(error.x, error.y);
    log_->report("[InitialPlace]  Iter: {} CG residual: {:0.8f} HPWL: {}",
                 iter,
                 error_max,
                 pbc_->hpwl());
    updateCoordi();

    if (graphics) {
      graphics->cellPlot(true);
    }

    if (error_max <= 1e-5 && iter >= 5) {
      break;
    }
  }
}

// starting point of initial place is center.
void InitialPlace::placeInstsCenter()
{
  const int centerX = pbc_->die().coreCx();
  const int centerY = pbc_->die().coreCy();

  for (auto& inst : pbc_->placeInsts()) {
    if (!inst->isLocked()) {
      const auto db_inst = inst->dbInst();
      const auto group = db_inst->getGroup();
      if (group && group->getType() == odb::dbGroupType::POWER_DOMAIN) {
        auto domain_region = group->getRegion();
        int domain_xMin = std::numeric_limits<int>::max();
        int domain_yMin = std::numeric_limits<int>::max();
        int domain_xMax = std::numeric_limits<int>::min();
        int domain_yMax = std::numeric_limits<int>::min();
        for (auto boundary : domain_region->getBoundaries()) {
          domain_xMin = std::min(domain_xMin, boundary->xMin());
          domain_yMin = std::min(domain_yMin, boundary->yMin());
          domain_xMax = std::max(domain_xMax, boundary->xMax());
          domain_yMax = std::max(domain_yMax, boundary->yMax());
        }
        inst->setCenterLocation(domain_xMax - (domain_xMax - domain_xMin) / 2,
                                domain_yMax - (domain_yMax - domain_yMin) / 2);
      } else if (ipVars_.maxIter == 0 && db_inst->isPlaced()) {
        // It is helpful to pick up the placement from mpl2 if available,
        // particularly when you are going to skip initial placement
        // (eg skip_io).
        const auto bbox = db_inst->getBBox()->getBox();
        inst->setCenterLocation(bbox.xCenter(), bbox.yCenter());
      } else {
        inst->setCenterLocation(centerX, centerY);
      }
    }
  }
}

void InitialPlace::setPlaceInstExtId()
{
  // reset ExtId for all instances
  for (auto& inst : pbc_->insts()) {
    inst->setExtId(INT_MAX);
  }
  // set index only with place-able instances
  for (auto& inst : pbc_->placeInsts()) {
    inst->setExtId(&inst - pbc_->placeInsts().data());
  }
}

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

// solve placeInstForceMatrixX_ * xcg_x_ = xcg_b_ and placeInstForceMatrixY_ *
// ycg_x_ = ycg_b_ eq.
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
    int idx = inst->extId();

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
            const int inst1 = pin1->instance()->extId();
            const int inst2 = pin2->instance()->extId();

            listX.emplace_back(inst1, inst1, weightX);
            listX.emplace_back(inst2, inst2, weightX);

            listX.emplace_back(inst1, inst2, -weightX);
            listX.emplace_back(inst2, inst1, -weightX);

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
            const int inst2 = pin2->instance()->extId();
            listX.emplace_back(inst2, inst2, weightX);

            fixedInstForceVecX_(inst2)
                += weightX
                   * (pin1->cx() - (pin2->cx() - pin2->instance()->cx()));
          }
          // pin1 from Instance / pin2 from IO port
          else if (pin1->isPlaceInstConnected()
                   && !pin2->isPlaceInstConnected()) {
            const int inst1 = pin1->instance()->extId();
            listX.emplace_back(inst1, inst1, weightX);

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
            const int inst1 = pin1->instance()->extId();
            const int inst2 = pin2->instance()->extId();

            listY.emplace_back(inst1, inst1, weightY);
            listY.emplace_back(inst2, inst2, weightY);

            listY.emplace_back(inst1, inst2, -weightY);
            listY.emplace_back(inst2, inst1, -weightY);

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
            const int inst2 = pin2->instance()->extId();
            listY.emplace_back(inst2, inst2, weightY);

            fixedInstForceVecY_(inst2)
                += weightY
                   * (pin1->cy() - (pin2->cy() - pin2->instance()->cy()));
          }
          // pin1 from Instance / pin2 from IO port
          else if (pin1->isPlaceInstConnected()
                   && !pin2->isPlaceInstConnected()) {
            const int inst1 = pin1->instance()->extId();
            listY.emplace_back(inst1, inst1, weightY);

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
  for (auto& inst : pbc_->placeInsts()) {
    int idx = inst->extId();
    if (!inst->isLocked()) {
      inst->dbSetCenterLocation(instLocVecX_(idx), instLocVecY_(idx));
      inst->dbSetPlaced();
    }
  }
}

}  // namespace gpl
