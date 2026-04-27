// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

///////////////////////////////////////////////////////////////////////////////
// High-level description
// This file includes the basic utility functions for operations
///////////////////////////////////////////////////////////////////////////////
#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iterator>
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

  Rect(int lx, int ly, int ux, int uy) : lx(lx), ly(ly), ux(ux), uy(uy) {}

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
  kCombStdCell,  // combinational standard cell
  kSeqStdCell,   // sequential standard cell
  kMacro,        // hard macros
  kPort          // IO ports
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

// Stable comparator for ODB objects ordered first by name, then by id. Uses
// getConstName() to avoid std::string allocation. The id tiebreaker ensures a
// strict weak ordering when two objects share a name.
template <typename T>
bool compareDbObjectsByNameAndId(T* lhs, T* rhs)
{
  const int name_cmp = std::strcmp(lhs->getConstName(), rhs->getConstName());
  if (name_cmp != 0) {
    return name_cmp < 0;
  }
  return lhs->getId() < rhs->getId();
}

// A fully specified Fisher-Yates shuffle so Bazel and CMake do not depend on
// implementation-defined std::shuffle behavior across standard libraries.
template <typename RandomIt, typename URBG>
void DeterministicShuffle(RandomIt first, RandomIt last, URBG& gen)
{
  using diff_t = typename std::iterator_traits<RandomIt>::difference_type;
  const diff_t count = last - first;
  if (count <= 1) {
    return;
  }

  constexpr std::uint64_t range = static_cast<std::uint64_t>(URBG::max())
                                  - static_cast<std::uint64_t>(URBG::min())
                                  + 1ULL;

  for (diff_t i = count - 1; i > 0; --i) {
    const std::uint64_t bound = static_cast<std::uint64_t>(i) + 1ULL;
    const std::uint64_t limit = range - (range % bound);
    std::uint64_t value = 0;
    do {
      value = static_cast<std::uint64_t>(gen())
              - static_cast<std::uint64_t>(URBG::min());
    } while (value >= limit);
    const diff_t j = static_cast<diff_t>(value % bound);
    std::iter_swap(first + i, first + j);
  }
}

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
