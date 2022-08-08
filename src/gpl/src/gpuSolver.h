#ifndef __GPUSOLVER__
#define __GPUSOLVER__

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
  GpuSolver();
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
#endif