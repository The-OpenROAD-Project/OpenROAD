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

#pragma once

#include <thrust/device_vector.h>
#include <thrust/execution_policy.h>
#include <thrust/functional.h>
#include <thrust/host_vector.h>
#include <thrust/iterator/counting_iterator.h>
#include <thrust/iterator/transform_iterator.h>

#include <ctime>
#include <memory>

#include "placerBase.h"
#include "placerObjects.h"
#include "util.h"
#include "utl/Logger.h"

namespace gpl2 {

class WirelengthOp
{
 public:
  WirelengthOp();
  WirelengthOp(PlacerBaseCommon* pbc);
  ~WirelengthOp();

  void computeWireLengthForce(float wlCoeffX,
                              float wlCoeffY,
                              float virtualWeightFactor,
                              float* wirelengthForceX,
                              float* wirelengthForceY);

  int64_t computeHPWL();
  int64_t computeWeightedHPWL(float virtualWeightFactor);

  void updatePinLocation(const int* instDCx, const int* instDCy);

 private:
  PlacerBaseCommon* pbc_;
  utl::Logger* logger_;

  // basic information
  int numInsts_;
  int numPins_;
  int numNets_;
  int numPlaceInsts_;

  // refer to hMETIS or TritonPart for this structure
  // map inst to pins
  thrust::device_vector<int> dInstPinIdx_;
  thrust::device_vector<int>
      dInstPinPos_;  // start - end position of pins of each inst
  thrust::device_vector<int> dPinInstId_;  // inst id of each pin
  int* dInstPinIdxPtr_;
  int* dInstPinPosPtr_;
  int* dPinInstIdPtr_;
  // map net to pins
  thrust::device_vector<int> dNetPinIdx_;
  thrust::device_vector<int>
      dNetPinPos_;  // start - end position of pins of each net
  thrust::device_vector<int> dPinNetId_;  // net id of each pin
  int* dNetPinIdxPtr_;
  int* dNetPinPosPtr_;
  int* dPinNetIdPtr_;

  // Pin information
  thrust::device_vector<int> dPinX_;
  thrust::device_vector<int> dPinY_;
  thrust::device_vector<int> dPinOffsetX_;
  thrust::device_vector<int> dPinOffsetY_;
  thrust::device_vector<float> dPinGradX_;
  thrust::device_vector<float> dPinGradY_;

  // For each pin, we have
  // aPosX, aPosY, aNegX, aNegY,
  // bPosX, bPosY, bNegX, bNegY,
  // cPosX, cPosY, cNegX, cNegY
  thrust::device_vector<float> dPinAPosX_;
  thrust::device_vector<float> dPinAPosY_;
  thrust::device_vector<float> dPinANegX_;
  thrust::device_vector<float> dPinANegY_;
  thrust::device_vector<float> dNetBPosX_;

  int* dPinXPtr_;
  int* dPinYPtr_;
  int* dPinOffsetXPtr_;
  int* dPinOffsetYPtr_;
  float* dPinGradXPtr_;
  float* dPinGradYPtr_;

  float* dPinAPosXPtr_;
  float* dPinAPosYPtr_;
  float* dPinANegXPtr_;
  float* dPinANegYPtr_;

  // Net information
  thrust::device_vector<int> dNetWidth_;
  thrust::device_vector<int> dNetHeight_;
  thrust::device_vector<int> dNetLx_;
  thrust::device_vector<int> dNetLy_;
  thrust::device_vector<int> dNetUx_;
  thrust::device_vector<int> dNetUy_;
  thrust::device_vector<float> dNetWeight_;
  thrust::device_vector<float> dNetVirtualWeight_;

  thrust::device_vector<float> dNetBPosY_;
  thrust::device_vector<float> dNetBNegX_;
  thrust::device_vector<float> dNetBNegY_;
  thrust::device_vector<float> dNetCPosX_;
  thrust::device_vector<float> dNetCPosY_;
  thrust::device_vector<float> dNetCNegX_;
  thrust::device_vector<float> dNetCNegY_;

  int* dNetWidthPtr_;
  int* dNetHeightPtr_;
  int* dNetLxPtr_;
  int* dNetLyPtr_;
  int* dNetUxPtr_;
  int* dNetUyPtr_;
  float* dNetWeightPtr_;
  float* dNetVirtualWeightPtr_;

  float* dNetBPosXPtr_;
  float* dNetBPosYPtr_;
  float* dNetBNegXPtr_;
  float* dNetBNegYPtr_;
  float* dNetCPosXPtr_;
  float* dNetCPosYPtr_;
  float* dNetCNegXPtr_;
  float* dNetCNegYPtr_;

  // device memory management
  void initCUDAKernel();
  void freeCUDAKernel();
};

}  // namespace gpl2
