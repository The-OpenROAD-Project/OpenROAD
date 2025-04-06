// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

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
