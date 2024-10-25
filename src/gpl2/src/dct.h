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

void dct_2d_fft(const int M,
                const int N,
                const Kokkos::View<const Kokkos::complex<float>*>& expkM,
                const Kokkos::View<const Kokkos::complex<float>*>& expkN,
                const Kokkos::View<const float*>& input,
                const Kokkos::View<float*>& pre,
                const Kokkos::View<Kokkos::complex<float>*>& fft,
                const Kokkos::View<float*>& post);

void idct_2d_fft(const int M,
                 const int N,
                 const Kokkos::View<const Kokkos::complex<float>*>& expkM,
                 const Kokkos::View<const Kokkos::complex<float>*>& expkN,
                 const Kokkos::View<const Kokkos::complex<float>*>& expkMN1,
                 const Kokkos::View<const Kokkos::complex<float>*>& expkMN2,
                 const Kokkos::View<const float*>& input,
                 const Kokkos::View<Kokkos::complex<float>*>& pre,
                 const Kokkos::View<float*>& ifft,
                 const Kokkos::View<float*>& post);

void idxst_idct(const int M,
                const int N,
                const Kokkos::View<const Kokkos::complex<float>*>& expkM,
                const Kokkos::View<const Kokkos::complex<float>*>& expkN,
                const Kokkos::View<const Kokkos::complex<float>*>& expkMN1,
                const Kokkos::View<const Kokkos::complex<float>*>& expkMN2,
                const Kokkos::View<const float*>& input,
                const Kokkos::View<float*>& workSpaceReal1,
                const Kokkos::View<Kokkos::complex<float>*>& workSpaceComplex,
                const Kokkos::View<float*>& workSpaceReal2,
                const Kokkos::View<float*>& workSpaceReal3,
                const Kokkos::View<float*>& output);

void idct_idxst(const int M,
                const int N,
                const Kokkos::View<const Kokkos::complex<float>*>& expkM,
                const Kokkos::View<const Kokkos::complex<float>*>& expkN,
                const Kokkos::View<const Kokkos::complex<float>*>& expkMN1,
                const Kokkos::View<const Kokkos::complex<float>*>& expkMN2,
                const Kokkos::View<const float*>& input,
                const Kokkos::View<float*>& workSpaceReal1,
                const Kokkos::View<Kokkos::complex<float>*>& workSpaceComplex,
                const Kokkos::View<float*>& workSpaceReal2,
                const Kokkos::View<float*>& workSpaceReal3,
                const Kokkos::View<float*>& output);
