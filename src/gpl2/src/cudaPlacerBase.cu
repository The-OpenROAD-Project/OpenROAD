///////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2023, Google LLC
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

#include <cuda.h>
#include <cuda_runtime.h>
#include <cufft.h>
#include <stdio.h>

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>

#include "odb/db.h"
#include "placerBase.h"
#include "placerObjects.h"
#include "poissonSolver.h"
#include "util.h"
#include "utl/Logger.h"
// basic vectors
#include <thrust/device_free.h>
#include <thrust/device_malloc.h>
#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#include <thrust/reduce.h>
#include <thrust/sequence.h>
// memory related
#include <thrust/copy.h>
#include <thrust/fill.h>
// algorithm related
#include <thrust/execution_policy.h>
#include <thrust/for_each.h>
#include <thrust/functional.h>
#include <thrust/replace.h>
#include <thrust/transform.h>

namespace gpl2 {

using utl::GPL2;

///////////////////////////////////////////////////////////////////////////////////
// PlacerBaseCommon
///////////////////////////////////////////////////////////////////////////////////

void PlacerBaseCommon::initCUDAKernel()
{
  // calculate the information on the host side
  hInstDCx_.resize(numPlaceInsts_);
  hInstDCy_.resize(numPlaceInsts_);
  int instIdx = 0;
  for (auto& inst : placeInsts_) {
    hInstDCx_[instIdx] = inst->cx();
    hInstDCy_[instIdx] = inst->cy();
    instIdx++;
  }

  // allocate the objects on host side
  dInstDCxPtr_ = setThrustVector<int>(numPlaceInsts_, dInstDCx_);
  dInstDCyPtr_ = setThrustVector<int>(numPlaceInsts_, dInstDCy_);

  // copy from host to device
  thrust::copy(hInstDCx_.begin(), hInstDCx_.end(), dInstDCx_.begin());
  thrust::copy(hInstDCy_.begin(), hInstDCy_.end(), dInstDCy_.begin());

  // allocate memory on device side
  dWLGradXPtr_ = setThrustVector<float>(numPlaceInsts_, dWLGradX_);
  dWLGradYPtr_ = setThrustVector<float>(numPlaceInsts_, dWLGradY_);

  // create the wlGradOp
  wlGradOp_ = new WirelengthOp(this);
}

void PlacerBaseCommon::freeCUDAKernel()
{
  dInstDCxPtr_ = nullptr;
  dInstDCyPtr_ = nullptr;

  dWLGradXPtr_ = nullptr;
  dWLGradYPtr_ = nullptr;
}

// Update the database information
void PlacerBaseCommon::updateDB()
{
  if (clusterFlag_ == true) {
    updateDBCluster();
    return;
  }

  thrust::copy(dInstDCx_.begin(), dInstDCx_.end(), hInstDCx_.begin());
  thrust::copy(dInstDCy_.begin(), dInstDCy_.end(), hInstDCy_.begin());

  int manufactureGird = db_->getTech()->getManufacturingGrid();

  for (auto inst : placeInsts_) {
    const int instId = inst->instId();
    inst->setCenterLocation(
        static_cast<int>(hInstDCx_[instId]) / manufactureGird * manufactureGird,
        static_cast<int>(hInstDCy_[instId]) / manufactureGird
            * manufactureGird);
    inst->dbSetLocation();
    inst->dbSetPlaced();
  }
}

void PlacerBaseCommon::updateDBCluster()
{
  thrust::copy(dInstDCx_.begin(), dInstDCx_.end(), hInstDCx_.begin());
  thrust::copy(dInstDCy_.begin(), dInstDCy_.end(), hInstDCy_.begin());

  int manufactureGird = db_->getTech()->getManufacturingGrid();
  odb::dbBlock* block = getBlock();
  // insts fill with real instances
  // update the clusters
  odb::dbSet<odb::dbInst> insts = block->getInsts();
  for (odb::dbInst* inst : insts) {
    auto type = inst->getMaster()->getType();
    if (!type.isCore() && !type.isBlock()) {
      continue;
    }
    const int clusterId
        = odb::dbIntProperty::find(inst, "cluster_id")->getValue();
    const int cx = hInstDCx_[clusterId];
    const int cy = hInstDCy_[clusterId];
    odb::dbBox* bbox = inst->getBBox();
    int width = bbox->getDX();
    int height = bbox->getDY();
    int lx = cx - width / 2;
    int ly = cy - height / 2;
    if (lx < die().coreLx()) {
      lx = die().coreLx();
    }

    if (ly < die().coreLy()) {
      ly = die().coreLy();
    }

    if (lx + width > die().coreUx()) {
      lx = die().coreUx() - width;
    }

    if (ly + height > die().coreUy()) {
      ly = die().coreUy() - height;
    }

    lx = lx / manufactureGird * manufactureGird;
    ly = ly / manufactureGird * manufactureGird;

    inst->setLocation(lx, ly);
    inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// PlacerBase
/////////////////////////////////////////////////////////////////////////////////////////////

void PlacerBase::initCUDAKernel()
{
  // calculate the information on the host side
  thrust::host_vector<int> hPlaceInstIds(numPlaceInsts_);
  int instIdx = 0;
  for (auto& inst : placeInsts_) {
    hPlaceInstIds[instIdx] = inst->instId();
    instIdx++;
  }

  // allocate the objects on host side
  dPlaceInstIdsPtr_ = setThrustVector<int>(numPlaceInsts_, dPlaceInstIds_);

  // copy from host to device
  thrust::copy(
      hPlaceInstIds.begin(), hPlaceInstIds.end(), dPlaceInstIds_.begin());

  thrust::host_vector<int> hInstDDx(numInsts_);
  thrust::host_vector<int> hInstDDy(numInsts_);
  thrust::host_vector<int> hInstDCx(numInsts_);
  thrust::host_vector<int> hInstDCy(numInsts_);
  thrust::host_vector<float> hWireLengthPrecondi(numInsts_);
  thrust::host_vector<float> hDensityPrecondi(numInsts_);

  // calculate the information on the host side
  instIdx = 0;
  for (auto& inst : insts_) {
    hInstDDx[instIdx] = inst->dDx();
    hInstDDy[instIdx] = inst->dDy();
    hInstDCx[instIdx] = inst->cx();
    hInstDCy[instIdx] = inst->cy();
    hWireLengthPrecondi[instIdx] = inst->wireLengthPreconditioner();
    hDensityPrecondi[instIdx] = inst->densityPreconditioner();
    instIdx++;
  }

  dInstDDxPtr_ = setThrustVector<int>(numInsts_, dInstDDx_);
  dInstDDyPtr_ = setThrustVector<int>(numInsts_, dInstDDy_);
  dInstDCxPtr_ = setThrustVector<int>(numInsts_, dInstDCx_);
  dInstDCyPtr_ = setThrustVector<int>(numInsts_, dInstDCy_);

  dWireLengthPrecondiPtr_
      = setThrustVector<float>(numInsts_, dWireLengthPrecondi_);
  dDensityPrecondiPtr_ = setThrustVector<float>(numInsts_, dDensityPrecondi_);

  thrust::copy(hInstDDx.begin(), hInstDDx.end(), dInstDDx_.begin());
  thrust::copy(hInstDDy.begin(), hInstDDy.end(), dInstDDy_.begin());
  thrust::copy(hInstDCx.begin(), hInstDCx.end(), dInstDCx_.begin());
  thrust::copy(hInstDCy.begin(), hInstDCy.end(), dInstDCy_.begin());
  thrust::copy(hWireLengthPrecondi.begin(),
               hWireLengthPrecondi.end(),
               dWireLengthPrecondi_.begin());
  thrust::copy(hDensityPrecondi.begin(),
               hDensityPrecondi.end(),
               dDensityPrecondi_.begin());

  dDensityGradXPtr_ = setThrustVector<float>(numInsts_, dDensityGradX_);
  dDensityGradYPtr_ = setThrustVector<float>(numInsts_, dDensityGradY_);

  dCurSLPCoordiPtr_ = setThrustVector<FloatPoint>(numInsts_, dCurSLPCoordi_);
  dCurSLPWireLengthGradXPtr_
      = setThrustVector<float>(numInsts_, dCurSLPWireLengthGradX_);
  dCurSLPWireLengthGradYPtr_
      = setThrustVector<float>(numInsts_, dCurSLPWireLengthGradY_);
  dCurSLPDensityGradXPtr_
      = setThrustVector<float>(numInsts_, dCurSLPDensityGradX_);
  dCurSLPDensityGradYPtr_
      = setThrustVector<float>(numInsts_, dCurSLPDensityGradY_);
  dCurSLPSumGradsPtr_
      = setThrustVector<FloatPoint>(numInsts_, dCurSLPSumGrads_);

  dPrevSLPCoordiPtr_ = setThrustVector<FloatPoint>(numInsts_, dPrevSLPCoordi_);
  dPrevSLPWireLengthGradXPtr_
      = setThrustVector<float>(numInsts_, dPrevSLPWireLengthGradX_);
  dPrevSLPWireLengthGradYPtr_
      = setThrustVector<float>(numInsts_, dPrevSLPWireLengthGradY_);
  dPrevSLPDensityGradXPtr_
      = setThrustVector<float>(numInsts_, dPrevSLPDensityGradX_);
  dPrevSLPDensityGradYPtr_
      = setThrustVector<float>(numInsts_, dPrevSLPDensityGradY_);
  dPrevSLPSumGradsPtr_
      = setThrustVector<FloatPoint>(numInsts_, dPrevSLPSumGrads_);

  dNextSLPCoordiPtr_ = setThrustVector<FloatPoint>(numInsts_, dNextSLPCoordi_);
  dNextSLPWireLengthGradXPtr_
      = setThrustVector<float>(numInsts_, dNextSLPWireLengthGradX_);
  dNextSLPWireLengthGradYPtr_
      = setThrustVector<float>(numInsts_, dNextSLPWireLengthGradY_);
  dNextSLPDensityGradXPtr_
      = setThrustVector<float>(numInsts_, dNextSLPDensityGradX_);
  dNextSLPDensityGradYPtr_
      = setThrustVector<float>(numInsts_, dNextSLPDensityGradY_);
  dNextSLPSumGradsPtr_
      = setThrustVector<FloatPoint>(numInsts_, dNextSLPSumGrads_);

  dCurCoordiPtr_ = setThrustVector<FloatPoint>(numInsts_, dCurCoordi_);
  dNextCoordiPtr_ = setThrustVector<FloatPoint>(numInsts_, dNextCoordi_);

  dSumGradsXPtr_ = setThrustVector<float>(numInsts_, dSumGradsX_);
  dSumGradsYPtr_ = setThrustVector<float>(numInsts_, dSumGradsY_);

  densityOp_ = new DensityOp(this);
}

void PlacerBase::freeCUDAKernel()
{
  densityOp_ = nullptr;
  dPlaceInstIdsPtr_ = nullptr;

  dDensityGradXPtr_ = nullptr;
  dDensityGradYPtr_ = nullptr;

  dCurSLPCoordiPtr_ = nullptr;
  dCurSLPSumGradsPtr_ = nullptr;

  dPrevSLPCoordiPtr_ = nullptr;
  dPrevSLPSumGradsPtr_ = nullptr;

  dNextSLPCoordiPtr_ = nullptr;
  dNextSLPSumGradsPtr_ = nullptr;

  dCurSLPCoordiPtr_ = nullptr;
  dCurSLPSumGradsPtr_ = nullptr;
}

// Make sure the instances are within the region
__device__ float getDensityCoordiLayoutInside(int instWidth,
                                              float cx,
                                              int coreLx,
                                              int coreUx)
{
  float adjVal = cx;
  if (cx - instWidth / 2 < coreLx) {
    adjVal = coreLx + instWidth / 2;
  }

  if (cx + instWidth / 2 > coreUx) {
    adjVal = coreUx - instWidth / 2;
  }

  return adjVal;
}

__global__ void updateDensityCoordiLayoutInsideKernel(const int numInsts,
                                                      const int coreLx,
                                                      const int coreLy,
                                                      const int coreUx,
                                                      const int coreUy,
                                                      const int* instDDx,
                                                      const int* instDDy,
                                                      int* instDCx,
                                                      int* instDCy)
{
  int instIdx = blockIdx.x * blockDim.x + threadIdx.x;
  if (instIdx < numInsts) {
    instDCx[instIdx] = getDensityCoordiLayoutInside(
        instDDx[instIdx], instDCx[instIdx], coreLx, coreUx);
    instDCy[instIdx] = getDensityCoordiLayoutInside(
        instDDy[instIdx], instDCy[instIdx], coreLy, coreUy);
  }
}

__global__ void initDensityCoordiKernel(int numInsts,
                                        const int* instDCx,
                                        const int* instDCy,
                                        FloatPoint* dCurCoordiPtr,
                                        FloatPoint* dCurSLPCoordiPtr,
                                        FloatPoint* dPrevSLPCoordiPtr)
{
  int instIdx = blockIdx.x * blockDim.x + threadIdx.x;
  if (instIdx < numInsts) {
    const FloatPoint loc(instDCx[instIdx], instDCy[instIdx]);
    dCurCoordiPtr[instIdx] = loc;
    dCurSLPCoordiPtr[instIdx] = loc;
    dPrevSLPCoordiPtr[instIdx] = loc;
  }
}

void PlacerBase::initDensity1()
{
  // update density coordinate for each instance
  int numThreads = 256;
  int numBlocks = (numInsts_ + numThreads - 1) / numThreads;
  updateDensityCoordiLayoutInsideKernel<<<numBlocks, numThreads>>>(
      numInsts_,
      bg_.lx(),
      bg_.ly(),
      bg_.ux(),
      bg_.uy(),
      dInstDDxPtr_,
      dInstDDyPtr_,
      dInstDCxPtr_,
      dInstDCyPtr_);

  // initialize the dCurSLPCoordiPtr_, dPrevSLPCoordiPtr_
  // and dCurCoordiPtr_
  initDensityCoordiKernel<<<numBlocks, numThreads>>>(numInsts_,
                                                     dInstDCxPtr_,
                                                     dInstDCyPtr_,
                                                     dCurCoordiPtr_,
                                                     dCurSLPCoordiPtr_,
                                                     dPrevSLPCoordiPtr_);

  // We need to sync up bewteen pb and pbCommon
  updateGCellDensityCenterLocation(dCurSLPCoordiPtr_);
  pbCommon_->updatePinLocation();
  // calculate the previous hpwl
  // update the location of instances within this region
  // while the instances in other regions will not change
  prevHpwl_ = pbCommon_->hpwl();

  // FFT update
  updateDensityForceBin();

  // update parameters
  baseWireLengthCoef_ = npVars_.initWireLengthCoef
                        / (static_cast<float>(binSizeX() + binSizeY()) * 0.5);

  sumOverflow_ = static_cast<float>(overflowArea())
                 / static_cast<float>(nesterovInstsArea());

  sumOverflowUnscaled_ = static_cast<float>(overflowAreaUnscaled())
                         / static_cast<float>(nesterovInstsArea());
}

// (a)  // (a) define the get distance method
// getDistance is only defined on the host side
struct getTupleDistanceFunctor
{
  __host__ __device__ float operator()(
      const thrust::tuple<FloatPoint, FloatPoint>& t)
  {
    const FloatPoint& a = thrust::get<0>(t);
    const FloatPoint& b = thrust::get<1>(t);
    float dist = 0.0f;
    dist += (a.x - b.x) * (a.x - b.x);
    dist += (a.y - b.y) * (a.y - b.y);
    return dist;
  }
};

__host__ float getDistance(const FloatPoint* a,
                           const FloatPoint* b,
                           const int numInsts)
{
  if (numInsts <= 0) {
    return 0.0;
  }

  thrust::device_ptr<FloatPoint> aBegin(const_cast<FloatPoint*>(a));
  thrust::device_ptr<FloatPoint> aEnd = aBegin + numInsts;

  thrust::device_ptr<FloatPoint> bBegin(const_cast<FloatPoint*>(b));
  thrust::device_ptr<FloatPoint> bEnd = bBegin + numInsts;

  float sumDistance = thrust::transform_reduce(
      thrust::make_zip_iterator(thrust::make_tuple(aBegin, bBegin)),
      thrust::make_zip_iterator(thrust::make_tuple(aEnd, bEnd)),
      getTupleDistanceFunctor(),
      0.0f,
      thrust::plus<float>());

  return std::sqrt(sumDistance / (2.0 * numInsts));
}

template <typename T>
struct myAbs
{
  __host__ __device__ double operator()(const T& x) const
  {
    if (x >= 0)
      return x;
    else
      return x * -1;
  }
};

__host__ float getAbsGradSum(const float* a, const int numInsts)
{
  thrust::device_ptr<float> aBegin(const_cast<float*>(a));
  thrust::device_ptr<float> aEnd = aBegin + numInsts;
  double sumAbs = thrust::transform_reduce(
      aBegin, aEnd, myAbs<float>(), 0.0, thrust::plus<double>());
  return sumAbs;
}

float PlacerBase::getStepLength(const FloatPoint* prevSLPCoordi,
                                const FloatPoint* prevSLPSumGrads,
                                const FloatPoint* curSLPCoordi,
                                const FloatPoint* curSLPSumGrads) const
{
  float coordiDistance = getDistance(prevSLPCoordi, curSLPCoordi, numInsts_);
  float gradDistance = getDistance(prevSLPSumGrads, curSLPSumGrads, numInsts_);
  return coordiDistance / gradDistance;
}

// Function: initDensity2
float PlacerBase::initDensity2()
{
  // the wirelength force on each instance is zero
  if (wireLengthGradSum_ == 0) {
    densityPenalty_ = npVars_.initDensityPenalty;
    updatePrevGradient();
  }

  if (wireLengthGradSum_ != 0) {
    densityPenalty_
        = (wireLengthGradSum_ / densityGradSum_) * npVars_.initDensityPenalty;
  }

  sumOverflow_ = static_cast<float>(overflowArea())
                 / static_cast<float>(nesterovInstsArea());

  sumOverflowUnscaled_ = static_cast<float>(overflowAreaUnscaled())
                         / static_cast<float>(nesterovInstsArea());

  stepLength_ = getStepLength(dPrevSLPCoordiPtr_,
                              dPrevSLPSumGradsPtr_,
                              dCurSLPCoordiPtr_,
                              dCurSLPSumGradsPtr_);

  return stepLength_;
}

__global__ void sumGradientKernel(const int numInsts,
                                  const float densityPenalty,
                                  const float minPrecondi,
                                  const float* wireLengthPrecondi,
                                  const float* densityPrecondi,
                                  const float* wireLengthGradientsX,
                                  const float* wireLengthGradientsY,
                                  const float* densityGradientsX,
                                  const float* densityGradientsY,
                                  FloatPoint* sumGrads)
{
  int instIdx = blockIdx.x * blockDim.x + threadIdx.x;
  if (instIdx < numInsts) {
    sumGrads[instIdx].x = wireLengthGradientsX[instIdx]
                          + densityPenalty * densityGradientsX[instIdx];
    sumGrads[instIdx].y = wireLengthGradientsY[instIdx]
                          + densityPenalty * densityGradientsY[instIdx];
    FloatPoint sumPrecondi(
        wireLengthPrecondi[instIdx] + densityPenalty * densityPrecondi[instIdx],
        wireLengthPrecondi[instIdx]
            + densityPenalty * densityPrecondi[instIdx]);

    if (sumPrecondi.x < minPrecondi) {
      sumPrecondi.x = minPrecondi;
    }

    if (sumPrecondi.y < minPrecondi) {
      sumPrecondi.y = minPrecondi;
    }

    sumGrads[instIdx].x /= sumPrecondi.x;
    sumGrads[instIdx].y /= sumPrecondi.y;
  }
}

void PlacerBase::updatePrevGradient()
{
  updateGradients(dPrevSLPWireLengthGradXPtr_,
                  dPrevSLPWireLengthGradYPtr_,
                  dPrevSLPDensityGradXPtr_,
                  dPrevSLPDensityGradYPtr_,
                  dPrevSLPSumGradsPtr_);
}

void PlacerBase::updateCurGradient()
{
  updateGradients(dCurSLPWireLengthGradXPtr_,
                  dCurSLPWireLengthGradYPtr_,
                  dCurSLPDensityGradXPtr_,
                  dCurSLPDensityGradYPtr_,
                  dCurSLPSumGradsPtr_);
}

void PlacerBase::updateNextGradient()
{
  updateGradients(dNextSLPWireLengthGradXPtr_,
                  dNextSLPWireLengthGradYPtr_,
                  dNextSLPDensityGradXPtr_,
                  dNextSLPDensityGradYPtr_,
                  dNextSLPSumGradsPtr_);
}

void PlacerBase::updateGradients(float* wireLengthGradientsX,
                                 float* wireLengthGradientsY,
                                 float* densityGradientsX,
                                 float* densityGradientsY,
                                 FloatPoint* sumGrads)
{
  if (isConverged_) {
    return;
  }

  wireLengthGradSum_ = 0;
  densityGradSum_ = 0;

  // get the forces on each instance
  getWireLengthGradientWA(wireLengthGradientsX, wireLengthGradientsY);
  getDensityGradient(densityGradientsX, densityGradientsY);

  wireLengthGradSum_ += getAbsGradSum(wireLengthGradientsX, numInsts_);
  wireLengthGradSum_ += getAbsGradSum(wireLengthGradientsY, numInsts_);
  densityGradSum_ += getAbsGradSum(densityGradientsX, numInsts_);
  densityGradSum_ += getAbsGradSum(densityGradientsY, numInsts_);

  int numThreads = 256;
  int numBlocks = (numInsts_ + numThreads - 1) / numThreads;
  sumGradientKernel<<<numBlocks, numThreads>>>(numInsts_,
                                               densityPenalty_,
                                               npVars_.minPreconditioner,
                                               dWireLengthPrecondiPtr_,
                                               dDensityPrecondiPtr_,
                                               wireLengthGradientsX,
                                               wireLengthGradientsY,
                                               densityGradientsX,
                                               densityGradientsY,
                                               sumGrads);
}

// sync up the instances location based on the corrodinates
__global__ void updateGCellDensityCenterLocationKernel(
    const int numInsts,
    const FloatPoint* coordis,
    int* instDCx,
    int* instDCy)
{
  int instIdx = blockIdx.x * blockDim.x + threadIdx.x;
  if (instIdx < numInsts) {
    instDCx[instIdx] = coordis[instIdx].x;
    instDCy[instIdx] = coordis[instIdx].y;
  }
}

// sync up the instances between pbCommon and current pb
__global__ void syncPlaceInstsCommonKernel(const int numPlaceInsts,
                                           const int* placeInstIds,
                                           const int* placeInstDCx,
                                           const int* placeInstDCy,
                                           int* instDCxCommon,
                                           int* instDCyCommon)
{
  int instIdx = blockIdx.x * blockDim.x + threadIdx.x;
  if (instIdx < numPlaceInsts) {
    int instId = placeInstIds[instIdx];
    instDCxCommon[instId] = placeInstDCx[instIdx];
    instDCyCommon[instId] = placeInstDCy[instIdx];
  }
}

void PlacerBase::updateGCellDensityCenterLocation(const FloatPoint* coordis)
{
  const int numThreads = 256;
  const int numBlocks = (numInsts_ + numThreads - 1) / numThreads;
  const int numPlaceInstBlocks = (numPlaceInsts_ + numThreads - 1) / numThreads;

  updateGCellDensityCenterLocationKernel<<<numBlocks, numThreads>>>(
      numInsts_, coordis, dInstDCxPtr_, dInstDCyPtr_);

  syncPlaceInstsCommonKernel<<<numPlaceInstBlocks, numThreads>>>(
      numPlaceInsts_,
      dPlaceInstIdsPtr_,
      dInstDCxPtr_,
      dInstDCyPtr_,
      pbCommon_->dInstDCxPtr(),
      pbCommon_->dInstDCyPtr());

  densityOp_->updateGCellLocation(dInstDCxPtr_, dInstDCyPtr_);
}

__global__ void getWireLengthGradientWAKernel(const int numPlaceInsts,
                                              const int* dPlaceInstIdsPtr,
                                              const float* dWLGradXCommonPtr,
                                              const float* dWLGradYCommonPtr,
                                              float* dWireLengthGradXPtr,
                                              float* dWireLengthGradYPtr)
{
  int instIdx = blockIdx.x * blockDim.x + threadIdx.x;
  if (instIdx < numPlaceInsts) {
    int instId = dPlaceInstIdsPtr[instIdx];
    dWireLengthGradXPtr[instIdx] = dWLGradXCommonPtr[instId];
    dWireLengthGradYPtr[instIdx] = dWLGradYCommonPtr[instId];
  }
}

void PlacerBase::getWireLengthGradientWA(float* wireLengthGradientsX,
                                         float* wireLengthGradientsY)
{
  int numThreads = 256;
  int numBlocks = (numPlaceInsts_ + numThreads - 1) / numThreads;

  getWireLengthGradientWAKernel<<<numBlocks, numThreads>>>(
      numPlaceInsts_,
      dPlaceInstIdsPtr_,
      pbCommon_->dWLGradXPtr(),
      pbCommon_->dWLGradYPtr(),
      wireLengthGradientsX,
      wireLengthGradientsY);
}

void PlacerBase::getDensityGradient(float* densityGradientsX,
                                    float* densityGradientsY)
{
  densityOp_->getDensityGradient(densityGradientsX, densityGradientsY);
}

// calculate the next state based on current state
__global__ void nesterovUpdateCooridnatesKernel(
    const int numInsts,
    const int coreLx,
    const int coreLy,
    const int coreUx,
    const int coreUy,
    const float stepLength,
    const float coeff,
    const int* instDDx,
    const int* instDDy,
    const FloatPoint* curCoordiPtr,
    const FloatPoint* curSLPCoordiPtr,
    const FloatPoint* curSLPSumGradsPtr,
    FloatPoint* nextCoordiPtr,
    FloatPoint* nextSLPCoordiPtr)
{
  int instIdx = blockIdx.x * blockDim.x + threadIdx.x;
  if (instIdx < numInsts) {
    FloatPoint nextCoordi(
        curSLPCoordiPtr[instIdx].x + stepLength * curSLPSumGradsPtr[instIdx].x,
        curSLPCoordiPtr[instIdx].y + stepLength * curSLPSumGradsPtr[instIdx].y);

    FloatPoint nextSLPCoordi(
        nextCoordi.x + coeff * (nextCoordi.x - curCoordiPtr[instIdx].x),
        nextCoordi.y + coeff * (nextCoordi.y - curCoordiPtr[instIdx].y));

    // check the boundary
    nextCoordiPtr[instIdx]
        = FloatPoint(getDensityCoordiLayoutInside(
                         instDDx[instIdx], nextCoordi.x, coreLx, coreUx),
                     getDensityCoordiLayoutInside(
                         instDDy[instIdx], nextCoordi.y, coreLy, coreUy));

    nextSLPCoordiPtr[instIdx]
        = FloatPoint(getDensityCoordiLayoutInside(
                         instDDx[instIdx], nextSLPCoordi.x, coreLx, coreUx),
                     getDensityCoordiLayoutInside(
                         instDDy[instIdx], nextSLPCoordi.y, coreLy, coreUy));
  }
}

void PlacerBase::nesterovUpdateCoordinates(float coeff)
{
  if (isConverged_) {
    return;
  }

  int numThreads = 256;
  int numBlocks = (numInsts_ + numThreads - 1) / numThreads;

  nesterovUpdateCooridnatesKernel<<<numBlocks, numThreads>>>(
      numInsts_,
      bg_.lx(),
      bg_.ly(),
      bg_.ux(),
      bg_.uy(),
      stepLength_,
      coeff,
      dInstDDxPtr_,
      dInstDDyPtr_,
      dCurCoordiPtr_,
      dCurSLPCoordiPtr_,
      dCurSLPSumGradsPtr_,
      dNextCoordiPtr_,
      dNextSLPCoordiPtr_);

  // update density
  updateGCellDensityCenterLocation(dNextSLPCoordiPtr_);
  updateDensityForceBin();
}

__global__ void updateInitialPrevSLPCoordiKernel(
    const int numInsts,
    const int coreLx,
    const int coreLy,
    const int coreUx,
    const int coreUy,
    const int* instDDx,
    const int* instDDy,
    const float initialPrevCoordiUpdateCoef,
    const FloatPoint* dCurSLPCoordiPtr,
    const FloatPoint* dCurSLPSumGradsPtr,
    FloatPoint* dPrevSLPCoordiPtr)
{
  int instIdx = blockIdx.x * blockDim.x + threadIdx.x;
  if (instIdx < numInsts) {
    const float preCoordiX
        = dCurSLPCoordiPtr[instIdx].x
          - initialPrevCoordiUpdateCoef * dCurSLPSumGradsPtr[instIdx].x;
    const float preCoordiY
        = dCurSLPCoordiPtr[instIdx].y
          - initialPrevCoordiUpdateCoef * dCurSLPSumGradsPtr[instIdx].y;
    const FloatPoint newCoordi(
        getDensityCoordiLayoutInside(
            instDDx[instIdx], preCoordiX, coreLx, coreUx),
        getDensityCoordiLayoutInside(
            instDDy[instIdx], preCoordiY, coreLy, coreUy));
    dPrevSLPCoordiPtr[instIdx] = newCoordi;
  }
}

void PlacerBase::updateInitialPrevSLPCoordi()
{
  const int numThreads = 256;
  const int numBlocks = (numInsts_ + numThreads - 1) / numThreads;
  updateInitialPrevSLPCoordiKernel<<<numBlocks, numThreads>>>(
      numInsts_,
      bg_.lx(),
      bg_.ly(),
      bg_.ux(),
      bg_.uy(),
      dInstDDxPtr_,
      dInstDDyPtr_,
      npVars_.initialPrevCoordiUpdateCoef,
      dCurSLPCoordiPtr_,
      dCurSLPSumGradsPtr_,
      dPrevSLPCoordiPtr_);
}

}  // namespace gpl2
