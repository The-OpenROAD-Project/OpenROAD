#ifndef __SOLVER__
#define __SOLVER__

#include <Eigen/IterativeLinearSolvers>
#include <Eigen/SparseCore>
#include <memory>

#ifdef ENABLE_GPU
#include "gpuSolver.h"
#endif
#include "graphics.h"
#include "odb/db.h"
#include "placerBase.h"
#include "plot.h"
#include "utl/Logger.h"

namespace utl {
class Logger;
}

namespace gpl {

struct err{
  float errorX; // The relative residual error for X
  float errorY; // The relative residual error for Y
};

using Eigen::BiCGSTAB;
using Eigen::IdentityPreconditioner;
using utl::GPL;

typedef Eigen::SparseMatrix<float, Eigen::RowMajor> SMatrix;

#ifdef ENABLE_GPU
 err cudaSparseSolve(int iter,
                     SMatrix& placeInstForceMatrixX,
                     Eigen::VectorXf& fixedInstForceVecX,
                     Eigen::VectorXf& instLocVecX,
                     SMatrix& placeInstForceMatrixY,
                     Eigen::VectorXf& fixedInstForceVecY,
                     Eigen::VectorXf& instLocVecY,
                     utl::Logger* logger)
{
  err error;
  GpuSolver SP1(placeInstForceMatrixX, fixedInstForceVecX, logger);
  SP1.cusolverCal(instLocVecX);
  error.errorX = SP1.error_cal();

  GpuSolver SP2(placeInstForceMatrixY, fixedInstForceVecY, logger);
  SP2.cusolverCal(instLocVecY);
  error.errorY = SP1.error_cal();
  return error;
}
#endif

err cpuSparseSolve(int maxSolverIter,
                    int iter,
                    SMatrix& placeInstForceMatrixX,
                    Eigen::VectorXf& fixedInstForceVecX,
                    Eigen::VectorXf& instLocVecX,
                    SMatrix& placeInstForceMatrixY,
                    Eigen::VectorXf& fixedInstForceVecY,
                    Eigen::VectorXf& instLocVecY,
                     utl::Logger* logger)
{
  err error;
  BiCGSTAB<SMatrix, IdentityPreconditioner> solver;
  solver.setMaxIterations(maxSolverIter);
  solver.compute(placeInstForceMatrixX);
  instLocVecX = solver.solveWithGuess(fixedInstForceVecX, instLocVecX);
  error.errorX = solver.error();

  solver.compute(placeInstForceMatrixY);
  instLocVecY = solver.solveWithGuess(fixedInstForceVecY, instLocVecY);
  error.errorY = solver.error();
  return error;
}
}  
#endif