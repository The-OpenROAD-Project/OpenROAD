// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

// The density force is calculated by solving the Poisson equation.
// Derived from UCSD's DG-RePlAce (github.com/ABKGroup/DG-RePlAce,
// BSD-3-Clause, Copyright (c) The Regents of the University of California),
// via the Kokkos conversion in PR #5352. Per the DG-RePlAce sources, the
// Poisson solver was originally developed by Jaekyung Kim (POSTECH).

#include "dct.h"

#include <KokkosFFT.hpp>
#include <Kokkos_Core.hpp>
#include <stdexcept>
#include <string>

#include "kokkosUtil.h"

namespace gpl {

namespace {

// Defensive guard: PoissonSolver's ctor validates power-of-2 dimensions at
// construction, so callers going through GpuFftBackend can't reach here
// with a bad N or M. Keep the per-function check as a safety net for any
// future caller of dct.cpp that bypasses PoissonSolver.
void requirePowerOf2Dims(int M, int N, const char* fn_name)
{
  if (!isPowerOf2(N) || !isPowerOf2(M)) {
    throw std::runtime_error(std::string(fn_name)
                             + ": input length is not a power of 2");
  }
}

}  // namespace

void dct_2d_fft(const int M,
                const int N,
                const Kokkos::View<const Kokkos::complex<float>*>& expkM,
                const Kokkos::View<const Kokkos::complex<float>*>& expkN,
                const Kokkos::View<const float*>& input,
                const Kokkos::View<float*>& pre,
                const Kokkos::View<Kokkos::complex<float>*>& fft,
                const Kokkos::View<float*>& post)
{
  requirePowerOf2Dims(M, N, "dct_2d_fft");

  auto halfN = N / 2;
  Kokkos::parallel_for(
      Kokkos::MDRangePolicy<Kokkos::Rank<2>>({0, 0}, {N, M}),
      KOKKOS_LAMBDA(const int wid, const int hid) {
        int index;
        int cond = (((hid & 1) == 0) << 1) | ((wid & 1) == 0);
        switch (cond) {
          case 0:
            index = INDEX((M << 1) - (hid + 1), N - ((wid + 1) >> 1), halfN);
            break;
          case 1:
            index = INDEX((M << 1) - (hid + 1), (wid >> 1), halfN);
            break;
          case 2:
            index = INDEX(hid, N - ((wid + 1) >> 1), halfN);
            break;
          case 3:
            index = INDEX(hid, (wid >> 1), halfN);
            break;
          default:
            Kokkos::abort("dct_2d_fft: unhandled cond");
            break;
        }
        pre[index] = input[INDEX(hid, wid, N)];
      });

  Kokkos::DefaultExecutionSpace exec;
  Kokkos::View<float**,
               Kokkos::LayoutRight,
               Kokkos::DefaultExecutionSpace,
               Kokkos::MemoryTraits<Kokkos::Unmanaged>>
      pre2d(pre.data(), M, N);
  Kokkos::View<Kokkos::complex<float>**,
               Kokkos::LayoutRight,
               Kokkos::DefaultExecutionSpace,
               Kokkos::MemoryTraits<Kokkos::Unmanaged>>
      fft2d(fft.data(), M, (N / 2) + 1);

  // For consistency we always calculate FFT on CPU (as Kokkos uses a different
  // implementation for GPU)
  Kokkos::DefaultHostExecutionSpace hostSpace;
  auto hPre2d = Kokkos::create_mirror_view_and_copy(hostSpace, pre2d);
  auto hFft2d = Kokkos::create_mirror_view(hostSpace, fft2d);

  KokkosFFT::Plan fftplan(hostSpace,
                          hPre2d,
                          hFft2d,
                          KokkosFFT::Direction::forward,
                          KokkosFFT::axis_type<2>{-2, -1});
  KokkosFFT::execute(fftplan, hPre2d, hFft2d, KokkosFFT::Normalization::none);

  Kokkos::deep_copy(fft2d, hFft2d);

  auto halfM = M / 2;
  auto two_over_MN = 2.0 / (M * N), four_over_MN = 4.0 / (M * N);
  Kokkos::parallel_for(
      Kokkos::MDRangePolicy<Kokkos::Rank<2>>({0, 0}, {N / 2, M / 2}),
      KOKKOS_LAMBDA(const int wid, const int hid) {
        int cond = ((hid != 0) << 1) | (wid != 0);
        switch (cond) {
          case 0: {
            post[0] = fft[0].real() * four_over_MN;
            post[halfN]
                = RealPartOfMul(expkN[halfN], fft[halfN]) * four_over_MN;

            post[INDEX(halfM, 0, N)] = expkM[halfM].real()
                                       * fft[INDEX(halfM, 0, halfN + 1)].real()
                                       * four_over_MN;

            post[INDEX(halfM, halfN, N)]
                = expkM[halfM].real()
                  * RealPartOfMul(expkN[halfN],
                                  fft[INDEX(halfM, halfN, halfN + 1)])
                  * four_over_MN;
            break;
          }

          case 1: {
            Kokkos::complex<float> tmp;

            tmp = fft[wid];
            post[wid] = RealPartOfMul(expkN[wid], tmp) * four_over_MN;
            post[N - wid] = -ImaginaryPartOfMul(expkN[wid], tmp) * four_over_MN;

            tmp = fft[INDEX(halfM, wid, halfN + 1)];
            post[INDEX(halfM, wid, N)] = expkM[halfM].real()
                                         * RealPartOfMul(expkN[wid], tmp)
                                         * four_over_MN;
            post[INDEX(halfM, N - wid, N)]
                = -expkM[halfM].real() * ImaginaryPartOfMul(expkN[wid], tmp)
                  * four_over_MN;
            break;
          }

          case 2: {
            Kokkos::complex<float> tmp1, tmp2, tmp_up, tmp_down;
            tmp1 = fft[INDEX(hid, 0, halfN + 1)];
            tmp2 = fft[INDEX(M - hid, 0, halfN + 1)];
            tmp_up.real() = expkM[hid].real() * (tmp1.real() + tmp2.real())
                            + expkM[hid].imag() * (tmp2.imag() - tmp1.imag());
            tmp_down.real() = -expkM[hid].imag() * (tmp1.real() + tmp2.real())
                              + expkM[hid].real() * (tmp2.imag() - tmp1.imag());
            post[INDEX(hid, 0, N)] = tmp_up.real() * two_over_MN;
            post[INDEX(M - hid, 0, N)] = tmp_down.real() * two_over_MN;

            tmp1 = complexAdd(fft[INDEX(hid, halfN, halfN + 1)],
                              fft[INDEX(M - hid, halfN, halfN + 1)]);
            tmp2 = complexSubtract(fft[INDEX(hid, halfN, halfN + 1)],
                                   fft[INDEX(M - hid, halfN, halfN + 1)]);
            tmp_up.real() = expkM[hid].real() * tmp1.real()
                            - expkM[hid].imag() * tmp2.imag();
            tmp_up.imag() = expkM[hid].real() * tmp1.imag()
                            + expkM[hid].imag() * tmp2.real();
            tmp_down.real() = -expkM[hid].imag() * tmp1.real()
                              - expkM[hid].real() * tmp2.imag();
            tmp_down.imag() = -expkM[hid].imag() * tmp1.imag()
                              + expkM[hid].real() * tmp2.real();
            post[INDEX(hid, halfN, N)]
                = RealPartOfMul(expkN[halfN], tmp_up) * two_over_MN;
            post[INDEX(M - hid, halfN, N)]
                = RealPartOfMul(expkN[halfN], tmp_down) * two_over_MN;
            break;
          }

          case 3: {
            Kokkos::complex<float> tmp1, tmp2, tmp_up, tmp_down;
            tmp1 = complexAdd(fft[INDEX(hid, wid, halfN + 1)],
                              fft[INDEX(M - hid, wid, halfN + 1)]);
            tmp2 = complexSubtract(fft[INDEX(hid, wid, halfN + 1)],
                                   fft[INDEX(M - hid, wid, halfN + 1)]);
            tmp_up.real() = expkM[hid].real() * tmp1.real()
                            - expkM[hid].imag() * tmp2.imag();
            tmp_up.imag() = expkM[hid].real() * tmp1.imag()
                            + expkM[hid].imag() * tmp2.real();
            tmp_down.real() = -expkM[hid].imag() * tmp1.real()
                              - expkM[hid].real() * tmp2.imag();
            tmp_down.imag() = -expkM[hid].imag() * tmp1.imag()
                              + expkM[hid].real() * tmp2.real();
            post[INDEX(hid, wid, N)]
                = RealPartOfMul(expkN[wid], tmp_up) * two_over_MN;
            post[INDEX(M - hid, wid, N)]
                = RealPartOfMul(expkN[wid], tmp_down) * two_over_MN;
            post[INDEX(hid, N - wid, N)]
                = -ImaginaryPartOfMul(expkN[wid], tmp_up) * two_over_MN;
            post[INDEX(M - hid, N - wid, N)]
                = -ImaginaryPartOfMul(expkN[wid], tmp_down) * two_over_MN;
            break;
          }

          default:
            Kokkos::abort("dct_2d_fft post: unhandled cond");
            break;
        }
      });
}

////////////////////////////////////////////////////////////////////////////////////

void idct_2d_fft(
    const int M,
    const int N,
    const Kokkos::View<const Kokkos::complex<float>*>& expkMForInverse,
    const Kokkos::View<const Kokkos::complex<float>*>& expkNForInverse,
    const Kokkos::View<const Kokkos::complex<float>*>& expkMN1,
    const Kokkos::View<const Kokkos::complex<float>*>& expkMN2,
    const Kokkos::View<const float*>& input,
    const Kokkos::View<Kokkos::complex<float>*>& pre,
    const Kokkos::View<float*>& ifft,
    const Kokkos::View<float*>& post)
{
  requirePowerOf2Dims(M, N, "idct_2d_fft");

  Kokkos::deep_copy(pre, 0);

  auto halfM = M / 2, halfN = N / 2;
  Kokkos::parallel_for(
      Kokkos::MDRangePolicy<Kokkos::Rank<2>>({0, 0}, {N / 2, M / 2}),
      KOKKOS_LAMBDA(const int wid, const int hid) {
        int cond = ((hid != 0) << 1) | (wid != 0);
        switch (cond) {
          case 0: {
            float tmp1;
            Kokkos::complex<float> tmp_up;

            pre[0].real() = input[0];
            pre[0].imag() = 0;

            tmp1 = input[halfN];
            tmp_up.real() = tmp1;
            tmp_up.imag() = tmp1;
            pre[halfN] = complexMulConj(expkNForInverse[halfN], tmp_up);

            tmp1 = input[INDEX(halfM, 0, N)];
            tmp_up.real() = tmp1;
            tmp_up.imag() = tmp1;
            pre[INDEX(halfM, 0, halfN + 1)]
                = complexMulConj(expkMForInverse[halfM], tmp_up);

            tmp1 = input[INDEX(halfM, halfN, N)];
            tmp_up.real() = 0;
            tmp_up.imag() = 2 * tmp1;
            pre[INDEX(halfM, halfN, halfN + 1)]
                = complexMulConj(expkMN1[halfM + halfN], tmp_up);
            break;
          }

          case 1: {
            Kokkos::complex<float> tmp_up;
            tmp_up.real() = input[wid];
            tmp_up.imag() = input[N - wid];
            pre[wid] = complexMulConj(expkNForInverse[wid], tmp_up);

            float tmp1 = input[INDEX(halfM, wid, N)];
            float tmp2 = input[INDEX(halfM, N - wid, N)];
            tmp_up.real() = tmp1 - tmp2;
            tmp_up.imag() = tmp1 + tmp2;
            pre[INDEX(halfM, wid, halfN + 1)]
                = complexMulConj(expkMN1[halfM + wid], tmp_up);
            break;
          }

          case 2: {
            float tmp1, tmp3;
            Kokkos::complex<float> tmp_up, tmp_down;

            tmp1 = input[INDEX(hid, 0, N)];
            tmp3 = input[INDEX(M - hid, 0, N)];
            tmp_down.real() = tmp3;
            tmp_down.imag() = tmp1;

            // two outputs are conjugate
            tmp_up = complexMul(expkMForInverse[M - hid], tmp_down);
            pre[INDEX(hid, 0, halfN + 1)] = tmp_up;
            pre[INDEX(M - hid, 0, halfN + 1)] = complexConj(tmp_up);

            tmp1 = input[INDEX(hid, halfN, N)];
            tmp3 = input[INDEX(M - hid, halfN, N)];
            tmp_up.real() = tmp1 - tmp3;
            tmp_up.imag() = tmp3 + tmp1;
            tmp_down.real() = tmp3 - tmp1;
            tmp_down.imag() = tmp1 + tmp3;

            pre[INDEX(hid, halfN, halfN + 1)]
                = complexMulConj(expkMN1[hid + halfN], tmp_up);
            pre[INDEX(M - hid, halfN, halfN + 1)]
                = complexMulConj(expkMN2[halfN - hid + (N - 1)], tmp_down);
            break;
          }

          case 3: {
            float tmp1 = input[INDEX(hid, wid, N)];
            float tmp2 = input[INDEX(hid, N - wid, N)];
            float tmp3 = input[INDEX(M - hid, wid, N)];
            float tmp4 = input[INDEX(M - hid, N - wid, N)];
            Kokkos::complex<float> tmp_up, tmp_down;
            tmp_up.real() = tmp1 - tmp4;
            tmp_up.imag() = tmp3 + tmp2;
            tmp_down.real() = tmp3 - tmp2;
            tmp_down.imag() = tmp1 + tmp4;

            pre[INDEX(hid, wid, halfN + 1)]
                = complexMulConj(expkMN1[hid + wid], tmp_up);
            pre[INDEX(M - hid, wid, halfN + 1)]
                = complexMulConj(expkMN2[wid - hid + (N - 1)], tmp_down);
            break;
          }

          default:
            Kokkos::abort("idct_2d_fft pre: unhandled cond");
            break;
        }
      });

  Kokkos::View<Kokkos::complex<float>**,
               Kokkos::LayoutRight,
               Kokkos::DefaultExecutionSpace,
               Kokkos::MemoryTraits<Kokkos::Unmanaged>>
      pre2d(pre.data(), M, (N / 2) + 1);
  Kokkos::View<float**,
               Kokkos::LayoutRight,
               Kokkos::DefaultExecutionSpace,
               Kokkos::MemoryTraits<Kokkos::Unmanaged>>
      ifft2d(ifft.data(), M, N);

  // For consistency we always calculate iFFT on CPU (as Kokkos uses a different
  // implementation for GPU)
  Kokkos::DefaultHostExecutionSpace hostSpace;
  auto hPre2d = Kokkos::create_mirror_view_and_copy(hostSpace, pre2d);
  auto hIfft2d = Kokkos::create_mirror_view(hostSpace, ifft2d);

  KokkosFFT::Plan fftplan(hostSpace,
                          hPre2d,
                          hIfft2d,
                          KokkosFFT::Direction::backward,
                          KokkosFFT::axis_type<2>{-2, -1});
  KokkosFFT::execute(fftplan, hPre2d, hIfft2d, KokkosFFT::Normalization::none);

  Kokkos::deep_copy(ifft2d, hIfft2d);

  Kokkos::parallel_for(
      Kokkos::MDRangePolicy<Kokkos::Rank<2>>({0, 0}, {N, M}),
      KOKKOS_LAMBDA(const int wid, const int hid) {
        int cond = ((hid < M / 2) << 1) | (wid < N / 2);
        int index;
        switch (cond) {
          case 0:
            index = INDEX(((M - hid) << 1) - 1, ((N - wid) << 1) - 1, N);
            break;
          case 1:
            index = INDEX(((M - hid) << 1) - 1, wid << 1, N);
            break;
          case 2:
            index = INDEX(hid << 1, ((N - wid) << 1) - 1, N);
            break;
          case 3:
            index = INDEX(hid << 1, wid << 1, N);
            break;
          default:
            Kokkos::abort("idct_2d_fft: unhandled cond");
            break;
        }
        post[index] = ifft[INDEX(hid, wid, N)];
      });
}

void idct_idxst(
    const int M,
    const int N,
    const Kokkos::View<const Kokkos::complex<float>*>& expkMForInverse,
    const Kokkos::View<const Kokkos::complex<float>*>& expkNForInverse,
    const Kokkos::View<const Kokkos::complex<float>*>& expkMN1,
    const Kokkos::View<const Kokkos::complex<float>*>& expkMN2,
    const Kokkos::View<const float*>& input,
    const Kokkos::View<float*>& workSpaceReal1,
    const Kokkos::View<Kokkos::complex<float>*>& workSpaceComplex,
    const Kokkos::View<float*>& workSpaceReal2,
    const Kokkos::View<float*>& workSpaceReal3,
    const Kokkos::View<float*>& output)
{
  requirePowerOf2Dims(M, N, "idct_idxst");

  Kokkos::parallel_for(
      Kokkos::MDRangePolicy<Kokkos::Rank<2>>({0, 0}, {N, M}),
      KOKKOS_LAMBDA(const int wid, const int hid) {
        int idx_in = INDEX(M - hid, wid, N);
        int idx_out = INDEX(hid, wid, N);

        if (hid == 0) {
          workSpaceReal1[idx_out] = 0;
        } else {
          workSpaceReal1[idx_out] = input[idx_in];
        }
      });

  idct_2d_fft(M,
              N,
              expkMForInverse,
              expkNForInverse,
              expkMN1,
              expkMN2,
              workSpaceReal1,
              workSpaceComplex,
              workSpaceReal2,
              workSpaceReal3);

  Kokkos::parallel_for(
      Kokkos::MDRangePolicy<Kokkos::Rank<2>>({0, 0}, {N, M}),
      KOKKOS_LAMBDA(const int wid, const int hid) {
        int idx = INDEX(hid, wid, N);

        if (hid % 2 == 0) {
          output[idx] = +workSpaceReal3[idx];
        } else {
          output[idx] = -workSpaceReal3[idx];
        }
      });
}

void idxst_idct(
    const int M,
    const int N,
    const Kokkos::View<const Kokkos::complex<float>*>& expkMForInverse,
    const Kokkos::View<const Kokkos::complex<float>*>& expkNForInverse,
    const Kokkos::View<const Kokkos::complex<float>*>& expkMN1,
    const Kokkos::View<const Kokkos::complex<float>*>& expkMN2,
    const Kokkos::View<const float*>& input,
    const Kokkos::View<float*>& workSpaceReal1,
    const Kokkos::View<Kokkos::complex<float>*>& workSpaceComplex,
    const Kokkos::View<float*>& workSpaceReal2,
    const Kokkos::View<float*>& workSpaceReal3,
    const Kokkos::View<float*>& output)
{
  requirePowerOf2Dims(M, N, "idxst_idct");

  Kokkos::parallel_for(
      Kokkos::MDRangePolicy<Kokkos::Rank<2>>({0, 0}, {N, M}),
      KOKKOS_LAMBDA(const int wid, const int hid) {
        int idx_in = INDEX(hid, N - wid, N);
        int idx_out = INDEX(hid, wid, N);

        if (wid == 0) {
          workSpaceReal1[idx_out] = 0;
        } else {
          workSpaceReal1[idx_out] = input[idx_in];
        }
      });

  idct_2d_fft(M,
              N,
              expkMForInverse,
              expkNForInverse,
              expkMN1,
              expkMN2,
              workSpaceReal1,
              workSpaceComplex,
              workSpaceReal2,
              workSpaceReal3);

  Kokkos::parallel_for(
      Kokkos::MDRangePolicy<Kokkos::Rank<2>>({0, 0}, {N, M}),
      KOKKOS_LAMBDA(const int wid, const int hid) {
        int idx = INDEX(hid, wid, N);

        if (wid % 2 == 0) {
          output[idx] = +workSpaceReal3[idx];
        } else {
          output[idx] = -workSpaceReal3[idx];
        }
      });
}

}  // namespace gpl
