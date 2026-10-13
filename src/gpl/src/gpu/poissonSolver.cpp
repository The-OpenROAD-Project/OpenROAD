// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

// The density force is calculated by solving the Poisson equation.
// Derived from UCSD's DG-RePlAce (github.com/ABKGroup/DG-RePlAce,
// BSD-3-Clause, Copyright (c) The Regents of the University of California),
// via the Kokkos conversion in PR #5352. The Poisson solver itself was
// originally written by Jaekyung Im (jkim97@postech.ac.kr, POSTECH); we
// thank him for his contribution.

#include "poissonSolver.h"

#include <Kokkos_Core.hpp>
#include <cstdio>
#include <stdexcept>

#include "kokkosUtil.h"

namespace gpl {

PoissonSolver::PoissonSolver()
    : binCntX_(0), binCntY_(0), binSizeX_(0), binSizeY_(0)
{
}

// The IDCT post-processing kernel in dct.cpp indexes
//   expkMN2[halfN - hid + (N-1)]      (hid up to M/2)
//   expkMN2[wid - hid + (N-1)]        (wid up to N/2, hid up to M/2)
// Both go negative when M is substantially larger than N. The expkMN1/2
// allocation is sized 2*max(N,M), so the upper bound is safe, but the
// lower bound requires M <= 2N (and symmetrically N <= 2M for the
// transposed path). Typical placer bin grids satisfy this with margin.
constexpr int kMaxBinAspectRatio = 2;

PoissonSolver::PoissonSolver(int binCntX,
                             int binCntY,
                             float binSizeX,
                             float binSizeY)
    : PoissonSolver()
{
  // Host-side preconditions: throw so the gpl error handler can log via
  // utl::Logger instead of process-abort with raw stderr only. Surface
  // these at construction so the first solve() can't be the first sign of
  // a misconfigured bin grid.
  if (!isPowerOf2(binCntX) || !isPowerOf2(binCntY)) {
    throw std::runtime_error(
        "PoissonSolver: bin grid dimensions must each be a power of 2 — "
        "the DCT/IDCT kernels in dct.cpp require this.");
  }
  if (binCntY > kMaxBinAspectRatio * binCntX
      || binCntX > kMaxBinAspectRatio * binCntY) {
    throw std::runtime_error(
        "PoissonSolver: bin grid aspect ratio exceeds the supported limit "
        "(kMaxBinAspectRatio=2) — IDCT indexing may go out of bounds. "
        "Increase the shorter dimension or extend the solver's expk index "
        "math to handle this case.");
  }

  binCntX_ = binCntX;
  binCntY_ = binCntY;
  binSizeX_ = binSizeX;
  binSizeY_ = binSizeY;

  initBackend();
}

KOKKOS_FUNCTION void divideByWSquare(const int wID,
                                     const int hID,
                                     const int binCntX,
                                     const int binCntY,
                                     const float binSizeX,
                                     const float binSizeY,
                                     Kokkos::View<float*> input)
{
  if (wID < binCntX && hID < binCntY) {
    int binID = wID + hID * binCntX;

    if (hID == 0 && wID == 0) {
      input[binID] = 0.0;
    } else {
      float denom1 = (2.0 * float(FFT_PI) * wID) / binCntX;
      float denom2
          = (2.0 * float(FFT_PI) * hID) / binCntY * binSizeY / binSizeX;

      input[binID] /= (denom1 * denom1 + denom2 * denom2);
    }
  }
}

void PoissonSolver::launchDivideByWSquare()
{
  const auto binCntX = binCntX_;
  const auto binCntY = binCntY_;
  const auto binSizeX = binSizeX_;
  const auto binSizeY = binSizeY_;
  auto d_auv = d_auv_;
  Kokkos::parallel_for(
      Kokkos::MDRangePolicy<Kokkos::Rank<2>>({0, 0}, {binCntX_, binCntY_}),
      KOKKOS_LAMBDA(const int wID, const int hID) {
        divideByWSquare(wID, hID, binCntX, binCntY, binSizeX, binSizeY, d_auv);
      });
}

void PoissonSolver::solvePoissonPotential(Kokkos::View<float*> binDensity,
                                          Kokkos::View<float*> potential)
{
  // Step #1. Compute Coefficient (a_uv)
  dct_2d_fft(binCntY_,
             binCntX_,
             d_expkM_,
             d_expkN_,
             binDensity,
             d_workSpaceReal1_,
             d_workSpaceComplex_,
             d_auv_);

  // Step #2. Divide by (w_u^2 + w_v^2)
  launchDivideByWSquare();

  // Step #3. Compute Potential
  idct_2d_fft(binCntY_,
              binCntX_,
              d_expkMForInverse_,
              d_expkNForInverse_,
              d_expkMN1_,
              d_expkMN2_,
              d_auv_,
              d_workSpaceComplex_,
              d_workSpaceReal1_,
              potential);
}

void PoissonSolver::solvePoisson(Kokkos::View<float*> binDensity,
                                 Kokkos::View<float*> potential,
                                 Kokkos::View<float*> electroForceX,
                                 Kokkos::View<float*> electroForceY)
{
  // Step #1. Compute Coefficient (a_uv)
  dct_2d_fft(binCntY_,
             binCntX_,
             d_expkM_,
             d_expkN_,
             binDensity,
             d_workSpaceReal1_,
             d_workSpaceComplex_,
             d_auv_);

  // Step #2. Divide by (w_u^2 + w_v^2)
  launchDivideByWSquare();

  // Step #3. Compute Potential
  idct_2d_fft(binCntY_,
              binCntX_,
              d_expkMForInverse_,
              d_expkNForInverse_,
              d_expkMN1_,
              d_expkMN2_,
              d_auv_,
              d_workSpaceComplex_,
              d_workSpaceReal1_,
              potential);

  solveFieldFromAuv(electroForceX, electroForceY);
}

void PoissonSolver::solvePoissonField(Kokkos::View<float*> binDensity,
                                      Kokkos::View<float*> electroForceX,
                                      Kokkos::View<float*> electroForceY)
{
  // Steps #1-#2 of solvePoisson; the potential IDCT (step #3) is skipped.
  dct_2d_fft(binCntY_,
             binCntX_,
             d_expkM_,
             d_expkN_,
             binDensity,
             d_workSpaceReal1_,
             d_workSpaceComplex_,
             d_auv_);

  launchDivideByWSquare();

  solveFieldFromAuv(electroForceX, electroForceY);
}

void PoissonSolver::solveFieldFromAuv(Kokkos::View<float*> electroForceX,
                                      Kokkos::View<float*> electroForceY)
{
  // Step #4. Multiply w_u , w_v
  const auto binCntX = binCntX_;
  const auto binCntY = binCntY_;
  const auto binSizeX = binSizeX_;
  const auto binSizeY = binSizeY_;
  auto d_auv = d_auv_;
  auto d_inputForX = d_inputForX_, d_inputForY = d_inputForY_;
  Kokkos::parallel_for(
      Kokkos::MDRangePolicy<Kokkos::Rank<2>>({0, 0}, {binCntX_, binCntY_}),
      KOKKOS_LAMBDA(const int wID, const int hID) {
        int binID = wID + hID * binCntX;

        float w_u = (2.0 * float(FFT_PI) * wID) / binCntX;
        float w_v = (2.0 * float(FFT_PI) * hID) / binCntY * binSizeY / binSizeX;

        d_inputForX[binID] = w_u * d_auv[binID];
        d_inputForY[binID] = w_v * d_auv[binID];
      });

  // Step #5. Compute ElectroForceX
  idxst_idct(binCntY_,
             binCntX_,
             d_expkMForInverse_,
             d_expkNForInverse_,
             d_expkMN1_,
             d_expkMN2_,
             d_inputForX_,
             d_workSpaceReal1_,
             d_workSpaceComplex_,
             d_workSpaceReal2_,
             d_workSpaceReal3_,
             electroForceX);

  // Step #6. Compute ElectroForceY
  idct_idxst(binCntY_,
             binCntX_,
             d_expkMForInverse_,
             d_expkNForInverse_,
             d_expkMN1_,
             d_expkMN2_,
             d_inputForY_,
             d_workSpaceReal1_,
             d_workSpaceComplex_,
             d_workSpaceReal2_,
             d_workSpaceReal3_,
             electroForceY);
}

void PoissonSolver::initBackend()
{
  d_auv_ = Kokkos::View<float*>("d_auv", binCntX_ * binCntY_);

  d_workSpaceReal1_
      = Kokkos::View<float*>("d_workSpaceReal1", binCntX_ * binCntY_);
  d_workSpaceReal2_
      = Kokkos::View<float*>("d_workSpaceReal2", binCntX_ * binCntY_);
  d_workSpaceReal3_
      = Kokkos::View<float*>("d_workSpaceReal3", binCntX_ * binCntY_);

  d_workSpaceComplex_ = Kokkos::View<Kokkos::complex<float>*>(
      "d_workSpaceComplex", (binCntX_ / 2 + 1) * binCntY_);

  // expk
  // For DCT2D
  d_expkM_ = Kokkos::View<Kokkos::complex<float>*>("d_expkM", binCntY_ / 2 + 1);
  d_expkN_ = Kokkos::View<Kokkos::complex<float>*>("d_expkN", binCntX_ / 2 + 1);

  // For IDCT2D & IDXST_IDCT & IDCT_IDXST
  d_expkMForInverse_
      = Kokkos::View<Kokkos::complex<float>*>("d_expkMForInverse", binCntY_);
  d_expkNForInverse_ = Kokkos::View<Kokkos::complex<float>*>(
      "d_expkNForInverse", binCntX_ / 2 + 1);

  d_expkMN1_ = Kokkos::View<Kokkos::complex<float>*>(
      "d_expkMN1", 2 * std::max(binCntX_, binCntY_));
  d_expkMN2_ = Kokkos::View<Kokkos::complex<float>*>(
      "d_expkMN2", 2 * std::max(binCntX_, binCntY_));

  // For Input For IDXST_IDCT & IDCT_IDXST
  d_inputForX_ = Kokkos::View<float*>("d_inputForX", binCntX_ * binCntY_);
  d_inputForY_ = Kokkos::View<float*>("d_inputForY", binCntX_ * binCntY_);

  auto M = binCntY_, N = binCntX_;
  auto expkM = d_expkM_, expkN = d_expkN_;
  Kokkos::parallel_for(
      std::max(binCntX_, binCntY_), KOKKOS_LAMBDA(const int tID) {
        if (tID <= M / 2) {
          int hID = tID;
          Kokkos::complex<float> W_h_4M = Kokkos::complex<float>(
              consistentCosf((float) FFT_PI * hID / (2 * M)),
              -consistentSinf((float) FFT_PI * hID / (M * 2)));
          expkM[hID] = W_h_4M;
        }
        if (tID <= N / 2) {
          int wid = tID;
          Kokkos::complex<float> W_w_4N = Kokkos::complex<float>(
              consistentCosf((float) FFT_PI * wid / (2 * N)),
              -consistentSinf((float) FFT_PI * wid / (N * 2)));
          expkN[wid] = W_w_4N;
        }
      });

  auto expkMForInverse = d_expkMForInverse_,
       expkNForInverse = d_expkNForInverse_;
  auto expkMN_1 = d_expkMN1_, expkMN_2 = d_expkMN2_;
  Kokkos::parallel_for(
      std::max(binCntX_, binCntY_), KOKKOS_LAMBDA(const int tid) {
        if (tid < M) {
          int hid = tid;
          Kokkos::complex<float> W_h_4M = Kokkos::complex<float>(
              consistentCosf((float) FFT_PI * hid / (2 * M)),
              -consistentSinf((float) FFT_PI * hid / (M * 2)));
          expkMForInverse[hid] = W_h_4M;
          // expkMN_1
          Kokkos::complex<float> W_h_4M_offset = Kokkos::complex<float>(
              consistentCosf((float) FFT_PI * (hid + M) / (2 * M)),
              -consistentSinf((float) FFT_PI * (hid + M) / (M * 2)));
          expkMN_1[hid] = W_h_4M;
          expkMN_1[hid + M] = W_h_4M_offset;

          // expkMN_2
          W_h_4M = Kokkos::complex<float>(
              -consistentSinf((float) FFT_PI * (hid - (N - 1)) / (M * 2)),
              -consistentCosf((float) FFT_PI * (hid - (N - 1)) / (2 * M)));

          W_h_4M_offset = Kokkos::complex<float>(
              -consistentSinf((float) FFT_PI * (hid - (N - 1) + M) / (M * 2)),
              -consistentCosf((float) FFT_PI * (hid - (N - 1) + M) / (2 * M)));
          expkMN_2[hid] = W_h_4M;
          expkMN_2[hid + M] = W_h_4M_offset;
        }
        if (tid <= N / 2) {
          int wid = tid;
          Kokkos::complex<float> W_w_4N = Kokkos::complex<float>(
              consistentCosf((float) FFT_PI * wid / (2 * N)),
              -consistentSinf((float) FFT_PI * wid / (N * 2)));
          expkNForInverse[wid] = W_w_4N;
        }
      });
}

}  // namespace gpl
