/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#include "solver.h"

namespace gpl {

#ifdef ENABLE_GPU
ResidualError cudaSparseSolve(int iter,
                              SMatrix& placeInstForceMatrixX,
                              Eigen::VectorXf& fixedInstForceVecX,
                              Eigen::VectorXf& instLocVecX,
                              SMatrix& placeInstForceMatrixY,
                              Eigen::VectorXf& fixedInstForceVecY,
                              Eigen::VectorXf& instLocVecY,
                              utl::Logger* logger)
{
  ResidualError error;
  GpuSolver SP1(placeInstForceMatrixX, fixedInstForceVecX, logger);
  SP1.cusolverCal(instLocVecX);
  error.x = SP1.error();

  GpuSolver SP2(placeInstForceMatrixY, fixedInstForceVecY, logger);
  SP2.cusolverCal(instLocVecY);
  error.y = SP1.error();
  return error;
}
#endif
ResidualError cpuSparseSolve(int maxSolverIter,
                             int iter,
                             SMatrix& placeInstForceMatrixX,
                             Eigen::VectorXf& fixedInstForceVecX,
                             Eigen::VectorXf& instLocVecX,
                             SMatrix& placeInstForceMatrixY,
                             Eigen::VectorXf& fixedInstForceVecY,
                             Eigen::VectorXf& instLocVecY,
                             utl::Logger* logger)
{
  ResidualError error;
  BiCGSTAB<SMatrix, IdentityPreconditioner> solver;
  solver.setMaxIterations(maxSolverIter);
  solver.compute(placeInstForceMatrixX);
  instLocVecX = solver.solveWithGuess(fixedInstForceVecX, instLocVecX);
  error.x = solver.error();

  solver.compute(placeInstForceMatrixY);
  instLocVecY = solver.solveWithGuess(fixedInstForceVecY, instLocVecY);
  error.y = solver.error();
  return error;
}
}  // namespace gpl