#include "cudalibs.h"

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

void cusolverSpQR(std::vector<int>& cooRowIndex, std::vector<int>& cooColIndex, std::vector<float>& cooVal, Eigen::VectorXf& fixedInstForceVec, Eigen::VectorXf& instLocVec, int m, int nnz, float tol, int reorder, int singularity){

    // Allocate device memeory and copy data to device
    int *d_cooRowIndex, *d_cooColIndex;
    float *d_cooVal, *d_instLocVec, *d_fixedInstForceVec;
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
          
    // Destroy what is not needed in both of device and host
    cudaerror(cudaFree(d_cooColIndex));
    cudaerror(cudaFree(d_cooRowIndex));
    cudaerror(cudaFree(d_cooVal));
    cudaerror(cudaFree(d_csrRowInd));
    cudaerror(cudaFree(d_instLocVec));
    cudaerror(cudaFree(d_fixedInstForceVec));
    cusparseerror(cusparseDestroyMatDescr( descr ) );
    cusparseerror(cusparseDestroy(handleCusparse));
    cusolvererror(cusolverSpDestroy(handleCusolver));  
}