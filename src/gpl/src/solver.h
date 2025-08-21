// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <Eigen/Core>
#include <Eigen/SparseCore>

#include "odb/db.h"
#include "placerBase.h"
#include "utl/Logger.h"

namespace utl {
class Logger;
}

namespace gpl {

struct ResidualError
{
  float x;  // The relative residual error for X
  float y;  // The relative residual error for Y
};

using utl::GPL;

using SMatrix = Eigen::SparseMatrix<float, Eigen::RowMajor>;

ResidualError cpuSparseSolve(int maxSolverIter,
                             float errorThreshold,
                             SMatrix& placeInstForceMatrixX,
                             Eigen::VectorXf& fixedInstForceVecX,
                             Eigen::VectorXf& instLocVecX,
                             SMatrix& placeInstForceMatrixY,
                             Eigen::VectorXf& fixedInstForceVecY,
                             Eigen::VectorXf& instLocVecY,
                             utl::Logger* logger,
                             int threads);
}  // namespace gpl
