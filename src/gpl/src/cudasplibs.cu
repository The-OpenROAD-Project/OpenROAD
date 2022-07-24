#include "cudasplibs.h"

void cudaerror(cudaError_t code) {
    if (code != cudaSuccess){
        fprintf(stderr, "Error %s at line %d in file %s\n", cudaGetErrorString(code), __LINE__, __FILE__); 
        exit(-1);
    }
}
void cusparseerror(cusparseStatus_t code) {
    if (code != CUSPARSE_STATUS_SUCCESS){
        fprintf(stderr, "Error %d at line %d in file %s\n", int(code), __LINE__, __FILE__); 
        exit(-1);
    }
}

void cusolvererror(cusolverStatus_t code) {
    if (code != CUSOLVER_STATUS_SUCCESS){
        fprintf(stderr, "Error %d at line %d in file %s\n", int(code), __LINE__, __FILE__); 
        exit(-1);
    }
}

cudasplibs::cudasplibs(std::vector<int>& cooRowIndex, std::vector<int>& cooColIndex, std::vector<float>& cooVal, Eigen::VectorXf& fixedInstForceVec){
    m = fixedInstForceVec.size();
    nnz = cooVal.size();

    // Allocate device memeory and copy data to device
    cudaerror(cudaMalloc((void**)&d_cooRowIndex, nnz * sizeof(int)));
    cudaerror(cudaMalloc((void**)&d_cooColIndex, nnz * sizeof(int)));
    cudaerror(cudaMalloc((void**)&d_cooVal, nnz * sizeof(float)));
    cudaerror(cudaMalloc((void**)&d_fixedInstForceVec, m * sizeof(float)));
    cudaerror(cudaMalloc((void**)&d_instLocVec, m * sizeof(float)));
          
    // Copy data (COO storage method)
    cudaerror(cudaMemcpy(d_cooRowIndex, cooRowIndex.data(), sizeof(int)*nnz, cudaMemcpyHostToDevice));
    cudaerror(cudaMemcpy(d_cooColIndex, cooColIndex.data(), sizeof(int)*nnz, cudaMemcpyHostToDevice));
    cudaerror(cudaMemcpy(d_cooVal, cooVal.data(), sizeof(float)*nnz, cudaMemcpyHostToDevice));
    cudaerror(cudaMemcpy(d_fixedInstForceVec, fixedInstForceVec.data(), sizeof(float)*m, cudaMemcpyHostToDevice));
}

void cudasplibs::cusolverSpQR(Eigen::VectorXf& instLocVec){
    // Parameters that don't change with iteration and used in the CUDA code
    float tol = 1e-6; // 	Tolerance to decide if singular or not.
    int reorder = 0;  // "0" for common matrix without ordering
    int singularity = -1; // Output. -1 = A means invertible
    
    // Set handler
    cusolverSpHandle_t handleCusolver = NULL;
    cusparseHandle_t handleCusparse = NULL;
    cudaStream_t streamX = NULL;
    
    // Initialize handler
    cusolvererror(cusolverSpCreate(&handleCusolver));
    cusparseerror(cusparseCreate(&handleCusparse));
    cudaerror(cudaStreamCreate(&streamX));
    cusolvererror(cusolverSpSetStream(handleCusolver, streamX));
    cusparseerror(cusparseSetStream(handleCusparse, streamX));

    // Create and define cusparse descriptor
    cusparseMatDescr_t descr = NULL;
    cusparseerror(cusparseCreateMatDescr(&descr));
    cusparseerror(cusparseSetMatType(descr, CUSPARSE_MATRIX_TYPE_GENERAL));
    cusparseerror(cusparseSetMatIndexBase(descr, CUSPARSE_INDEX_BASE_ZERO));

    // transform from coordinates (COO) values to compressed row pointers (CSR) values
    // https://docs.nvidia.com/cuda/cusparse/index.html
    int *d_csrRowInd = NULL;
    cudaerror(cudaMalloc((void**)&d_csrRowInd, (m+1) * sizeof(int)));
    cusparseerror(cusparseXcoo2csr(handleCusparse, d_cooRowIndex, nnz, m, d_csrRowInd, CUSPARSE_INDEX_BASE_ZERO));
    cusolvererror(cusolverSpScsrlsvqr(handleCusolver, m, nnz, descr, d_cooVal, d_csrRowInd, d_cooColIndex, d_fixedInstForceVec, tol, reorder, d_instLocVec, &singularity));

    // Sync and Copy data to host
    cudaerror(cudaMemcpyAsync(instLocVec.data(), d_instLocVec, sizeof(float)*m, cudaMemcpyDeviceToHost, streamX));

    cudaerror(cudaFree(d_csrRowInd));
    cusparseerror(cusparseDestroyMatDescr( descr ) );
    cusparseerror(cusparseDestroy(handleCusparse));
    cusolvererror(cusolverSpDestroy(handleCusolver));  
}

__global__ void Multi_MatVec(float error, int nnz, int m , float* d_Ax, float* d_fixedInstForceVec, float* d_instLocVec, int* d_cooRowIndex, int* d_cooColIndex, float* d_cooVal){
    float sum = 0;
    int num = blockIdx.x * blockDim.x + threadIdx. x;
    if (num < nnz){
        d_Ax[d_cooRowIndex[num]] += d_cooVal[num] * d_instLocVec[d_cooColIndex[num]];
    }
    for (size_t row = 0; row < m; row++){
        sum += (d_fixedInstForceVec[row] > 0) ? d_fixedInstForceVec[row] : -d_fixedInstForceVec[row];
        if (d_fixedInstForceVec[row] > d_Ax[row])
            error += d_fixedInstForceVec[row] - d_Ax[row];
        else
            error -= d_fixedInstForceVec[row]- d_Ax[row];
    }
    if (sum != 0) error = error / sum;
}

float cudasplibs::error_cal(){
    float error = 0;
    float *d_Ax;
    cudaerror(cudaMalloc((void**)&d_Ax, m * sizeof(float)));
    cudaerror(cudaMemset(d_Ax, 0, m));
    unsigned int threads = 512;
    unsigned int blocks = (m+threads-1) / threads;
    Multi_MatVec<<<blocks, threads>>>(error, nnz, m, d_Ax, d_fixedInstForceVec, d_instLocVec, d_cooRowIndex, d_cooColIndex, d_cooVal);
    cudaerror(cudaFree(d_Ax));
    return (error > 0) ? error : -error;
}

void cudasplibs::release(){
          
    // Destroy what is not needed in both of device and host
    cudaerror(cudaFree(d_cooColIndex));
    cudaerror(cudaFree(d_cooRowIndex));
    cudaerror(cudaFree(d_cooVal));
    cudaerror(cudaFree(d_instLocVec));
    cudaerror(cudaFree(d_fixedInstForceVec));

}