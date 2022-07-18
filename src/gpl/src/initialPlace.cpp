///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018-2020, The Regents of the University of California
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
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////
  
#include <fstream>

#include "initialPlace.h"
#include "placerBase.h"
#include <iostream>

#include <Eigen/IterativeLinearSolvers>

#include "plot.h"
#include "graphics.h"

#include "utl/Logger.h"
#include <cuda.h>
#include <cuda_runtime.h>
#include "cusparse.h"
#include "cusolverDn.h"
#include "cusolverSp.h"

#define CUDA_ERROR(value) do { \
   cudaError_t m_cudaStat = value; \
   if(m_cudaStat != cudaSuccess){ \
       fprintf(stderr, "Error %s at line %d in file %s\n", cudaGetErrorString(m_cudaStat), __LINE__, __FILE__); \
       exit(-1); \
   } \
} while(0)
#define CUSPARSE_ERROR(value) do { \
    cusparseStatus_t _m_status = value;\
    if (_m_status != CUSPARSE_STATUS_SUCCESS){\
        fprintf(stderr, "Error %d at line %d in file %s\n", (int)_m_status, __LINE__, __FILE__);\
    exit(-5); \
    } \
} while(0)

#define CUSOLVER_ERROR(value) do { \
    cusolverStatus_t _m_status = value; \
    if (_m_status != CUSOLVER_STATUS_SUCCESS){ \
        fprintf(stderr, "Error %d at line %d in file %s\n", (int)_m_status, __LINE__, __FILE__);\
    exit(-5);  \
    } \
} while(0)

// Compares two intervals according to starting times.
bool compareTri(tri i1, tri i2)
{
    return (i1.rowInd < i2.rowInd);
}

namespace gpl {
using namespace std;

using Eigen::BiCGSTAB;
using Eigen::IdentityPreconditioner;
using utl::GPL;

typedef Eigen::Triplet< float > T;

InitialPlaceVars::InitialPlaceVars() 
{
  reset();
}

void InitialPlaceVars::reset() {
  maxIter = 20;
  minDiffLength = 1500;
  maxSolverIter = 100;
  maxFanout = 200;
  netWeightScale = 800.0;
  debug = false;
}

InitialPlace::InitialPlace()
: ipVars_(), pb_(nullptr), log_(nullptr) {} 

InitialPlace::InitialPlace(InitialPlaceVars ipVars, 
    std::shared_ptr<PlacerBase> pb,
    utl::Logger* log)
: ipVars_(ipVars), pb_(pb), log_(log)
{
}

InitialPlace::~InitialPlace() {
  reset();
}

void InitialPlace::reset() {
  pb_ = nullptr;
  ipVars_.reset();
}

#ifdef ENABLE_CIMG_LIB
static PlotEnv pe;
#endif

void InitialPlace::doBicgstabPlace() {
  float errorX = 0.0f, errorY = 0.0f;

#ifdef ENABLE_CIMG_LIB
  pe.setPlacerBase(pb_);
  pe.setLogger(log_);
  pe.Init();
#endif

  std::unique_ptr<Graphics> graphics;
  if (ipVars_.debug && Graphics::guiActive()) {
    graphics = make_unique<Graphics>(log_, pb_);
  }

  placeInstsCenter();

  // set ExtId for idx reference // easy recovery
  setPlaceInstExtId();

  for(int i=1; i<=ipVars_.maxIter; i++) {
    updatePinInfo();
    createSparseMatrix();

    // Background variables for CUDA. Also don't change with the iterating.
    int m = pb_->placeInsts().size(); // number of rows of matrix A
    float tol = 1e-6;
    int reorder = 0;
    int singularity = -1;

    cooRowIndexX.clear();
    cooColIndexX.clear();
    cooValX.clear();
    // Sort the triplets by the indices of rows.
    sort(triX.begin(), triX.end(), compareTri);
    // Sort by the indices of columns for each row.
    int left = 0, right = 0;
    for(size_t i = 0; i < m; i++){
      if (i > 0 && triX[i].rowInd > triX[i-1].rowInd){
        left = i;
      }
      if (i < m && triX[i].rowInd < triX[i+1].rowInd){
        right = i;
      }
      if (left < right){
        for (size_t j = left; j < right; j++){
          for (size_t k = left; k < right-j+left; k++){
            if (triX[k].colInd > triX[k+1].colInd){
              swap(triX[k], triX[k+1]);
            }
          }
        }
      }
    }
    // Sum the repeated values for each position
    for (size_t i = 0; i < triX.size(); i++){ 
      if(i > 0 && (triX[i-1].rowInd == triX[i].rowInd && triX[i-1].colInd == triX[i].colInd)){
        // triX[i-1].val += triX[i].val;
        cooValX[i-1] += triX[i].val;
        triX.erase(triX.begin()+i);
        i -= 1;
      }
      else{
        cooRowIndexX.push_back(triX[i].rowInd);
        cooColIndexX.push_back(triX[i].colInd);
        cooValX.push_back(triX[i].val);
      }
    }
    
    // transfer the sparse matrix to dense one
    std::vector<float> mat;

    std::vector< T > trip;
    trip.reserve(1000000);
    for (int i = 0; i < cooColIndexX.size(); i++){
      trip.push_back(T(cooRowIndexX[i], cooColIndexX[i], cooValX[i]));
    }

    std::ofstream outFile;
    outFile.open("dif.txt");
    SMatrix placeInstForceMatrixX_fake;
    placeInstForceMatrixX_fake.resize(m, m);
    placeInstForceMatrixX_fake.setFromTriplets(trip.begin(), trip.end());
    for(int i = 0; i < placeInstForceMatrixX_fake.rows(); i++){
      outFile << placeInstForceMatrixX_fake.row(i)-placeInstForceMatrixX_.row(i) << std::endl;
      outFile << ", " << std::endl;
    }
    outFile.close();

    std::ofstream outFile2;
    outFile2.open("new.txt");
    if (outFile2){
      for(int i = 0; i < placeInstForceMatrixX_fake.rows(); i++){
        outFile2 << placeInstForceMatrixX_fake.row(i) << std::endl;
        outFile2 << ", " << std::endl;
      }
    }
    outFile2.close();
    // from Dense matrix
    cooRowIndexX.clear();
    cooColIndexX.clear();
    cooValX.clear();
    for (size_t i = 0; i < placeInstForceMatrixX_.rows(); i ++){
      for (size_t j = 0; j < placeInstForceMatrixX_.cols(); j ++){
        if (placeInstForceMatrixX_.coeffRef(i,j) != 0){
          cooRowIndexX.push_back(i);
          cooColIndexX.push_back(j);
          cooValX.push_back(placeInstForceMatrixX_.coeffRef(i,j));

        } 
      }
    }

    int nnz = cooRowIndexX.size(); // number of non-zeros uncompressed row indices.

    std::cout << "cooRowIndexX: "<< cooRowIndexX[0] <<", " << cooRowIndexX[nnz-1] << std::endl;

    std::cout << "cooColIndexX: "<< cooColIndexX[0] <<", " << cooColIndexX[nnz-1] << std::endl;

    std::cout << "cooValX: "<< cooValX[0] <<", "  << cooValX[nnz-1] << std::endl;

    std::cout << "fixedInstForceVecX_: "<< fixedInstForceVecX_[0] <<", "  << fixedInstForceVecX_[m-1] << std::endl;
    // Allocate device memeory and copy data to device
    int *d_cooRowIndexX, *d_cooColIndexX;
    float *d_cooValX, *d_instLocVecX, *d_fixedInstForceVecX_;
    CUDA_ERROR(cudaMalloc((void**)&d_cooRowIndexX, nnz * sizeof(int)));
    CUDA_ERROR(cudaMalloc((void**)&d_cooColIndexX, nnz * sizeof(int)));
    CUDA_ERROR(cudaMalloc((void**)&d_cooValX, nnz * sizeof(float)));
    CUDA_ERROR(cudaMalloc((void**)&d_fixedInstForceVecX_, m * sizeof(float)));
    CUDA_ERROR(cudaMalloc((void**)&d_instLocVecX, m * sizeof(float)));

    // Copy data (COO storage method)
    CUDA_ERROR(cudaMemcpy(d_cooRowIndexX, cooRowIndexX.data(), sizeof(int)*nnz, cudaMemcpyHostToDevice));
    CUDA_ERROR(cudaMemcpy(d_cooColIndexX, cooColIndexX.data(), sizeof(int)*nnz, cudaMemcpyHostToDevice));
    CUDA_ERROR(cudaMemcpy(d_cooValX, cooValX.data(), sizeof(float)*nnz, cudaMemcpyHostToDevice));
    CUDA_ERROR(cudaMemcpy(d_fixedInstForceVecX_, fixedInstForceVecX_.data(), sizeof(float)*m, cudaMemcpyHostToDevice));
    // CUDA_ERROR(cudaMemset(d_instLocVecX, 0, sizeof(float)*m));
    
    // Set handler
    cusolverSpHandle_t handleCusolver = NULL;
    cusparseHandle_t handleCusparse = NULL;
    cudaStream_t stream = NULL;

    // Initialize handler
    CUSOLVER_ERROR(cusolverSpCreate(&handleCusolver));
    CUSPARSE_ERROR(cusparseCreate(&handleCusparse));
    CUDA_ERROR(cudaStreamCreate(&stream));
    CUSOLVER_ERROR(cusolverSpSetStream(handleCusolver, stream));
    CUSPARSE_ERROR(cusparseSetStream(handleCusparse, stream));

    std::cout << "/ " << std::endl;
    // Create and define cusparse descriptor
    cusparseMatDescr_t descrA = NULL;
    CUSPARSE_ERROR(cusparseCreateMatDescr(&descrA));
    CUSPARSE_ERROR(cusparseSetMatType(descrA, CUSPARSE_MATRIX_TYPE_GENERAL));
    CUSPARSE_ERROR(cusparseSetMatIndexBase(descrA, CUSPARSE_INDEX_BASE_ZERO));

    std::cout << ". " << std::endl;
    // transform from coordinates (COO) values to compressed row pointers (CSR) values
    // https://docs.nvidia.com/cuda/cusparse/index.html
    int *d_csrRowIndX = NULL;
    CUDA_ERROR(cudaMalloc((void**)&d_csrRowIndX, (m+1) * sizeof(int)));
    // CUDA_ERROR(cudaMemset(d_csrRowIndX, 0, sizeof(int)*(m+1)));
    CUSPARSE_ERROR(cusparseXcoo2csr(handleCusparse, d_cooRowIndexX, nnz, m, d_csrRowIndX, CUSPARSE_INDEX_BASE_ZERO));
    
    // test
    float* cooVal;
    float* cooVal1;
    int* c;
    int* co;
    cooVal = (float*)malloc(m * sizeof(float));
    cooVal1 = (float*)malloc(nnz * sizeof(float));
    c = (int*)malloc(nnz * sizeof(int));
    co = (int*)malloc((m+1)* sizeof(int));

    CUDA_ERROR(cudaMemcpy(c, d_cooRowIndexX, sizeof(int)*nnz, cudaMemcpyDeviceToHost));
    std::cout << "d_cooRowIndexX: "<< c[0] <<", " << c[nnz-1] << std::endl;

    CUDA_ERROR(cudaMemcpy(c, d_cooColIndexX, sizeof(int)*nnz, cudaMemcpyDeviceToHost));
    std::cout << "d_cooColIndexX: "<< c[0] <<", " << c[nnz-1] << std::endl;

    CUDA_ERROR(cudaMemcpy(co, d_csrRowIndX, sizeof(int)*(m+1), cudaMemcpyDeviceToHost));
    std::cout << "d_csrRowIndX: "<< co[0] <<", "  << co[m] << std::endl;

    CUDA_ERROR(cudaMemcpy(cooVal1, d_cooValX, sizeof(float)*nnz, cudaMemcpyDeviceToHost));
    std::cout << "d_cooValX: "<< cooVal1[0] <<", "  << cooVal1[nnz-1] << std::endl;

    CUDA_ERROR(cudaMemcpy(cooVal, d_fixedInstForceVecX_, sizeof(float)*m, cudaMemcpyDeviceToHost));
    // std::cout << "fixedInstForceVecX_: " << fixedInstForceVecX_[m-1] << std::endl;
    std::cout << "d_fixedInstForceVecX_: "<< cooVal[0] <<", "  << cooVal[m-1] << std::endl;
    std::vector<float> row;
    row.resize(m);
    CUDA_ERROR(cudaMemcpy(row.data(), d_instLocVecX, sizeof(float)*(m), cudaMemcpyDeviceToHost));
    // std::cout << "instLocVecX: " << instLocVecX_[m-1] << std::endl;
    std::cout << "d_instLocVecX: "<< row[0] <<", " << row[m-1] << std::endl;

    // Call fixedInstForceVecX_
    // https://docs.nvidia.com/cuda/cusolver/index.html
    std::cout << "! " << std::endl;
    CUSOLVER_ERROR(cusolverSpScsrlsvqr(handleCusolver, m, nnz, descrA, d_cooValX, d_csrRowIndX, d_cooColIndexX, d_fixedInstForceVecX_, tol, reorder, d_instLocVecX, &singularity));
    
    std::cout << ", " << std::endl;
    float* instLocVec;
    instLocVec = (float*) malloc(sizeof(float)*m);
    // Sync and Copy data to host
    CUDA_ERROR(cudaMemcpyAsync(instLocVec, d_instLocVecX, sizeof(float)*m, cudaMemcpyDeviceToHost, stream));

    std::cout << instLocVec[0] << ", " << instLocVec[1] << ", " << instLocVec[200] << std::endl;
    
    // Test the correctness of GPU computing.
    // float* b_ = (float*) calloc(m, sizeof(float));
    std::vector<float> b_;
    b_.resize(m);
    std::fill(b_.begin(), b_.end(), 0);
    for (size_t i=0; i<nnz; i++){
      // Row number: cooRowIndX[i]. Column number: cooColIndX[i]
      b_[cooRowIndexX[i]] += cooValX[i] * instLocVec[cooColIndexX[i]];
    }  

    for (size_t i=0; i<m; i++){
      // Row number: cooRowIndX[i]. Column number: cooColIndX[i]
      b_[i] -= fixedInstForceVecX_[i];
    }  
    // Destroy what is not needed in both of device and host
    CUDA_ERROR(cudaFree(d_cooColIndexX));
    CUDA_ERROR(cudaFree(d_cooRowIndexX));
    CUDA_ERROR(cudaFree(d_cooValX));
    CUDA_ERROR(cudaFree(d_csrRowIndX));
    CUDA_ERROR(cudaFree(d_instLocVecX));
    CUDA_ERROR(cudaFree(d_fixedInstForceVecX_));
    CUSPARSE_ERROR(cusparseDestroyMatDescr( descrA ) );
    CUSPARSE_ERROR(cusparseDestroy(handleCusparse));
    CUSOLVER_ERROR(cusolverSpDestroy(handleCusolver));

    // BiCGSTAB solver for initial place
    BiCGSTAB< SMatrix, IdentityPreconditioner > solver;
    solver.setMaxIterations(ipVars_.maxSolverIter);
    solver.compute(placeInstForceMatrixX_);
    instLocVecX_ = solver.solve(fixedInstForceVecX_);
    std::cout << instLocVecX_[0] << ", " << instLocVecX_[1] << ", " << instLocVec[200] << std::endl;

    instLocVecX_ = solver.solveWithGuess(fixedInstForceVecX_, instLocVecX_);
    errorX = solver.error();

    std::cout << instLocVecX_[0] << ", " << instLocVecX_[1] << std::endl;
    
    // Test the correctness of GPU computing.
    // float* b_ = (float*) calloc(m, sizeof(float));
    std::vector<float> b;
    b.resize(m);
    std::fill(b.begin(), b.end(), 0);
    for (size_t i=0; i<nnz; i++){
      // Row number: cooRowIndX[i]. Column number: cooColIndX[i]
      b[cooRowIndexX[i]] += cooValX[i] * instLocVecX_[cooColIndexX[i]];
    }  

    std::cout << fixedInstForceVecX_[0] << ", " << fixedInstForceVecX_[1] << std::endl;
    for (size_t i=0; i<m; i++){
      // Row number: cooRowIndX[i]. Column number: cooColIndX[i]
      b[i] -= fixedInstForceVecX_[i];
    }  
    // solver.compute(placeInstForceMatrixY_);
    // instLocVecY_ = solver.solveWithGuess(fixedInstForceVecY_, instLocVecY_);
    // errorY = solver.error();

    log_->report("[InitialPlace]  Iter: {} CG residual: {:0.8f} HPWL: {}",
       i, max(errorX, errorY), pb_->hpwl());
    updateCoordi();

#ifdef ENABLE_CIMG_LIB
    if (PlotEnv::isPlotEnabled()) pe.SaveCellPlotAsJPEG(
        string("InitPlace ") + to_string(i), false,
        string("ip_") + to_string(i));
#endif

    if (graphics) {
        graphics->cellPlot(true);
    }

    if( max(errorX, errorY) <= 1e-5 && i >= 5 ) {
      break;
    }
  }
}

// starting point of initial place is center.
void InitialPlace::placeInstsCenter() {
  const int centerX = pb_->die().coreCx();
  const int centerY = pb_->die().coreCy();

  for(auto& inst: pb_->placeInsts()) {
    if (!inst->isLocked()) {
      inst->setCenterLocation(centerX, centerY);
    }
  }
}

void InitialPlace::setPlaceInstExtId() {
  // reset ExtId for all instances
  for(auto& inst : pb_->insts()) {
    inst->setExtId(INT_MAX);
  }
  // set index only with place-able instances
  for(auto& inst : pb_->placeInsts()) {
    inst->setExtId(&inst - &(pb_->placeInsts()[0]));
  }
}

void InitialPlace::updatePinInfo() {
  // reset all MinMax attributes
  for(auto& pin : pb_->pins()) {
    pin->unsetMinPinX();
    pin->unsetMinPinY();
    pin->unsetMaxPinX();
    pin->unsetMaxPinY();
  }

  for(auto& net : pb_->nets()) {
    Pin* pinMinX = nullptr, *pinMinY = nullptr;
    Pin* pinMaxX = nullptr, *pinMaxY = nullptr;  
    int lx = INT_MAX, ly = INT_MAX;
    int ux = INT_MIN, uy = INT_MIN;

    // Mark B2B info on Pin structures
    for(auto& pin : net->pins()) {
      if( lx > pin->cx() ) {
        if( pinMinX ) {
          pinMinX->unsetMinPinX();
        }
        lx = pin->cx();
        pinMinX = pin; 
        pinMinX->setMinPinX();
      } 
      
      if( ux < pin->cx() ) {
        if( pinMaxX ) {
          pinMaxX->unsetMaxPinX();
        }
        ux = pin->cx();
        pinMaxX = pin; 
        pinMaxX->setMaxPinX();
      } 

      if( ly > pin->cy() ) {
        if( pinMinY ) {
          pinMinY->unsetMinPinY();
        }
        ly = pin->cy();
        pinMinY = pin; 
        pinMinY->setMinPinY();
      } 
      
      if( uy < pin->cy() ) {
        if( pinMaxY ) {
          pinMaxY->unsetMaxPinY();
        }
        uy = pin->cy();
        pinMaxY = pin; 
        pinMaxY->setMaxPinY();
      } 
    }
  } 
}

// solve placeInstForceMatrixX_ * xcg_x_ = xcg_b_ and placeInstForceMatrixY_ * ycg_x_ = ycg_b_ eq.
void InitialPlace::createSparseMatrix() {
  const int placeCnt = pb_->placeInsts().size();
  instLocVecX_.resize( placeCnt );
  fixedInstForceVecX_.resize( placeCnt );
  instLocVecY_.resize( placeCnt );
  fixedInstForceVecY_.resize( placeCnt );

  placeInstForceMatrixX_.resize( placeCnt, placeCnt );
  placeInstForceMatrixY_.resize( placeCnt, placeCnt );


  // 
  // listX and listY is a temporary vector that have tuples, (idx1, idx2, val)
  //
  // listX finally becomes placeInstForceMatrixX_
  // listY finally becomes placeInstForceMatrixY_
  //
  // The triplet vector is recommended usages 
  // to fill in SparseMatrix from Eigen docs.
  //

  vector< T > listX, listY;
  listX.reserve(1000000);
  listY.reserve(1000000);

  // initialize vector
  for(auto& inst : pb_->placeInsts()) {
    int idx = inst->extId(); 
    
    instLocVecX_(idx) = inst->cx();
    instLocVecY_(idx) = inst->cy();

    fixedInstForceVecX_(idx) = fixedInstForceVecY_(idx) = 0;
  }

  // for each net
  for(auto& net : pb_->nets()) {

    // skip for small nets.
    if( net->pins().size() <= 1 ) {
      continue;
    }
 
    // escape long time cals on huge fanout.
    //
    if( net->pins().size() >= ipVars_.maxFanout) { 
      continue;
    }

    float netWeight = ipVars_.netWeightScale 
      / (net->pins().size() - 1);
    //cout << "net: " << net.net()->getConstName() << endl;

    // foreach two pins in single nets.
    auto& pins = net->pins();
    for(int pinIdx1 = 1; pinIdx1 < pins.size(); ++pinIdx1) {
      Pin* pin1 = pins[pinIdx1];
      for(int pinIdx2 = 0; pinIdx2 < pinIdx1; ++pinIdx2) {
        Pin* pin2 = pins[pinIdx2];

        // no need to fill in when instance is same
        if( pin1->instance() == pin2->instance() ) {
          continue;
        }

        // B2B modeling on min/maxX pins.
        if( pin1->isMinPinX() || pin1->isMaxPinX() ||
            pin2->isMinPinX() || pin2->isMaxPinX() ) {
          int diffX = abs(pin1->cx() - pin2->cx());
          float weightX = 0;
          if( diffX > ipVars_.minDiffLength ) {
            weightX = netWeight / diffX;
          }
          else {
            weightX = netWeight 
              / ipVars_.minDiffLength;
          }

          // both pin cames from instance
          if( pin1->isPlaceInstConnected() 
              && pin2->isPlaceInstConnected() ) {
            const int inst1 = pin1->instance()->extId();
            const int inst2 = pin2->instance()->extId();
            //cout << "inst: " << inst1 << " " << inst2 << endl;

            listX.push_back( T(inst1, inst1, weightX) );
            listX.push_back( T(inst2, inst2, weightX) );

            listX.push_back( T(inst1, inst2, -weightX) );
            listX.push_back( T(inst2, inst1, -weightX) );

            if (GPU == 1){
              triX.push_back({inst1, inst1, weightX});
              triX.push_back({inst2, inst2, weightX});
              triX.push_back({inst1, inst2, -weightX});
              triX.push_back({inst2, inst1, -weightX});
            }
            // cooRowIndexX.push_back(inst1);
            // cooColIndexX.push_back(inst1);
            // cooValX.push_back(weightX);
            // cooRowIndexX.push_back(inst2);
            // cooColIndexX.push_back(inst2);
            // cooValX.push_back(weightX);

            // cooRowIndexX.push_back(inst1);
            // cooColIndexX.push_back(inst2);
            // cooValX.push_back(-weightX);
            // cooRowIndexX.push_back(inst2);
            // cooColIndexX.push_back(inst1);
            // cooValX.push_back(-weightX);

            //cout << pin1->cx() << " " 
            //  << pin1->instance()->cx() << endl;
            fixedInstForceVecX_(inst1) += 
              -weightX * (
              (pin1->cx() - pin1->instance()->cx()) - 
              (pin2->cx() - pin2->instance()->cx()));

            fixedInstForceVecX_(inst2) +=
              -weightX * (
              (pin2->cx() - pin2->instance()->cx()) -
              (pin1->cx() - pin1->instance()->cx())); 
          }
          // pin1 from IO port / pin2 from Instance
          else if( !pin1->isPlaceInstConnected() 
              && pin2->isPlaceInstConnected() ) {
            const int inst2 = pin2->instance()->extId();
            //cout << "inst2: " << inst2 << endl;
            listX.push_back( T(inst2, inst2, weightX) );

            if (GPU == 1){
              triX.push_back({inst2, inst2, weightX});
            }
            // cooRowIndexX.push_back(inst2);
            // cooColIndexX.push_back(inst2);
            // cooValX.push_back(weightX);

            fixedInstForceVecX_(inst2) += weightX * 
              ( pin1->cx() - 
                ( pin2->cx() - pin2->instance()->cx()) );
          }
          // pin1 from Instance / pin2 from IO port
          else if( pin1->isPlaceInstConnected() 
              && !pin2->isPlaceInstConnected() ) {
            const int inst1 = pin1->instance()->extId();
            //cout << "inst1: " << inst1 << endl;
            listX.push_back( T(inst1, inst1, weightX) );
            if (GPU == 1){
              triX.push_back({inst1, inst1, weightX});
            }
            // cooRowIndexX.push_back(inst1);
            // cooColIndexX.push_back(inst1);
            // cooValX.push_back(weightX);

            fixedInstForceVecX_(inst1) += weightX *
              ( pin2->cx() -
                ( pin1->cx() - pin1->instance()->cx()) );
          }
        }
        
        // B2B modeling on min/maxY pins.
        if( pin1->isMinPinY() || pin1->isMaxPinY() ||
            pin2->isMinPinY() || pin2->isMaxPinY() ) {
          
          int diffY = abs(pin1->cy() - pin2->cy());
          float weightY = 0;
          if( diffY > ipVars_.minDiffLength ) {
            weightY = netWeight / diffY;
          }
          else {
            weightY = netWeight 
              / ipVars_.minDiffLength;
          }

          // both pin cames from instance
          if( pin1->isPlaceInstConnected() 
              && pin2->isPlaceInstConnected() ) {
            const int inst1 = pin1->instance()->extId();
            const int inst2 = pin2->instance()->extId();

            listY.push_back( T(inst1, inst1, weightY) );
            listY.push_back( T(inst2, inst2, weightY) );

            listY.push_back( T(inst1, inst2, -weightY) );
            listY.push_back( T(inst2, inst1, -weightY) );
            if (GPU == 1){
              triY.push_back({inst1, inst1, weightY});
              triY.push_back({inst2, inst2, weightY});
              triY.push_back({inst1, inst2, -weightY});
              triY.push_back({inst2, inst1, -weightY});
            }
            // cooRowIndexY.push_back(inst1);
            // cooColIndexY.push_back(inst1);
            // cooValY.push_back(weightY);
            // cooRowIndexY.push_back(inst2);
            // cooColIndexY.push_back(inst2);
            // cooValY.push_back(weightY);

            // cooRowIndexY.push_back(inst1);
            // cooColIndexY.push_back(inst2);
            // cooValY.push_back(-weightY);
            // cooRowIndexY.push_back(inst2);
            // cooColIndexY.push_back(inst1);
            // cooValY.push_back(-weightY);

            fixedInstForceVecY_(inst1) += 
              -weightY * (
              (pin1->cy() - pin1->instance()->cy()) - 
              (pin2->cy() - pin2->instance()->cy()));

            fixedInstForceVecY_(inst2) +=
              -weightY * (
              (pin2->cy() - pin2->instance()->cy()) -
              (pin1->cy() - pin1->instance()->cy())); 
          }
          // pin1 from IO port / pin2 from Instance
          else if( !pin1->isPlaceInstConnected() 
              && pin2->isPlaceInstConnected() ) {
            const int inst2 = pin2->instance()->extId();
            listY.push_back( T(inst2, inst2, weightY) );

            if (GPU == 1){
              triY.push_back({inst2, inst2, weightY});
            }
            // cooRowIndexY.push_back(inst2);
            // cooColIndexY.push_back(inst2);
            // cooValY.push_back(weightY);

            fixedInstForceVecY_(inst2) += weightY * 
              ( pin1->cy() - 
                ( pin2->cy() - pin2->instance()->cy()) );
          }
          // pin1 from Instance / pin2 from IO port
          else if( pin1->isPlaceInstConnected() 
              && !pin2->isPlaceInstConnected() ) {
            const int inst1 = pin1->instance()->extId();
            listY.push_back( T(inst1, inst1, weightY) );
            if (GPU == 1){
              triY.push_back({inst1, inst1, weightY});
            }
            // cooRowIndexY.push_back(inst1);
            // cooColIndexY.push_back(inst1);
            // cooValY.push_back(weightY);

            fixedInstForceVecY_(inst1) += weightY *
              ( pin2->cy() -
                ( pin1->cy() - pin1->instance()->cy()) );
          }
        }
      }
    }
  } 
  
  placeInstForceMatrixX_.setFromTriplets(listX.begin(), listX.end());
  placeInstForceMatrixY_.setFromTriplets(listY.begin(), listY.end());

  std::ofstream outFile;
  outFile.open("original.txt");
  if (outFile){
    for(int i = 0; i < placeInstForceMatrixX_.rows(); i++){
      outFile << placeInstForceMatrixX_.row(i) << std::endl;
      outFile << ", " << std::endl;
      // outFile << listX[i] << std::endl;
      // outFile << listX[i] << std::endl;

    }
  }
  outFile.close();
}

void InitialPlace::updateCoordi() {
  for(auto& inst : pb_->placeInsts()) {
    int idx = inst->extId();
    if (!inst->isLocked()) {
      inst->dbSetCenterLocation( instLocVecX_(idx), instLocVecY_(idx) );
      inst->dbSetPlaced();
    }
  }
}

}
