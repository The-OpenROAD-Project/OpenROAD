// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#include "initialPlace.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "AbstractGraphics.h"
#include "odb/dbTypes.h"
#include "placerBase.h"
#include "solver.h"
#include "utl/Logger.h"

namespace gpl {

using T = Eigen::Triplet<float>;

InitialPlaceVars::InitialPlaceVars(const PlaceOptions& options,
                                   const bool debug)
    : maxIter(options.initialPlaceMaxIter),
      minDiffLength(options.initialPlaceMinDiffLength),
      maxSolverIter(options.initialPlaceMaxSolverIter),
      maxFanout(options.initialPlaceMaxFanout),
      netWeightScale(options.initialPlaceNetWeightScale),
      debug(debug),
      forceCenter(options.forceCenterInitialPlace)
{
}

InitialPlace::InitialPlace(InitialPlaceVars ipVars,
                           std::shared_ptr<PlacerBaseCommon> pbc,
                           std::vector<std::shared_ptr<PlacerBase>>& pbVec,
                           std::unique_ptr<AbstractGraphics> graphics,
                           utl::Logger* log)
    : ipVars_(ipVars),
      pbc_(std::move(pbc)),
      pbVec_(pbVec),
      graphics_(std::move(graphics)),
      log_(log)
{
}

void InitialPlace::doBicgstabPlace(int threads)
{
  log_->info(utl::GPL, 5, "---- Execute Conjugate Gradient Initial Placement.");
  ResidualError error;

  if (graphics_) {
    graphics_->setDebugOn(ipVars_.debug);
  }
  const bool graphics_enabled = graphics_ && graphics_->enabled();
  if (graphics_enabled) {
    graphics_->debugForInitialPlace(pbc_, pbVec_);
  }

  placeInstsInitialPositions();

  // set ExtId for idx reference // easy recovery
  setPlaceInstExtId();

  if (graphics_enabled) {
    gif_key_ = graphics_->gifStart("initPlacement.gif");
  }

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
                           log_,
                           threads);

    if (graphics_enabled) {
      graphics_->cellPlot(true);

      odb::Rect region;
      odb::Rect bbox = pbc_->db()->getChip()->getBlock()->getBBox()->getBox();
      int max_dim = std::max(bbox.dx(), bbox.dy());
      double dbu_per_pixel = static_cast<double>(max_dim) / 1000.0;
      graphics_->gifAddFrame(gif_key_, region, 500, dbu_per_pixel, 20);
    }

    if (std::isnan(error.x) || std::isnan(error.y)) {
      log_->warn(utl::GPL,
                 325,
                 "Conjugate gradient initial placement solver failed at "
                 "iteration {}. ",
                 iter);
      break;
    }

    float error_max = std::max(error.x, error.y);
    log_->report(
        "[InitialPlace]  Iter: {} conjugate gradient residual: {:0.8f} HPWL: "
        "{}",
        iter,
        error_max,
        pbc_->getHpwl());
    updateCoordi();

    if (error_max <= 1e-5 && iter >= 5) {
      break;
    }
  }

  if (graphics_enabled) {
    graphics_->gifEnd(gif_key_);
    graphics_->setDebugOn(false);
    graphics_->cellPlot(false);
  }
}

// starting point of initial place is center.
void InitialPlace::placeInstsInitialPositions()
{
  const int core_center_x = pbc_->getDie().coreCx();
  const int core_center_y = pbc_->getDie().coreCy();

  int count_region_center = 0;
  int count_db_location = 0;
  int count_core_center = 0;

  for (auto& inst : pbc_->placeInsts()) {
    if (inst->isLocked()) {
      continue;
    }

    const auto db_inst = inst->dbInst();
    const auto group = db_inst->getGroup();

    if (group && group->getRegion()) {
      auto region = group->getRegion();
      int region_x_min = std::numeric_limits<int>::max();
      int region_y_min = std::numeric_limits<int>::max();
      int region_x_max = std::numeric_limits<int>::min();
      int region_y_max = std::numeric_limits<int>::min();

      for (auto boundary : region->getBoundaries()) {
        region_x_min = std::min(region_x_min, boundary->xMin());
        region_y_min = std::min(region_y_min, boundary->yMin());
        region_x_max = std::max(region_x_max, boundary->xMax());
        region_y_max = std::max(region_y_max, boundary->yMax());
      }

      inst->setCenterLocation(region_x_max - (region_x_max - region_x_min) / 2,
                              region_y_max - (region_y_max - region_y_min) / 2);
      ++count_region_center;
    } else if (!ipVars_.forceCenter && db_inst->isPlaced()) {
      const auto bbox = db_inst->getBBox()->getBox();
      inst->setCenterLocation(bbox.xCenter(), bbox.yCenter());
      ++count_db_location;
    } else {
      inst->setCenterLocation(core_center_x, core_center_y);
      ++count_core_center;
    }
  }

  log_->info(utl::GPL,
             51,
             "Source of initial instance position counters:\n"
             "\tOdb location = {}"
             "\tCore center = {}"
             "\tRegion center = {}",
             count_db_location,
             count_core_center,
             count_region_center);
}

void InitialPlace::setPlaceInstExtId()
{
  // reset ExtId for all instances
  for (auto& inst : pbc_->getInsts()) {
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
  for (auto& pin : pbc_->getPins()) {
    pin->unsetMinPinX();
    pin->unsetMinPinY();
    pin->unsetMaxPinX();
    pin->unsetMaxPinY();
  }

  for (auto& net : pbc_->getNets()) {
    Pin *pinMinX = nullptr, *pinMinY = nullptr;
    Pin *pinMaxX = nullptr, *pinMaxY = nullptr;
    int lx = INT_MAX, ly = INT_MAX;
    int ux = INT_MIN, uy = INT_MIN;

    // Mark B2B info on Pin structures
    for (auto& pin : net->getPins()) {
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
    int idx = inst->getExtId();

    instLocVecX_(idx) = inst->cx();
    instLocVecY_(idx) = inst->cy();

    fixedInstForceVecX_(idx) = fixedInstForceVecY_(idx) = 0;
  }

  // for each net
  for (auto& net : pbc_->getNets()) {
    // skip for small nets.
    if (net->getPins().size() <= 1) {
      continue;
    }

    // escape long time cals on huge fanout.
    //
    if (net->getPins().size() >= ipVars_.maxFanout) {
      continue;
    }

    float netWeight = ipVars_.netWeightScale / (net->getPins().size() - 1);

    // foreach two pins in single nets.
    auto& pins = net->getPins();
    for (int pinIdx1 = 1; pinIdx1 < pins.size(); ++pinIdx1) {
      Pin* pin1 = pins[pinIdx1];
      for (int pinIdx2 = 0; pinIdx2 < pinIdx1; ++pinIdx2) {
        Pin* pin2 = pins[pinIdx2];

        // no need to fill in when instance is same
        if (pin1->getInstance() == pin2->getInstance()) {
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
            const int inst1 = pin1->getInstance()->getExtId();
            const int inst2 = pin2->getInstance()->getExtId();

            listX.emplace_back(inst1, inst1, weightX);
            listX.emplace_back(inst2, inst2, weightX);

            listX.emplace_back(inst1, inst2, -weightX);
            listX.emplace_back(inst2, inst1, -weightX);

            fixedInstForceVecX_(inst1)
                += -weightX
                   * ((pin1->cx() - pin1->getInstance()->cx())
                      - (pin2->cx() - pin2->getInstance()->cx()));

            fixedInstForceVecX_(inst2)
                += -weightX
                   * ((pin2->cx() - pin2->getInstance()->cx())
                      - (pin1->cx() - pin1->getInstance()->cx()));
          }
          // pin1 from IO port / pin2 from Instance
          else if (!pin1->isPlaceInstConnected()
                   && pin2->isPlaceInstConnected()) {
            const int inst2 = pin2->getInstance()->getExtId();
            listX.emplace_back(inst2, inst2, weightX);

            fixedInstForceVecX_(inst2)
                += weightX
                   * (pin1->cx() - (pin2->cx() - pin2->getInstance()->cx()));
          }
          // pin1 from Instance / pin2 from IO port
          else if (pin1->isPlaceInstConnected()
                   && !pin2->isPlaceInstConnected()) {
            const int inst1 = pin1->getInstance()->getExtId();
            listX.emplace_back(inst1, inst1, weightX);

            fixedInstForceVecX_(inst1)
                += weightX
                   * (pin2->cx() - (pin1->cx() - pin1->getInstance()->cx()));
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
            const int inst1 = pin1->getInstance()->getExtId();
            const int inst2 = pin2->getInstance()->getExtId();

            listY.emplace_back(inst1, inst1, weightY);
            listY.emplace_back(inst2, inst2, weightY);

            listY.emplace_back(inst1, inst2, -weightY);
            listY.emplace_back(inst2, inst1, -weightY);

            fixedInstForceVecY_(inst1)
                += -weightY
                   * ((pin1->cy() - pin1->getInstance()->cy())
                      - (pin2->cy() - pin2->getInstance()->cy()));

            fixedInstForceVecY_(inst2)
                += -weightY
                   * ((pin2->cy() - pin2->getInstance()->cy())
                      - (pin1->cy() - pin1->getInstance()->cy()));
          }
          // pin1 from IO port / pin2 from Instance
          else if (!pin1->isPlaceInstConnected()
                   && pin2->isPlaceInstConnected()) {
            const int inst2 = pin2->getInstance()->getExtId();
            listY.emplace_back(inst2, inst2, weightY);

            fixedInstForceVecY_(inst2)
                += weightY
                   * (pin1->cy() - (pin2->cy() - pin2->getInstance()->cy()));
          }
          // pin1 from Instance / pin2 from IO port
          else if (pin1->isPlaceInstConnected()
                   && !pin2->isPlaceInstConnected()) {
            const int inst1 = pin1->getInstance()->getExtId();
            listY.emplace_back(inst1, inst1, weightY);

            fixedInstForceVecY_(inst1)
                += weightY
                   * (pin2->cy() - (pin1->cy() - pin1->getInstance()->cy()));
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
    int idx = inst->getExtId();
    if (!inst->isLocked()) {
      int new_x = instLocVecX_(idx);
      int new_y = instLocVecY_(idx);

      // Constrain to core area
      const auto& die = pbc_->getDie();
      new_x = std::max(new_x, die.coreLx());
      new_x = std::min(new_x, die.coreUx());
      new_y = std::max(new_y, die.coreLy());
      new_y = std::min(new_y, die.coreUy());

      // If instance has a region constraint, use that instead
      const auto db_inst = inst->dbInst();
      const auto group = db_inst->getGroup();
      if (group && group->getRegion()) {
        auto region = group->getRegion();
        int region_x_min = std::numeric_limits<int>::max();
        int region_y_min = std::numeric_limits<int>::max();
        int region_x_max = std::numeric_limits<int>::min();
        int region_y_max = std::numeric_limits<int>::min();

        for (auto boundary : region->getBoundaries()) {
          region_x_min = std::min(region_x_min, boundary->xMin());
          region_y_min = std::min(region_y_min, boundary->yMin());
          region_x_max = std::max(region_x_max, boundary->xMax());
          region_y_max = std::max(region_y_max, boundary->yMax());
        }

        new_x = std::max(new_x, region_x_min);
        new_x = std::min(new_x, region_x_max);
        new_y = std::max(new_y, region_y_min);
        new_y = std::min(new_y, region_y_max);
      }

      inst->dbSetCenterLocation(new_x, new_y);
      inst->dbSetPlaced();
    }
  }
}

}  // namespace gpl
