///////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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
// High-level description
// This file includes the basic utility functions for operations
///////////////////////////////////////////////////////////////////////////////
#include "Utilities.h"

#include <ortools/linear_solver/linear_solver.h>

#include <algorithm>
#include <cassert>
#include <climits>
#include <cmath>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

namespace par {

using operations_research::MPConstraint;
using operations_research::MPObjective;
using operations_research::MPSolver;
using operations_research::MPVariable;

std::string GetVectorString(const std::vector<float>& vec)
{
  std::string line;
  for (auto value : vec) {
    line += std::to_string(value) + " ";
  }
  return line;
}

// Convert Tcl list to vector

// char_match:  determine if the char is part of deliminators
bool CharMatch(char c, const std::string& delim)
{
  auto it = delim.begin();
  while (it != delim.end()) {
    if ((*it) == c) {
      return true;
    }
    ++it;
  }
  return false;
}

// find the next position for deliminator char
std::string::const_iterator FindDelim(std::string::const_iterator start,
                                      std::string::const_iterator end,
                                      const std::string& delim)
{
  while (start != end && !CharMatch(*start, delim)) {
    start++;
  }
  return start;
}

// find the next position for non deliminator char
std::string::const_iterator FindNotDelim(std::string::const_iterator start,
                                         std::string::const_iterator end,
                                         const std::string& delim)
{
  while (start != end && CharMatch(*start, delim)) {
    start++;
  }
  return start;
}

// Split a string based on deliminator : empty space and ","
std::vector<std::string> SplitLine(const std::string& line)
{
  std::vector<std::string> items;
  std::string deliminators(", ");  // empty space ,
  auto start = line.cbegin();
  while (start != line.cend()) {
    start = FindNotDelim(start, line.cend(), deliminators);
    auto end = FindDelim(start, line.cend(), deliminators);
    if (start != line.cend()) {
      items.emplace_back(start, end);
      start = end;
    }
  }
  return items;
}

// Add right vector to left vector
void Accumulate(std::vector<float>& a, const std::vector<float>& b)
{
  assert(a.size() == b.size());
  std::transform(a.begin(), a.end(), b.begin(), a.begin(), std::plus<float>());
}

// weighted sum
std::vector<float> WeightedSum(const std::vector<float>& a,
                               const float a_factor,
                               const std::vector<float>& b,
                               const float b_factor)
{
  assert(a.size() == b.size());
  std::vector<float> result;
  result.reserve(a.size());
  auto a_iter = a.begin();
  auto b_iter = b.begin();
  while (a_iter != a.end()) {
    result.push_back(((*a_iter++) * a_factor + (*b_iter++) * b_factor)
                     / (a_factor + b_factor));
  }
  return result;
}

// divide the vector
std::vector<float> DivideFactor(const std::vector<float>& a, const float factor)
{
  std::vector<float> result = a;
  for (auto& value : result) {
    value /= factor;
  }
  return result;
}

// multiply the vector
std::vector<float> MultiplyFactor(const std::vector<float>& a,
                                  const float factor)
{
  std::vector<float> result = a;
  for (auto& value : result) {
    value *= factor;
  }
  return result;
}

// divide the vectors element by element
std::vector<float> DivideVectorElebyEle(const std::vector<float>& emb,
                                        const std::vector<float>& factor)
{
  std::vector<float> result;
  auto emb_iter = emb.begin();
  auto factor_iter = factor.begin();
  while (emb_iter != emb.end() && factor_iter != factor.end()) {
    if ((*factor_iter) != 0.0) {
      result.push_back((*emb_iter) / (*factor_iter));
    } else {
      result.push_back(*emb_iter);
    }
    emb_iter++;
    factor_iter++;
  }
  return result;
}

// operation for two vectors +, -, *,  ==, <
std::vector<float> operator+(const std::vector<float>& a,
                             const std::vector<float>& b)
{
  assert(a.size() == b.size());
  std::vector<float> result;
  result.reserve(a.size());
  std::transform(a.begin(),
                 a.end(),
                 b.begin(),
                 std::back_inserter(result),
                 std::plus<float>());
  return result;
}

std::vector<float> operator-(const std::vector<float>& a,
                             const std::vector<float>& b)
{
  assert(a.size() == b.size());
  std::vector<float> result;
  result.reserve(a.size());
  std::transform(a.begin(),
                 a.end(),
                 b.begin(),
                 std::back_inserter(result),
                 std::minus<float>());
  return result;
}

std::vector<float> operator*(const std::vector<float>& a,
                             const std::vector<float>& b)
{
  assert(a.size() == b.size());
  std::vector<float> result;
  result.reserve(a.size());
  std::transform(a.begin(),
                 a.end(),
                 b.begin(),
                 std::back_inserter(result),
                 std::multiplies<float>());
  return result;
}

std::vector<float> operator*(const std::vector<float>& a, const float factor)
{
  std::vector<float> result;
  result.reserve(a.size());
  for (auto value : a) {
    result.push_back(value * factor);
  }
  return result;
}

bool operator<(const std::vector<float>& a, const std::vector<float>& b)
{
  assert(a.size() == b.size());
  auto a_iter = a.begin();
  auto b_iter = b.begin();
  while (a_iter != a.end()) {
    if ((*a_iter++) >= (*b_iter++)) {
      return false;
    }
  }
  return true;
}

bool operator==(const std::vector<float>& a, const std::vector<float>& b)
{
  return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

bool operator<=(const Matrix<float>& a, const Matrix<float>& b)
{
  const int num_dim
      = std::min(static_cast<int>(a.size()), static_cast<int>(b.size()));
  for (int dim = 0; dim < num_dim; dim++) {
    if ((a[dim] < b[dim]) || (a[dim] == b[dim])) {
      continue;
    }
    return false;
  }
  return true;
}

// Basic functions for a vector
std::vector<float> abs(const std::vector<float>& a)
{
  std::vector<float> result;
  result.reserve(a.size());
  std::transform(a.begin(),
                 a.end(),
                 std::back_inserter(result),
                 static_cast<float (*)(float)>(&std::abs));
  return result;
}

float norm2(const std::vector<float>& a)
{
  float result{0};
  result = std::inner_product(a.begin(), a.end(), a.begin(), result);
  return std::sqrt(result);
}

float norm2(const std::vector<float>& a, const std::vector<float>& factor)
{
  float result{0};
  assert(a.size() <= factor.size());
  auto a_iter = a.begin();
  auto factor_iter = factor.begin();
  while (a_iter != a.end()) {
    result += (*a_iter) * (*a_iter) * std::abs(*factor_iter);
    a_iter++;
    factor_iter++;
  }
  return std::sqrt(result);
}

// ILP-based Partitioning Instance
// Call ILP Solver to partition the design
// We use Google OR-Tools as our ILP solver
// Based on our experiments, even there are multiple solutions
// available, the google OR-Tool will always return the same value
bool ILPPartitionInst(
    int num_parts,
    int vertex_weight_dimension,
    std::vector<int>& solution,
    const std::map<int, int>& fixed_vertices,  // vertex_id, block_id
    const Matrix<int>& hyperedges,
    const std::vector<float>& hyperedge_weights,  // one-dimensional
    const Matrix<float>& vertex_weights,          // two-dimensional
    const Matrix<float>& upper_block_balance,
    const Matrix<float>& lower_block_balance)
{
  const int num_vertices = static_cast<int>(vertex_weights.size());
  const int num_hyperedges = static_cast<int>(hyperedge_weights.size());

  if (num_vertices <= 0 || num_hyperedges <= 0 || num_parts <= 1
      || vertex_weight_dimension <= 0) {
    return false;  // no need to call ILP-based partitioning
  }

#ifdef LOAD_CPLEX
  // call CPLEX to solve the ILP issue
  return OptimalPartCplex(num_parts,
                          vertex_weight_dimension,
                          solution,
                          fixed_vertices,
                          hyperedges,
                          hyperedge_weights,
                          vertex_weights,
                          upper_block_balance,
                          lower_block_balance);
#endif

  // reset variable
  solution.clear();
  solution.resize(num_vertices);
  std::fill(solution.begin(), solution.end(), -1);

  // Google OR-Tools Implementation
  std::unique_ptr<MPSolver> solver(MPSolver::CreateSolver("SCIP"));

  // Define constraints
  // For each vertex, define a variable x
  // For each hyperedge, define a variable y
  std::vector<std::vector<const MPVariable*>> x(
      num_parts, std::vector<const MPVariable*>(num_vertices));
  std::vector<std::vector<const MPVariable*>> y(
      num_parts, std::vector<const MPVariable*>(num_hyperedges));
  // initialize variables
  for (auto& x_v_vector : x) {
    for (auto& x_v : x_v_vector) {
      x_v = solver->MakeIntVar(
          0.0, 1.0, "");  // represent whether the vertex is within block
    }
  }
  for (auto& y_e_vector : y) {
    for (auto& y_e : y_e_vector) {
      y_e = solver->MakeIntVar(
          0.0, 1.0, "");  // represent whether the hyperedge is within block
    }
  }
  const double infinity = solver->infinity();  // single-side constraints
  // handle different types of constraints
  // balance constraint
  for (int i = 0; i < num_parts; i++) {
    // check the balance for each block
    for (int j = 0; j < vertex_weight_dimension; j++) {
      // check balance for each dimension
      MPConstraint* upper_constraint
          = solver->MakeRowConstraint(0.0, upper_block_balance[i][j], "");
      MPConstraint* lower_constraint
          = solver->MakeRowConstraint(lower_block_balance[i][j], infinity, "");
      for (int v = 0; v < num_vertices; v++) {
        const float vwt = vertex_weights[v][j];
        upper_constraint->SetCoefficient(x[i][v], vwt);
        lower_constraint->SetCoefficient(x[i][v], vwt);
      }
    }
  }
  // fixed vertices constraints
  for (const auto& [vertex_id, block_id] : fixed_vertices) {
    MPConstraint* constraint = solver->MakeRowConstraint(1, 1, "");
    constraint->SetCoefficient(x[block_id][vertex_id], 1);
  }
  // each vertex can only belong to one part
  for (int v = 0; v < num_vertices; v++) {
    MPConstraint* constraint = solver->MakeRowConstraint(1, 1, "");
    for (int i = 0; i < num_parts; i++) {
      constraint->SetCoefficient(x[i][v], 1);
    }
  }

  // Hyperedge constraint: x - y >= 0
  // y[i][e] represents the hyperedge e is fully within the block i
  for (int e = 0; e < num_hyperedges; e++) {
    const std::vector<int>& hyperedge = hyperedges[e];
    for (const auto& v : hyperedge) {
      for (int i = 0; i < num_parts; i++) {
        MPConstraint* constraint = solver->MakeRowConstraint(0, infinity, "");
        constraint->SetCoefficient(x[i][v], 1);
        constraint->SetCoefficient(y[i][e], -1);
      }
    }
  }
  // Maximize the number of hyperedges fully within one block
  // -> minimize the cutsize
  // add a penalty term to ensure the solution is unique
  float cost_step = 1.0;
  for (auto& weight : hyperedge_weights) {
    cost_step = std::min(cost_step, weight);
  }
  cost_step = cost_step / num_parts / (num_vertices * (num_vertices + 1) / 2.0);
  MPObjective* const obj_expr = solver->MutableObjective();
  for (int e = 0; e < num_hyperedges; e++) {
    for (int i = 0; i < num_parts; i++) {
      obj_expr->SetCoefficient(y[i][e], hyperedge_weights[e]);
    }
  }
  for (int v = 0; v < num_vertices; v++) {
    for (int block_id = 1; block_id < num_parts; block_id++) {
      obj_expr->SetCoefficient(x[block_id][v], cost_step * block_id);
    }
  }
  obj_expr->SetMaximization();

  // Solve the ILP Problem
  const MPSolver::ResultStatus result_status = solver->Solve();

  // Check that the problem has an optimal solution.
  if (result_status == MPSolver::OPTIMAL) {
    // update the solution
    // all the fixed vertices has been encoded into fixed vertices constraints
    // so we do not handle fixed vertices here
    for (int v = 0; v < num_vertices; v++) {
      for (int i = 0; i < num_parts; i++) {
        if (x[i][v]->solution_value() == 1.0) {
          solution[v] = i;
          break;
        }
      }
    }
    return true;
  }
  return false;
}

// Call CPLEX to solve the ILP Based Partitioning
#ifdef LOAD_CPLEX
bool OptimalPartCplex(
    int num_parts,
    int vertex_weight_dimension,
    std::vector<int>& solution,
    const std::map<int, int>& fixed_vertices,     // vertex_id, block_id
    const Matrix<int>& hyperedges,                // hyperedges
    const std::vector<float>& hyperedge_weights,  // one-dimensional
    const Matrix<float>& vertex_weights,          // two-dimensional
    const Matrix<float>& upper_block_balance,
    const Matrix<float>& lower_block_balance)
{
  const int num_vertices = static_cast<int>(vertex_weights.size());
  const int num_hyperedges = static_cast<int>(hyperedge_weights.size());

  // set the environment
  IloEnv myenv;
  IloModel mymodel(myenv);
  IloArray<IloNumVarArray> var_x(myenv, num_parts);
  IloArray<IloNumVarArray> var_y(myenv, num_parts);
  for (int block_id = 0; block_id < num_parts; ++block_id) {
    var_x[block_id] = IloNumVarArray(myenv, num_vertices, 0, 1, ILOINT);
    var_y[block_id] = IloNumVarArray(myenv, num_hyperedges, 0, 1, ILOINT);
  }
  // define constraints
  // balance constraint
  // check each dimension
  for (int i = 0; i < vertex_weight_dimension; ++i) {
    // allowed balance for each dimension
    for (int block_id = 0; block_id < num_parts; ++block_id) {
      IloExpr balance_expr(myenv);
      for (int v = 0; v < num_vertices; ++v) {
        balance_expr += vertex_weights[v][i] * var_x[block_id][v];
      }  // finish traversing vertices
      mymodel.add(IloRange(myenv,
                           lower_block_balance[block_id][i],
                           balance_expr,
                           upper_block_balance[block_id][i]));
      balance_expr.end();
    }  // finish traversing blocks
  }    // finish dimension check
  // Fixed vertices constraint
  for (const auto& [vertex_id, block_id] : fixed_vertices) {
    mymodel.add(var_x[block_id][vertex_id] == 1);
  }
  // each vertex can only belong to one part
  for (int v = 0; v < num_vertices; ++v) {
    IloExpr vertex_expr(myenv);
    for (int block_id = 0; block_id < num_parts; ++block_id) {
      vertex_expr += var_x[block_id][v];
    }
    mymodel.add(vertex_expr == 1);
    vertex_expr.end();
  }
  // Hyperedge constraint
  for (int e = 0; e < num_hyperedges; e++) {
    const std::vector<int>& hyperedge = hyperedges[e];
    for (const auto& v : hyperedge) {
      for (int block_id = 0; block_id < num_parts; block_id++) {
        mymodel.add(var_y[block_id][e] <= var_x[block_id][v]);
      }
    }
  }

  // add a penalty term to ensure the solution is unique
  float cost_step = 1.0;
  for (auto& weight : hyperedge_weights) {
    cost_step = std::min(cost_step, weight);
  }
  cost_step = cost_step / num_parts / (num_vertices * (num_vertices + 1) / 2.0);
  // Maximize cutsize objective
  IloExpr obj_expr(myenv);  // empty expression
  for (int e = 0; e < num_hyperedges; e++) {
    for (int block_id = 0; block_id < num_parts; block_id++) {
      obj_expr += hyperedge_weights[e] * var_y[block_id][e];
    }
  }
  for (int v = 0; v < num_vertices; v++) {
    for (int block_id = 1; block_id < num_parts; block_id++) {
      obj_expr += cost_step * var_x[block_id][v] * block_id;
    }
  }

  mymodel.add(IloMaximize(myenv, obj_expr));  // adding minimization objective
  obj_expr.end();                             // clear memory
  // Model Solution
  IloCplex mycplex(myenv);
  // Add warm-start if applicable
  if (static_cast<int>(solution.size()) == num_vertices) {
    IloNumVarArray startVarX(myenv);
    IloNumArray startValX(
        myenv);  // We just need to provide the partial solution
    for (int v = 0; v < num_vertices; v++) {
      startVarX.add(var_x[solution[v]][v]);
      startValX.add(1);
    }
    mycplex.extract(mymodel);
    mycplex.setOut(myenv.getNullStream());
    mycplex.addMIPStart(startVarX, startValX);
    // mycplex.addMIPStart(startVarX, startValX, IloCplex::MIPStartAuto,
    // "secondMIPStart"); The IloCplex::Param::MIP::Limits::Solutions parameter
    // in CPLEX is used to set a limit on the maximum number of feasible
    // solutions that the solver should find before stopping. This can be useful
    // in cases where you want to terminate the search early once a certain
    // number of solutions have been found.
    // mycplex.setParam(IloCplex::Param::MIP::Limits::Solutions, 10);
    startVarX.end();
    startValX.end();
  }

  // Solve the problem
  mycplex.solve();
  IloBool feasible = mycplex.solve();
  bool feasible_flag = false;
  if (mycplex.getStatus() == IloAlgorithm::Status::Optimal) {
    // update the solution
    // all the fixed vertices has been encoded into fixed vertices constraints
    // so we do not handle fixed vertices here
    for (int v = 0; v < num_vertices; v++) {
      for (int block_id = 0; block_id < num_parts; block_id++) {
        if (mycplex.getValue(var_x[block_id][v]) == 1.0) {
          solution[v] = block_id;
          break;
        }
      }
    }
    // some solution may invalid due to the limitation of ILP solver
    for (auto& value : solution) {
      value = (value == -1) ? 0 : value;
    }
    feasible_flag = true;
  }

  // closing the model
  mycplex.clear();
  myenv.end();
  return feasible_flag;
}
#endif

}  // namespace par
