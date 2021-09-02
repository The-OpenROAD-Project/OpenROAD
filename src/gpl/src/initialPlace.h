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

#ifndef __REPLACE_INIT_PLACE__
#define __REPLACE_INIT_PLACE__

#include <Eigen/SparseCore>
#include "odb/db.h"
#include <memory>

namespace utl {
class Logger;
}

namespace gpl {

class PlacerBase;
class Graphics;

class InitialPlaceVars {
public:
  int maxIter;
  int minDiffLength;
  int maxSolverIter;
  int maxFanout;
  float netWeightScale;
  bool debug;

  InitialPlaceVars();
  void reset();
};

typedef Eigen::SparseMatrix<float, Eigen::RowMajor> SMatrix;

class InitialPlace {
  public:
    InitialPlace();
    InitialPlace(InitialPlaceVars ipVars, 
        std::shared_ptr<PlacerBase> pb,
        utl::Logger* logger);
    ~InitialPlace();

    void doBicgstabPlace();

  private:
    InitialPlaceVars ipVars_;
    std::shared_ptr<PlacerBase> pb_;
    utl::Logger* log_;

    // Solve two SparseMatrix equations here;
    //
    // find instLocVecX_
    // s.t. satisfies placeInstForceMatrixX_ * instLocVecX_ = fixedInstForceVecX_
    //
    // find instLocVecY_
    // s.t. satisfies placeInstForceMatrixY_ * instLocVecY_ = fixedInstForceVecY_
    //
    // instLocVecX_ : current/target instances' center X coordinates. 1-col vector.
    // instLocVecY_ : current/target instances' center Y coordinates. 1-col vector.
    //
    // fixedInstForceVecX_ : contains fixed instances' forces toward X coordi. 1-col vector.
    // fixedInstForceVecY_ : contains fixed instances' forces toward Y coordi. 1-col vector.
    //
    // placeInstForceMatrixX_ :
    //        SparseMatrix that contains connectivity forces on X // B2B model is used
    //
    // placeInstForceMatrixY_ :
    //        SparseMatrix that contains connectivity forces on Y // B2B model is used
    //
    // Used the interative BiCGSTAB solver to solve matrix eqs.

    Eigen::VectorXf instLocVecX_, fixedInstForceVecX_;
    Eigen::VectorXf instLocVecY_, fixedInstForceVecY_;
    SMatrix placeInstForceMatrixX_, placeInstForceMatrixY_;

    void placeInstsCenter();
    void setPlaceInstExtId();
    void updatePinInfo();
    void createSparseMatrix();
    void updateCoordi();
    void reset();
};

}
#endif
