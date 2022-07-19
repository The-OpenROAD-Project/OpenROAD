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

  // Background variables for CUDA. Don't change with the iterating
  int m = pb_->placeInsts().size(); // number of rows of matrix A
  float tol = 1e-6; // Threshold to decide if the 
  int reorder = 0;  // "0" for common matrix meaning no extra processing
  int singularity = -1; // Output. Showing if the sparse matrix is signular or not. -1 = A is invertible

  for(int i=1; i<=ipVars_.maxIter; i++) {
    updatePinInfo();
    createSparseMatrix();
    if (GPU == 1){

      // Set sparse matrices from dense matrices.
      cooRowIndexX.clear();
      cooColIndexX.clear();
      cooValX.clear();
      cooRowIndexY.clear();
      cooColIndexY.clear();
      cooValY.clear();
      for (size_t i = 0; i < placeInstForceMatrixX_.rows(); i ++){
        for (size_t j = 0; j < placeInstForceMatrixX_.cols(); j ++){
          if (placeInstForceMatrixX_.coeffRef(i,j) != 0){
            cooRowIndexX.push_back(i);
            cooColIndexX.push_back(j);
            cooValX.push_back(placeInstForceMatrixX_.coeffRef(i,j));
          } 
          if (placeInstForceMatrixY_.coeffRef(i,j) != 0){
            cooRowIndexY.push_back(i);
            cooColIndexY.push_back(j);
            cooValY.push_back(placeInstForceMatrixY_.coeffRef(i,j));
          } 
        }
      }

      int nnz_x = cooRowIndexX.size(); // number of non-zeros for matrix X
      int nnz_y = cooRowIndexY.size(); // number of non-zeros for matrix Y

      // Allocate device memeory and copy data to device
      int *d_cooRowIndexX, *d_cooColIndexX,  *d_cooRowIndexY, *d_cooColIndexY;
      float *d_cooValX, *d_instLocVecX, *d_fixedInstForceVecX_, *d_cooValY, *d_instLocVecY, *d_fixedInstForceVecY_;
      CUDA_ERROR(cudaMalloc((void**)&d_cooRowIndexX, nnz_x * sizeof(int)));
      CUDA_ERROR(cudaMalloc((void**)&d_cooColIndexX, nnz_x * sizeof(int)));
      CUDA_ERROR(cudaMalloc((void**)&d_cooValX, nnz_x * sizeof(float)));
      CUDA_ERROR(cudaMalloc((void**)&d_fixedInstForceVecX_, m * sizeof(float)));
      CUDA_ERROR(cudaMalloc((void**)&d_instLocVecX, m * sizeof(float)));

      CUDA_ERROR(cudaMalloc((void**)&d_cooRowIndexY, nnz_y * sizeof(int)));
      CUDA_ERROR(cudaMalloc((void**)&d_cooColIndexY, nnz_y * sizeof(int)));
      CUDA_ERROR(cudaMalloc((void**)&d_cooValY, nnz_y * sizeof(float)));
      CUDA_ERROR(cudaMalloc((void**)&d_fixedInstForceVecY_, m * sizeof(float)));
      CUDA_ERROR(cudaMalloc((void**)&d_instLocVecY, m * sizeof(float)));

      // Copy data (COO storage method)
      CUDA_ERROR(cudaMemcpy(d_cooRowIndexX, cooRowIndexX.data(), sizeof(int)*nnz_x, cudaMemcpyHostToDevice));
      CUDA_ERROR(cudaMemcpy(d_cooColIndexX, cooColIndexX.data(), sizeof(int)*nnz_x, cudaMemcpyHostToDevice));
      CUDA_ERROR(cudaMemcpy(d_cooValX, cooValX.data(), sizeof(float)*nnz_x, cudaMemcpyHostToDevice));
      CUDA_ERROR(cudaMemcpy(d_fixedInstForceVecX_, fixedInstForceVecX_.data(), sizeof(float)*m, cudaMemcpyHostToDevice));
            
      CUDA_ERROR(cudaMemcpy(d_cooRowIndexY, cooRowIndexY.data(), sizeof(int)*nnz_y, cudaMemcpyHostToDevice));
      CUDA_ERROR(cudaMemcpy(d_cooColIndexY, cooColIndexY.data(), sizeof(int)*nnz_y, cudaMemcpyHostToDevice));
      CUDA_ERROR(cudaMemcpy(d_cooValY, cooValY.data(), sizeof(float)*nnz_y, cudaMemcpyHostToDevice));
      CUDA_ERROR(cudaMemcpy(d_fixedInstForceVecY_, fixedInstForceVecY_.data(), sizeof(float)*m, cudaMemcpyHostToDevice));
      
      // Set handler
      cusolverSpHandle_t handleCusolverX = NULL;
      cusparseHandle_t handleCusparseX = NULL;
      cudaStream_t streamX = NULL;

      cusolverSpHandle_t handleCusolverY = NULL;
      cusparseHandle_t handleCusparseY = NULL;
      cudaStream_t streamY = NULL;

      // Initialize handler
      CUSOLVER_ERROR(cusolverSpCreate(&handleCusolverX));
      CUSPARSE_ERROR(cusparseCreate(&handleCusparseX));
      CUDA_ERROR(cudaStreamCreate(&streamX));
      CUSOLVER_ERROR(cusolverSpSetStream(handleCusolverX, streamX));
      CUSPARSE_ERROR(cusparseSetStream(handleCusparseX, streamX));

      CUSOLVER_ERROR(cusolverSpCreate(&handleCusolverY));
      CUSPARSE_ERROR(cusparseCreate(&handleCusparseY));
      CUDA_ERROR(cudaStreamCreate(&streamY));
      CUSOLVER_ERROR(cusolverSpSetStream(handleCusolverY, streamY));
      CUSPARSE_ERROR(cusparseSetStream(handleCusparseY, streamY));

      // Create and define cusparse descriptor
      cusparseMatDescr_t descrX = NULL;
      CUSPARSE_ERROR(cusparseCreateMatDescr(&descrX));
      CUSPARSE_ERROR(cusparseSetMatType(descrX, CUSPARSE_MATRIX_TYPE_GENERAL));
      CUSPARSE_ERROR(cusparseSetMatIndexBase(descrX, CUSPARSE_INDEX_BASE_ZERO));
            
      cusparseMatDescr_t descrY = NULL;
      CUSPARSE_ERROR(cusparseCreateMatDescr(&descrY));
      CUSPARSE_ERROR(cusparseSetMatType(descrY, CUSPARSE_MATRIX_TYPE_GENERAL));
      CUSPARSE_ERROR(cusparseSetMatIndexBase(descrY, CUSPARSE_INDEX_BASE_ZERO));

      // transform from coordinates (COO) values to compressed row pointers (CSR) values
      // https://docs.nvidia.com/cuda/cusparse/index.html
      int *d_csrRowIndX = NULL, *d_csrRowIndY = NULL;;
      CUDA_ERROR(cudaMalloc((void**)&d_csrRowIndX, (m+1) * sizeof(int)));
      CUDA_ERROR(cudaMalloc((void**)&d_csrRowIndY, (m+1) * sizeof(int)));
      CUSPARSE_ERROR(cusparseXcoo2csr(handleCusparseX, d_cooRowIndexX, nnz_x, m, d_csrRowIndX, CUSPARSE_INDEX_BASE_ZERO));
      CUSPARSE_ERROR(cusparseXcoo2csr(handleCusparseY, d_cooRowIndexY, nnz_y, m, d_csrRowIndY, CUSPARSE_INDEX_BASE_ZERO));
      CUSOLVER_ERROR(cusolverSpScsrlsvqr(handleCusolverX, m, nnz_x, descrX, d_cooValX, d_csrRowIndX, d_cooColIndexX, d_fixedInstForceVecX_, tol, reorder, d_instLocVecX, &singularity));
      CUSOLVER_ERROR(cusolverSpScsrlsvqr(handleCusolverY, m, nnz_y, descrY, d_cooValY, d_csrRowIndY, d_cooColIndexY, d_fixedInstForceVecY_, tol, reorder, d_instLocVecY, &singularity));
      
      // Sync and Copy data to host
      CUDA_ERROR(cudaMemcpyAsync(instLocVecX_.data(), d_instLocVecX, sizeof(float)*m, cudaMemcpyDeviceToHost, streamX));
      CUDA_ERROR(cudaMemcpyAsync(instLocVecY_.data(), d_instLocVecY, sizeof(float)*m, cudaMemcpyDeviceToHost, streamY));

      // Destroy what is not needed in both of device and host
      CUDA_ERROR(cudaFree(d_cooColIndexX));
      CUDA_ERROR(cudaFree(d_cooRowIndexX));
      CUDA_ERROR(cudaFree(d_cooValX));
      CUDA_ERROR(cudaFree(d_csrRowIndX));
      CUDA_ERROR(cudaFree(d_instLocVecX));
      CUDA_ERROR(cudaFree(d_fixedInstForceVecX_));
      CUDA_ERROR(cudaFree(d_cooColIndexY));
      CUDA_ERROR(cudaFree(d_cooRowIndexY));
      CUDA_ERROR(cudaFree(d_cooValY));
      CUDA_ERROR(cudaFree(d_csrRowIndY));
      CUDA_ERROR(cudaFree(d_instLocVecY));
      CUDA_ERROR(cudaFree(d_fixedInstForceVecY_));
      CUSPARSE_ERROR(cusparseDestroyMatDescr( descrX ) );
      CUSPARSE_ERROR(cusparseDestroy(handleCusparseX));
      CUSOLVER_ERROR(cusolverSpDestroy(handleCusolverX));  
      CUSPARSE_ERROR(cusparseDestroyMatDescr( descrY ) );
      CUSPARSE_ERROR(cusparseDestroy(handleCusparseY));
      CUSOLVER_ERROR(cusolverSpDestroy(handleCusolverY));   

      std::cout << "GPU X: " << instLocVecX_[0] << ", " << instLocVecX_[1] << ", " << instLocVecX_[200] << std:: endl;
      std::cout << "GPU Y: " << instLocVecY_[0] << ", " << instLocVecY_[1] << ", " << instLocVecY_[200] << std:: endl;
         
    }
    else{
      // BiCGSTAB solver for initial place
      BiCGSTAB< SMatrix, IdentityPreconditioner > solver;
      solver.setMaxIterations(ipVars_.maxSolverIter);
      solver.compute(placeInstForceMatrixX_);
      instLocVecX_ = solver.solveWithGuess(fixedInstForceVecX_, instLocVecX_);
      errorX = solver.error();
      
      solver.compute(placeInstForceMatrixY_);
      instLocVecY_ = solver.solveWithGuess(fixedInstForceVecY_, instLocVecY_);
      errorY = solver.error();

      log_->report("[InitialPlace]  Iter: {} CG residual: {:0.8f} HPWL: {}",
        i, max(errorX, errorY), pb_->hpwl());

      std::cout << "CPU X: " << instLocVecX_[0] << ", " << instLocVecX_[1] << ", " << instLocVecX_[200] << std:: endl;
      std::cout << "CPU Y: " << instLocVecY_[0] << ", " << instLocVecY_[1] << ", " << instLocVecY_[200] << std:: endl;
    }
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

            // if (GPU == 1){
            //   triX.push_back({inst1, inst1, weightX});
            //   triX.push_back({inst2, inst2, weightX});
            //   triX.push_back({inst1, inst2, -weightX});
            //   triX.push_back({inst2, inst1, -weightX});
            // }
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

            // if (GPU == 1){
            //   triX.push_back({inst2, inst2, weightX});
            // }
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
            // if (GPU == 1){
            //   triX.push_back({inst1, inst1, weightX});
            // }
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
            // if (GPU == 1){
            //   triY.push_back({inst1, inst1, weightY});
            //   triY.push_back({inst2, inst2, weightY});
            //   triY.push_back({inst1, inst2, -weightY});
            //   triY.push_back({inst2, inst1, -weightY});
            // }
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

            // if (GPU == 1){
            //   triY.push_back({inst2, inst2, weightY});
            // }
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
            // if (GPU == 1){
            //   triY.push_back({inst1, inst1, weightY});
            // }
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
