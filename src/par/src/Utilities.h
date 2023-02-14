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
#include <memory>
#include <queue>
#include <random>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace par {

class TimingCuts
{
 public:
  TimingCuts() = default;
  TimingCuts(const int total_critical_paths_cut,
             const float average_critical_paths_cut,
             int worst_cut,
             int total_paths)
      : total_critical_paths_cut_(total_critical_paths_cut),
        average_critical_paths_cut_(average_critical_paths_cut),
        worst_cut_(worst_cut),
        total_paths_(total_paths)
  {
  }
  TimingCuts(const TimingCuts&) = default;
  TimingCuts(TimingCuts&&) = default;
  TimingCuts& operator=(const TimingCuts&) = default;
  TimingCuts& operator=(TimingCuts&&) = default;
  int GetTotalCriticalPathsCut() const { return total_critical_paths_cut_; }
  float GetAvereageCriticalPathsCut() const
  {
    return average_critical_paths_cut_;
  }
  int GetWorstCut() const { return worst_cut_; }
  int GetTotalPaths() const { return total_paths_; }

 private:
  int total_critical_paths_cut_;
  float average_critical_paths_cut_;
  int worst_cut_;
  int total_paths_;
};

// Function for write solution
void WriteSolution(const char* solution_file, std::vector<int>& solution);

// Analyze a timing paths file and a partition to find timing related metrics
void AnalyzeTimingOfPartition(std::vector<std::vector<int>>& paths,
                              const char* solution_file = "");
std::shared_ptr<TimingCuts> AnalyzeTimingOfPartition(
    std::vector<std::vector<int>>& paths,
    std::vector<int>& solution);

std::string GetVectorString(std::vector<float> vec);

// Add right vector to left vector
void Accumulate(std::vector<float>& a, const std::vector<float>& b);

// weighted sum
std::vector<float> WeightedSum(const std::vector<float> a,
                               float a_factor,
                               const std::vector<float> b,
                               float b_factor);

// divide the vector
std::vector<float> DivideFactor(const std::vector<float> a, float factor);

// multiplty the vector
std::vector<float> MultiplyFactor(const std::vector<float> a, float factor);

// operation for two vectors +, -, *,  ==, <
std::vector<float> operator+(const std::vector<float> a,
                             const std::vector<float> b);

std::vector<float> operator*(const std::vector<float> a, float factor);

std::vector<float> operator-(const std::vector<float> a,
                             const std::vector<float> b);

std::vector<float> operator*(const std::vector<float> a,
                             const std::vector<float> b);

int PartitionWithMinWt(std::vector<std::vector<float>>& area);

int PartitionWithMaxWt(std::vector<std::vector<float>>& area);

bool operator<(const std::vector<float> a, const std::vector<float> b);

bool operator==(const std::vector<float> a, const std::vector<float> b);

// Basic functions for a vector
std::vector<float> abs(const std::vector<float> a);

float norm2(const std::vector<float> a);

float norm2(std::vector<float> a, std::vector<float> factor);
}  // namespace par
