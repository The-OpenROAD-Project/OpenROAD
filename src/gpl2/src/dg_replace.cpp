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

#include "gpl2/DgReplace.h"

#include "routeBase.h"
#include "timingBase.h"
#include "initialPlace.h"
#include "nesterovPlace.h"
#include "odb/db.h"
#include "placerBase.h"
#include "rsz/Resizer.hh"
#include "utl/Logger.h"

namespace gpl2 {

using utl::GPL2;

DgReplace::DgReplace()
    : db_(nullptr),
      rs_(nullptr),
      fr_(nullptr),
      log_(nullptr),
      pbc_(nullptr),
      rb_(nullptr),
      tb_(nullptr),
      ip_(nullptr),
      np_(nullptr),
      initialPlaceMaxIter_(20),
      initialPlaceMinDiffLength_(1500),
      initialPlaceMaxSolverIter_(100),
      initialPlaceMaxFanout_(200),
      initialPlaceNetWeightScale_(800),
      nesterovPlaceMaxIter_(5000),
      binGridCntX_(0),
      binGridCntY_(0),
      overflow_(0.1),
      density_(1.0),
      initDensityPenalityFactor_(0.00008),
      initWireLengthCoef_(0.25),
      minPhiCoef_(0.95),
      maxPhiCoef_(1.05),
      referenceHpwl_(446000000),
      routabilityCheckOverflow_(0.20),
      routabilityMaxDensity_(0.99),
      routabilityTargetRcMetric_(1.25),
      routabilityInflationRatioCoef_(2.5),
      routabilityMaxInflationRatio_(2.5),
      routabilityRcK1_(1.0),
      routabilityRcK2_(1.0),
      routabilityRcK3_(0.0),
      routabilityRcK4_(0.0),
      routabilityMaxBloatIter_(1),
      routabilityMaxInflationIter_(4),
      timingNetWeightMax_(1.9),
      timingDrivenMode_(true),
      routabilityDrivenMode_(true),
      uniformTargetDensityMode_(false),
      skipIoMode_(false),
      datapathFlag_(false),
      dataflowFlag_(false),
      clusterConstraintFlag_(false),
      padLeft_(0),
      padRight_(0){};

DgReplace::~DgReplace()
{
}

void DgReplace::init(sta::dbNetwork* network,
                      odb::dbDatabase* odb,
                      rsz::Resizer* resizer,
                      grt::GlobalRouter* router,
                      utl::Logger* logger)
{
  network_ = network;
  db_ = odb;
  rs_ = resizer;
  fr_ = router;
  log_ = logger;
}

void DgReplace::reset()
{
  ip_.reset();
  np_.reset();

  pbc_.reset();
  pbVec_.clear();
  pbVec_.shrink_to_fit();

  tb_.reset();
  rb_.reset();

  initialPlaceMaxIter_ = 20;
  initialPlaceMinDiffLength_ = 1500;
  initialPlaceMaxSolverIter_ = 100;
  initialPlaceMaxFanout_ = 200;
  initialPlaceNetWeightScale_ = 800;

  nesterovPlaceMaxIter_ = 5000;
  binGridCntX_ = binGridCntY_ = 0;
  overflow_ = 0.1;
  density_ = 1.0;
  initDensityPenalityFactor_ = 0.00008;
  initWireLengthCoef_ = 0.25;
  minPhiCoef_ = 0.95;
  maxPhiCoef_ = 1.05;
  referenceHpwl_ = 446000000;

  routabilityCheckOverflow_ = 0.20;
  routabilityMaxDensity_ = 0.99;
  routabilityTargetRcMetric_ = 1.25;
  routabilityInflationRatioCoef_ = 2.5;
  routabilityMaxInflationRatio_ = 2.5;
  routabilityRcK1_ = routabilityRcK2_ = 1.0;
  routabilityRcK3_ = routabilityRcK4_ = 0.0;
  routabilityMaxBloatIter_ = 1;
  routabilityMaxInflationIter_ = 4;

  timingDrivenMode_ = true;
  routabilityDrivenMode_ = true;
  uniformTargetDensityMode_ = false;
  skipIoMode_ = false;

  padLeft_ = padRight_ = 0;

  timingNetWeightOverflows_.clear();
  timingNetWeightOverflows_.shrink_to_fit();
  timingNetWeightMax_ = 1.9;
}

void DgReplace::doInitialPlace()
{
  PlacerBaseVars pbVars;
  pbVars.padLeft = padLeft_;
  pbVars.padRight = padRight_;
  pbVars.skipIoMode = skipIoMode_;

  NesterovBaseVars nbVars;
  nbVars.targetDensity = density_;
  nbVars.useUniformTargetDensity = uniformTargetDensityMode_;

  InitialPlaceVars ipVars;
  ipVars.maxIter = initialPlaceMaxIter_;
  ipVars.minDiffLength = initialPlaceMinDiffLength_;
  ipVars.maxSolverIter = initialPlaceMaxSolverIter_;
  ipVars.maxFanout = initialPlaceMaxFanout_;
  ipVars.netWeightScale = initialPlaceNetWeightScale_;

  NesterovPlaceVars npVars;
  npVars.minPhiCoef = minPhiCoef_;
  npVars.maxPhiCoef = maxPhiCoef_;
  npVars.referenceHpwl = referenceHpwl_;
  npVars.routabilityCheckOverflow = routabilityCheckOverflow_;
  npVars.initDensityPenalty = initDensityPenalityFactor_;
  npVars.initWireLengthCoef = initWireLengthCoef_;
  npVars.targetOverflow = overflow_;
  npVars.maxNesterovIter = nesterovPlaceMaxIter_;
  npVars.timingDrivenMode = timingDrivenMode_;
  npVars.routabilityDrivenMode = routabilityDrivenMode_;

  std::unique_ptr<InitialPlace> ip(new InitialPlace(
      ipVars, haloWidth_, numHops_, dataflowFlag_, network_, db_, log_));
  ip_ = std::move(ip);
  ip_->setPlacerBaseVars(pbVars);
  ip_->setNesterovBaseVars(nbVars);
  ip_->setNesterovPlaceVars(npVars);
  ip_->doInitialPlace();
}

bool DgReplace::initNesterovPlace()
{
  if (pbc_ == nullptr) {
    PlacerBaseVars pbVars;
    pbVars.padLeft = padLeft_;
    pbVars.padRight = padRight_;
    pbVars.skipIoMode = skipIoMode_;

    NesterovBaseVars nbVars;
    nbVars.targetDensity = density_;
    nbVars.useUniformTargetDensity = uniformTargetDensityMode_;
    if (binGridCntX_ != 0 && binGridCntY_ != 0) {
      nbVars.isSetBinCnt = 1;
      nbVars.binCntX = binGridCntX_;
      nbVars.binCntY = binGridCntY_;
    }

    // for standard cell placement
    const int virtualIter = 0;
    const int numHops = 0;
    const float bloatFactor = 1.0;
    const bool clusterFlag = false;
    const bool dataflowFlag = false;
    const bool datapathFlag = false;
    const bool clusterConstraintFlag = false;
    pbc_ = std::make_shared<PlacerBaseCommon>(network_,
                                              db_,
                                              pbVars,
                                              log_,
                                              haloWidth_,
                                              virtualIter,
                                              numHops,
                                              bloatFactor,
                                              clusterFlag,
                                              dataflowFlag,
                                              datapathFlag,
                                              clusterConstraintFlag);
    pbVec_.push_back(std::make_shared<PlacerBase>(nbVars, db_, pbc_, log_));
    // Each power domain is represented by a group
    for (auto pd : db_->getChip()->getBlock()->getPowerDomains()) {
      if (pd->getGroup()) {
        pbVec_.push_back(std::make_shared<PlacerBase>(
            nbVars, db_, pbc_, log_, pd->getGroup()));
      }
    }

    // check how many place instances
    totalPlaceableInsts_ = 0;
    for (const auto& pb : pbVec_) {
      totalPlaceableInsts_ += pb->numPlaceInsts();
    }

    if (totalPlaceableInsts_ == 0) {
      log_->warn(GPL2, 136, "No placeable instances - skipping placement.");
      return false;
    }
  }

  if (!np_) {
    NesterovPlaceVars npVars;

    npVars.minPhiCoef = minPhiCoef_;
    npVars.maxPhiCoef = maxPhiCoef_;
    npVars.referenceHpwl = referenceHpwl_;
    npVars.routabilityCheckOverflow = routabilityCheckOverflow_;
    npVars.initDensityPenalty = initDensityPenalityFactor_;
    npVars.initWireLengthCoef = initWireLengthCoef_;
    npVars.targetOverflow = overflow_;
    npVars.maxNesterovIter = nesterovPlaceMaxIter_;
    npVars.timingDrivenMode = timingDrivenMode_;
    npVars.routabilityDrivenMode = routabilityDrivenMode_;

    for (const auto& pb : pbVec_) {
      pb->setNpVars(npVars);
    }

    // TODO:  we do not have timing-driven or routability-driven mode
    rb_ = nullptr;
    tb_ = nullptr;

    std::unique_ptr<NesterovPlace> np(
        new NesterovPlace(npVars, pbc_, pbVec_, rb_, tb_, log_));
    np_ = std::move(np);
  }

  return true;
}

int DgReplace::doNesterovPlace(int start_iter)
{
  if (!initNesterovPlace()) {
    return 0;
  }

  int val = np_->doNesterovPlace(start_iter);

  auto block = db_->getChip()->getBlock();
  for (odb::dbBTerm* bterm : block->getBTerms()) {
    odb::dbProperty::destroyProperties(bterm);
  }

  return val;
}

void DgReplace::setInitialPlaceMaxIter(int iter)
{
  initialPlaceMaxIter_ = iter;
}

void DgReplace::setInitialPlaceMinDiffLength(int length)
{
  initialPlaceMinDiffLength_ = length;
}

void DgReplace::setInitialPlaceMaxSolverIter(int iter)
{
  initialPlaceMaxSolverIter_ = iter;
}

void DgReplace::setInitialPlaceMaxFanout(int fanout)
{
  initialPlaceMaxFanout_ = fanout;
}

void DgReplace::setInitialPlaceNetWeightScale(float scale)
{
  initialPlaceNetWeightScale_ = scale;
}

void DgReplace::setNesterovPlaceMaxIter(int iter)
{
  nesterovPlaceMaxIter_ = iter;
}

void DgReplace::setBinGridCnt(int binGridCntX, int binGridCntY)
{
  binGridCntX_ = binGridCntX;
  binGridCntY_ = binGridCntY;
}

void DgReplace::setTargetOverflow(float overflow)
{
  overflow_ = overflow;
}

void DgReplace::setTargetDensity(float density)
{
  density_ = density;
}

void DgReplace::setUniformTargetDensityMode(bool mode)
{
  uniformTargetDensityMode_ = mode;
}

// TODO: update to be compatible with multiple target densities
float DgReplace::getUniformTargetDensity()
{
  return 0.0;
}

void DgReplace::setInitDensityPenalityFactor(float penaltyFactor)
{
  initDensityPenalityFactor_ = penaltyFactor;
}

void DgReplace::setInitWireLengthCoef(float coef)
{
  initWireLengthCoef_ = coef;
}

void DgReplace::setMinPhiCoef(float minPhiCoef)
{
  minPhiCoef_ = minPhiCoef;
}

void DgReplace::setMaxPhiCoef(float maxPhiCoef)
{
  maxPhiCoef_ = maxPhiCoef;
}

void DgReplace::setReferenceHpwl(float refHpwl)
{
  referenceHpwl_ = refHpwl;
}

void DgReplace::setSkipIoMode(bool mode)
{
  skipIoMode_ = mode;
}

void DgReplace::setTimingDrivenMode(bool mode)
{
  timingDrivenMode_ = mode;
}

void DgReplace::setRoutabilityDrivenMode(bool mode)
{
  routabilityDrivenMode_ = mode;
}

void DgReplace::setRoutabilityCheckOverflow(float overflow)
{
  routabilityCheckOverflow_ = overflow;
}

void DgReplace::setRoutabilityMaxDensity(float density)
{
  routabilityMaxDensity_ = density;
}

void DgReplace::setRoutabilityMaxBloatIter(int iter)
{
  routabilityMaxBloatIter_ = iter;
}

void DgReplace::setRoutabilityMaxInflationIter(int iter)
{
  routabilityMaxInflationIter_ = iter;
}

void DgReplace::setRoutabilityTargetRcMetric(float rc)
{
  routabilityTargetRcMetric_ = rc;
}

void DgReplace::setRoutabilityInflationRatioCoef(float coef)
{
  routabilityInflationRatioCoef_ = coef;
}

void DgReplace::setRoutabilityMaxInflationRatio(float ratio)
{
  routabilityMaxInflationRatio_ = ratio;
}

void DgReplace::setRoutabilityRcCoefficients(float k1,
                                              float k2,
                                              float k3,
                                              float k4)
{
  routabilityRcK1_ = k1;
  routabilityRcK2_ = k2;
  routabilityRcK3_ = k3;
  routabilityRcK4_ = k4;
}

void DgReplace::setPadLeft(int pad)
{
  padLeft_ = pad;
}

void DgReplace::setPadRight(int pad)
{
  padRight_ = pad;
}

void DgReplace::setHaloWidth(float haloWidth)
{
  haloWidth_ = haloWidth;
}

void DgReplace::setVirtualIter(int iter)
{
  virtualIter_ = iter;
}

void DgReplace::setNumHops(int numHops)
{
  numHops_ = numHops;
}

void DgReplace::setDataflowFlag(bool flag)
{
  dataflowFlag_ = flag;
}

void DgReplace::setDatapathFlag(bool flag)
{
  datapathFlag_ = flag;
}

void DgReplace::setClusterConstraintFlag(bool flag)
{
  clusterConstraintFlag_ = flag;
}

void DgReplace::addTimingNetWeightOverflow(int overflow)
{
  timingNetWeightOverflows_.push_back(overflow);
}

void DgReplace::setTimingNetWeightMax(float max)
{
  timingNetWeightMax_ = max;
}

}  // namespace gpl2
