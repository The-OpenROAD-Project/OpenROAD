

#include "gpuSolver.h"

namespace gpl {
using namespace std;
using utl::GPL;

void GpuSolver::cudaerror(cudaError_t code)
{
  if (code != cudaSuccess) {
    log_->error(GPL, 1, "[CUDA ERROR] {} at line {} in file {} \n",
                 cudaGetErrorString(code),
                 __LINE__,
                 __FILE__);
    cudaDeviceReset();
  }
}
void GpuSolver::cusparseerror(cusparseStatus_t code)
{
  if (code != CUSPARSE_STATUS_SUCCESS) {
    log_->error(GPL, 1, "[CUSPARSE ERROR] {} at line {} in file {}\n",
                 cusparseGetErrorString(code),
                 __LINE__,
                 __FILE__);
    cudaDeviceReset();
  }
}

void GpuSolver::cusolvererror(cusolverStatus_t code)
{
  if (code != CUSOLVER_STATUS_SUCCESS) {
    log_->error(GPL, 1, "[CUSOLVER ERROR] {} at line {} in file {}\n",
                 cudaGetErrorString(*(cudaError_t*) &code),
                 __LINE__,
                 __FILE__);
    cudaDeviceReset();
  }
}

GpuSolver::GpuSolver(){}

GpuSolver::GpuSolver(SMatrix& placeInstForceMatrix, Eigen::VectorXf& fixedInstForceVec, utl::Logger* logger)
{
    // {cooRowIndex_, cooColIndex_, cooVal_} is the triplet vector stored as COO format to represent placeInstForceMatrix
    vector<int> cooRowIndex_, cooColIndex_;
    vector<float> cooVal_;

    for (size_t row = 0; row < placeInstForceMatrix.rows(); row++) {
      for (size_t col = 0; col < placeInstForceMatrix.cols(); col++) {
        if (placeInstForceMatrix.coeffRef(row, col) != 0) {
          cooRowIndex_.push_back(row);
          cooColIndex_.push_back(col);
          cooVal_.push_back(placeInstForceMatrix.coeffRef(row, col));
        }
      }
    }

    m_ = fixedInstForceVec.size();
    nnz_ = cooVal_.size();
    log_ = logger;

    cudaerror(cudaMalloc((void**)&d_cooRowIndex_, nnz_ * sizeof(int)));
    cudaerror(cudaMalloc((void**)&d_cooColIndex_, nnz_ * sizeof(int)));
    cudaerror(cudaMalloc((void**)&d_cooVal_, nnz_ * sizeof(float)));
    cudaerror(cudaMalloc((void**) &d_fixedInstForceVec_, m_ * sizeof(float)));
    cudaerror(cudaMalloc((void**) &d_instLocVec_, m_ * sizeof(float)));

    //Copy data (COO storage method)
    cudaerror(cudaMemcpy(d_cooRowIndex_, cooRowIndex_.data(), sizeof(int)*nnz_,
    cudaMemcpyHostToDevice));
    cudaerror(cudaMemcpy(d_cooColIndex_,
                        cooColIndex_.data(),
                        sizeof(int) * nnz_,
                        cudaMemcpyHostToDevice));
    cudaerror(cudaMemcpy(d_cooVal_, cooVal_.data(), sizeof(float)*nnz_,
    cudaMemcpyHostToDevice));
    cudaerror(cudaMemcpy(d_fixedInstForceVec_,
                        fixedInstForceVec.data(),
                        sizeof(float) * m_,
                        cudaMemcpyHostToDevice));

    std:: cout << "Yes!" << std::endl;
}

void GpuSolver::cusolverCal(Eigen::VectorXf& instLocVec){
  // Parameters that don't change with iteration and used in the CUDA code
  float tol = 1e-6;      // 	Tolerance to decide if singular or not.
  int reorder = 0;       // "0" for common matrix without ordering
  int singularity = -1;  // Output. -1 = A means invertible

  // Set handler
  cusolverSpHandle_t handleCusolver = NULL;
  cusparseHandle_t handleCusparse = NULL;
  cudaStream_t stream = NULL;

  // Initialize handler
  cusolvererror(cusolverSpCreate(&handleCusolver));
  cusparseerror(cusparseCreate(&handleCusparse));
  cudaerror(cudaStreamCreate(&stream));
  cusolvererror(cusolverSpSetStream(handleCusolver, stream));
  cusparseerror(cusparseSetStream(handleCusparse, stream));

  // Create and define cusparse descriptor
  cusparseMatDescr_t descr = NULL;
  cusparseerror(cusparseCreateMatDescr(&descr));
  cusparseerror(cusparseSetMatType(descr, CUSPARSE_MATRIX_TYPE_GENERAL));
  cusparseerror(cusparseSetMatIndexBase(descr, CUSPARSE_INDEX_BASE_ZERO));

  // transform from coordinates (COO) values to compressed row pointers (CSR)
  // values https://docs.nvidia.com/cuda/cusparse/index.html
  int* d_csrRowInd = NULL;
  //   thrust::device_vector<int> t_csrRowInd(m_ + 1, 0);
  // d_csrRowInd = thrust::raw_pointer_cast(t_csrRowInd.data());
  cudaerror(cudaMalloc((void**)&d_csrRowInd, (m_+1) * sizeof(int)));
  cusparseerror(cusparseXcoo2csr(handleCusparse,
                                 d_cooRowIndex_,
                                 nnz_,
                                 m_,
                                 d_csrRowInd,
                                 CUSPARSE_INDEX_BASE_ZERO));

  cusolvererror(cusolverSpScsrlsvqr(handleCusolver,
                                    m_,
                                    nnz_,
                                    descr,
                                    d_cooVal_,
                                    d_csrRowInd,
                                    d_cooColIndex_,
                                    d_fixedInstForceVec_,
                                    tol,
                                    reorder,
                                    d_instLocVec_,
                                    &singularity));

  // Sync and Copy data to host
  cudaerror(cudaMemcpyAsync(instLocVec.data(),
                            d_instLocVec_,
                            sizeof(float) * m_,
                            cudaMemcpyDeviceToHost,
                            stream));

  cudaerror(cudaFree(d_csrRowInd));
  cusparseerror(cusparseDestroyMatDescr( descr ) );
  cusparseerror(cusparseDestroy(handleCusparse));
  cusolvererror(cusolverSpDestroy(handleCusolver));
}

__global__ void Multi_MatVec(float error,
                             int nnz_,
                             int m_,
                             float* d_Ax,
                             float* d_fixedInstForceVec_,
                             float* d_instLocVec_,
                             int* d_cooRowIndex_,
                             int* d_cooColIndex_,
                             float* d_cooVal_)
{
  float sum = 0;
  int num = blockIdx.x * blockDim.x + threadIdx.x;
  if (num < nnz_) {
    d_Ax[d_cooRowIndex_[num]]
        += d_cooVal_[num] * d_instLocVec_[d_cooColIndex_[num]];
  }
  for (size_t row = 0; row < m_; row++) {
    sum += (d_fixedInstForceVec_[row] > 0) ? d_fixedInstForceVec_[row]
                                           : -d_fixedInstForceVec_[row];
    if (d_fixedInstForceVec_[row] > d_Ax[row])
      error += d_fixedInstForceVec_[row] - d_Ax[row];
    else
      error -= d_fixedInstForceVec_[row] - d_Ax[row];
  }
  if (sum != 0)
    error = error / sum;
}

float GpuSolver::error_cal()
{
  float error = 0;
  float* d_Ax;
  cudaerror(cudaMalloc((void**)&d_Ax, m_ * sizeof(float)));
  cudaerror(cudaMemset(d_Ax, 0, m_));
  unsigned int threads = 512;
  unsigned int blocks = (m_ + threads - 1) / threads;
  Multi_MatVec<<<blocks, threads>>>(error,
                                    nnz_,
                                    m_,
                                    d_Ax,
                                    d_fixedInstForceVec_,
                                    d_instLocVec_,
                                    d_cooRowIndex_,
                                    d_cooColIndex_,
                                    d_cooVal_);
  cudaerror(cudaFree(d_Ax));
  return (error > 0) ? error : -error;
}

GpuSolver::~GpuSolver()
{
  // Destroy what is not needed in both of device and host
  cudaerror(cudaFree(d_cooColIndex_));
  cudaerror(cudaFree(d_cooRowIndex_));
  cudaerror(cudaFree(d_cooVal_));
  cudaerror(cudaFree(d_instLocVec_));
  cudaerror(cudaFree(d_fixedInstForceVec_));
}
}
