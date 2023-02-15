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

namespace par {

// Write solution
void WriteSolution(const char* solution_file, std::vector<int>& solution)
{
  std::ofstream solution_file_output;
  solution_file_output.open(solution_file);
  for (auto part_id : solution)
    solution_file_output << part_id << std::endl;
  solution_file_output.close();
}

void AnalyzeTimingOfPartition(std::vector<std::vector<int>>& paths,
                              const char* solution_file)
{
}

std::shared_ptr<TimingCuts> AnalyzeTimingOfPartition(
    std::vector<std::vector<int>>& paths,
    std::vector<int>& solution)
{
  int total_cuts = 0;
  int worst_cut = -std::numeric_limits<int>::max();
  int total_paths_cut = 0;
  for (auto& path : paths) {
    std::vector<int> block_path;
    for (auto& v : path) {
      int block_id = solution[v];
      if (block_path.empty() == true || block_path.back() != block_id) {
        block_path.push_back(block_id);
      }
    }
    int cut = block_path.size() - 1;
    if (cut > 0) {
      ++total_paths_cut;
    }
    total_cuts += cut;
    if (cut > worst_cut) {
      worst_cut = cut;
    }
  }

  return std::make_shared<TimingCuts>(
      total_paths_cut,
      static_cast<float>(total_cuts) / static_cast<float>(paths.size()),
      worst_cut,
      paths.size());
}

std::string GetVectorString(std::vector<float> vec)
{
  std::string line = "";
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
std::vector<float> WeightedSum(const std::vector<float> a,
                               float a_factor,
                               const std::vector<float> b,
                               float b_factor)
{
  assert(a.size() == b.size());
  std::vector<float> result;
  result.reserve(a.size());
  auto a_iter = a.begin();
  auto b_iter = b.begin();
  while (a_iter != a.end())
    result.push_back(((*a_iter++) * a_factor + (*b_iter++) * b_factor)
                     / (a_factor + b_factor));
  return result;
}

// divide the vector
std::vector<float> DivideFactor(const std::vector<float> a, float factor)
{
  std::vector<float> result = a;
  for (auto& value : result)
    value = value / factor;
  return result;
}

// multiply the vector
std::vector<float> MultiplyFactor(const std::vector<float> a, float factor)
{
  std::vector<float> result = a;
  for (auto& value : result)
    value = value * factor;
  return result;
}

// operation for two vectors +, -, *,  ==, <
std::vector<float> operator+(const std::vector<float> a,
                             const std::vector<float> b)
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

std::vector<float> operator-(const std::vector<float> a,
                             const std::vector<float> b)
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

std::vector<float> operator*(const std::vector<float> a,
                             const std::vector<float> b)
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

std::vector<float> operator*(const std::vector<float> a, float factor)
{
  std::vector<float> result;
  result.reserve(a.size());
  for (auto value : a)
    result.push_back(value * factor);
  return result;
}

int PartitionWithMinWt(std::vector<std::vector<float>>& area)
{
  int min_part = 0;
  std::vector<float> min_area(area[0].size(),
                              std::numeric_limits<float>::max());
  for (int i = 0; i < area.size(); ++i) {
    if (area[i] < min_area) {
      min_area = area[i];
      min_part = i;
    }
  }
  return min_part;
}

int PartitionWithMaxWt(std::vector<std::vector<float>>& area)
{
  int max_part = 0;
  std::vector<float> max_area(area[0].size(),
                              -std::numeric_limits<float>::max());
  for (int i = 0; i < area.size(); ++i) {
    if (area[i] > max_area) {
      max_area = area[i];
      max_part = i;
    }
  }
  return max_part;
}

bool operator<(const std::vector<float> a, const std::vector<float> b)
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

bool operator==(const std::vector<float> a, const std::vector<float> b)
{
  return a.size() == a.size() && std::equal(a.begin(), a.end(), a.begin());
}

// Basic functions for a vector
std::vector<float> abs(const std::vector<float> a)
{
  std::vector<float> result;
  result.reserve(a.size());
  std::transform(a.begin(),
                 a.end(),
                 std::back_inserter(result),
                 static_cast<float (*)(float)>(&std::abs));
  return result;
}

float norm2(const std::vector<float> a)
{
  float result{0};
  result = std::inner_product(a.begin(), a.end(), a.begin(), result);
  return std::sqrt(result);
}

float norm2(std::vector<float> a, std::vector<float> factor)
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

}  // namespace par
