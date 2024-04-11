///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018-2023, The Regents of the University of California
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

#pragma once

#include "placerObjects.h"
#include "placerBase.h"
#include "nesterovPlace.h"
#include "gpuTimingBase.h"
#include "gpuRouteBase.h"
#include <Eigen/SparseCore>
#include <memory>
#include <cuda.h>
#include <cuda_runtime.h>
#include <thrust/copy.h>
#include <thrust/device_vector.h>
#include <thrust/fill.h>
#include <thrust/host_vector.h>
#include <thrust/sequence.h>
#include <thrust/for_each.h>
#include <thrust/device_vector.h>
#include <thrust/execution_policy.h>
#include <iostream>
#include <thrust/transform.h>
#include <thrust/iterator/zip_iterator.h>
#include <Eigen/SparseCore>
#include <memory>


#include "odb/db.h"

namespace utl {
class Logger;
}

namespace gpl2 {

class PlacerBaseCommon;
class PlacerBase;

typedef Eigen::SparseMatrix<float, Eigen::RowMajor> SMatrix;

// Classes related to Initial Placement
// Compare to the original implementation, 
// we remove the forceCPU and debug variable
class InitialPlaceVars
{
 public:
  int maxIter;
  int minDiffLength;
  int maxSolverIter;
  int maxFanout;
  float netWeightScale;

  InitialPlaceVars();
  void reset();
};



class InitialPlace
{
  public:
    InitialPlace();
    InitialPlace(InitialPlaceVars ipVars,
                 std::shared_ptr<PlacerBaseCommon> pbc,
                 std::vector<std::shared_ptr<PlacerBase> >& pbVec,
                 utl::Logger* logger);
    InitialPlace(InitialPlaceVars ipVars,
                 float haloWidth,
                 int numHops,
                 bool dataflowFlag,
                 sta::dbNetwork* network,
                 odb::dbDatabase* db,
                 utl::Logger* logger);

    ~InitialPlace();
    void doInitialPlace();
    void doBicgstabPlace();
    bool doClusterNesterovPlace();
    bool doClusterNesterovPlaceIter(float& bloatFactor);
    void setPlacerBaseVars(PlacerBaseVars pbVars);
    void setNesterovBaseVars(NesterovBaseVars nbVars);
    void setNesterovPlaceVars(NesterovPlaceVars npVars);

  private:
    InitialPlaceVars ipVars_;
    std::shared_ptr<PlacerBaseCommon> pbc_;
    std::vector<std::shared_ptr<PlacerBase> > pbVec_;
    utl::Logger* log_;
    sta::dbNetwork* network_;
    odb::dbDatabase* db_; 
    std::shared_ptr<GpuRouteBase> rb_;
    std::shared_ptr<GpuTimingBase> tb_;

    // Nesterov placement
    PlacerBaseVars pbVars_;
    NesterovBaseVars nbVars_;
    NesterovPlaceVars npVars_;

    float haloWidth_ = 0;
    int numHops_ = 4;
    bool dataflowFlag_ = true;

    // Solve two SparseMatrix equations here;
    //
    // find instLocVecX_
    // s.t. satisfies placeInstForceMatrixX_ * instLocVecX_ = fixedInstForceVecX_
    //
    // find instLocVecY_
    // s.t. satisfies placeInstForceMatrixY_ * instLocVecY_ = fixedInstForceVecY_
    //
    // instLocVecX_ : current/target instances' center X coordinates. 1-col
    // vector. instLocVecY_ : current/target instances' center Y coordinates.
    // 1-col vector.
    //
    // fixedInstForceVecX_ : contains fixed instances' forces toward X coordi.
    // 1-col vector. fixedInstForceVecY_ : contains fixed instances' forces toward
    // Y coordi. 1-col vector.
    //
    // placeInstForceMatrixX_ :
    //        SparseMatrix that contains connectivity forces on X // B2B model is
    //        used
    //
    // placeInstForceMatrixY_ :
    //        SparseMatrix that contains connectivity forces on Y // B2B model is
    //        used
    //
    // Used the interative BiCGSTAB solver to solve matrix eqs.
    Eigen::VectorXf instLocVecX_, fixedInstForceVecX_;
    Eigen::VectorXf instLocVecY_, fixedInstForceVecY_;
    SMatrix placeInstForceMatrixX_, placeInstForceMatrixY_;


    // for DAC submission
    bool dpFlag_ = false;
    void placeInstsCenter();
    void updatePinInfo();
    void createSparseMatrix();
    void updateCoordi();
    void reset();
};

}  // namespace gpl
