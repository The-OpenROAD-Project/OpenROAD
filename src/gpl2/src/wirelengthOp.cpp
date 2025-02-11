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

#include "wirelengthOp.h"

#include "placerBase.h"
#include "placerObjects.h"

#include <Kokkos_Core.hpp>

#include <climits>
#include <cmath>

namespace gpl2 {

//////////////////////////////////////////////////////////////
// Class WirelengthOp

WirelengthOp::WirelengthOp()
    : pbc_(nullptr),
      logger_(nullptr),
      numInsts_(0),
      numPins_(0),
      numNets_(0),
      numPlaceInsts_(0)
{
}

WirelengthOp::WirelengthOp(PlacerBaseCommon* pbc) : WirelengthOp()
{
  pbc_ = pbc;
  logger_ = pbc_->logger();
  logger_->report("[WirelengthOp] Start Initialization.");

  // placeable instances + fixed instances
  numInsts_ = pbc_->numInsts();
  numPlaceInsts_ = pbc_->numPlaceInsts();
  numPins_ = pbc_->numPins();
  numNets_ = pbc_->numNets();

  initBackend();
  logger_->report("[WirelengthOp] Initialization Succeed.");
}

/////////////////////////////////////////////////////////
// Class WirelengthOp
void WirelengthOp::initBackend()
{
  // Initialize related information
  size_t instPinCount = 0;
  size_t netPinCount = 0;
  for (auto& inst : pbc_->insts()) {
    instPinCount += inst->numPins();
  }
  for (auto& net : pbc_->nets()) {
    netPinCount += net->numPins();
  }

  Kokkos::View<int*, Kokkos::HostSpace> hInstPinIdx("hInstPinIdx", instPinCount);
  Kokkos::View<int*, Kokkos::HostSpace> hInstPinPos("hInstPinPos", numInsts_ + 1);
  Kokkos::View<int*, Kokkos::HostSpace> hPinInstId("hPinInstId", numPins_);

  Kokkos::View<int*, Kokkos::HostSpace> hNetPinIdx("hNetPinIdx", netPinCount);
  Kokkos::View<int*, Kokkos::HostSpace> hNetPinPos("hNetPinPos", numNets_ + 1);
  Kokkos::View<int*, Kokkos::HostSpace> hPinNetId("hPinNetId", numPins_);

  Kokkos::View<float*, Kokkos::HostSpace> hNetWeight("hNetWeight", numNets_);
  Kokkos::View<float*, Kokkos::HostSpace> hNetVirtualWeight("hNetVirtualWeight", numNets_);

  int pinIdx = 0;
  for (auto pin : pbc_->pins()) {
    hPinInstId[pinIdx] = pin->instId();
    hPinNetId[pinIdx] = pin->netId();
    pinIdx++;
  }

  int instIdx = 0;
  int instPinIdx = 0;
  hInstPinPos[0] = 0;
  for (auto& inst : pbc_->insts()) {
    for (auto& pin : inst->pins()) {
      hInstPinIdx[instPinIdx++] = pin->pinId();
    }
    hInstPinPos[instIdx + 1] = hInstPinPos[instIdx] + inst->numPins();
    instIdx++;
  }

  int netIdx = 0;
  int netPinIdx = 0;
  hNetPinPos[0] = 0;
  for (auto& net : pbc_->nets()) {
    for (auto& pin : net->pins()) {
      hNetPinIdx[netPinIdx++] = pin->pinId();
    }

    hNetWeight[netIdx] = net->weight();
    hNetVirtualWeight[netIdx] = net->virtualWeight();
    hNetPinPos[netIdx + 1] = hNetPinPos[netIdx] + net->numPins();
    netIdx++;
  }

  // Allocate memory on the device side
  dInstPinIdx_ = Kokkos::View<int*>("dInstPinIdx", hInstPinIdx.size());
  dInstPinPos_ = Kokkos::View<int*>("dInstPinPos", numInsts_ + 1);
  dPinInstId_ = Kokkos::View<int*>("dPinInstId", numPins_);

  dNetPinIdx_ = Kokkos::View<int*>("dNetPinIdx", hNetPinIdx.size());
  dNetWeight_ = Kokkos::View<float*>("dNetWeight", numNets_);
  dNetVirtualWeight_ = Kokkos::View<float*>("dNetVirtualWeight", numNets_);
  dNetPinPos_ = Kokkos::View<int*>("dNetPinPos", numNets_ + 1);
  dPinNetId_ = Kokkos::View<int*>("dPinNetId", numPins_);

  // copy from host to device
  Kokkos::deep_copy(dInstPinIdx_, hInstPinIdx);
  Kokkos::deep_copy(dInstPinPos_, hInstPinPos);
  Kokkos::deep_copy(dPinInstId_, hPinInstId);

  Kokkos::deep_copy(dNetWeight_, hNetWeight);
  Kokkos::deep_copy(dNetVirtualWeight_, hNetVirtualWeight);

  Kokkos::deep_copy(dNetPinIdx_, hNetPinIdx);
  Kokkos::deep_copy(dNetPinPos_, hNetPinPos);
  Kokkos::deep_copy(dPinNetId_, hPinNetId);

  // Check the pin information
  Kokkos::View<int*, Kokkos::HostSpace> hPinX("hPinX", numPins_);
  Kokkos::View<int*, Kokkos::HostSpace> hPinY("hPinY", numPins_);
  Kokkos::View<int*, Kokkos::HostSpace> hPinOffsetX("hPinOffsetX", numPins_);
  Kokkos::View<int*, Kokkos::HostSpace> hPinOffsetY("hPinOffsetY", numPins_);

  // This is for fixed instances
  for (auto& pin : pbc_->pins()) {
    const int pinId = pin->pinId();
    hPinX[pinId] = pin->cx();
    hPinY[pinId] = pin->cy();
    hPinOffsetX[pinId] = pin->offsetCx();
    hPinOffsetY[pinId] = pin->offsetCy();
  }

  // allocate memory on the device side
  dPinX_ = Kokkos::View<int*>("dPinX", numPins_);
  dPinY_ = Kokkos::View<int*>("dPinY", numPins_);
  dPinOffsetX_ = Kokkos::View<int*>("dPinOffsetX", numPins_);
  dPinOffsetY_ = Kokkos::View<int*>("dPinOffsetY", numPins_);
  dPinGradX_ = Kokkos::View<float*>("dPinGradX", numPins_);
  dPinGradY_ = Kokkos::View<float*>("dPinGradY", numPins_);

  dPinAPosX_ = Kokkos::View<float*>("dPinAPosX", numPins_);
  dPinANegX_ = Kokkos::View<float*>("dPinANegX", numPins_);
  dPinAPosY_ = Kokkos::View<float*>("dPinAPosY", numPins_);
  dPinANegY_ = Kokkos::View<float*>("dPinANegY", numPins_);
  dNetBPosX_ = Kokkos::View<float*>("dNetBPosX", numNets_);
  dNetBNegX_ = Kokkos::View<float*>("dNetBNegX", numNets_);
  dNetBPosY_ = Kokkos::View<float*>("dNetBPosY", numNets_);
  dNetBNegY_ = Kokkos::View<float*>("dNetBNegY", numNets_);
  dNetCPosX_ = Kokkos::View<float*>("dNetCPosX", numNets_);
  dNetCNegX_ = Kokkos::View<float*>("dNetCNegX", numNets_);
  dNetCPosY_ = Kokkos::View<float*>("dNetCPosY", numNets_);
  dNetCNegY_ = Kokkos::View<float*>("dNetCNegY", numNets_);

  dNetLx_ = Kokkos::View<int*>("dNetLx", numNets_);
  dNetLy_ = Kokkos::View<int*>("dNetLy", numNets_);
  dNetUx_ = Kokkos::View<int*>("dNetUx", numNets_);
  dNetUy_ = Kokkos::View<int*>("dNetUy", numNets_);
  dNetWidth_ = Kokkos::View<int*>("dNetWidth", numNets_);
  dNetHeight_ = Kokkos::View<int*>("dNetHeight", numNets_);

  // copy from host to device
  Kokkos::deep_copy(dPinX_, hPinX);
  Kokkos::deep_copy(dPinY_, hPinY);
  Kokkos::deep_copy(dPinOffsetX_, hPinOffsetX);
  Kokkos::deep_copy(dPinOffsetY_, hPinOffsetY);
}

// All other operations only for placeable Instances
void updatePinLocationKernel(const int numPlaceInsts,
                                        const Kokkos::View<const int*>& dInstPinIdx,
                                        const Kokkos::View<const int*>& dInstPinPos,
                                        const Kokkos::View<const int*>& dPinOffsetX,
                                        const Kokkos::View<const int*>& dPinOffsetY,
                                        const Kokkos::View<const int*>& instDCx,
                                        const Kokkos::View<const int*>& instDCy,
                                        const Kokkos::View<int*>& dPinX,
                                        const Kokkos::View<int*>& dPinY)
{
  Kokkos::parallel_for(numPlaceInsts, KOKKOS_LAMBDA (const int instId) {
    const int pinStart = dInstPinPos[instId];
    const int pinEnd = dInstPinPos[instId + 1];
    const float instDCxVal = instDCx[instId];
    const float instDCyVal = instDCy[instId];
    for (int pinId = pinStart; pinId < pinEnd; ++pinId) {
      const int pinIdx = dInstPinIdx[pinId];
      dPinX[pinIdx] = instDCxVal + dPinOffsetX[pinIdx];
      dPinY[pinIdx] = instDCyVal + dPinOffsetY[pinIdx];
    }
  });
}

void updateNetBBoxKernel(int numNets,
                                    const Kokkos::View<const int*>& dNetPinIdx,
                                    const Kokkos::View<const int*>& dNetPinPos,
                                    const Kokkos::View<const int*>& dPinX,
                                    const Kokkos::View<const int*>& dPinY,
                                    const Kokkos::View<int*>& dNetLx,
                                    const Kokkos::View<int*>& dNetLy,
                                    const Kokkos::View<int*>& dNetUx,
                                    const Kokkos::View<int*>& dNetUy,
                                    const Kokkos::View<int*>& dNetWidth,
                                    const Kokkos::View<int*>& dNetHeight)
{
  Kokkos::parallel_for(numNets, KOKKOS_LAMBDA (const int netId) {
    const int pinStart = dNetPinPos[netId];
    const int pinEnd = dNetPinPos[netId + 1];
    int netLx = INT_MAX;
    int netLy = INT_MAX;
    int netUx = 0;
    int netUy = 0;
    for (int pinId = pinStart; pinId < pinEnd; ++pinId) {
      const int pinIdx = dNetPinIdx[pinId];
      const int pinX = dPinX[pinIdx];
      const int pinY = dPinY[pinIdx];
      netLx = Kokkos::min(netLx, pinX);
      netLy = Kokkos::min(netLy, pinY);
      netUx = Kokkos::max(netUx, pinX);
      netUy = Kokkos::max(netUy, pinY);
    }

    if (netLx > netUx || netLy > netUy) {
      netLx = 0;
      netUx = 0;
      netLy = 0;
      netUy = 0;
    }

    dNetLx[netId] = netLx;
    dNetLy[netId] = netLy;
    dNetUx[netId] = netUx;
    dNetUy[netId] = netUy;
    dNetWidth[netId] = netUx - netLx;
    dNetHeight[netId] = netUy - netLy;
  });
}

void WirelengthOp::updatePinLocation(const Kokkos::View<const int*>& instDCx, const Kokkos::View<const int*>& instDCy)
{
  updatePinLocationKernel(numPlaceInsts_,
                                                     dInstPinIdx_,
                                                     dInstPinPos_,
                                                     dPinOffsetX_,
                                                     dPinOffsetY_,
                                                     instDCx,
                                                     instDCy,
                                                     dPinX_,
                                                     dPinY_);

  updateNetBBoxKernel(numNets_,
                                                    dNetPinIdx_,
                                                    dNetPinPos_,
                                                    dPinX_,
                                                    dPinY_,
                                                    dNetLx_,
                                                    dNetLy_,
                                                    dNetUx_,
                                                    dNetUy_,
                                                    dNetWidth_,
                                                    dNetHeight_);
}

struct TypeConvertor
{
  KOKKOS_FUNCTION int64_t operator()(const int& x) const
  {
    return static_cast<int64_t>(x);
  }
};

int64_t WirelengthOp::computeHPWL()
{
  int64_t hpwl = 0;
  auto dNetWidth = dNetWidth_, dNetHeight = dNetHeight_;
  Kokkos::parallel_reduce(numNets_, KOKKOS_LAMBDA (const int i, int64_t& hpwl) {
    hpwl += dNetWidth[i] + dNetHeight[i];
  }, hpwl);

  return hpwl;
}

int64_t WirelengthOp::computeWeightedHPWL(float virtualWeightFactor)
{
  int64_t hpwl = 0;
  auto dNetWidth = dNetWidth_, dNetHeight = dNetHeight_;
  auto dNetWeight = dNetWeight_;
  auto dNetVirtualWeight = dNetVirtualWeight_;
  Kokkos::parallel_reduce(numNets_, KOKKOS_LAMBDA (const int i, int64_t& hpwl) {
    hpwl += (dNetWeight[i] + dNetVirtualWeight[i] * virtualWeightFactor) * (dNetWidth[i] + dNetHeight[i]);
  }, hpwl);

  return hpwl;
}

// Compute aPos and aNeg
void computeAPosNegKernel(const int numPins,
                                     const float wlCoeffX,
                                     const float wlCoeffY,
                                     const Kokkos::View<const int*>& dPinX,
                                     const Kokkos::View<const int*>& dPinY,
                                     const Kokkos::View<const int*>& dPinNetId,
                                     const Kokkos::View<const int*>& dNetLx,
                                     const Kokkos::View<const int*>& dNetLy,
                                     const Kokkos::View<const int*>& dNetUx,
                                     const Kokkos::View<const int*>& dNetUy,
                                     const Kokkos::View<float*>& dPinAPosX,
                                     const Kokkos::View<float*>& dPinANegX,
                                     const Kokkos::View<float*>& dPinAPosY,
                                     const Kokkos::View<float*>& dPinANegY)
{
  Kokkos::parallel_for(numPins, KOKKOS_LAMBDA (const int pinId) {
    const int netId = dPinNetId[pinId];
    dPinAPosX[pinId] = expf(wlCoeffX * (dPinX[pinId] - dNetUx[netId]));
    dPinANegX[pinId]
        = expf(-1.0 * wlCoeffX * (dPinX[pinId] - dNetLx[netId]));
    dPinAPosY[pinId] = expf(wlCoeffY * (dPinY[pinId] - dNetUy[netId]));
    dPinANegY[pinId]
        = expf(-1.0 * wlCoeffY * (dPinY[pinId] - dNetLy[netId]));
  });
}

void computeBCPosNegKernel(int numNets,
                                      const Kokkos::View<const int*>& dNetPinPos,
                                      const Kokkos::View<const int*>& dNetPinIdx,
                                      const Kokkos::View<const int*>& dPinX,
                                      const Kokkos::View<const int*>& dPinY,
                                      const Kokkos::View<const float*>& dPinAPosX,
                                      const Kokkos::View<const float*>& dPinANegX,
                                      const Kokkos::View<const float*>& dPinAPosY,
                                      const Kokkos::View<const float*>& dPinANegY,
                                      const Kokkos::View<float*>& dNetBPosX,
                                      const Kokkos::View<float*>& dNetBNegX,
                                      const Kokkos::View<float*>& dNetBPosY,
                                      const Kokkos::View<float*>& dNetBNegY,
                                      const Kokkos::View<float*>& dNetCPosX,
                                      const Kokkos::View<float*>& dNetCNegX,
                                      const Kokkos::View<float*>& dNetCPosY,
                                      const Kokkos::View<float*>& dNetCNegY)
{
  using team_policy = Kokkos::TeamPolicy<>;
  using team_member = typename team_policy::member_type;

  Kokkos::parallel_for("ComputeBCPosNeg", team_policy(numNets, Kokkos::AUTO), KOKKOS_LAMBDA(const team_member& team) {
    const int netId = team.league_rank();
    const int pinStart = dNetPinPos[netId];
    const int pinEnd = dNetPinPos[netId + 1];

    float bPosX, bNegX, bPosY, bNegY;
    float cPosX, cNegX, cPosY, cNegY;


    Kokkos::parallel_reduce(Kokkos::TeamThreadRange(team, pinStart, pinEnd), KOKKOS_LAMBDA (
      const int pinId,
      float& localBPosX,
      float& localBNegX,
      float& localBPosY,
      float& localBNegY,
      float& localCPosX,
      float& localCNegX,
      float& localCPosY,
      float& localCNegY
    ) {
      const int pinIdx = dNetPinIdx[pinId];
      localBPosX += dPinAPosX[pinIdx];
      localBNegX += dPinANegX[pinIdx];
      localBPosY += dPinAPosY[pinIdx];
      localBNegY += dPinANegY[pinIdx];

      localCPosX += dPinX[pinIdx] * dPinAPosX[pinIdx];
      localCNegX += dPinX[pinIdx] * dPinANegX[pinIdx];
      localCPosY += dPinY[pinIdx] * dPinAPosY[pinIdx];
      localCNegY += dPinY[pinIdx] * dPinANegY[pinIdx];
    }, bPosX, bNegX, bPosY, bNegY, cPosX, cNegX, cPosY, cNegY);

    Kokkos::single(Kokkos::PerTeam(team), [&]() {
      dNetBPosX[netId] = bPosX;
      dNetBNegX[netId] = bNegX;
      dNetBPosY[netId] = bPosY;
      dNetBNegY[netId] = bNegY;

      dNetCPosX[netId] = cPosX;
      dNetCNegX[netId] = cNegX;
      dNetCPosY[netId] = cPosY;
      dNetCNegY[netId] = cNegY;
    });
  });
}
void computePinWAGradKernel(const int numPins,
                                       const float wlCoeffX,
                                       const float wlCoeffY,
                                       const Kokkos::View<const int*>& dPinNetId,
                                       const Kokkos::View<const int*>& dNetPinPos,
                                       const Kokkos::View<const int*>& dPinX,
                                       const Kokkos::View<const int*>& dPinY,
                                       const Kokkos::View<const float*>& dPinAPosX,
                                       const Kokkos::View<const float*>& dPinANegX,
                                       const Kokkos::View<const float*>& dPinAPosY,
                                       const Kokkos::View<const float*>& dPinANegY,
                                       const Kokkos::View<const float*>& dNetBPosX,
                                       const Kokkos::View<const float*>& dNetBNegX,
                                       const Kokkos::View<const float*>& dNetBPosY,
                                       const Kokkos::View<const float*>& dNetBNegY,
                                       const Kokkos::View<const float*>& dNetCPosX,
                                       const Kokkos::View<const float*>& dNetCNegX,
                                       const Kokkos::View<const float*>& dNetCPosY,
                                       const Kokkos::View<const float*>& dNetCNegY,
                                       const Kokkos::View<float*>& dPinGradX,
                                       const Kokkos::View<float*>& dPinGradY)
{

  Kokkos::parallel_for(numPins, KOKKOS_LAMBDA (const int pinIdx) {
    const int netId = dPinNetId[pinIdx];

    // TODO:  if we need to remove high-fanout nets,
    // we can remove it here

    float netBNegX2 = dNetBNegX[netId] * dNetBNegX[netId];
    float netBPosX2 = dNetBPosX[netId] * dNetBPosX[netId];
    float netBNegY2 = dNetBNegY[netId] * dNetBNegY[netId];
    float netBPosY2 = dNetBPosY[netId] * dNetBPosY[netId];

    float pinXWlCoeffX = dPinX[pinIdx] * wlCoeffX;
    float pinYWlCoeffY = dPinY[pinIdx] * wlCoeffY;

    dPinGradX[pinIdx] = ((1.0f - pinXWlCoeffX) * dNetBNegX[netId]
                            + wlCoeffX * dNetCNegX[netId])
                               * dPinANegX[pinIdx] / netBNegX2
                           - ((1.0f + pinXWlCoeffX) * dNetBPosX[netId]
                              - wlCoeffX * dNetCPosX[netId])
                                 * dPinAPosX[pinIdx] / netBPosX2;

    dPinGradY[pinIdx] = ((1.0f - pinYWlCoeffY) * dNetBNegY[netId]
                            + wlCoeffY * dNetCNegY[netId])
                               * dPinANegY[pinIdx] / netBNegY2
                           - ((1.0f + pinYWlCoeffY) * dNetBPosY[netId]
                              - wlCoeffY * dNetCPosY[netId])
                                 * dPinAPosY[pinIdx] / netBPosY2;
  });
}

// define the kernel for updating wirelength force
// on each instance
void computeWirelengthGradientWAKernel(
    const int numPlaceInsts,
    const float virtualWeightFactor,
    const Kokkos::View<const int*>& dPinNetId,
    const Kokkos::View<const float*>& dNetWeight,
    const Kokkos::View<const float*>& dNetVirtualWeight,
    const Kokkos::View<const int*>& dInstPinIdx,
    const Kokkos::View<const int*>& dInstPinPos,
    const Kokkos::View<const float*>& dPinGradX,
    const Kokkos::View<const float*>& dPinGradY,
    const Kokkos::View<float*>& wirelengthForceX,
    const Kokkos::View<float*>& wirelengthForceY)
{
  Kokkos::parallel_for(numPlaceInsts, KOKKOS_LAMBDA (const int instId) {
    const int pinStart = dInstPinPos[instId];
    const int pinEnd = dInstPinPos[instId + 1];
    float wlGradX = 0.0;
    float wlGradY = 0.0;
    for (int pinId = pinStart; pinId < pinEnd; ++pinId) {
      const int pinIdx = dInstPinIdx[pinId];
      const int netId = dPinNetId[pinIdx];
      const float weight = dNetWeight[netId]
                           + dNetVirtualWeight[netId] * virtualWeightFactor;
      wlGradX += dPinGradX[pinIdx] * weight;
      wlGradY += dPinGradY[pinIdx] * weight;
    }

    wirelengthForceX[instId] = wlGradX;
    wirelengthForceY[instId] = wlGradY;
  });
}

void WirelengthOp::computeWireLengthForce(const float wlCoeffX,
                                          const float wlCoeffY,
                                          const float virtualWeightFactor,
                                          const Kokkos::View<float*>& wirelengthForceX,
                                          const Kokkos::View<float*>& wirelengthForceY)
{

  computeAPosNegKernel(numPins_,
                                                     wlCoeffX,
                                                     wlCoeffY,
                                                     dPinX_,
                                                     dPinY_,
                                                     dPinNetId_,
                                                     dNetLx_,
                                                     dNetLy_,
                                                     dNetUx_,
                                                     dNetUy_,
                                                     dPinAPosX_,
                                                     dPinANegX_,
                                                     dPinAPosY_,
                                                     dPinANegY_);

  computeBCPosNegKernel(numNets_,
                                                      dNetPinPos_,
                                                      dNetPinIdx_,
                                                      dPinX_,
                                                      dPinY_,
                                                      dPinAPosX_,
                                                      dPinANegX_,
                                                      dPinAPosY_,
                                                      dPinANegY_,
                                                      dNetBPosX_,
                                                      dNetBNegX_,
                                                      dNetBPosY_,
                                                      dNetBNegY_,
                                                      dNetCPosX_,
                                                      dNetCNegX_,
                                                      dNetCPosY_,
                                                      dNetCNegY_);

  computePinWAGradKernel(numPins_,
                                                       wlCoeffX,
                                                       wlCoeffY,
                                                       dPinNetId_,
                                                       dNetPinPos_,
                                                       dPinX_,
                                                       dPinY_,
                                                       dPinAPosX_,
                                                       dPinANegX_,
                                                       dPinAPosY_,
                                                       dPinANegY_,
                                                       dNetBPosX_,
                                                       dNetBNegX_,
                                                       dNetBPosY_,
                                                       dNetBNegY_,
                                                       dNetCPosX_,
                                                       dNetCNegX_,
                                                       dNetCPosY_,
                                                       dNetCNegY_,
                                                       dPinGradX_,
                                                       dPinGradY_);

  // get the force on each instance
  computeWirelengthGradientWAKernel(
      numPlaceInsts_,
      virtualWeightFactor,
      dPinNetId_,
      dNetWeight_,
      dNetVirtualWeight_,
      dInstPinIdx_,
      dInstPinPos_,
      dPinGradX_,
      dPinGradY_,
      wirelengthForceX,
      wirelengthForceY);
  }

}  // namespace gpl2
