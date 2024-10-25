///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018-2023, The Regents of the University of California
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
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include "densityOp.h"

#include <Kokkos_Core.hpp>

#include "placerBase.h"
#include "placerObjects.h"

namespace gpl2 {

//////////////////////////////////////////////////////////////
// Class DensityOp

DensityOp::DensityOp(PlacerBase* pb)
{
  pb_ = pb;
  logger_ = pb_->logger();
  logger_->report("[DensityOp] Start Initialization.");

  numBins_ = pb_->numBins();
  binCntX_ = pb_->binCntX();
  binCntY_ = pb_->binCntY();
  binSizeX_ = pb_->binSizeX();
  binSizeY_ = pb_->binSizeY();

  coreLx_ = pb_->coreLx();
  coreLy_ = pb_->coreLy();
  coreUx_ = pb_->coreUx();
  coreUy_ = pb_->coreUy();

  // placeable insts + filler insts
  numInsts_ = pb_->numInsts();

  // Initialize fft structure based on bins
  fft_ = std::make_unique<PoissonSolver>(
      binCntX_, binCntY_, binSizeX_, binSizeY_);

  initBackend();
  logger_->report("[DensityOp] Initialization Succeed.");
}

/////////////////////////////////////////////////////////
// Class Density Op

void DensityOp::initBackend()
{
  // Initialize the bin related information
  // Copy from host to device
  Kokkos::View<int*, Kokkos::HostSpace> hBinLx("hBinLx", numBins_);
  Kokkos::View<int*, Kokkos::HostSpace> hBinLy("hBinLy", numBins_);
  Kokkos::View<int*, Kokkos::HostSpace> hBinUx("hBinUx", numBins_);
  Kokkos::View<int*, Kokkos::HostSpace> hBinUy("hBinUy", numBins_);
  Kokkos::View<int64_t_cu*, Kokkos::HostSpace> hBinNonPlaceArea("hBinNonPlaceArea", numBins_);
  Kokkos::View<float*, Kokkos::HostSpace> hBinScaledArea("hBinScaledArea", numBins_);
  Kokkos::View<float*, Kokkos::HostSpace> hBinTargetDensity("hBinTargetDensity", numBins_);

  int binIdx = 0;
  for (auto& bin : pb_->bins()) {
    hBinLx[binIdx] = bin->lx();
    hBinLy[binIdx] = bin->ly();
    hBinUx[binIdx] = bin->ux();
    hBinUy[binIdx] = bin->uy();
    hBinNonPlaceArea[binIdx] = bin->nonPlaceArea();
    hBinScaledArea[binIdx] = bin->area() * bin->targetDensity();
    hBinTargetDensity[binIdx] = bin->targetDensity();
    binIdx++;
  }

  // allocate memory on device side
  dBinLx_ = Kokkos::View<int*>("dBinLx", numBins_);
  dBinLy_ = Kokkos::View<int*>("dBinLy", numBins_);
  dBinUx_ = Kokkos::View<int*>("dBinUx", numBins_);
  dBinUy_ = Kokkos::View<int*>("dBinUy", numBins_);
  dBinTargetDensity_ = Kokkos::View<float*>("dBinTargetDensity", numBins_);

  dBinNonPlaceArea_ = Kokkos::View<int64_t_cu*>("dBinNonPlaceArea", numBins_);
  dBinInstPlacedArea_ = Kokkos::View<int64_t_cu*>("dBinInstPlacedArea", numBins_);
  dBinFillerArea_ = Kokkos::View<int64_t_cu*>("dBinFillerArea", numBins_);
  dBinScaledArea_ = Kokkos::View<float*>("dBinScaledArea", numBins_);
  dBinOverflowArea_ = Kokkos::View<float*>("dBinOverflowArea", numBins_);

  dBinDensity_ = Kokkos::View<float*>("dBinDensity", numBins_);
  dBinElectroPhi_ = Kokkos::View<float*>("dBinElectroPhi", numBins_);
  dBinElectroForceX_ = Kokkos::View<float*>("dBinElectroForceX", numBins_);
  dBinElectroForceY_ = Kokkos::View<float*>("dBinElectroForceY", numBins_);

  // copy from host to device
  Kokkos::deep_copy(dBinLx_, hBinLx);
  Kokkos::deep_copy(dBinLy_, hBinLy);
  Kokkos::deep_copy(dBinUx_, hBinUx);
  Kokkos::deep_copy(dBinUy_, hBinUy);
  Kokkos::deep_copy(dBinNonPlaceArea_, hBinNonPlaceArea);
  Kokkos::deep_copy(dBinScaledArea_, hBinScaledArea);
  Kokkos::deep_copy(dBinTargetDensity_, hBinTargetDensity);

  // Initialize the instance related information
  Kokkos::View<int*, Kokkos::HostSpace> hGCellDensityWidth("hGCellDenistyWidth", numInsts_);
  Kokkos::View<int*, Kokkos::HostSpace> hGCellDensityHeight("hGCellDensityHeight", numInsts_);
  Kokkos::View<float*, Kokkos::HostSpace> hGCellDensityScale("hGCellDensityScale", numInsts_);
  Kokkos::View<int*, Kokkos::HostSpace> hGCellIsFiller("hGCellIsFiller", numInsts_);
  Kokkos::View<int*, Kokkos::HostSpace> hGCellIsMacro("hGCellIsMacro", numInsts_);

  int instIdx = 0;
  for (auto& inst : pb_->insts()) {
    hGCellDensityWidth[instIdx] = inst->dDx();
    hGCellDensityHeight[instIdx] = inst->dDy();
    hGCellDensityScale[instIdx] = inst->densityScale();
    hGCellIsFiller[instIdx] = inst->isFiller();
    hGCellIsMacro[instIdx] = inst->isMacro();
    instIdx++;
  }

  // allocate memory on device side
  dGCellDensityWidth_ = Kokkos::View<int*>("dGCellDensityWidth", numInsts_);
  dGCellDensityHeight_ = Kokkos::View<int*>("dGCellDensityHeight", numInsts_);
  dGCellDensityScale_ = Kokkos::View<float*>("dGCellDensityScale", numInsts_);
  dGCellIsFiller_ = Kokkos::View<int*>("dGCellIsFiller", numInsts_);
  dGCellIsMacro_ = Kokkos::View<int*>("dGCellIsMacro", numInsts_);

  dGCellDCx_ = Kokkos::View<int*>("dGCellDCx", numInsts_);
  dGCellDCy_ = Kokkos::View<int*>("dGCellDCy", numInsts_);

  // copy from host to device
  Kokkos::deep_copy(dGCellDensityWidth_, hGCellDensityWidth);
  Kokkos::deep_copy(dGCellDensityHeight_, hGCellDensityHeight);
  Kokkos::deep_copy(dGCellDensityScale_, hGCellDensityScale);
  Kokkos::deep_copy(dGCellIsFiller_, hGCellIsFiller);
  Kokkos::deep_copy(dGCellIsMacro_, hGCellIsMacro);
}

KOKKOS_FUNCTION inline IntRect getMinMaxIdxXY(const int numBins,
                                         const int binSizeX,
                                         const int binSizeY,
                                         const int binCntX,
                                         const int binCntY,
                                         const int coreLx,
                                         const int coreLy,
                                         const int coreUx,
                                         const int coreUy,
                                         const int instDCx,
                                         const int instDCy,
                                         const float instDDx,
                                         const float instDDy)
{
  IntRect binRect;

  const float lx = instDCx - instDDx / 2;
  const float ly = instDCy - instDDy / 2;
  const float ux = instDCx + instDDx / 2;
  const float uy = instDCy + instDDy / 2;

  int minIdxX = (int) Kokkos::floor((lx - coreLx) / binSizeX);
  int minIdxY = (int) Kokkos::floor((ly - coreLy) / binSizeY);
  int maxIdxX = (int) Kokkos::ceil((ux - coreLx) / binSizeX);
  int maxIdxY = (int) Kokkos::ceil((uy - coreLy) / binSizeY);

  binRect.lx = Kokkos::max(minIdxX, 0);
  binRect.ly = Kokkos::max(minIdxY, 0);
  binRect.ux = Kokkos::min(maxIdxX, binCntX);
  binRect.uy = Kokkos::min(maxIdxY, binCntY);

  return binRect;
}

// Utility functions
KOKKOS_FUNCTION inline float getOverlapWidth(const float& instDLx,
                                        const float& instDUx,
                                        const float& binLx,
                                        const float& binUx)
{
  if (instDUx <= binLx || instDLx >= binUx) {
    return 0.0;
  }
  return Kokkos::min(instDUx, binUx) - Kokkos::max(instDLx, binLx);
}

void DensityOp::updateDensityForceBin()
{
  // Step 1: Initialize the bin density information
  auto dBinInstPlacedArea = dBinInstPlacedArea_, dBinFillerArea = dBinFillerArea_;
  Kokkos::parallel_for(numBins_, KOKKOS_LAMBDA (const int binIdx) {
    dBinInstPlacedArea[binIdx] = 0;
    dBinFillerArea[binIdx] = 0;
  });

  // Step 2: compute the overlap between bin and instance
  auto numBins = numBins_, binSizeX = binSizeX_, binSizeY = binSizeY_, binCntX = binCntX_, binCntY = binCntY_,
      coreLx = coreLx_, coreLy = coreLy_, coreUx = coreUx_, coreUy = coreUy_;
  auto dGCellDCx = dGCellDCx_, dGCellDCy = dGCellDCy_, dGCellDensityWidth = dGCellDensityWidth_,
       dGCellDensityHeight = dGCellDensityHeight_;
  auto dBinLx = dBinLx_, dBinUx = dBinUx_, dBinUy = dBinUy_, dBinLy = dBinLy_;
  auto dGCellDensityScale = dGCellDensityScale_;
  auto dBinTargetDensity = dBinTargetDensity_;
  auto dGCellIsFiller = dGCellIsFiller_, dGCellIsMacro = dGCellIsMacro_;
  Kokkos::parallel_for(numInsts_, KOKKOS_LAMBDA (const int instIdx) {
    IntRect binRect = getMinMaxIdxXY(numBins,
                                     binSizeX,
                                     binSizeY,
                                     binCntX,
                                     binCntY,
                                     coreLx,
                                     coreLy,
                                     coreUx,
                                     coreUy,
                                     dGCellDCx[instIdx],
                                     dGCellDCy[instIdx],
                                     dGCellDensityWidth[instIdx],
                                     dGCellDensityHeight[instIdx]);

    for (int i = binRect.lx; i < binRect.ux; i++) {
      for (int j = binRect.ly; j < binRect.uy; j++) {
        const int binIdx = j * binCntX + i;
        const float instDLx
            = dGCellDCx[instIdx] - dGCellDensityWidth[instIdx] / 2.f;
        const float instDLy
            = dGCellDCy[instIdx] - dGCellDensityHeight[instIdx] / 2.f;
        const float instDUx
            = dGCellDCx[instIdx] + dGCellDensityWidth[instIdx] / 2.f;
        const float instDUy
            = dGCellDCy[instIdx] + dGCellDensityHeight[instIdx] / 2.f;
        const float overlapWidth = getOverlapWidth(
            instDLx, instDUx, dBinLx[binIdx], dBinUx[binIdx]);
        const float overlapHeight = getOverlapWidth(
            instDLy, instDUy, dBinLy[binIdx], dBinUy[binIdx]);
        float overlapArea
            = overlapWidth * overlapHeight * dGCellDensityScale[instIdx];
        // Atomic addition is used to safely update each bin's value in the
        // global grid array to account for the area occupied by the instance.
        // This ensures that updates from different threads don’t interfere with
        // each other, providing a correct total even when multiple threads
        // update the same bin simultaneously.
        if (dGCellIsFiller[instIdx]) {
          Kokkos::atomic_add(&dBinFillerArea[binIdx],
                    static_cast<int64_t_cu>(overlapArea));
        } else {
          if (dGCellIsMacro[instIdx] == true) {
            overlapArea = overlapArea * dBinTargetDensity[binIdx];
          }
          Kokkos::atomic_add(&dBinInstPlacedArea[binIdx],
                    static_cast<int64_t_cu>(overlapArea));
        }
      }
    }
  });

  // Step 3: update overflow
  auto dBinDensity = dBinDensity_, dBinOverflowArea = dBinOverflowArea_;
  auto dBinNonPlaceArea = dBinNonPlaceArea_;
  auto dBinScaledArea = dBinScaledArea_;
  Kokkos::parallel_for(numBins_, KOKKOS_LAMBDA (const int binIdx) {
  dBinDensity[binIdx]
        = (static_cast<float>(dBinNonPlaceArea[binIdx])
           + static_cast<float>(dBinInstPlacedArea[binIdx])
           + static_cast<float>(dBinFillerArea[binIdx]))
          / dBinScaledArea[binIdx];

    dBinOverflowArea[binIdx]
        = Kokkos::max(0.0f,
              static_cast<float>(dBinInstPlacedArea[binIdx])
                  + static_cast<float>(dBinNonPlaceArea[binIdx])
                  - dBinScaledArea[binIdx]);
  });

  sumOverflow_ = 0.0;
  Kokkos::parallel_reduce(numBins_, KOKKOS_LAMBDA (const int binIdx, float& sumOverflow) {
    sumOverflow += dBinOverflowArea[binIdx];
  }, Kokkos::Sum<float>(sumOverflow_));
  Kokkos::fence();

  // Step 4: solve the poisson equation
  fft_->solvePoisson(dBinDensity_,
                     dBinElectroPhi_,
                     dBinElectroForceX_,
                     dBinElectroForceY_);
}

void DensityOp::getDensityGradient(const Kokkos::View<float*>& densityGradientX,
                                   const Kokkos::View<float*>& densityGradientY)
{
  // Step 5: Compute electro force for each instance
  auto numBins = numBins_, binSizeX = binSizeX_, binSizeY = binSizeY_, binCntX = binCntX_, binCntY = binCntY_,
      coreLx = coreLx_, coreLy = coreLy_, coreUx = coreUx_, coreUy = coreUy_;
  auto dGCellDCx = dGCellDCx_, dGCellDCy = dGCellDCy_, dGCellDensityWidth = dGCellDensityWidth_,
       dGCellDensityHeight = dGCellDensityHeight_;
  auto dBinLx = dBinLx_, dBinLy = dBinLy_, dBinUx = dBinUx_, dBinUy = dBinUy_;
  auto dGCellDensityScale = dGCellDensityScale_;
  auto dBinElectroForceX = dBinElectroForceX_, dBinElectroForceY = dBinElectroForceY_;
  Kokkos::parallel_for(numInsts_, KOKKOS_LAMBDA (const int instIdx) {
    IntRect binRect = getMinMaxIdxXY(numBins,
                                     binSizeX,
                                     binSizeY,
                                     binCntX,
                                     binCntY,
                                     coreLx,
                                     coreLy,
                                     coreUx,
                                     coreUy,
                                     dGCellDCx[instIdx],
                                     dGCellDCy[instIdx],
                                     dGCellDensityWidth[instIdx],
                                     dGCellDensityHeight[instIdx]);

    float electroForceSumX = 0.0;
    float electroForceSumY = 0.0;

    for (int i = binRect.lx; i < binRect.ux; i++) {
      for (int j = binRect.ly; j < binRect.uy; j++) {
        const int binIdx = j * binCntX + i;
        const float instDLx
            = dGCellDCx[instIdx] - dGCellDensityWidth[instIdx] / 2.f;
        const float instDLy
            = dGCellDCy[instIdx] - dGCellDensityHeight[instIdx] / 2.f;
        const float instDUx
            = dGCellDCx[instIdx] + dGCellDensityWidth[instIdx] / 2.f;
        const float instDUy
            = dGCellDCy[instIdx] + dGCellDensityHeight[instIdx] / 2.f;
        const float overlapWidth = getOverlapWidth(
            instDLx, instDUx, dBinLx[binIdx], dBinUx[binIdx]);
        const float overlapHeight = getOverlapWidth(
            instDLy, instDUy, dBinLy[binIdx], dBinUy[binIdx]);
        const float overlapArea
            = overlapWidth * overlapHeight * dGCellDensityScale[instIdx];
        electroForceSumX += 0.5 * overlapArea * dBinElectroForceX[binIdx];
        electroForceSumY += 0.5 * overlapArea * dBinElectroForceY[binIdx];
      }
    }

    densityGradientX[instIdx] = electroForceSumX;
    densityGradientY[instIdx] = electroForceSumY;
  });
}

void DensityOp::updateGCellLocation(const Kokkos::View<const int*>& instDCx, const Kokkos::View<const int*>& instDCy)
{
  auto dGCellDCx = dGCellDCx_, dGCellDCy = dGCellDCy_;
  Kokkos::parallel_for(numInsts_, KOKKOS_LAMBDA (const int instIdx) {
    dGCellDCx[instIdx] = instDCx[instIdx];
    dGCellDCy[instIdx] = instDCy[instIdx];
  });
}

}  // namespace gpl2
