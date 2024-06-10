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

#include <cufft.h>

#include "cudaUtil.h"

void dct_2d_fft(const int M,
                const int N,
                cufftHandle& plan,
                const cufftComplex* expkM,
                const cufftComplex* expkN,
                const cufftReal* input,
                cufftReal* pre,
                cufftComplex* fft,
                cufftReal* post);

void idct_2d_fft(const int M,
                 const int N,
                 cufftHandle& plan,
                 const cufftComplex* expkM,
                 const cufftComplex* expkN,
                 const cufftComplex* expkMN1,
                 const cufftComplex* expkMN2,
                 const cufftReal* input,
                 cufftComplex* pre,
                 cufftReal* ifft,
                 cufftReal* post);

void idxst_idct(const int M,
                const int N,
                cufftHandle& plan,
                const cufftComplex* expkM,
                const cufftComplex* expkN,
                const cufftComplex* expkMN1,
                const cufftComplex* expkMN2,
                const cufftReal* input,
                cufftReal* workSpaceReal1,
                cufftComplex* workSpaceComplex,
                cufftReal* workSpaceReal2,
                cufftReal* workSpaceReal3,
                cufftReal* output);

void idct_idxst(const int M,
                const int N,
                cufftHandle& plan,
                const cufftComplex* expkM,
                const cufftComplex* expkN,
                const cufftComplex* expkMN1,
                const cufftComplex* expkMN2,
                const cufftReal* input,
                cufftReal* workSpaceReal1,
                cufftComplex* workSpaceComplex,
                cufftReal* workSpaceReal2,
                cufftReal* workSpaceReal3,
                cufftReal* output);
