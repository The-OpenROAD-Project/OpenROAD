// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "solver.h"

#include <limits>

#include "Eigen/Core"
#include "Eigen/IterativeLinearSolvers"
#include "Eigen/SparseCore"
#include "omp.h"

namespace gpl {
namespace {

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
  Eigen::SparseMatrix<double> reg(a.rows(), a.cols());
  reg.setIdentity();
  a += 1e-6 * reg;
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
  return {solveAxis(maxSolverIter,
                    placeInstForceMatrixX,
                    fixedInstForceVecX,
                    instLocVecX),
          solveAxis(maxSolverIter,
                    placeInstForceMatrixY,
                    fixedInstForceVecY,
                    instLocVecY)};
}
}  // namespace gpl
