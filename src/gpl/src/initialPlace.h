// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#pragma once

#include <Eigen/SparseCore>
#include <memory>
#include <vector>

#include "nesterovPlace.h"
#include "odb/db.h"

namespace utl {
class Logger;
}

namespace gpl {

class PlacerBaseCommon;
class PlacerBase;
class Graphics;

class InitialPlaceVars
{
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

using SMatrix = Eigen::SparseMatrix<float, Eigen::RowMajor>;

class InitialPlace
{
 public:
  InitialPlace(InitialPlaceVars ipVars,
               std::shared_ptr<PlacerBaseCommon> pbc,
               std::vector<std::shared_ptr<PlacerBase>>& pbVec,
               utl::Logger* logger);
  void doBicgstabPlace(int threads);

 private:
  InitialPlaceVars ipVars_;
  std::shared_ptr<PlacerBaseCommon> pbc_;
  std::vector<std::shared_ptr<PlacerBase>> pbVec_;
  utl::Logger* log_ = nullptr;

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

  void placeInstsCenter();
  void setPlaceInstExtId();
  void updatePinInfo();
  void createSparseMatrix();
  void updateCoordi();
};

}  // namespace gpl
