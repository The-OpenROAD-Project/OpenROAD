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

#include "TPHypergraph.h"

#include <string>
#include <iostream>
#include "Utilities.h"
#include "utl/Logger.h"

using utl::PAR;

namespace par {

std::vector<float> TPHypergraph::GetTotalVertexWeights() const
{
  std::vector<float> total_weight(vertex_dimensions_, 0.0);
  for (auto& weight : vertex_weights_) {
    total_weight = total_weight + weight;
  }
  return total_weight;
}

// Get the vertex balance constraint
std::vector<std::vector<float> > TPHypergraph::GetVertexBalance(int num_parts,
                                                               float ub_factor) const
{
  std::vector<float> vertex_balance = GetTotalVertexWeights();
  vertex_balance = MultiplyFactor(
      vertex_balance, ub_factor * 0.01 + 1.0 / static_cast<float>(num_parts));
  return std::vector<std::vector<float> >(num_parts, vertex_balance);
}

// Get the vertex balance constraint (upper bound)
std::vector<std::vector<float> > TPHypergraph::GetUpperVertexBalance(int num_parts,
                                                                     float ub_factor) const
{
  std::vector<float> vertex_balance = GetTotalVertexWeights();
  vertex_balance = MultiplyFactor(
      vertex_balance, ub_factor * 0.01 + 1.0 / static_cast<float>(num_parts));
  return std::vector<std::vector<float> >(num_parts, vertex_balance);
}

// Get the vertex balance constraint (lower bound)
std::vector<std::vector<float> > TPHypergraph::GetLowerVertexBalance(int num_parts,
                                                               float ub_factor) const
{
  std::vector<float> vertex_balance = GetTotalVertexWeights();
  ub_factor = std::max(-1.0 * ub_factor * 0.01 + 1.0 / static_cast<float>(num_parts), 0.0);
  vertex_balance = MultiplyFactor(vertex_balance, ub_factor);
  return std::vector<std::vector<float> >(num_parts, vertex_balance);
}


}  // namespace par
