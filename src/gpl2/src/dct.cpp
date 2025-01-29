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

#include <Kokkos_Core.hpp>
#include <KokkosFFT.hpp>
#include "kokkosUtil.h"

#include <cassert>

#include "dct.h"

void dct_2d_fft(const int M,
                const int N,
                const Kokkos::View<const Kokkos::complex<float>*>& expkM,
                const Kokkos::View<const Kokkos::complex<float>*>& expkN,
                const Kokkos::View<const float*>& input,
                const Kokkos::View<float*>& pre,
                const Kokkos::View<Kokkos::complex<float>*>& fft,
                const Kokkos::View<float*>& post)
{
  if (!isPowerOf2(N) || !isPowerOf2(M)) {
    printf("Input length is not power of 2.\n");
    assert(0);
  }

  auto halfN = N / 2;
  Kokkos::parallel_for(Kokkos::MDRangePolicy<Kokkos::Rank<2>>({0, 0}, {N, M}),
  KOKKOS_LAMBDA (const int wid, const int hid) {
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
        Kokkos::printf("Error: unhandled case in dct_2d_fft\n");
        index = 0;
        assert(0);
        break;
    }
    pre[index] = input[INDEX(hid, wid, N)];
  });

  Kokkos::DefaultExecutionSpace exec;
  Kokkos::View<float**, Kokkos::LayoutRight, Kokkos::DefaultExecutionSpace, Kokkos::MemoryTraits<Kokkos::Unmanaged>> pre2d(pre.data(), M, N);
  Kokkos::View<Kokkos::complex<float>**, Kokkos::LayoutRight, Kokkos::DefaultExecutionSpace, Kokkos::MemoryTraits<Kokkos::Unmanaged>> fft2d(fft.data(), M, (N / 2) + 1);
  KokkosFFT::rfft2(exec, pre2d, fft2d, KokkosFFT::Normalization::none);

  auto halfM = M / 2;
  auto two_over_MN = 2.0 / (M * N), four_over_MN = 4.0 / (M * N);
  Kokkos::parallel_for(Kokkos::MDRangePolicy<Kokkos::Rank<2>>({0, 0}, {N / 2, M / 2}),
  KOKKOS_LAMBDA (const int wid, const int hid) {
    int cond = ((hid != 0) << 1) | (wid != 0);
    switch (cond) {
      case 0: {
        post[0] = fft[0].real() * four_over_MN;
        post[halfN] = RealPartOfMul(expkN[halfN], fft[halfN]) * four_over_MN;

        post[INDEX(halfM, 0, N)]
            = expkM[halfM].real() * fft[INDEX(halfM, 0, halfN + 1)].real() * four_over_MN;

        post[INDEX(halfM, halfN, N)]
            = expkM[halfM].real()
              * RealPartOfMul(expkN[halfN], fft[INDEX(halfM, halfN, halfN + 1)])
              * four_over_MN;
        break;
      }

      case 1: {
        Kokkos::complex<float> tmp;

        tmp = fft[wid];
        post[wid] = RealPartOfMul(expkN[wid], tmp) * four_over_MN;
        post[N - wid] = -ImaginaryPartOfMul(expkN[wid], tmp) * four_over_MN;

        tmp = fft[INDEX(halfM, wid, halfN + 1)];
        post[INDEX(halfM, wid, N)]
            = expkM[halfM].real() * RealPartOfMul(expkN[wid], tmp) * four_over_MN;
        post[INDEX(halfM, N - wid, N)] = -expkM[halfM].real()
                                      * ImaginaryPartOfMul(expkN[wid], tmp)
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
        tmp_up.real() = expkM[hid].real() * tmp1.real() - expkM[hid].imag() * tmp2.imag();
        tmp_up.imag() = expkM[hid].real() * tmp1.imag() + expkM[hid].imag() * tmp2.real();
        tmp_down.real() = -expkM[hid].imag() * tmp1.real() - expkM[hid].real() * tmp2.imag();
        tmp_down.imag() = -expkM[hid].imag() * tmp1.imag() + expkM[hid].real() * tmp2.real();
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
        tmp_up.real() = expkM[hid].real() * tmp1.real() - expkM[hid].imag() * tmp2.imag();
        tmp_up.imag() = expkM[hid].real() * tmp1.imag() + expkM[hid].imag() * tmp2.real();
        tmp_down.real() = -expkM[hid].imag() * tmp1.real() - expkM[hid].real() * tmp2.imag();
        tmp_down.imag() = -expkM[hid].imag() * tmp1.imag() + expkM[hid].real() * tmp2.real();
        post[INDEX(hid, wid, N)] = RealPartOfMul(expkN[wid], tmp_up) * two_over_MN;
        post[INDEX(M - hid, wid, N)]
            = RealPartOfMul(expkN[wid], tmp_down) * two_over_MN;
        post[INDEX(hid, N - wid, N)]
            = -ImaginaryPartOfMul(expkN[wid], tmp_up) * two_over_MN;
        post[INDEX(M - hid, N - wid, N)]
            = -ImaginaryPartOfMul(expkN[wid], tmp_down) * two_over_MN;
        break;
      }

      default:
        assert(0);
        break;
    }
  });
}

////////////////////////////////////////////////////////////////////////////////////

void idct_2d_fft(const int M,
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
  if (!isPowerOf2(N) || !isPowerOf2(M)) {
    printf("Input length is not power of 2.\n");
    assert(0);
  }

  Kokkos::deep_copy(pre, 0);

  auto halfM = M / 2, halfN = N / 2;
  Kokkos::parallel_for(Kokkos::MDRangePolicy<Kokkos::Rank<2>>({0, 0}, {N / 2, M / 2}),
  KOKKOS_LAMBDA (const int wid, const int hid) {
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
        assert(0);
        break;
    }
  });

  Kokkos::DefaultExecutionSpace exec;
  Kokkos::View<Kokkos::complex<float>**, Kokkos::LayoutRight, Kokkos::DefaultExecutionSpace, Kokkos::MemoryTraits<Kokkos::Unmanaged>> pre2d(pre.data(), M, (N / 2) + 1);
  Kokkos::View<float**, Kokkos::LayoutRight, Kokkos::DefaultExecutionSpace, Kokkos::MemoryTraits<Kokkos::Unmanaged>> ifft2d(ifft.data(), M, N);
  KokkosFFT::irfft2(exec, pre2d, ifft2d, KokkosFFT::Normalization::none);

  Kokkos::parallel_for(Kokkos::MDRangePolicy<Kokkos::Rank<2>>({0, 0}, {N, M}),
  KOKKOS_LAMBDA (const int wid, const int hid) {
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
        Kokkos::printf("Unhandled case in idct_2d_fft\n");
        index = 0;
        assert(0);
        break;
    }
    post[index] = ifft[INDEX(hid, wid, N)];
  });
}

void idct_idxst(const int M,
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
  if (!isPowerOf2(N) || !isPowerOf2(M)) {
    printf("Input length is not power of 2.\n");
    assert(0);
  }

  Kokkos::parallel_for(Kokkos::MDRangePolicy<Kokkos::Rank<2>>({0, 0}, {N, M}),
  KOKKOS_LAMBDA (const int wid, const int hid) {
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

  Kokkos::parallel_for(Kokkos::MDRangePolicy<Kokkos::Rank<2>>({0, 0}, {N, M}),
  KOKKOS_LAMBDA (const int wid, const int hid) {
    int idx = INDEX(hid, wid, N);

    if (hid % 2 == 0) {
      output[idx] = +workSpaceReal3[idx];
    } else {
      output[idx] = -workSpaceReal3[idx];
    }
  });
}

void idxst_idct(const int M,
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
  if (!isPowerOf2(N) || !isPowerOf2(M)) {
    printf("Input length is not power of 2.\n");
    assert(0);
  }

  Kokkos::parallel_for(Kokkos::MDRangePolicy<Kokkos::Rank<2>>({0, 0}, {N, M}),
  KOKKOS_LAMBDA (const int wid, const int hid) {
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

  Kokkos::parallel_for(Kokkos::MDRangePolicy<Kokkos::Rank<2>>({0, 0}, {N, M}),
  KOKKOS_LAMBDA (const int wid, const int hid) {
    int idx = INDEX(hid, wid, N);

    if (wid % 2 == 0) {
      output[idx] = +workSpaceReal3[idx];
    } else {
      output[idx] = -workSpaceReal3[idx];
    }
  });
}

