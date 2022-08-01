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

using Eigen::BiCGSTAB;
using Eigen::IdentityPreconditioner;
using utl::GPL;

using namespace std;
typedef Eigen::SparseMatrix<float, Eigen::RowMajor> SMatrix;

#ifdef ENABLE_GPU
 void cudaSparseSolve(int iter,
                     SMatrix& placeInstForceMatrixX,
                     Eigen::VectorXf& fixedInstForceVecX,
                     Eigen::VectorXf& instLocVecX,
                     SMatrix& placeInstForceMatrixY,
                     Eigen::VectorXf& fixedInstForceVecY,
                     Eigen::VectorXf& instLocVecY,
                     float errorX,
                     float errorY,
                     utl::Logger* logger,
                     int hpwl)
{
  class GpuSolver SP1(placeInstForceMatrixX, fixedInstForceVecX, logger);
  SP1.cusolverCal(instLocVecX);
  errorX = SP1.error_cal();

  class GpuSolver SP2(placeInstForceMatrixY, fixedInstForceVecY, logger);
  SP2.cusolverCal(instLocVecY);
  errorY = SP2.error_cal();
  logger->report("[InitialPlace]  Iter: {} CG residual: {:0.8f} HPWL: {}",
                 iter,
                 std::max(errorX, errorY),
                 hpwl);
}
#endif

void cpuSparseSolve(int maxSolverIter,
                    int iter,
                    SMatrix& placeInstForceMatrixX,
                    Eigen::VectorXf& fixedInstForceVecX,
                    Eigen::VectorXf& instLocVecX,
                    SMatrix& placeInstForceMatrixY,
                    Eigen::VectorXf& fixedInstForceVecY,
                    Eigen::VectorXf& instLocVecY,
                    float errorX,
                    float errorY,
                    utl::Logger* logger,
                    int hpwl)
{
  BiCGSTAB<SMatrix, IdentityPreconditioner> solver;
  solver.setMaxIterations(maxSolverIter);
  solver.compute(placeInstForceMatrixX);
  instLocVecX = solver.solveWithGuess(fixedInstForceVecX, instLocVecX);
  errorX = solver.error();

  solver.compute(placeInstForceMatrixY);
  instLocVecY = solver.solveWithGuess(fixedInstForceVecY, instLocVecY);
  errorY = solver.error();
  logger->report("[InitialPlace]  Iter: {} CG residual: {:0.8f} HPWL: {}",
                 iter,
                 std::max(errorX, errorY),
                 hpwl);
}
}  
#endif