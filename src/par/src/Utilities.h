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
#pragma once
#include <map>
#include <string>
#include <vector>

#ifdef LOAD_CPLEX
// for ILP solver in CPLEX
#include "ilcplex/cplex.h"
#include "ilcplex/ilocplex.h"
#endif

namespace par {

// Matrix is a two-dimensional vectors
template <typename T>
using Matrix = std::vector<std::vector<T>>;

struct Rect
{
  // all the values are in db unit
  int lx = 0;
  int ly = 0;
  int ux = 0;
  int uy = 0;

  Rect(int lx_, int ly_, int ux_, int uy_) : lx(lx_), ly(ly_), ux(ux_), uy(uy_)
  {
  }

  // check if the Rect is valid
  bool IsValid() const { return ux > lx && uy > ly; }

  // reset the fence
  void Reset()
  {
    lx = 0;
    ly = 0;
    ux = 0;
    uy = 0;
  }
};

// Define the type for vertices
enum VertexType
{
  COMB_STD_CELL,  // combinational standard cell
  SEQ_STD_CELL,   // sequential standard cell
  MACRO,          // hard macros
  PORT            // IO ports
};

std::string GetVectorString(const std::vector<float>& vec);

// Split a string based on deliminator : empty space and ","
std::vector<std::string> SplitLine(const std::string& line);

// Add right vector to left vector
void Accumulate(std::vector<float>& a, const std::vector<float>& b);

// weighted sum
std::vector<float> WeightedSum(const std::vector<float>& a,
                               float a_factor,
                               const std::vector<float>& b,
                               float b_factor);

// divide the vector
std::vector<float> DivideFactor(const std::vector<float>& a, float factor);

// divide the vectors element by element
std::vector<float> DivideVectorElebyEle(const std::vector<float>& emb,
                                        const std::vector<float>& factor);

// multiplty the vector
std::vector<float> MultiplyFactor(const std::vector<float>& a, float factor);

// operation for two vectors +, -, *,  ==, <
std::vector<float> operator+(const std::vector<float>& a,
                             const std::vector<float>& b);

std::vector<float> operator*(const std::vector<float>& a, float factor);

std::vector<float> operator-(const std::vector<float>& a,
                             const std::vector<float>& b);

std::vector<float> operator*(const std::vector<float>& a,
                             const std::vector<float>& b);

bool operator<(const std::vector<float>& a, const std::vector<float>& b);

bool operator<=(const Matrix<float>& a, const Matrix<float>& b);

bool operator==(const std::vector<float>& a, const std::vector<float>& b);

// Basic functions for a vector
std::vector<float> abs(const std::vector<float>& a);

float norm2(const std::vector<float>& a);

float norm2(const std::vector<float>& a, const std::vector<float>& factor);

// ILP-based Partitioning Instance
// Call ILP Solver to partition the design
bool ILPPartitionInst(
    int num_parts,
    int vertex_weight_dimension,
    std::vector<int>& solution,
    const std::map<int, int>& fixed_vertices,     // vertex_id, block_id
    const Matrix<int>& hyperedges,                // hyperedges
    const std::vector<float>& hyperedge_weights,  // one-dimensional
    const Matrix<float>& vertex_weights,          // two-dimensional
    const Matrix<float>& upper_block_balance,
    const Matrix<float>& lower_block_balance);

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
    const Matrix<float>& lower_block_balance);
#endif

}  // namespace par
