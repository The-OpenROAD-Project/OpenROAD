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

#include "placerObjects.h"
#include "placerBase.h"
#include "densityOp.h"

namespace gpl2 {   

//////////////////////////////////////////////////////////////
// Class DensityOp

DensityOp::DensityOp()
  : pb_(nullptr),
    fft_(nullptr),
    logger_(nullptr),
    // Bin information
    numBins_(0),
    binCntX_(0),
    binCntY_(0),
    binSizeX_(0),
    binSizeY_(0),
    // region information
    coreLx_(0),
    coreLy_(0),
    coreUx_(0),
    coreUy_(0),
    // bin pointers
    dBinLxPtr_(nullptr),
    dBinLyPtr_(nullptr),
    dBinUxPtr_(nullptr),
    dBinUyPtr_(nullptr),
    dBinNonPlaceAreaPtr_(nullptr),
    dBinInstPlacedAreaPtr_(nullptr),
    dBinFillerAreaPtr_(nullptr),
    dBinScaledAreaPtr_(nullptr),
    dBinOverflowAreaPtr_(nullptr),
    dBinDensityPtr_(nullptr),
    dBinElectroPhiPtr_(nullptr),
    dBinElectroForceXPtr_(nullptr),
    dBinElectroForceYPtr_(nullptr),
    // instance information
    numInsts_(0),
    sumOverflow_(0),
    dGCellDensityWidthPtr_(nullptr),
    dGCellDensityHeightPtr_(nullptr),
    dGCellDCxPtr_(nullptr),
    dGCellDCyPtr_(nullptr),
    dGCellDensityScalePtr_(nullptr),
    dGCellIsFillerPtr_(nullptr),
    // runtime
    densityTime_(0),
    fftTime_(0)
    {  }


DensityOp::DensityOp(PlacerBase* pb)
    : DensityOp()
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
  fft_ = std::make_unique<PoissonSolver>(binCntX_, 
                                         binCntY_,
                                         binSizeX_,
                                         binSizeY_);

  initCUDAKernel();
  logger_->report("[DensityOp] Initialization Succeed.");
}


DensityOp::~DensityOp()
{
  freeCUDAKernel();
}


}