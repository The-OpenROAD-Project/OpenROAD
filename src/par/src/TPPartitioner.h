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
//
// This file define the Paritioner class
// Paritioner is an operator class, which takes a hypergraph
// and perform partitioning based on multiple options.
// The Partitioner class implements following types of partitioning algorithm:
// 1) Greedy partitioning
// 2) Priority-queue FM-based partitioning
// 3) ILP-based partitioning
// 4) Iterative k-way partitioning
// 5) Direct k-way partitioning
// Currently we only consider the balance constraints on vertices
// TO DOs:
//    1) Handle the constraints on pins
//
//
#include <set>

#include "TPHypergraph.h"
#include "TPRefiner.h"
#include "Utilities.h"
#include "utl/Logger.h"

namespace par {

// Define the partitioning algorithm
enum class PartitionType
{
  INIT_RANDOM,
  INIT_VILE,
  INIT_DIRECT_ILP,
  INIT_DIRECT_WARM_ILP,
};

using TP_partition_token = std::pair<float, std::vector<std::vector<float>>>;
template <typename T>
using matrix = std::vector<std::vector<T>>;

class TPpartitioner
{
 public:
  TPpartitioner() = default;
  TPpartitioner(int num_parts,
                const std::vector<float>& e_wt_factors,
                float path_wt_factor,
                float snaking_wt_factor,
                float early_stop_ratio,
                int max_num_fm_pass,
                int seed,
                TP_two_way_refining_ptr tritonpart_two_way_refiner,
                utl::Logger* logger)
      : tritonpart_two_way_refiner_(tritonpart_two_way_refiner),
        num_parts_(num_parts),
        e_wt_factors_(e_wt_factors),
        path_wt_factor_(path_wt_factor),
        snaking_wt_factor_(snaking_wt_factor),
        early_stop_ratio_(early_stop_ratio),
        max_num_fm_pass_(max_num_fm_pass),
        seed_(seed),
        logger_(logger)
  {
  }
  TPpartitioner(int num_parts,
                const std::vector<float>& e_wt_factors,
                float path_wt_factor,
                float snaking_wt_factor,
                float early_stop_ratio,
                int max_num_fm_pass,
                int seed,
                TP_k_way_refining_ptr tritonpart_k_way_refiner,
                utl::Logger* logger)
      : tritonpart_k_way_refiner_(tritonpart_k_way_refiner),
        num_parts_(num_parts),
        e_wt_factors_(e_wt_factors),
        path_wt_factor_(path_wt_factor),
        snaking_wt_factor_(snaking_wt_factor),
        early_stop_ratio_(early_stop_ratio),
        max_num_fm_pass_(max_num_fm_pass),
        seed_(seed),
        logger_(logger)
  {
  }

  TPpartitioner(const TPpartitioner&) = default;
  TPpartitioner(TPpartitioner&&) = default;
  TPpartitioner& operator=(const TPpartitioner&) = default;
  TPpartitioner& operator=(TPpartitioner&&) = default;
  ~TPpartitioner() = default;
  TP_partition_token GoldenEvaluator(const HGraph hgraph,
                                     std::vector<int>& solution,
                                     bool print_flag = true);
  void TimingCutsEvaluator(const HGraph hgraph, std::vector<int>& solution);
  void Partition(const HGraph hgraph,
                 const matrix<float>& max_block_balance,
                 std::vector<int>& solutione);
  inline void SetPartitionerSeed(int seed) { seed_ = seed; }
  inline int GetPartitionerSeed() const { return seed_; }
  inline void SetPartitionerChoice(const PartitionType choice)
  {
    partitioner_choice_ = choice;
  }
  inline PartitionType GetPartitionerChoice() const
  {
    return partitioner_choice_;
  }
  float CalculatePathCost(int path_id,
                          const HGraph hgraph,
                          const std::vector<int>& solution,
                          int v = -1,
                          int to_pid = -1);

 private:
  matrix<float> GetBlockBalance(const HGraph hgraph,
                                std::vector<int>& solution);
  void InitPartVileTwoWay(const HGraph hgraph,
                          const matrix<float>& max_block_balance,
                          std::vector<int>& solution);
  void InitPartVileKWay(const HGraph hgraph,
                        const matrix<float>& max_block_balance,
                        std::vector<int>& solution);
  void RandomPart(const HGraph graph,
                  const matrix<float>& max_block_balance,
                  std::vector<int>& solution);
  void OptimalPartCplex(const HGraph graph,
                        const matrix<float>& max_block_balance,
                        std::vector<int>& solution);
  void OptimalPartCplexWarmStart(const HGraph graph,
                                 const matrix<float>& max_block_balance,
                                 std::vector<int>& solution);
  TP_two_way_refining_ptr tritonpart_two_way_refiner_ = nullptr;
  TP_k_way_refining_ptr tritonpart_k_way_refiner_ = nullptr;
  int num_parts_;
  std::vector<float> e_wt_factors_;
  float path_wt_factor_;
  float snaking_wt_factor_;
  float early_stop_ratio_;
  float max_num_moves_;
  int max_num_fm_pass_;
  int seed_;
  PartitionType partitioner_choice_ = PartitionType::INIT_RANDOM;
  utl::Logger* logger_ = nullptr;
};

using TP_partitioning_ptr = std::shared_ptr<TPpartitioner>;

}  // namespace par
