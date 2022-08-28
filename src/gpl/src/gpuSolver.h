/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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

#include <cuda_runtime.h>
#include <thrust/copy.h>
#include <thrust/device_vector.h>
#include <thrust/fill.h>
#include <thrust/host_vector.h>
#include <thrust/sequence.h>

#include <Eigen/SparseCore>
#include <memory>
#include <vector>

#include "cusolverSp.h"
#include "cusparse.h"
#include "device_launch_parameters.h"
#include "odb/db.h"
#include "utl/Logger.h"

namespace utl {
class Logger;
}

namespace gpl {

using namespace std;
typedef Eigen::SparseMatrix<float, Eigen::RowMajor> SMatrix;
class GpuSolver
{
 public:
  GpuSolver(SMatrix& placeInstForceMatrix,
            Eigen::VectorXf& fixedInstForceVec,
            utl::Logger* logger);
  void cusolverCal(Eigen::VectorXf& instLocVec);
  float error();
  ~GpuSolver();

 private:
  int m_;    // Rows of the SP matrix
  int nnz_;  // non-zeros
  utl::Logger* log_;
  float error_;

  // {d_cooRowIndex_, d_cooColIndex_, d_cooVal_} are the device vectors used to
  // store the COO formatted triplets.
  // https://en.wikipedia.org/wiki/Sparse_matrix#Coordinate_list_(COO)
  // d_instLocVec_ and d_fixedInstForceVec_ are the device lists corresponding
  // to instLocVec and fixedInstForceVec.
  thrust::device_vector<int> d_cooRowIndex_, d_cooColIndex_;
  thrust::device_vector<float> d_cooVal_, d_fixedInstForceVec_, d_instLocVec_;

  // {r_cooRowIndex_, r_cooColIndex_, r_cooVal_} are the raw pointers to the
  // device vectors above.
  int *r_cooRowIndex_, *r_cooColIndex_;
  float *r_cooVal_, *r_instLocVec_, *r_fixedInstForceVec_;

  void cudaerror(cudaError_t code);
  void cusparseerror(cusparseStatus_t code);
  void cusolvererror(cusolverStatus_t code);
};
}  // namespace gpl