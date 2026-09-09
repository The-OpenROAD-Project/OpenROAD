// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

// The density force is calculated by solving the Poisson equation.
// Derived from UCSD's DG-RePlAce (github.com/ABKGroup/DG-RePlAce,
// BSD-3-Clause, Copyright (c) The Regents of the University of California),
// via the Kokkos conversion in PR #5352. The Poisson solver itself was
// originally written by Jaekyung Im (jkim97@postech.ac.kr, POSTECH); we
// thank him for his contribution.

#pragma once

#include <Kokkos_Core.hpp>

#include "dct.h"

#define FFT_PI 3.141592653589793238462L

namespace gpl {

// Solver-frame → gpl-frame electric field adapter.
//
// The Poisson solver runs with its X/Y axes swapped relative to gpl's
// convention (see GpuFftBackend::Impl ctor: bin_cnt_y/bin_cnt_x are passed
// in solver order). The solver's DCT-derived field is also 2× the magnitude
// the legacy CPU Ooura backend produces. Both fix-ups apply at the point
// the solver output is consumed by gpl — the host unpack in
// GpuFftBackend::solve and the on-device gather in densityOp.cpp. Pinned by
// GpuFFTTest in src/gpl/test/fft_gpu_test.cc.
inline constexpr float kSolverToGplFieldScale = 0.5f;

// Result of solverToGplField — kept Kokkos-free POD so the helper is usable
// from both host code and KOKKOS_LAMBDA device kernels.
struct GplField
{
  float x;
  float y;
};

// Apply the solver→gpl axis swap and 0.5× field scale in one place.
KOKKOS_INLINE_FUNCTION GplField solverToGplField(float solver_elec_x,
                                                 float solver_elec_y)
{
  return {kSolverToGplFieldScale * solver_elec_y,
          kSolverToGplFieldScale * solver_elec_x};
}

class PoissonSolver
{
 public:
  PoissonSolver();
  PoissonSolver(int binCntX, int binCntY, float binSizeX, float binSizeY);
  ~PoissonSolver() = default;

  // Compute Potential and Electric Force in the row-major order
  void solvePoisson(Kokkos::View<float*> binDensity,
                    Kokkos::View<float*> potential,
                    Kokkos::View<float*> electroForceX,
                    Kokkos::View<float*> electroForceY);

  // Compute Potential Only (not Electric Force) the row-major order
  void solvePoissonPotential(Kokkos::View<float*> binDensity,
                             Kokkos::View<float*> potential);

  // Compute Electric Force only — skips the potential IDCT (one of the
  // four transforms). Used by the device-resident Nesterov hot loop, where
  // the potential feeds only the debug-only sumPhi metric.
  void solvePoissonField(Kokkos::View<float*> binDensity,
                         Kokkos::View<float*> electroForceX,
                         Kokkos::View<float*> electroForceY);

  // device memory management
  void initBackend();

  // Step #2 of solvePoisson/solvePoissonPotential — divide a_uv coefficients
  // by w_u^2 + w_v^2 per (wID, hID) bin index. Public because it contains an
  // extended __host__ __device__ lambda, which NVCC requires in a non-private
  // enclosing function.
  void launchDivideByWSquare();

  // Steps #4-#6 of solvePoisson — w_u/w_v multiply + the two field IDCTs,
  // reading the a_uv coefficients prepared by steps #1-#2. Public for the
  // same NVCC extended-lambda reason as launchDivideByWSquare.
  void solveFieldFromAuv(Kokkos::View<float*> electroForceX,
                         Kokkos::View<float*> electroForceY);

 private:
  int binCntX_;
  int binCntY_;
  float binSizeX_;
  float binSizeY_;

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

}  // namespace gpl
