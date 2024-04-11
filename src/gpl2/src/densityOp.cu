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

#pragma once

#include <memory>
#include <ctime>
#include <algorithm> 
#include <cassert>
#include <cfloat>  
#include <climits> 
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <random> 
#include <stdio.h>
#include <vector>
#include <cufft.h>

#include <thrust/transform.h>
#include <thrust/execution_policy.h>
#include <thrust/functional.h>
#include <thrust/iterator/counting_iterator.h>
#include <thrust/iterator/transform_iterator.h>
#include <thrust/device_vector.h>
#include <thrust/host_vector.h>

#include "placerObjects.h"
#include "placerBase.h"
#include "densityOp.h"

namespace gpl2 {   
  
/////////////////////////////////////////////////////////
// Class Density Op

void DensityOp::initCUDAKernel()
{
  // Initialize the bin related information
  // Copy from host to device
  thrust::host_vector<int> hBinLx(numBins_);
  thrust::host_vector<int> hBinLy(numBins_);
  thrust::host_vector<int> hBinUx(numBins_);
  thrust::host_vector<int> hBinUy(numBins_); 
  thrust::host_vector<int64_t_cu> hBinNonPlaceArea(numBins_);
  thrust::host_vector<float> hBinScaledArea(numBins_);
  thrust::host_vector<float> hBinTargetDensity(numBins_);

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
  dBinLxPtr_ = setThrustVector<int>(numBins_, dBinLx_);
  dBinLyPtr_ = setThrustVector<int>(numBins_, dBinLy_);
  dBinUxPtr_ = setThrustVector<int>(numBins_, dBinUx_);
  dBinUyPtr_ = setThrustVector<int>(numBins_, dBinUy_);
  dBinTargetDensityPtr_ = setThrustVector<float>(numBins_, dBinTargetDensity_);

  dBinNonPlaceAreaPtr_ = setThrustVector<int64_t_cu>(numBins_, 
        dBinNonPlaceArea_);
  dBinInstPlacedAreaPtr_ = setThrustVector<int64_t_cu>(numBins_, 
        dBinInstPlacedArea_);
  dBinFillerAreaPtr_ = setThrustVector<int64_t_cu>(numBins_, 
        dBinFillerArea_);
  dBinScaledAreaPtr_ = setThrustVector<float>(numBins_, 
        dBinScaledArea_);
  dBinOverflowAreaPtr_ = setThrustVector<float>(numBins_, 
        dBinOverflowArea_);

  dBinDensityPtr_ = setThrustVector<float>(numBins_, 
        dBinDensity_);
  dBinElectroPhiPtr_ = setThrustVector<float>(numBins_, 
        dBinElectroPhi_);
  dBinElectroForceXPtr_ = setThrustVector<float>(numBins_, 
        dBinElectroForceX_);
  dBinElectroForceYPtr_ = setThrustVector<float>(numBins_,
        dBinElectroForceY_);

  // copy from host to device
  thrust::copy(hBinLx.begin(), hBinLx.end(), dBinLx_.begin());
  thrust::copy(hBinLy.begin(), hBinLy.end(), dBinLy_.begin());
  thrust::copy(hBinUx.begin(), hBinUx.end(), dBinUx_.begin());
  thrust::copy(hBinUy.begin(), hBinUy.end(), dBinUy_.begin());
  thrust::copy(hBinNonPlaceArea.begin(), hBinNonPlaceArea.end(), 
               dBinNonPlaceArea_.begin());  
  thrust::copy(hBinScaledArea.begin(), hBinScaledArea.end(),
               dBinScaledArea_.begin());
  thrust::copy(hBinTargetDensity.begin(), hBinTargetDensity.end(),
               dBinTargetDensity_.begin());

  // Initialize the instance related information
  thrust::host_vector<int> hGCellDensityWidth(numInsts_);
  thrust::host_vector<int> hGCellDensityHeight(numInsts_);
  thrust::host_vector<float> hGCellDensityScale(numInsts_);
  thrust::host_vector<bool> hGCellIsFiller(numInsts_);
  thrust::host_vector<bool> hGCellIsMacro(numInsts_);
  
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
  dGCellDensityWidthPtr_ = setThrustVector<int>(numInsts_, 
        dGCellDensityWidth_);
  dGCellDensityHeightPtr_ = setThrustVector<int>(numInsts_, 
        dGCellDensityHeight_);
  dGCellDensityScalePtr_ = setThrustVector<float>(numInsts_,
        dGCellDensityScale_); 
  dGCellIsFillerPtr_ = setThrustVector<bool>(numInsts_,
        dGCellIsFiller_);      
  dGCellIsMacroPtr_ = setThrustVector<bool>(numInsts_,
        dGCellIsMacro_);

  dGCellDCxPtr_ = setThrustVector<int>(numInsts_, dGCellDCx_);
  dGCellDCyPtr_ = setThrustVector<int>(numInsts_, dGCellDCy_);

  // copy from host to device
  thrust::copy(hGCellDensityWidth.begin(), hGCellDensityWidth.end(), 
               dGCellDensityWidth_.begin());
  thrust::copy(hGCellDensityHeight.begin(), hGCellDensityHeight.end(),
               dGCellDensityHeight_.begin());
  thrust::copy(hGCellDensityScale.begin(), hGCellDensityScale.end(),
               dGCellDensityScale_.begin());
  thrust::copy(hGCellIsFiller.begin(), hGCellIsFiller.end(),
               dGCellIsFiller_.begin());
  thrust::copy(hGCellIsMacro.begin(), hGCellIsMacro.end(),
               dGCellIsMacro_.begin());
}

void DensityOp::freeCUDAKernel()
{
  // since we use thrust::device_vector,
  // we don't need to free the memory explicitly
  pb_ = nullptr;
  fft_ = nullptr;
  logger_ = nullptr;

  dBinLxPtr_ = nullptr;
  dBinLyPtr_ = nullptr;
  dBinUxPtr_ = nullptr;
  dBinUyPtr_ = nullptr;
  dBinTargetDensityPtr_ = nullptr;

  dBinNonPlaceAreaPtr_ = nullptr;
  dBinInstPlacedAreaPtr_ = nullptr;
  dBinFillerAreaPtr_ = nullptr;
  dBinScaledAreaPtr_ = nullptr;
  dBinOverflowAreaPtr_ = nullptr;
  
  dBinDensityPtr_ = nullptr;
  dBinElectroPhiPtr_ = nullptr;
  dBinElectroForceXPtr_ = nullptr;
  dBinElectroForceYPtr_ = nullptr;

  dGCellDensityWidthPtr_ = nullptr;
  dGCellDensityHeightPtr_ = nullptr;
  dGCellDensityScalePtr_ = nullptr;
  dGCellIsFillerPtr_ = nullptr;
  dGCellIsMacroPtr_ = nullptr;
}


// Step1 :  Initialize bin density information
__global__
void initBinsGCellDensityArea(
  const int numBins,
  int64_t_cu* dBinInstPlacedAreaPtr,
  int64_t_cu* dBinFillerAreaPtr) 
{
  const int binIdx = blockIdx.x * blockDim.x + threadIdx.x;
  if (binIdx < numBins) {
    dBinInstPlacedAreaPtr[binIdx] = 0;
    dBinFillerAreaPtr[binIdx] = 0;
  }
}


__device__
inline IntRect getMinMaxIdxXY(
  const int numBins,
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

  int minIdxX = (int) floor((lx - coreLx) / binSizeX);
  int minIdxY = (int) floor((ly - coreLy) / binSizeY);
  int maxIdxX = (int) ceil((ux - coreLx) / binSizeX);
  int maxIdxY = (int) ceil((uy - coreLy) / binSizeY);

  binRect.lx = max(minIdxX, 0);
  binRect.ly = max(minIdxY, 0);
  binRect.ux = min(maxIdxX, binCntX);
  binRect.uy = min(maxIdxY, binCntY);

  return binRect;
}



// Utility functions
__device__ 
inline float getOverlapWidth(
  const float& instDLx,
  const float& instDUx,
  const float& binLx,
  const float& binUx)
{
  if (instDUx <= binLx || instDLx >= binUx) {
    return 0.0;
  } else {
    return min(instDUx, binUx) - max(instDLx, binLx);
  }
}

// Step2:  compute the overlap between bin and instance
// The following function is critical runtime
// hotspot in the original RePlAce implementation.
// We solve the problem through parallel updating.
__global__
void updateBinsGCellDensityArea(
  // bin information
  const int numBins,
  const int binSizeX,
  const int binSizeY,
  const int binCntX,
  const int binCntY,
  const int* dBinLxPtr,
  const int* dBinLyPtr,
  const int* dBinUxPtr,
  const int* dBinUyPtr,
  const float* dBinTargetDensityPtr,
  const int64_t_cu* dBinNonPlaceAreaPtr,
  // die information
  const int coreLx,
  const int coreLy,
  const int coreUx,
  const int coreUy,
  // Instance information
  const int numInsts,
  const int* dGCellDCxPtr,
  const int* dGCellDCyPtr,
  const int* dGCellDensityWidthPtr,
  const int* dGCellDensityHeightPtr,
  const float* dGCellDensityScalePtr,
  const bool* dGCellIsFillerPtr,
  const bool* dGCellIsMacroPtr,
  // output
  int64_t_cu* dBinInstPlacedAreaPtr,
  int64_t_cu* dBinFillerAreaPtr)
{  
  const int instIdx = blockIdx.x * blockDim.x + threadIdx.x;
  if (instIdx < numInsts) {
    IntRect binRect = getMinMaxIdxXY(
        numBins,
        binSizeX,
        binSizeY,
        binCntX,
        binCntY,
        coreLx,
        coreLy,
        coreUx,
        coreUy,
        dGCellDCxPtr[instIdx],
        dGCellDCyPtr[instIdx],
        dGCellDensityWidthPtr[instIdx],
        dGCellDensityHeightPtr[instIdx]);
        
    for (int i = binRect.lx;  i < binRect.ux; i++) {
      for (int j = binRect.ly; j < binRect.uy; j++) {
        const int binIdx = j * binCntX + i;
        const float instDLx = dGCellDCxPtr[instIdx] - 
                            dGCellDensityWidthPtr[instIdx] / 2;
        const float instDLy = dGCellDCyPtr[instIdx] - 
                            dGCellDensityHeightPtr[instIdx] / 2;
        const float instDUx = dGCellDCxPtr[instIdx] + 
                            dGCellDensityWidthPtr[instIdx] / 2 ;
        const float instDUy = dGCellDCyPtr[instIdx] +
                            dGCellDensityHeightPtr[instIdx] / 2;
        const float overlapWidth = 
           getOverlapWidth(instDLx, instDUx, dBinLxPtr[binIdx], dBinUxPtr[binIdx]);
        const float overlapHeight = 
           getOverlapWidth(instDLy, instDUy, dBinLyPtr[binIdx], dBinUyPtr[binIdx]);
        float overlapArea = overlapWidth * overlapHeight * dGCellDensityScalePtr[instIdx];
        // Atomic addition is used to safely update each bin's value in the global grid array 
        // to account for the area occupied by the instance.
        // This ensures that updates from different threads don’t interfere with each other, 
        // providing a correct total even when multiple threads 
        // update the same bin simultaneously.
        if (dGCellIsFillerPtr[instIdx]) {
          atomicAdd(&dBinFillerAreaPtr[binIdx], static_cast<int64_t_cu>(overlapArea));
        } else {
          if (dGCellIsMacroPtr[instIdx] == true) {
            overlapArea = overlapArea * dBinTargetDensityPtr[binIdx];
          }
          atomicAdd(&dBinInstPlacedAreaPtr[binIdx], static_cast<int64_t_cu>(overlapArea));
        }

      }
    }
  }
}

// Step 3: update the bin overflow information
__global__
void updateBinDensityAndOverflow(
  const int numBins,
  const int64_t_cu* dBinNonPlaceAreaPtr_,
  const int64_t_cu* dBinInstPlacedAreaPtr_,
  const int64_t_cu* dBinFillerAreaPtr_,
  const float* dBinScaledAreaPtr_,
  float* dBinDensityPtr_,
  float* dBinOverflowAreaPtr_)
{
  const int binIdx = blockIdx.x * blockDim.x + threadIdx.x;
  if (binIdx < numBins) {
    dBinDensityPtr_[binIdx] = (static_cast<float>(dBinNonPlaceAreaPtr_[binIdx]) + 
                               static_cast<float>(dBinInstPlacedAreaPtr_[binIdx]) + 
                               static_cast<float>(dBinFillerAreaPtr_[binIdx])) / 
                               dBinScaledAreaPtr_[binIdx];
   
    dBinOverflowAreaPtr_[binIdx] = max(0.0, 
        static_cast<float>(dBinInstPlacedAreaPtr_[binIdx]) + 
        static_cast<float>(dBinNonPlaceAreaPtr_[binIdx]) -
        dBinScaledAreaPtr_[binIdx]);
  }
}

void DensityOp::updateDensityForceBin()
{
  const int numThreadBin = 256;
  const int numBlockBin = (numBins_ + numThreadBin - 1) / numThreadBin;

  const int numThreadInst = 256;
  const int numBlockInst = (numInsts_ + numThreadInst - 1) / numThreadInst;

  // get the timestamp
  auto startTimestamp = std::chrono::high_resolution_clock::now();

  // Step 1: Initialize the bin density information
  initBinsGCellDensityArea<<<numBlockBin, numThreadBin>>>(
      numBins_,
      dBinInstPlacedAreaPtr_,
      dBinFillerAreaPtr_);


  // Step 2: compute the overlap between bin and instance
  updateBinsGCellDensityArea<<<numBlockInst, numThreadInst>>>(
    numBins_,
    binSizeX_,
    binSizeY_,
    binCntX_,
    binCntY_,
    dBinLxPtr_,
    dBinLyPtr_,
    dBinUxPtr_,
    dBinUyPtr_,
    dBinTargetDensityPtr_,
    dBinNonPlaceAreaPtr_,
    coreLx_,
    coreLy_,
    coreUx_,
    coreUy_,
    numInsts_,
    dGCellDCxPtr_,
    dGCellDCyPtr_,
    dGCellDensityWidthPtr_,
    dGCellDensityHeightPtr_,
    dGCellDensityScalePtr_,
    dGCellIsFillerPtr_,
    dGCellIsMacroPtr_,
    dBinInstPlacedAreaPtr_,
    dBinFillerAreaPtr_);

  // Step 3: update overflow
  updateBinDensityAndOverflow<<<numBlockBin, numThreadBin>>>(
    numBins_,
    dBinNonPlaceAreaPtr_,
    dBinInstPlacedAreaPtr_,
    dBinFillerAreaPtr_,
    dBinScaledAreaPtr_,
    dBinDensityPtr_,
    dBinOverflowAreaPtr_);
 
  /*
  std::cout << "[Test a] updateDensityForceBin" << std::endl;
  thrust::device_vector<float> dBinDensity(numBins_);
  thrust::device_vector<int64_t_cu> dBinNonPlaceArea(numBins_);
  thrust::device_vector<int64_t_cu> dBinInstPlacedArea(numBins_);
  thrust::device_vector<int64_t_cu> dBinFillerArea(numBins_);
  thrust::device_vector<float> dBinScaledArea(numBins_);
  
  thrust::copy(dBinDensityPtr_, dBinDensityPtr_ + numBins_, dBinDensity.begin());
  thrust::copy(dBinNonPlaceAreaPtr_, dBinNonPlaceAreaPtr_ + numBins_, 
               dBinNonPlaceArea.begin());
  thrust::copy(dBinInstPlacedAreaPtr_, dBinInstPlacedAreaPtr_ + numBins_,
                dBinInstPlacedArea.begin());  
  thrust::copy(dBinFillerAreaPtr_, dBinFillerAreaPtr_ + numBins_,
                dBinFillerArea.begin());
  thrust::copy(dBinScaledAreaPtr_, dBinScaledAreaPtr_ + numBins_,
                dBinScaledArea.begin());  

  thrust::host_vector<float> hBinDensity = dBinDensity;
  thrust::host_vector<int64_t_cu> hBinNonPlaceArea = dBinNonPlaceArea;
  thrust::host_vector<int64_t_cu> hBinInstPlacedArea = dBinInstPlacedArea;
  thrust::host_vector<int64_t_cu> hBinFillerArea = dBinFillerArea;
  thrust::host_vector<float> hBinScaledArea = dBinScaledArea;
  for (int binId = 0; binId < numBins_; binId++) {
    std::cout << "[Test a] binId = " << binId << "  "
              << "nonPlaceArea = " << hBinNonPlaceArea[binId] << "  "
              << "instPlacedArea = " << hBinInstPlacedArea[binId] << "  "
              << "fillerArea = " << hBinFillerArea[binId] << "  "
              << "scaledArea = " << hBinScaledArea[binId] << "  "
              << "bin.density = " << hBinDensity[binId] << std::endl;
  }
  */

  sumOverflow_ = thrust::reduce(dBinOverflowArea_.begin(), 
                                dBinOverflowArea_.end(), 
                                0.0, 
                                thrust::plus<float>());

  auto densityTimestamp = std::chrono::high_resolution_clock::now();
  double densityTime = std::chrono::duration_cast<std::chrono::nanoseconds>(
      densityTimestamp- startTimestamp).count();
  densityTime_ += densityTime * 1e-9;   

  // Step 4: solve the poisson equation
  fft_->solvePoisson(dBinDensityPtr_,
                     dBinElectroPhiPtr_,
                     dBinElectroForceXPtr_,
                     dBinElectroForceYPtr_);

  auto fftTimestamp = std::chrono::high_resolution_clock::now();
  double fftTime = std::chrono::duration_cast<std::chrono::nanoseconds>(
     fftTimestamp - densityTimestamp).count();
  fftTime_ += fftTime * 1e-9;   
}


// Compute electro force for each instance
 __global__
 void computeInstElectroForce(
    // bin information
    const int numBins,
    const int binSizeX,
    const int binSizeY,
    const int binCntX,
    const int binCntY,
    const int* dBinLxPtr,
    const int* dBinLyPtr,
    const int* dBinUxPtr,
    const int* dBinUyPtr,
    const float* dBinElectroForceXPtr,
    const float* dBinElectroForceYPtr,
    // die information
    const int coreLx,
    const int coreLy,
    const int coreUx,
    const int coreUy,
    // Instance information
    const int numInsts,
    const int* dGCellDCxPtr,
    const int* dGCellDCyPtr,
    const int* dGCellDensityWidthPtr,
    const int* dGCellDensityHeightPtr,
    const float* dGCellDensityScalePtr,
    // output
    float* electroForceX,
    float* electroForceY)
{  
  const int instIdx = blockIdx.x * blockDim.x + threadIdx.x;
  if (instIdx < numInsts) {
    IntRect binRect = getMinMaxIdxXY(
      numBins,
      binSizeX,
      binSizeY,
      binCntX,
      binCntY,
      coreLx,
      coreLy,
      coreUx,
      coreUy,
      dGCellDCxPtr[instIdx],
      dGCellDCyPtr[instIdx],
      dGCellDensityWidthPtr[instIdx],
      dGCellDensityHeightPtr[instIdx]);
     
    float electroForceSumX = 0.0;
    float electroForceSumY = 0.0;
       
    for (int i = binRect.lx;  i < binRect.ux; i++) {
      for (int j = binRect.ly; j < binRect.uy; j++) {
        const int binIdx = j * binCntX + i;
        const float instDLx = dGCellDCxPtr[instIdx] - 
                            dGCellDensityWidthPtr[instIdx] / 2;
        const float instDLy = dGCellDCyPtr[instIdx] - 
                            dGCellDensityHeightPtr[instIdx] / 2;
        const float instDUx = dGCellDCxPtr[instIdx] + 
                            dGCellDensityWidthPtr[instIdx] / 2;
        const float instDUy = dGCellDCyPtr[instIdx] +
                            dGCellDensityHeightPtr[instIdx] / 2 ;
        const float overlapWidth = 
          getOverlapWidth(instDLx, instDUx, dBinLxPtr[binIdx], dBinUxPtr[binIdx]);
        const float overlapHeight = 
          getOverlapWidth(instDLy, instDUy, dBinLyPtr[binIdx], dBinUyPtr[binIdx]);
        const float overlapArea = overlapWidth * overlapHeight * dGCellDensityScalePtr[instIdx];
        electroForceSumX += 0.5 * overlapArea * dBinElectroForceXPtr[binIdx];
        electroForceSumY += 0.5 * overlapArea * dBinElectroForceYPtr[binIdx];
      }
    }
   
    electroForceX[instIdx] = electroForceSumX;
    electroForceY[instIdx] = electroForceSumY;
 }
}


void DensityOp::getDensityGradient(float* densityGradientX,
                                   float* densityGradientY)
{
  const int numThreadInst = 256;
  const int numBlockInst = (numInsts_ + numThreadInst - 1) / numThreadInst;

  // Step 5: Compute electro force for each instance
  computeInstElectroForce<<<numBlockInst, numThreadInst>>>(
    numBins_,
    binSizeX_,
    binSizeY_,
    binCntX_,
    binCntY_,
    dBinLxPtr_,
    dBinLyPtr_,
    dBinUxPtr_,
    dBinUyPtr_,
    dBinElectroForceXPtr_,
    dBinElectroForceYPtr_,
    coreLx_,
    coreLy_,
    coreUx_,
    coreUy_,
    numInsts_,
    dGCellDCxPtr_,
    dGCellDCyPtr_,
    dGCellDensityWidthPtr_,
    dGCellDensityHeightPtr_,
    dGCellDensityScalePtr_,
    densityGradientX,
    densityGradientY);


  /*
  // for debug
  std::cout << std::endl;
  std::cout << "Bin Infomation" << std::endl;
  thrust::device_vector<float> dBinElectroForceX(numBins_);
  thrust::device_vector<float> dBinElectroForceY(numBins_);
  thrust::device_vector<float> dBinDensity(numBins_);
  thrust::copy(dBinElectroForceXPtr_, dBinElectroForceXPtr_ + numBins_, 
               dBinElectroForceX.begin());
  thrust::copy(dBinElectroForceYPtr_, dBinElectroForceYPtr_ + numBins_,
                dBinElectroForceY.begin()); 
  thrust::copy(dBinDensityPtr_, dBinDensityPtr_ + numBins_,
                dBinDensity.begin());
  thrust::host_vector<float> hBinElectroForceX = dBinElectroForceX;
  thrust::host_vector<float> hBinElectroForceY = dBinElectroForceY;  
  thrust::host_vector<float> hBinDensity = dBinDensity;
  for (int binId = 0; binId < numBins_; binId++) {
    std::cout << "binId = " << binId << "  "
              << "bin.density = " << hBinDensity[binId] << "  "
              << "bin.x = " << hBinElectroForceX[binId] * 0.5 << "  "
              << "bin.y = " << hBinElectroForceY[binId] * 0.5 << std::endl;
  }
  */
}



__global__
void updateGCellLocationKernel(
  const int numInsts,
  const int* instDCx,
  const int* instDCy,
  int* dGCellDCxPtr,
  int* dGCellDCyPtr)
{
  const int instIdx = blockIdx.x * blockDim.x + threadIdx.x;
  if (instIdx < numInsts) {
    dGCellDCxPtr[instIdx] = instDCx[instIdx];
    dGCellDCyPtr[instIdx] = instDCy[instIdx];
  }
}


void DensityOp::updateGCellLocation(const int* instDCx,
                                    const int* instDCy)
{
  const int numThreadInst = 256;
  const int numBlockInst = (numInsts_ + numThreadInst - 1) / numThreadInst;

  updateGCellLocationKernel<<<numBlockInst, numThreadInst>>>(
      numInsts_,
      instDCx,
      instDCy,
      dGCellDCxPtr_,
      dGCellDCyPtr_);
}

}
