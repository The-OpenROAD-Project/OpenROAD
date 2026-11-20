// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "solver.h"

#include <limits>

#include "omp.h"

namespace gpl {
namespace {

// Eigen symbols come from solver.h's "Eigen/*" headers; Eigen has no
// IWYU-mappable headers for misc-include-cleaner.
// NOLINTBEGIN(misc-include-cleaner)
float solveAxis(int maxSolverIter,
                const SMatrix& placeInstForceMatrix,
                const Eigen::VectorXf& fixedInstForceVec,
                Eigen::VectorXf& instLocVec)
{
  if (fixedInstForceVec.squaredNorm() == 0) {
    return 0;
  }
  Eigen::SparseMatrix<double> a = placeInstForceMatrix.cast<double>();
  const Eigen::VectorXd b = fixedInstForceVec.cast<double>();
  const Eigen::VectorXd x = instLocVec.cast<double>();
  const Eigen::VectorXd rhs = b - a * x;
  if (rhs.squaredNorm() == 0) {
    return 0;
  }
  for (int i = 0; i < a.rows(); ++i) {
    a.coeffRef(i, i) += 1e-6;
  }
  Eigen::ConjugateGradient<Eigen::SparseMatrix<double>,
                           Eigen::Lower | Eigen::Upper>
      cg(a);
  const Eigen::VectorXd dx
      = cg.setMaxIterations(maxSolverIter)
            .setTolerance(Eigen::NumTraits<float>::epsilon())
            .solve(rhs);
  if (cg.info() != Eigen::Success && cg.info() != Eigen::NoConvergence) {
    return std::numeric_limits<float>::quiet_NaN();
  }
  instLocVec = (x + dx).cast<float>();
  return cg.error() * (rhs.norm() / b.norm());
}
// NOLINTEND(misc-include-cleaner)

}  // namespace

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
  return {
      .x = solveAxis(maxSolverIter,
                     placeInstForceMatrixX,
                     fixedInstForceVecX,
                     instLocVecX),
      .y = solveAxis(maxSolverIter,
                     placeInstForceMatrixY,
                     fixedInstForceVecY,
                     instLocVecY),
  };
}
}  // namespace gpl
