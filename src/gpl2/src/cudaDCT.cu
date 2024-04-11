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
// The CudaDCT implementation is based on the algorithms proposed by the paper
// https://arxiv.org/abs/2110.01172.
// It is developed by the graduate student Jaekyung Kim (jkim97@postech.ac.kr)
// at Pohang University of Science and Technology (POSTECH).
// We thank Jaekyung Kim for his contribution.
//
///////////////////////////////////////////////////////////////////////////////

#include <cufft.h>

#include <cassert>

#include "cudaDCT.h"
#include "cudaUtil.h"

#define TPB 16

__global__ void dct2d_preprocess(const cufftReal* x,
                                 cufftReal* y,
                                 const int M,
                                 const int N,
                                 const int halfN)
{
  const int wid = blockDim.x * blockIdx.x + threadIdx.x;
  const int hid = blockDim.y * blockIdx.y + threadIdx.y;
  if (hid < M && wid < N) {
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
        break;
    }
    y[index] = x[INDEX(hid, wid, N)];
  }
}

__global__ __launch_bounds__(TPB* TPB, 10) void dct2d_postprocess(
    const cufftComplex* V,
    cufftReal* y,
    const int M,
    const int N,
    const int halfM,
    const int halfN,
    const cufftReal two_over_MN,
    const cufftReal four_over_MN,
    const cufftComplex* __restrict__ expkM,
    const cufftComplex* __restrict__ expkN)
{
  const int wid = blockDim.x * blockIdx.x + threadIdx.x;
  const int hid = blockDim.y * blockIdx.y + threadIdx.y;
  if (hid < halfM && wid < halfN) {
    int cond = ((hid != 0) << 1) | (wid != 0);
    switch (cond) {
      case 0: {
        y[0] = V[0].x * four_over_MN;
        y[halfN] = RealPartOfMul(expkN[halfN], V[halfN]) * four_over_MN;

        y[INDEX(halfM, 0, N)]
            = expkM[halfM].x * V[INDEX(halfM, 0, halfN + 1)].x * four_over_MN;

        y[INDEX(halfM, halfN, N)]
            = expkM[halfM].x
              * RealPartOfMul(expkN[halfN], V[INDEX(halfM, halfN, halfN + 1)])
              * four_over_MN;
        break;
      }

      case 1: {
        cufftComplex tmp;

        tmp = V[wid];
        y[wid] = RealPartOfMul(expkN[wid], tmp) * four_over_MN;
        y[N - wid] = -ImaginaryPartOfMul(expkN[wid], tmp) * four_over_MN;

        tmp = V[INDEX(halfM, wid, halfN + 1)];
        y[INDEX(halfM, wid, N)]
            = expkM[halfM].x * RealPartOfMul(expkN[wid], tmp) * four_over_MN;
        y[INDEX(halfM, N - wid, N)] = -expkM[halfM].x
                                      * ImaginaryPartOfMul(expkN[wid], tmp)
                                      * four_over_MN;
        break;
      }

      case 2: {
        cufftComplex tmp1, tmp2, tmp_up, tmp_down;
        tmp1 = V[INDEX(hid, 0, halfN + 1)];
        tmp2 = V[INDEX(M - hid, 0, halfN + 1)];
        tmp_up.x = expkM[hid].x * (tmp1.x + tmp2.x)
                   + expkM[hid].y * (tmp2.y - tmp1.y);
        tmp_down.x = -expkM[hid].y * (tmp1.x + tmp2.x)
                     + expkM[hid].x * (tmp2.y - tmp1.y);
        y[INDEX(hid, 0, N)] = tmp_up.x * two_over_MN;
        y[INDEX(M - hid, 0, N)] = tmp_down.x * two_over_MN;

        tmp1 = complexAdd(V[INDEX(hid, halfN, halfN + 1)],
                          V[INDEX(M - hid, halfN, halfN + 1)]);
        tmp2 = complexSubtract(V[INDEX(hid, halfN, halfN + 1)],
                               V[INDEX(M - hid, halfN, halfN + 1)]);
        tmp_up.x = expkM[hid].x * tmp1.x - expkM[hid].y * tmp2.y;
        tmp_up.y = expkM[hid].x * tmp1.y + expkM[hid].y * tmp2.x;
        tmp_down.x = -expkM[hid].y * tmp1.x - expkM[hid].x * tmp2.y;
        tmp_down.y = -expkM[hid].y * tmp1.y + expkM[hid].x * tmp2.x;
        y[INDEX(hid, halfN, N)]
            = RealPartOfMul(expkN[halfN], tmp_up) * two_over_MN;
        y[INDEX(M - hid, halfN, N)]
            = RealPartOfMul(expkN[halfN], tmp_down) * two_over_MN;
        break;
      }

      case 3: {
        cufftComplex tmp1, tmp2, tmp_up, tmp_down;
        tmp1 = complexAdd(V[INDEX(hid, wid, halfN + 1)],
                          V[INDEX(M - hid, wid, halfN + 1)]);
        tmp2 = complexSubtract(V[INDEX(hid, wid, halfN + 1)],
                               V[INDEX(M - hid, wid, halfN + 1)]);
        tmp_up.x = expkM[hid].x * tmp1.x - expkM[hid].y * tmp2.y;
        tmp_up.y = expkM[hid].x * tmp1.y + expkM[hid].y * tmp2.x;
        tmp_down.x = -expkM[hid].y * tmp1.x - expkM[hid].x * tmp2.y;
        tmp_down.y = -expkM[hid].y * tmp1.y + expkM[hid].x * tmp2.x;
        y[INDEX(hid, wid, N)] = RealPartOfMul(expkN[wid], tmp_up) * two_over_MN;
        y[INDEX(M - hid, wid, N)]
            = RealPartOfMul(expkN[wid], tmp_down) * two_over_MN;
        y[INDEX(hid, N - wid, N)]
            = -ImaginaryPartOfMul(expkN[wid], tmp_up) * two_over_MN;
        y[INDEX(M - hid, N - wid, N)]
            = -ImaginaryPartOfMul(expkN[wid], tmp_down) * two_over_MN;
        break;
      }

      default:
        assert(0);
        break;
    }
  }
}

void fft2D(cufftReal* d_input, cufftComplex* d_output, cufftHandle& plan)
{
  cufftExecR2C(plan, d_input, d_output);
  // cudaDeviceSynchronize();
}

void dct_2d_fft(const int M,
                const int N,
                cufftHandle& plan,
                const cufftComplex* expkM,
                const cufftComplex* expkN,
                const cufftReal* input,
                cufftReal* pre,
                cufftComplex* fft,
                cufftReal* post)
{
  if (!isPowerOf2(N) || !isPowerOf2(M)) {
    printf("Input length is not power of 2.\n");
    assert(0);
  }

  dim3 gridSize((N + TPB - 1) / TPB, (M + TPB - 1) / TPB, 1);
  dim3 gridSize2((N / 2 + TPB - 1) / TPB, (M / 2 + TPB - 1) / TPB, 1);
  dim3 blockSize(TPB, TPB, 1);

  dct2d_preprocess<<<gridSize, blockSize>>>(input, pre, M, N, N / 2);

  // cudaDeviceSynchronize();

  fft2D(pre, fft, plan);

  dct2d_postprocess<<<gridSize2, blockSize>>>(fft,
                                              post,
                                              M,
                                              N,
                                              M / 2,
                                              N / 2,
                                              2.0 / (M * N),
                                              4.0 / (M * N),
                                              expkM,
                                              expkN);
  // cuda2eviceSynchronize();
}

////////////////////////////////////////////////////////////////////////////////////

__global__ void idct2d_postprocess(const cufftReal* x,
                                   cufftReal* y,
                                   const int M,
                                   const int N,
                                   const int halfN)
{
  const int wid = blockDim.x * blockIdx.x + threadIdx.x;
  const int hid = blockDim.y * blockIdx.y + threadIdx.y;
  if (hid < M && wid < N) {
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
        assert(0);
        break;
    }
    y[index] = x[INDEX(hid, wid, N)];
  }
}

__global__ __launch_bounds__(TPB* TPB, 8) void idct2d_preprocess(
    const cufftReal* input,
    cufftComplex* output,
    const int M,
    const int N,
    const int halfM,
    const int halfN,
    const cufftComplex* __restrict__ expkM,
    const cufftComplex* __restrict__ expkN,
    const cufftComplex* __restrict__ expkMN_1,
    const cufftComplex* __restrict__ expkMN_2)
{
  const int wid = blockDim.x * blockIdx.x + threadIdx.x;
  const int hid = blockDim.y * blockIdx.y + threadIdx.y;
  if (hid < halfM && wid < halfN) {
    int cond = ((hid != 0) << 1) | (wid != 0);
    switch (cond) {
      case 0: {
        cufftReal tmp1;
        cufftComplex tmp_up;

        output[0].x = input[0];
        output[0].y = 0;

        tmp1 = input[halfN];
        tmp_up.x = tmp1;
        tmp_up.y = tmp1;
        output[halfN] = complexMulConj(expkN[halfN], tmp_up);

        tmp1 = input[INDEX(halfM, 0, N)];
        tmp_up.x = tmp1;
        tmp_up.y = tmp1;
        output[INDEX(halfM, 0, halfN + 1)]
            = complexMulConj(expkM[halfM], tmp_up);

        tmp1 = input[INDEX(halfM, halfN, N)];
        tmp_up.x = 0;
        tmp_up.y = 2 * tmp1;
        output[INDEX(halfM, halfN, halfN + 1)]
            = complexMulConj(expkMN_1[halfM + halfN], tmp_up);
        break;
      }

      case 1: {
        cufftComplex tmp_up;
        tmp_up.x = input[wid];
        tmp_up.y = input[N - wid];
        output[wid] = complexMulConj(expkN[wid], tmp_up);

        cufftReal tmp1 = input[INDEX(halfM, wid, N)];
        cufftReal tmp2 = input[INDEX(halfM, N - wid, N)];
        tmp_up.x = tmp1 - tmp2;
        tmp_up.y = tmp1 + tmp2;
        output[INDEX(halfM, wid, halfN + 1)]
            = complexMulConj(expkMN_1[halfM + wid], tmp_up);
        break;
      }

      case 2: {
        cufftReal tmp1, tmp3;
        cufftComplex tmp_up, tmp_down;

        tmp1 = input[INDEX(hid, 0, N)];
        tmp3 = input[INDEX(M - hid, 0, N)];
        tmp_down.x = tmp3;
        tmp_down.y = tmp1;

        // two outputs are conjugate
        tmp_up = complexMul(expkM[M - hid], tmp_down);
        output[INDEX(hid, 0, halfN + 1)] = tmp_up;
        output[INDEX(M - hid, 0, halfN + 1)] = complexConj(tmp_up);

        tmp1 = input[INDEX(hid, halfN, N)];
        tmp3 = input[INDEX(M - hid, halfN, N)];
        tmp_up.x = tmp1 - tmp3;
        tmp_up.y = tmp3 + tmp1;
        tmp_down.x = tmp3 - tmp1;
        tmp_down.y = tmp1 + tmp3;

        output[INDEX(hid, halfN, halfN + 1)]
            = complexMulConj(expkMN_1[hid + halfN], tmp_up);
        output[INDEX(M - hid, halfN, halfN + 1)]
            = complexMulConj(expkMN_2[halfN - hid + (N - 1)], tmp_down);
        break;
      }

      case 3: {
        cufftReal tmp1 = input[INDEX(hid, wid, N)];
        cufftReal tmp2 = input[INDEX(hid, N - wid, N)];
        cufftReal tmp3 = input[INDEX(M - hid, wid, N)];
        cufftReal tmp4 = input[INDEX(M - hid, N - wid, N)];
        cufftComplex tmp_up, tmp_down;
        tmp_up.x = tmp1 - tmp4;
        tmp_up.y = tmp3 + tmp2;
        tmp_down.x = tmp3 - tmp2;
        tmp_down.y = tmp1 + tmp4;

        output[INDEX(hid, wid, halfN + 1)]
            = complexMulConj(expkMN_1[hid + wid], tmp_up);
        output[INDEX(M - hid, wid, halfN + 1)]
            = complexMulConj(expkMN_2[wid - hid + (N - 1)], tmp_down);
        break;
      }

      default:
        assert(0);
        break;
    }
  }
}

void ifft2D(cufftComplex* d_input, cufftReal* d_output, cufftHandle& plan)
{
  cufftExecC2R(plan, d_input, d_output);
  // cudaDeviceSynchronize();
}

void idct_2d_fft(const int M,
                 const int N,
                 cufftHandle& plan,
                 const cufftComplex* expkMForInverse,
                 const cufftComplex* expkNForInverse,
                 const cufftComplex* expkMN1,
                 const cufftComplex* expkMN2,
                 const cufftReal* input,
                 cufftComplex* pre,
                 cufftReal* ifft,
                 cufftReal* post)
{
  if (!isPowerOf2(N) || !isPowerOf2(M)) {
    printf("Input length is not power of 2.\n");
    assert(0);
  }

  cudaMemset(pre, 0, M * (N / 2 + 1) * sizeof(cufftComplex));

  dim3 gridSize((N + TPB - 1) / TPB, (M + TPB - 1) / TPB, 1);
  dim3 gridSize2((N / 2 + TPB - 1) / TPB, (M / 2 + TPB - 1) / TPB, 1);
  dim3 blockSize(TPB, TPB, 1);

  idct2d_preprocess<<<gridSize2, blockSize>>>(input,
                                              pre,
                                              M,
                                              N,
                                              M / 2,
                                              N / 2,
                                              expkMForInverse,
                                              expkNForInverse,
                                              expkMN1,
                                              expkMN2);
  // cudaDeviceSynchronize();

  ifft2D(pre, ifft, plan);

  idct2d_postprocess<<<gridSize, blockSize>>>(ifft, post, M, N, N / 2);

  // cudaDeviceSynchronize();
}

__global__ void idct_idxst_preprocess(const cufftReal* input,
                                      cufftReal* output,
                                      const int M,
                                      const int N)
{
  const int wid = blockDim.x * blockIdx.x + threadIdx.x;
  const int hid = blockDim.y * blockIdx.y + threadIdx.y;

  if (hid < M && wid < N) {
    int idx_in = INDEX(M - hid, wid, N);
    int idx_out = INDEX(hid, wid, N);

    if (hid == 0)
      output[idx_out] = 0;
    else
      output[idx_out] = input[idx_in];
  }
}

__global__ void idct_idxst_postprocess(const cufftReal* input,
                                       cufftReal* output,
                                       const int M,
                                       const int N)
{
  const int wid = blockDim.x * blockIdx.x + threadIdx.x;
  const int hid = blockDim.y * blockIdx.y + threadIdx.y;

  if (hid < M && wid < N) {
    int idx = INDEX(hid, wid, N);

    if (hid % 2 == 0)
      output[idx] = +input[idx];
    else
      output[idx] = -input[idx];
  }
}

void idct_idxst(const int M,
                const int N,
                cufftHandle& plan,
                const cufftComplex* expkMForInverse,
                const cufftComplex* expkNForInverse,
                const cufftComplex* expkMN1,
                const cufftComplex* expkMN2,
                const cufftReal* input,
                cufftReal* workSpaceReal1,
                cufftComplex* workSpaceComplex,
                cufftReal* workSpaceReal2,
                cufftReal* workSpaceReal3,
                cufftReal* output)
{
  if (!isPowerOf2(N) || !isPowerOf2(M)) {
    printf("Input length is not power of 2.\n");
    assert(0);
  }

  dim3 gridSize((N + TPB - 1) / TPB, (M + TPB - 1) / TPB, 1);
  dim3 blockSize(TPB, TPB, 1);

  idct_idxst_preprocess<<<gridSize, blockSize>>>(input, workSpaceReal1, M, N);

  // cudaDeviceSynchronize();

  idct_2d_fft(M,
              N,
              plan,
              expkMForInverse,
              expkNForInverse,
              expkMN1,
              expkMN2,
              workSpaceReal1,
              workSpaceComplex,
              workSpaceReal2,
              workSpaceReal3);

  idct_idxst_postprocess<<<gridSize, blockSize>>>(workSpaceReal3, output, M, N);

  // cudaDeviceSynchronize();
}

__global__ void idxst_idct_preprocess(const cufftReal* input,
                                      cufftReal* output,
                                      const int M,
                                      const int N)
{
  const int wid = blockDim.x * blockIdx.x + threadIdx.x;
  const int hid = blockDim.y * blockIdx.y + threadIdx.y;

  if (hid < M && wid < N) {
    int idx_in = INDEX(hid, N - wid, N);
    int idx_out = INDEX(hid, wid, N);

    if (wid == 0)
      output[idx_out] = 0;
    else
      output[idx_out] = input[idx_in];
  }
}

__global__ void idxst_idct_postprocess(const cufftReal* input,
                                       cufftReal* output,
                                       const int M,
                                       const int N)
{
  const int wid = blockDim.x * blockIdx.x + threadIdx.x;
  const int hid = blockDim.y * blockIdx.y + threadIdx.y;

  if (hid < M && wid < N) {
    int idx = INDEX(hid, wid, N);

    if (wid % 2 == 0)
      output[idx] = +input[idx];
    else
      output[idx] = -input[idx];
  }
}

void idxst_idct(const int M,
                const int N,
                cufftHandle& plan,
                const cufftComplex* expkMForInverse,
                const cufftComplex* expkNForInverse,
                const cufftComplex* expkMN1,
                const cufftComplex* expkMN2,
                const cufftReal* input,
                cufftReal* workSpaceReal1,
                cufftComplex* workSpaceComplex,
                cufftReal* workSpaceReal2,
                cufftReal* workSpaceReal3,
                cufftReal* output)
{
  if (!isPowerOf2(N) || !isPowerOf2(M)) {
    printf("Input length is not power of 2.\n");
    assert(0);
  }

  dim3 gridSize((N + TPB - 1) / TPB, (M + TPB - 1) / TPB, 1);
  dim3 blockSize(TPB, TPB, 1);

  idxst_idct_preprocess<<<gridSize, blockSize>>>(input, workSpaceReal1, M, N);

  // cudaDeviceSynchronize();

  idct_2d_fft(M,
              N,
              plan,
              expkMForInverse,
              expkNForInverse,
              expkMN1,
              expkMN2,
              workSpaceReal1,
              workSpaceComplex,
              workSpaceReal2,
              workSpaceReal3);

  idxst_idct_postprocess<<<gridSize, blockSize>>>(workSpaceReal3, output, M, N);

  // cudaDeviceSynchronize();
}
