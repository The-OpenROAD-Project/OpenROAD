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

std::vector<float> TPHypergraph::GetTotalHyperedgeWeights() const
{
  std::vector<float> total_weight(hyperedge_dimensions_, 0.0);
  for (auto& weight : hyperedge_weights_) {
    Accumulate(total_weight, weight);
  }
  return total_weight;
}

// Get the vertex balance constraint
std::vector<std::vector<float> > TPHypergraph::GetVertexBalance(int num_parts,
                                                               float ub_factor) const
{
  std::vector<float> vertex_balance = GetTotalVertexWeights();
  vertex_balance = MultiplyFactor(
      vertex_balance, ubfactor * 0.01 + 1.0 / static_cast<float>(num_parts));
  return std::vector<std::vector<float> >(num_parts, vertex_balance);
}

// write the hypergraph out in general hmetis format
// current two files:  hypergraph, dimension
void TPHypergraph::WriteHypergraph(std::string hypergraph) const
{
  std::string hypergraph_file = hypergraph + ".hgr";
  std::ofstream file_output;
  file_output.open(hypergraph_file);
  file_output << num_hyperedges_ << "  " << num_vertices_ << " 11" << std::endl;
  // write hyperedge weight and hyperedge first
  for (int e = 0; e < num_hyperedges_; e++) {
    for (auto weight : hyperedge_weights_[e]) {
      file_output << weight << " ";
    }
    for (auto idx = eptr_[e]; idx < eptr_[e + 1]; idx++) {
      file_output << eind_[idx] + 1 << " ";
    }
    file_output << std::endl;
  }
  // write vertex weight
  for (const auto& v_weight : vertex_weights_) {
    for (auto weight : v_weight) {
      file_output << weight << " ";
    }
    file_output << std::endl;
  }
  file_output.close();

  // write out the dimension
  std::string dimension_file = hypergraph + ".dim";
  file_output.open(dimension_file);
  file_output << "vertex_dimensions = " << vertex_dimensions_ << std::endl;
  file_output << "hyperedge_dimensions = " << hyperedge_dimensions_
              << std::endl;
  file_output.close();
}

}  // namespace par
