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
// The density force is calculated by solving the Poisson equation.
// It is originally developed by the graduate student Jaekyung Kim
// (jkim97@postech.ac.kr) at Pohang University of Science and Technology
// (POSTECH), then modified by our UCSD team. We thank Jaekyung Kim for his
// contribution.
//
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cufft.h>
#include <stdio.h>
#include "util.h"
#include "cudaDCT.h"

#include <memory>

#define FFT_PI 3.141592653589793238462L

namespace gpl2 {

class PoissonSolver
{
 public:
  PoissonSolver();
  PoissonSolver(int binCntX, int binCntY, int binSizeX, int binSizeY);
  ~PoissonSolver();

  // Compute Potential and Electric Force in the row-major order
  void solvePoisson(const float* binDensity,
                    float* potential,
                    float* electroForceX,
                    float* electroForceY);

  // Compute Potential Only (not Electric Force) the row-major order
  void solvePoissonPotential(const float* binDensity, float* potential);

 private:
  int binCntX_;
  int binCntY_;
  int binSizeX_;
  int binSizeY_;

  // device memory management
  void initCUDAKernel();
  void freeCUDAKernel();

  cufftHandle plan_;
  cufftHandle planInverse_;

  cufftComplex* d_expkN_;
  cufftComplex* d_expkM_;

  cufftComplex* d_expkNForInverse_;
  cufftComplex* d_expkMForInverse_;

  cufftComplex* d_expkMN1_;
  cufftComplex* d_expkMN2_;

  cufftReal* d_binDensity_;
  cufftReal* d_auv_;
  cufftReal* d_potential_;

  cufftReal* d_efX_;
  cufftReal* d_efY_;

  cufftReal* d_workSpaceReal1_;
  cufftReal* d_workSpaceReal2_;
  cufftReal* d_workSpaceReal3_;

  cufftComplex* d_workSpaceComplex_;

  cufftReal* d_inputForX_;
  cufftReal* d_inputForY_;
};

};  // namespace gpl2
