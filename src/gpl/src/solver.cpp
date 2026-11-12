// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "solver.h"

#include <limits>

#include "omp.h"

namespace gpl {

ResidualError cpuSparseSolve(int maxSolverIter,
                             int iter,
                             SMatrix& placeInstForceMatrixX,
                             Eigen::VectorXf& fixedInstForceVecX,
                             Eigen::VectorXf& instLocVecX,
                             SMatrix& placeInstForceMatrixY,
                             Eigen::VectorXf& fixedInstForceVecY,
                             Eigen::VectorXf& instLocVecY,
                             utl::Logger* logger,
                             int threads)
{
  omp_set_num_threads(threads);

  ResidualError residual_error;
  BiCGSTAB<SMatrix, IdentityPreconditioner> solver;
  solver.setMaxIterations(maxSolverIter);

  // With no pin or locked cell holding a position in an axis, the
  // right-hand side is zero and BiCGSTAB would put every cell at 0. Keep the
  // current positions instead.

  if (fixedInstForceVecX.squaredNorm() == 0) {
    residual_error.x = 0;
  } else {
    solver.compute(placeInstForceMatrixX);
    instLocVecX = solver.solveWithGuess(fixedInstForceVecX, instLocVecX);
    if (solver.info() == Eigen::NoConvergence
        || solver.info() == Eigen::Success) {
      residual_error.x = solver.error();
    } else {
      residual_error.x = std::numeric_limits<float>::quiet_NaN();
    }
  }

  if (fixedInstForceVecY.squaredNorm() == 0) {
    residual_error.y = 0;
  } else {
    solver.compute(placeInstForceMatrixY);
    instLocVecY = solver.solveWithGuess(fixedInstForceVecY, instLocVecY);
    if (solver.info() == Eigen::NoConvergence
        || solver.info() == Eigen::Success) {
      residual_error.y = solver.error();
    } else {
      residual_error.y = std::numeric_limits<float>::quiet_NaN();
    }
  }

  return residual_error;
}
}  // namespace gpl
