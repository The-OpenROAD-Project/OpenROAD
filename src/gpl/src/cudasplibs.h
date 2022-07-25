#ifndef __CUDASPLIBS__
#define __CUDASPLIBS__

#include <cuda_runtime.h>
#include "cusparse.h"
#include "cusolverSp.h"

#include <thrust/host_vector.h>
#include <thrust/device_vector.h>
#include <thrust/copy.h>
#include <thrust/fill.h>
#include <thrust/sequence.h>
 #include "device_launch_parameters.h"
#include <vector>
#include <Eigen/SparseCore>

namespace utl {
class Logger;
}

class cudasplibs{
private:
int m;      // Rows of the SP matrix
int nnz;    // non-zeros
utl::Logger* log_;

void cudaerror(cudaError_t code);
void cusparseerror(cusparseStatus_t code);
void cusolvererror(cusolverStatus_t code);

public:

int *d_cooRowIndex, *d_cooColIndex;
float *d_cooVal, *d_instLocVec, *d_fixedInstForceVec;
cudasplibs(std::vector<int>& cooRowIndex, std::vector<int>& cooColIndex, std::vector<float>& cooVal, Eigen::VectorXf& fixedInstForceVec, utl::Logger* logger);
void cusolverSpQR(Eigen::VectorXf& instLocVec);
float error_cal();
~cudasplibs();

};

#endif