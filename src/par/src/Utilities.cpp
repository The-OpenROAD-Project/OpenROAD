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

#include <algorithm>
#include <cassert>
#include <chrono>
#include <climits>
#include <cmath>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <queue>
#include <random>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <ortools/base/logging.h>
#include <ortools/linear_solver/linear_solver.h>
#include <ortools/linear_solver/linear_solver.pb.h>
#include <ortools/base/commandlineflags.h>

namespace par {

using operations_research::MPConstraint;
using operations_research::MPObjective;
using operations_research::MPSolver;
using operations_research::MPVariable;


std::string GetVectorString(const std::vector<float>& vec)
{
  std::string line;
  for (auto value : vec)
    line += std::to_string(static_cast<int>(value)) + " ";
  return line;
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
  for (auto& value : result)
    value /= factor;
  return result;
}

// multiply the vector
std::vector<float> MultiplyFactor(const std::vector<float>& a,
                                  const float factor)
{
  std::vector<float> result = a;
  for (auto& value : result)
    value *= factor;
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
  for (auto value : a)
    result.push_back(value * factor);
  return result;
}


bool operator<(const std::vector<float>& a, const std::vector<float>& b)
{
  assert(a.size() == b.size());
  auto a_iter = a.begin();
  auto b_iter = b.begin();
  while (a_iter != a.end()) {
    if ((*a_iter++) >= (*b_iter++))
      return false;
  }
  return true;
}

bool operator==(const std::vector<float>& a, const std::vector<float>& b)
{
  return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
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
bool ILPPartitionInst(int num_parts,
                      int vertex_weight_dimension,
                      std::vector<int>& solution,
                      const std::map<int, int>& fixed_vertices, // vertex_id, block_id
                      const matrix<int>& hyperedges,
                      const std::vector<float>& hyperedge_weights,  // one-dimensional
                      const matrix<float>& vertex_weights, // two-dimensional
                      const matrix<float>& max_block_balance)
{
  const int num_vertices = static_cast<int>(vertex_weights.size());
  const int num_hyperedges = static_cast<int>(hyperedge_weights.size());
  if (num_vertices <= 0 || num_hyperedges <= 0 || num_parts <= 1 || vertex_weight_dimension <= 0) {
    return false; // no need to call ILP-based partitioning
  }

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
  // handle different types of constraints
  // balance constraint
  for (int i = 0; i < num_parts; i++) {
    // check the balance for each block
    for (int j = 0; j < vertex_weight_dimension; j++) {
      // check balance for each dimension
      MPConstraint* constraint
        = solver->MakeRowConstraint(0.0, max_block_balance[i][j], "");
      for (int v = 0; v < num_vertices; v++) {
        const float vwt = vertex_weights[v][j];
        constraint->SetCoefficient(x[i][v], vwt); 
      }
    }
  }
  // fixed vertices constraints
  for (const auto& [vertex_id, block_id] : fixed_vertices) {
    MPConstraint* constraint = solver->MakeRowConstraint(1, 1, "");
    constraint->SetCoefficient(x[block_id][vertex_id], 1);
  }
  // each vertex can only belong to one part
  for (int v = 0;  v < num_vertices; v++) {
    MPConstraint* constraint = solver->MakeRowConstraint(1, 1, "");
    for (int i = 0; i < num_parts; i++) {
      constraint->SetCoefficient(x[i][v], 1);
    }
  }
  const double infinity = solver->infinity(); // single-side constraints
  // Hyperedge constraint: x - y >= 0
  // y[i][e] represents the hyperedge e is fully within the block i
  for (int e = 0; e < num_hyperedges; e++) {
    const std::vector<float>& hyperedge = hyperedges[i];
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
  MPObjective* const obj_expr = solver->MutableObjective();
  for (int e = 0; e < num_hyperedges; e++) {
    for (int i = 0; i < num_parts; i++) {
      obj_expr->SetCoefficient(y[i][e], hyperedge_weights[e]);
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
  } else {
    return false;
  }
}

}  // namespace par
