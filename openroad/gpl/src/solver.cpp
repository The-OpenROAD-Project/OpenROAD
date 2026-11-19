// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "solver.h"

#include <cmath>
#include <limits>

#include "Eigen/Core"
#include "Eigen/IterativeLinearSolvers"
#include "Eigen/SparseCore"
#include "omp.h"

namespace gpl {
namespace {

bool isAnchored(const SMatrix& mat)
{
  for (int row = 0; row < mat.outerSize(); ++row) {
    double diag = 0;
    double off_diag = 0;
    for (SMatrix::InnerIterator it(mat, row); it; ++it) {
      (it.col() == row ? diag : off_diag) += std::abs(it.value());
    }
    if (diag - off_diag > 1e-4 * diag) {
      return true;
    }
  }
  return false;
}

float solveAxis(int maxSolverIter,
                SMatrix& placeInstForceMatrix,
                Eigen::VectorXf& fixedInstForceVec,
                Eigen::VectorXf& instLocVec)
{
  // With no pin or locked cell holding a position in an axis, an unconnected
  // design has a zero right-hand side and a connected design has a singular
  // Laplacian whose nullspace is uniform translation. Solve for displacement
  // around the current positions with weak regularization so cells arrange by
  // pin offsets without drifting or diverging.
  if (fixedInstForceVec.squaredNorm() == 0) {
    return 0;
  }
  if (!isAnchored(placeInstForceMatrix)) {
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
        cg;
    cg.setMaxIterations(maxSolverIter);
    cg.setTolerance(Eigen::NumTraits<float>::epsilon());
    cg.compute(a);
    const Eigen::VectorXd dx = cg.solve(rhs);
    if (cg.info() != Eigen::NoConvergence && cg.info() != Eigen::Success) {
      return std::numeric_limits<float>::quiet_NaN();
    }
    instLocVec = (x + dx).cast<float>();
    return (rhs - a * dx).norm() / b.norm();
  }

  BiCGSTAB<SMatrix, IdentityPreconditioner> solver;
  solver.setMaxIterations(maxSolverIter);
  solver.compute(placeInstForceMatrix);
  instLocVec = solver.solveWithGuess(fixedInstForceVec, instLocVec);
  if (solver.info() == Eigen::NoConvergence
      || solver.info() == Eigen::Success) {
    return solver.error();
  }
  return std::numeric_limits<float>::quiet_NaN();
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
