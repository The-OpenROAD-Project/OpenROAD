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
// The density force is calculated by solving the Poisson equation.
// It is originally developed by the graduate student Jaekyung Kim
// (jkim97@postech.ac.kr) at Pohang University of Science and Technology
// (POSTECH), then modified by our UCSD team. We thank Jaekyung Kim for his
// contribution.
//
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Kokkos_Core.hpp>

#include "dct.h"

#define FFT_PI 3.141592653589793238462L

namespace gpl2 {

class PoissonSolver
{
 public:
  PoissonSolver();
  PoissonSolver(int binCntX, int binCntY, int binSizeX, int binSizeY);
  ~PoissonSolver() = default;

  // Compute Potential and Electric Force in the row-major order
  void solvePoisson(Kokkos::View<float*> binDensity,
                    Kokkos::View<float*> potential,
                    Kokkos::View<float*> electroForceX,
                    Kokkos::View<float*> electroForceY);

  // Compute Potential Only (not Electric Force) the row-major order
  void solvePoissonPotential(Kokkos::View<float*> binDensity,
                             Kokkos::View<float*> potential);

  // device memory management
  void initBackend();

 private:
  int binCntX_;
  int binCntY_;
  int binSizeX_;
  int binSizeY_;

  Kokkos::View<Kokkos::complex<float>*> d_expkN_;
  Kokkos::View<Kokkos::complex<float>*> d_expkM_;

  Kokkos::View<Kokkos::complex<float>*> d_expkNForInverse_;
  Kokkos::View<Kokkos::complex<float>*> d_expkMForInverse_;

  Kokkos::View<Kokkos::complex<float>*> d_expkMN1_;
  Kokkos::View<Kokkos::complex<float>*> d_expkMN2_;

  Kokkos::View<float*> d_auv_;

  Kokkos::View<float*> d_workSpaceReal1_;
  Kokkos::View<float*> d_workSpaceReal2_;
  Kokkos::View<float*> d_workSpaceReal3_;

  Kokkos::View<Kokkos::complex<float>*> d_workSpaceComplex_;

  Kokkos::View<float*> d_inputForX_;
  Kokkos::View<float*> d_inputForY_;
};

};  // namespace gpl2
