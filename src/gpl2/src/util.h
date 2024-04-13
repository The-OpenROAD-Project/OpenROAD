
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

#include <cuda.h>
#include <cuda_runtime.h>

#include <algorithm>
#include <iostream>
#include <random>
#include <cuda.h>
#include <cufft.h>
#include <stdio.h>
#include <thrust/device_vector.h>
#include <thrust/functional.h>
#include <thrust/host_vector.h>
#include <thrust/transform_reduce.h>


namespace gpl2 {

#define PI (3.141592653589793238462643383279502884197169)
#ifndef CUDA_CHECK
#define CUDA_CHECK(status) __CUDA_CHECK(status, __FILE__, __LINE__)
#endif

// utilities for free memory on gpu
void freeCUDA(void* cuda_pointer);
void getLastCUDAErr();

// Define the functor
struct FloatToIntFunctor
{
  __host__ __device__ int operator()(float x) const
  {
    return static_cast<int>(x);
  }
};

inline __device__ __host__ bool isPowerOf2(int val);

template <typename T>
inline T* setThrustVector(const size_t size, thrust::device_vector<T>& d_vector);

template <typename T>
inline const T* getRawPointer(const thrust::device_vector<T>& thrustVector);

template <typename T>
inline T* getRawPointer(thrust::device_vector<T>& thrustVector);

inline void __CUDA_CHECK(cudaError_t status, const char* file, const int line);

inline __device__ int INDEX(const int hid, const int wid, const int N);

inline __device__ cufftDoubleComplex complexMul(const cufftDoubleComplex& x,
                                                const cufftDoubleComplex& y);

inline __device__ cufftComplex complexMul(const cufftComplex& x,
                                          const cufftComplex& y);

inline __device__ cufftDoubleReal RealPartOfMul(const cufftDoubleComplex& x,
                                                const cufftDoubleComplex& y);

inline __device__ cufftReal RealPartOfMul(const cufftComplex& x,
                                          const cufftComplex& y);

inline __device__ cufftDoubleReal
ImaginaryPartOfMul(const cufftDoubleComplex& x, const cufftDoubleComplex& y);

inline __device__ cufftReal ImaginaryPartOfMul(const cufftComplex& x,
                                               const cufftComplex& y);

inline __device__ cufftDoubleComplex complexAdd(const cufftDoubleComplex& x,
                                                const cufftDoubleComplex& y);

inline __device__ cufftComplex complexAdd(const cufftComplex& x,
                                          const cufftComplex& y);

inline __device__ cufftDoubleComplex
complexSubtract(const cufftDoubleComplex& x, const cufftDoubleComplex& y);

inline __device__ cufftComplex complexSubtract(const cufftComplex& x,
                                               const cufftComplex& y);

inline __device__ cufftDoubleComplex complexConj(const cufftDoubleComplex& x);

inline __device__ cufftComplex complexConj(const cufftComplex& x);

inline __device__ cufftDoubleComplex complexMulConj(const cufftDoubleComplex& x,
                                                    const cufftDoubleComplex& y);

inline __device__ cufftComplex complexMulConj(const cufftComplex& x,
                                              const cufftComplex& y);



}  // namespace gpl2
