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

#pragma once

#include "Kokkos_Core.hpp"

#include <memory>

#include "placerBase.h"
#include "placerObjects.h"
#include "poissonSolver.h"
#include "utl/Logger.h"

namespace gpl2 {

using utl::GPL2;

using int64_t_cu = uint64_t;

class DensityOp
{
 public:
  DensityOp(PlacerBase* pb);

  ~DensityOp() = default;

  void getDensityGradient(const Kokkos::View<float*>& densityGradientsX,
                          const Kokkos::View<float*>& densityGradientsY,
                          const Kokkos::View<float*>& densityGradAbsXPlusY);

  void updateDensityForceBin();

  void updateGCellLocation(const Kokkos::View<const int*>& instDCx, const Kokkos::View<const int*>& instDCy);

  float sumOverflow() const { return sumOverflow_; }
  float sumOverflowUnscaled() const { return sumOverflowUnscaled_; }

 private:
  PlacerBase* pb_;
  std::unique_ptr<PoissonSolver> fft_;  // fft to solve poisson equation
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

  // We need to store all the statistics information for each bin
  Kokkos::View<int*> dBinLx_;
  Kokkos::View<int*> dBinLy_;
  Kokkos::View<int*> dBinUx_;
  Kokkos::View<int*> dBinUy_;
  Kokkos::View<float*> dBinTargetDensity_;

  Kokkos::View<int64_t_cu*> dBinNonPlaceArea_;
  Kokkos::View<int64_t_cu*> dBinInstPlacedArea_;
  Kokkos::View<int64_t_cu*> dBinFillerArea_;
  Kokkos::View<float*> dBinScaledArea_;
  Kokkos::View<float*> dBinOverflowArea_;

  Kokkos::View<float*> dBinDensity_;
  Kokkos::View<float*> dBinElectroPhi_;
  Kokkos::View<float*> dBinElectroForceX_;
  Kokkos::View<float*> dBinElectroForceY_;

  // placeable instance information
  int numInsts_;  // placeInsts_ + fillerInsts_

  // overflow
  float sumOverflow_;
  float sumOverflowUnscaled_;

  // instance information
  Kokkos::View<int*> dGCellDensityWidth_;
  Kokkos::View<int*> dGCellDensityHeight_;
  Kokkos::View<int*> dGCellDCx_;
  Kokkos::View<int*> dGCellDCy_;

  // We modify the density scale for macro
  Kokkos::View<float*> dGCellDensityScale_;
  Kokkos::View<int*> dGCellIsFiller_;
  Kokkos::View<int*> dGCellIsMacro_;

  // device memory management
  void initBackend();
};

}  // namespace gpl2
