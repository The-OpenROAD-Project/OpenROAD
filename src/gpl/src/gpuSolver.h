#ifndef __GPUSOLVER__
#define __GPUSOLVER__

#include <cuda_runtime.h>
#include <thrust/copy.h>
#include <thrust/device_vector.h>
#include <thrust/fill.h>
#include <thrust/host_vector.h>
#include <thrust/sequence.h>

#include <Eigen/SparseCore>
#include "utl/Logger.h"
#include <vector>
#include <memory>

#include "odb/db.h"
#include "cusolverSp.h"
#include "cusparse.h"
#include "device_launch_parameters.h"

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
      GpuSolver(SMatrix& placeInstForceMatrix, Eigen::VectorXf& fixedInstForceVec, utl::Logger* logger);
      void cusolverCal(Eigen::VectorXf& instLocVec);
      float error_cal();
      ~GpuSolver();

    private:
      int m_;    // Rows of the SP matrix
      int nnz_;  // non-zeros
      utl::Logger* log_;

      // {d_cooRowIndex_, d_cooColIndex_, d_cooVal_} is its corresponding device triplet vector
      // d_instLocVec_ and d_fixedInstForceVec_ are the device list corresponding to instLocVec and fixedInstForceVec.
      int *d_cooRowIndex_, *d_cooColIndex_;
      float *d_cooVal_, *d_instLocVec_, *d_fixedInstForceVec_;

      void cudaerror(cudaError_t code);
      void cusparseerror(cusparseStatus_t code);
      void cusolvererror(cusolverStatus_t code);
  };
}
#endif