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

#include <thrust/execution_policy.h>
#include <thrust/functional.h>
#include <thrust/iterator/counting_iterator.h>
#include <thrust/iterator/transform_iterator.h>
#include <thrust/device_vector.h>
#include <thrust/host_vector.h>

#include "cudaUtil.h"
#include "placerObjects.h"
#include "placerBase.h"
#include "utl/Logger.h"

namespace gpl2 {    

using namespace std;
using utl::GPL2;

typedef unsigned long long int int64_t_cu;

class DensityOp 
{ 
  public:
    DensityOp();
    DensityOp(PlacerBase* pb);

    ~DensityOp();

    void getDensityGradient(float* densityGradientsX, 
                            float* densityGradientsY);

    void updateDensityForceBin();

    void updateGCellLocation(const int* instDCx,
                             const int* instDCy);
  
    float sumOverflow() const { return sumOverflow_; }
    float sumOverflowUnscaled() const { return sumOverflowUnscaled_; }

    // for runtime debug
    double densityTime() const { return densityTime_; }
    double fftTime() const { return fftTime_; }


  private:
    PlacerBase* pb_;
    std::unique_ptr<PoissonSolver> fft_; // fft to solve poisson equation
    utl::Logger* logger_;
    
    // Bin information
    int numBins_;
    int binCntX_;
    int binCntY_;
    int binSizeX_;
    int binSizeY_;
    
    // region information (same as BinGrid (lx, ly, ux, uy)
    int coreLx_;
    int coreLy_;
    int coreUx_;
    int coreUy_;

    // We need to store all the statictis information for each bin
    thrust::device_vector<int> dBinLx_;
    thrust::device_vector<int> dBinLy_;
    thrust::device_vector<int> dBinUx_;
    thrust::device_vector<int> dBinUy_;
    thrust::device_vector<float> dBinTargetDensity_;

    int* dBinLxPtr_;
    int* dBinLyPtr_;
    int* dBinUxPtr_;
    int* dBinUyPtr_;
    float* dBinTargetDensityPtr_;

    thrust::device_vector<int64_t_cu> dBinNonPlaceArea_;
    thrust::device_vector<int64_t_cu> dBinInstPlacedArea_;
    thrust::device_vector<int64_t_cu> dBinFillerArea_;
    thrust::device_vector<float> dBinScaledArea_;
    thrust::device_vector<float> dBinOverflowArea_;

    int64_t_cu* dBinNonPlaceAreaPtr_;
    int64_t_cu* dBinInstPlacedAreaPtr_;
    int64_t_cu* dBinFillerAreaPtr_;
    float* dBinScaledAreaPtr_;
    float* dBinOverflowAreaPtr_;

    thrust::device_vector<float> dBinDensity_;
    thrust::device_vector<float> dBinElectroPhi_;
    thrust::device_vector<float> dBinElectroForceX_;
    thrust::device_vector<float> dBinElectroForceY_;  

    float* dBinDensityPtr_;
    float* dBinElectroPhiPtr_;
    float* dBinElectroForceXPtr_;
    float* dBinElectroForceYPtr_;
    
    // placeable instance information
    int numInsts_; // placeInsts_ + fillerInsts_

    // overflow
    float sumOverflow_;
    float sumOverflowUnscaled_;

    // instance information
    thrust::device_vector<int> dGCellDensityWidth_;
    thrust::device_vector<int> dGCellDensityHeight_;
    thrust::device_vector<int> dGCellDCx_;
    thrust::device_vector<int> dGCellDCy_;
    
    // We modify the density scale for macro
    thrust::device_vector<float> dGCellDensityScale_; 
    thrust::device_vector<bool> dGCellIsFiller_;
    thrust::device_vector<bool> dGCellIsMacro_;

    int *dGCellDensityWidthPtr_;
    int *dGCellDensityHeightPtr_;
    int *dGCellDCxPtr_;
    int *dGCellDCyPtr_;
  
    float *dGCellDensityScalePtr_;
    bool *dGCellIsFillerPtr_;
    bool *dGCellIsMacroPtr_;

    // device memory management
    void initCUDAKernel();
    void freeCUDAKernel();

    // for debug
    double densityTime_;
    double fftTime_;
};



}














