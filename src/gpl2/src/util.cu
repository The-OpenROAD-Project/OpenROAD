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

#include <cuda.h>
#include <cuda_runtime.h>

#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <numeric>

#include "util.h"
// basic vectors
#include <thrust/device_free.h>
#include <thrust/device_malloc.h>
#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#include <thrust/reduce.h>
#include <thrust/sequence.h>
// memory related
#include <thrust/copy.h>
#include <thrust/fill.h>
// algorithm related
#include <thrust/execution_policy.h>
#include <thrust/for_each.h>
#include <thrust/functional.h>
#include <thrust/replace.h>
#include <thrust/transform.h>
#include <cuda.h>
#include <cufft.h>
#include <stdio.h>
#include <thrust/device_vector.h>
#include <thrust/functional.h>
#include <thrust/host_vector.h>
#include <thrust/transform_reduce.h>

namespace gpl2 {

// utilities function
void freeCUDA(void* cuda_pointer)
{
  cudaError_t err = cudaFree(cuda_pointer);
  if (err != cudaSuccess) {
    std::cerr << "Failed to free the pointer (error code ";
    std::cerr << cudaGetErrorString(err) << ")!\n";
  }
}

void getLastCUDAErr()
{
  // Check for any errors launching the kernel
  cudaError_t cudaerr = cudaGetLastError();
  if (cudaerr != cudaSuccess) {
    std::cerr << "CUDA failed with error: " << cudaGetErrorString(cudaerr);
  }
}

inline __device__ __host__ bool isPowerOf2(int val)
{
  return val && (val & (val - 1)) == 0;
}

template <typename T>
inline T* setThrustVector(const size_t size, thrust::device_vector<T>& d_vector)
{
  d_vector.resize(size);
  return thrust::raw_pointer_cast(&d_vector[0]);
}

template <typename T>
inline const T* getRawPointer(const thrust::device_vector<T>& thrustVector)
{
  return thrust::raw_pointer_cast(&thrustVector[0]);
}

template <typename T>
inline T* getRawPointer(thrust::device_vector<T>& thrustVector)
{
  return thrust::raw_pointer_cast(&thrustVector[0]);
}

inline void __CUDA_CHECK(cudaError_t status, const char* file, const int line)
{
  if (status != cudaSuccess) {
    fprintf(stderr,
            "[CUDA-ERROR] Error %s at line %d in file %s\n",
            cudaGetErrorString(status),
            line,
            file);
    exit(status);
  }
}

inline __device__ int INDEX(const int hid, const int wid, const int N)
{
  return (hid * N + wid);
}

inline __device__ cufftDoubleComplex complexMul(const cufftDoubleComplex& x,
                                                const cufftDoubleComplex& y)
{
  cufftDoubleComplex res;
  res.x = x.x * y.x - x.y * y.y;
  res.y = x.x * y.y + x.y * y.x;
  return res;
}

inline __device__ cufftComplex complexMul(const cufftComplex& x,
                                          const cufftComplex& y)
{
  cufftComplex res;
  res.x = x.x * y.x - x.y * y.y;
  res.y = x.x * y.y + x.y * y.x;
  return res;
}

inline __device__ cufftDoubleReal RealPartOfMul(const cufftDoubleComplex& x,
                                                const cufftDoubleComplex& y)
{
  return x.x * y.x - x.y * y.y;
}

inline __device__ cufftReal RealPartOfMul(const cufftComplex& x,
                                          const cufftComplex& y)
{
  return x.x * y.x - x.y * y.y;
}

inline __device__ cufftDoubleReal
ImaginaryPartOfMul(const cufftDoubleComplex& x, const cufftDoubleComplex& y)
{
  return x.x * y.y + x.y * y.x;
}

inline __device__ cufftReal ImaginaryPartOfMul(const cufftComplex& x,
                                               const cufftComplex& y)
{
  return x.x * y.y + x.y * y.x;
}

inline __device__ cufftDoubleComplex complexAdd(const cufftDoubleComplex& x,
                                                const cufftDoubleComplex& y)
{
  cufftDoubleComplex res;
  res.x = x.x + y.x;
  res.y = x.y + y.y;
  return res;
}

inline __device__ cufftComplex complexAdd(const cufftComplex& x,
                                          const cufftComplex& y)
{
  cufftComplex res;
  res.x = x.x + y.x;
  res.y = x.y + y.y;
  return res;
}

inline __device__ cufftDoubleComplex
complexSubtract(const cufftDoubleComplex& x, const cufftDoubleComplex& y)
{
  cufftDoubleComplex res;
  res.x = x.x - y.x;
  res.y = x.y - y.y;
  return res;
}

inline __device__ cufftComplex complexSubtract(const cufftComplex& x,
                                               const cufftComplex& y)
{
  cufftComplex res;
  res.x = x.x - y.x;
  res.y = x.y - y.y;
  return res;
}

inline __device__ cufftDoubleComplex complexConj(const cufftDoubleComplex& x)
{
  cufftDoubleComplex res;
  res.x = x.x;
  res.y = -x.y;
  return res;
}

inline __device__ cufftComplex complexConj(const cufftComplex& x)
{
  cufftComplex res;
  res.x = x.x;
  res.y = -x.y;
  return res;
}

inline __device__ cufftDoubleComplex complexMulConj(const cufftDoubleComplex& x,
                                                    const cufftDoubleComplex& y)
{
  cufftDoubleComplex res;
  res.x = x.x * y.x - x.y * y.y;
  res.y = -(x.x * y.y + x.y * y.x);
  return res;
}

inline __device__ cufftComplex complexMulConj(const cufftComplex& x,
                                              const cufftComplex& y)
{
  cufftComplex res;
  res.x = x.x * y.x - x.y * y.y;
  res.y = -(x.x * y.y + x.y * y.x);
  return res;
}


}  // namespace gpl2
