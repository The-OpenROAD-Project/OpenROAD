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

#include <memory>
#include <vector>

#include "Evaluator.h"
#include "Hypergraph.h"
#include "Refiner.h"
#include "Utilities.h"

namespace par {

class IlpRefine;
using IlpRefinerPtr = std::shared_ptr<IlpRefine>;

// ------------------------------------------------------------------------------
// K-way ILP Based Refinement
// ILP Based Refinement is usually very slow and cannot handle path related
// cost.  Please try to avoid using ILP-Based Refinement for K-way partitioning
// and path-related partitioning.  But ILP Based Refinement is good for 2=way
// min-cut problem
// --------------------------------------------------------------------------------
class IlpRefine : public Refiner
{
 public:
  using Refiner::Refiner;

 private:
  // In each pass, we only move the boundary vertices
  // here we pass block_balance and net_degrees as reference
  // because we only move a few vertices during each pass
  // i.e., block_balance and net_degs will not change too much
  // so we precompute the block_balance and net_degs
  // the return value is the gain improvement
  float Pass(const HGraphPtr& hgraph,
             const Matrix<float>& upper_block_balance,
             const Matrix<float>& lower_block_balance,
             Matrix<float>& block_balance,        // the current block balance
             Matrix<int>& net_degs,               // the current net degree
             std::vector<float>& cur_paths_cost,  // the current path cost
             Partitions& solution,
             std::vector<bool>& visited_vertices_flag) override;
};

}  // namespace par
