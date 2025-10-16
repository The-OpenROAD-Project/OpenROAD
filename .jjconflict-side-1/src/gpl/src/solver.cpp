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

  solver.compute(placeInstForceMatrixX);
  instLocVecX = solver.solveWithGuess(fixedInstForceVecX, instLocVecX);
  if (solver.info() == Eigen::NoConvergence
      || solver.info() == Eigen::Success) {
    residual_error.x = solver.error();
  } else {
    residual_error.x = std::numeric_limits<float>::quiet_NaN();
  }

  solver.compute(placeInstForceMatrixY);
  instLocVecY = solver.solveWithGuess(fixedInstForceVecY, instLocVecY);
  if (solver.info() == Eigen::NoConvergence
      || solver.info() == Eigen::Success) {
    residual_error.y = solver.error();
  } else {
    residual_error.y = std::numeric_limits<float>::quiet_NaN();
  }

  return residual_error;
}
}  // namespace gpl
