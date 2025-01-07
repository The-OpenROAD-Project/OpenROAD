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
#pragma once

// This file define the Paritioner class
// Paritioner is an operator class, which takes a hypergraph
// and perform partitioning based on multiple options.
// The Partitioner class implements following types of partitioning algorithm:
// 1) Random Paritioning (INIT_RANDOM)
// 2) VILE Partitioning (INIT_VILE) : randomly pick one vertex into each block,
// then put remaining vertices into the first block 3) ILP-based Partitioning
// (INIT_DIRECT_ILP)

#include <memory>
#include <vector>

#include "Evaluator.h"
#include "Hypergraph.h"
#include "Utilities.h"
#include "utl/Logger.h"

namespace par {

// Define the partitioning algorithm
enum class PartitionType
{
  INIT_RANDOM,
  INIT_RANDOM_VILE,
  INIT_VILE,
  INIT_DIRECT_ILP
};

class Partitioner;
using PartitioningPtr = std::shared_ptr<Partitioner>;

class Partitioner
{
 public:
  // Note that please do NOT set ilp_accelerator_factor here
  Partitioner(int num_parts,
              int seed,
              const EvaluatorPtr& evaluator,
              utl::Logger* logger);

  // The main function of Partitioning
  void Partition(const HGraphPtr& hgraph,
                 const Matrix<float>& upper_block_balance,
                 const Matrix<float>& lower_block_balance,
                 std::vector<int>& solution,
                 PartitionType partitioner_choice) const;

  void SetRandomSeed(int seed) { seed_ = seed; }

  void EnableIlpAcceleration(float acceleration_factor);

  void DisableIlpAcceleration();

 private:
  // random partitioning
  // Different to other random partitioning,
  // we enable two modes of random partitioning.
  // If vile_mode == false, we try to generate balanced random partitioning
  // If vile_mode == true,  we try to generate unbalanced random partitioning
  void RandomPart(const HGraphPtr& hgraph,
                  const Matrix<float>& upper_block_balance,
                  const Matrix<float>& lower_block_balance,
                  std::vector<int>& solution,
                  bool vile_mode = false) const;

  // ILP-based partitioning
  void ILPPart(const HGraphPtr& hgraph,
               const Matrix<float>& upper_block_balance,
               const Matrix<float>& lower_block_balance,
               std::vector<int>& solution) const;

  // Vile partitioning
  void VilePart(const HGraphPtr& hgraph, std::vector<int>& solution) const;

  const int num_parts_ = 2;
  float ilp_accelerator_factor_
      = 1.0;  // In the default mode, we do not use ilp acceleration
              // If the ilp acceleration is enabled, we only use
              // top ilp_accelerator_factor hyperedges. Range: 0, 1
  int seed_ = 0;
  EvaluatorPtr evaluator_ = nullptr;  // evaluator
  utl::Logger* logger_ = nullptr;
};

}  // namespace par
