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

#include "Utilities.h"
#include "utl/Logger.h"

using utl::PAR;

namespace par {

// Basic constructors
TPHypergraph::TPHypergraph(
    int vertex_dimensions,
    int hyperedge_dimensions,
    const std::vector<std::vector<int>>& hyperedges,
    const std::vector<std::vector<float>>& vertex_weights,
    const std::vector<std::vector<float>>& hyperedge_weights,
    const std::vector<int>& fixed_attr,
    const std::vector<int>& community_attr,
    int placement_dimensions,
    const std::vector<std::vector<float>>& placement_attr,
    const std::vector<std::vector<int>>& paths,
    const std::vector<float>& timing_attr,
    utl::Logger* logger)
{
  num_vertices_ = static_cast<int>(vertex_weights.size());
  num_hyperedges_ = static_cast<int>(hyperedge_weights.size());
  vertex_dimensions_ = vertex_dimensions;
  hyperedge_dimensions_ = hyperedge_dimensions;
  vertex_weights_ = vertex_weights;
  hyperedge_weights_ = hyperedge_weights;
  // hyperedges
  eind_.clear();
  eptr_.clear();
  eptr_.push_back(static_cast<int>(eind_.size()));
  for (const auto& hyperedge : hyperedges) {
    eind_.insert(eind_.end(), hyperedge.begin(), hyperedge.end());
    eptr_.push_back(static_cast<int>(eind_.size()));
  }
  // vertices
  std::vector<std::vector<int>> vertices(num_vertices_);
  for (int i = 0; i < num_hyperedges_; i++)
    for (auto v : hyperedges[i])
      vertices[v].push_back(i);  // i is the hyperedge id
  vind_.clear();
  vptr_.clear();
  vptr_.push_back(static_cast<int>(vind_.size()));
  for (auto& vertex : vertices) {
    vind_.insert(vind_.end(), vertex.begin(), vertex.end());
    vptr_.push_back(static_cast<int>(vind_.size()));
  }
  // fixed vertices
  fixed_vertex_flag_ = (fixed_attr.size() > 0);
  fixed_attr_ = fixed_attr;
  // community structure
  community_flag_ = (community_attr.size() > 0);
  community_attr_ = community_attr;
  // placement information
  placement_flag_ = (placement_dimensions > 0 && placement_attr.size() > 0);
  placement_dimensions_ = placement_dimensions;
  placement_attr_ = placement_attr;

  // timing flag
  num_timing_paths_ = static_cast<int>(timing_attr.size());
  timing_attr_ = timing_attr;
  // timing paths
  // each timing path is a sequence of vertices: vind_p, vptr_p
  vind_p_.clear();
  vptr_p_.clear();
  vptr_p_.push_back(static_cast<int>(vind_p_.size()));
  for (auto path : paths) {
    vind_p_.insert(vind_p_.end(), path.begin(), path.end());
    vptr_p_.push_back(static_cast<int>(vind_p_.size()));
  }
  // check the timing paths related to each vertex
  vertices.clear();
  vertices.resize(num_vertices_);
  const int num_paths = static_cast<int>(paths.size());
  for (int p = 0; p < num_paths; p++) {
    auto path = paths[p];
    for (auto v : path) {
      // std::cout << "[debug] vertex v " << v << std::endl;
      vertices[v].push_back(p);
    }
  }
  pind_v_.clear();
  pptr_v_.clear();
  pptr_v_.push_back(static_cast<int>(pind_v_.size()));
  for (auto& vertex : vertices) {
    pind_v_.insert(pind_v_.end(), vertex.begin(), vertex.end());
    pptr_v_.push_back(static_cast<int>(pind_v_.size()));
  }
  // logger
  logger_ = logger;
}

TPHypergraph::TPHypergraph(
    int vertex_dimensions,
    int hyperedge_dimensions,
    const std::vector<std::vector<int>>& hyperedges,
    const std::vector<std::vector<float>>& vertex_weights,
    const std::vector<std::vector<float>>& hyperedge_weights,
    const std::vector<std::vector<float>>& nonscaled_hyperedge_weights,
    const std::vector<int>& fixed_attr,
    const std::vector<int>& community_attr,
    int placement_dimensions,
    const std::vector<std::vector<float>>& placement_attr,
    const std::vector<std::vector<int>>& paths,
    const std::vector<float>& timing_attr,
    utl::Logger* logger)
{
  num_vertices_ = static_cast<int>(vertex_weights.size());
  num_hyperedges_ = static_cast<int>(hyperedge_weights.size());
  vertex_dimensions_ = vertex_dimensions;
  hyperedge_dimensions_ = hyperedge_dimensions;
  vertex_weights_ = vertex_weights;
  hyperedge_weights_ = hyperedge_weights;
  nonscaled_hyperedge_weights_ = nonscaled_hyperedge_weights;
  // hyperedges
  eind_.clear();
  eptr_.clear();
  eptr_.push_back(static_cast<int>(eind_.size()));
  for (auto hyperedge : hyperedges) {
    eind_.insert(eind_.end(), hyperedge.begin(), hyperedge.end());
    eptr_.push_back(static_cast<int>(eind_.size()));
  }
  // vertices
  std::vector<std::vector<int>> vertices(num_vertices_);
  for (int i = 0; i < num_hyperedges_; i++)
    for (auto v : hyperedges[i])
      vertices[v].push_back(i);  // i is the hyperedge id
  vind_.clear();
  vptr_.clear();
  vptr_.push_back(static_cast<int>(vind_.size()));
  for (auto& vertex : vertices) {
    vind_.insert(vind_.end(), vertex.begin(), vertex.end());
    vptr_.push_back(static_cast<int>(vind_.size()));
  }
  // fixed vertices
  fixed_vertex_flag_ = (fixed_attr.size() > 0);
  fixed_attr_ = fixed_attr;
  // community structure
  community_flag_ = (community_attr.size() > 0);
  community_attr_ = community_attr;
  // placement information
  placement_flag_ = (placement_dimensions > 0 && placement_attr.size() > 0);
  placement_dimensions_ = placement_dimensions;
  placement_attr_ = placement_attr;

  // timing flag
  num_timing_paths_ = static_cast<int>(timing_attr.size());
  timing_attr_ = timing_attr;
  // timing paths
  // each timing path is a sequence of vertices: vind_p, vptr_p
  vind_p_.clear();
  vptr_p_.clear();
  vptr_p_.push_back(static_cast<int>(vind_p_.size()));
  for (const auto& path : paths) {
    vind_p_.insert(vind_p_.end(), path.begin(), path.end());
    vptr_p_.push_back(static_cast<int>(vind_p_.size()));
  }
  // check the timing paths related to each vertex
  vertices.clear();
  vertices.resize(num_vertices_);
  const int num_paths = static_cast<int>(paths.size());
  for (int p = 0; p < num_paths; p++) {
    auto path = paths[p];
    for (auto v : path) {
      // std::cout << "[debug] vertex v " << v << std::endl;
      vertices[v].push_back(p);
    }
  }
  pind_v_.clear();
  pptr_v_.clear();
  pptr_v_.push_back(static_cast<int>(pind_v_.size()));
  for (auto& vertex : vertices) {
    pind_v_.insert(pind_v_.end(), vertex.begin(), vertex.end());
    pptr_v_.push_back(static_cast<int>(pind_v_.size()));
  }
  // logger
  logger_ = logger;
}

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
  for (auto& weight : hyperedge_weights_)
    Accumulate(total_weight, weight);
  return total_weight;
}

// Get the vertex balance constraint
std::vector<std::vector<float>> TPHypergraph::GetVertexBalance(int num_parts,
                                                               float ubfactor)
{
  std::vector<float> vertex_balance = GetTotalVertexWeights();
  vertex_balance = MultiplyFactor(
      vertex_balance, ubfactor * 0.01 + 1.0 / static_cast<float>(num_parts));
  return std::vector<std::vector<float>>(num_parts, vertex_balance);
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
    for (auto weight : hyperedge_weights_[e])
      file_output << weight << " ";
    for (auto idx = eptr_[e]; idx < eptr_[e + 1]; idx++)
      file_output << eind_[idx] + 1 << " ";
    file_output << std::endl;
  }
  // write vertex weight
  for (const auto& v_weight : vertex_weights_) {
    for (auto weight : v_weight)
      file_output << weight << " ";
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

void TPHypergraph::WriteReducedHypergraph(
    const std::string reduced_file,
    const std::vector<float> vertex_w_factor,
    const std::vector<float> hyperedge_w_factor) const
{
  std::ofstream file_output;
  file_output.open(reduced_file);
  file_output << num_hyperedges_ << "  " << num_vertices_ << " 11" << std::endl;
  // write hyperedge weight and hyperedge first
  for (int e = 0; e < num_hyperedges_; e++) {
    file_output << std::inner_product(hyperedge_weights_[e].begin(),
                                      hyperedge_weights_[e].end(),
                                      hyperedge_w_factor.begin(),
                                      0.0)

                << "  ";
    for (auto idx = eptr_[e]; idx < eptr_[e + 1]; idx++)
      file_output << eind_[idx] + 1 << " ";
    file_output << std::endl;
  }
  // write vertex weight
  for (auto v_weight : vertex_weights_) {
    file_output << std::inner_product(
        v_weight.begin(), v_weight.end(), vertex_w_factor.begin(), 0.0)
                << std::endl;
  }
  file_output.close();
}

std::vector<float> TPHypergraph::ComputeAlgebraicWights() const
{
  std::vector<float> algebraic_weight(num_hyperedges_, 1.0);
  std::vector<float> weight_factor(100, 1.0);
  if (placement_flag_ == false || placement_dimensions_ <= 0)
    return algebraic_weight;
  const float omega = 0.75;  // relexation factor
  const int num_iters = 20;  // number of iterations
  // based on the convergence requirement, rescale each placement attribute to
  // [0.5, 0.5]
  std::vector<std::vector<float>> v_embed(
      num_vertices_, std::vector<float>(placement_dimensions_, 0));

  std::vector<float> v_weight;
  // Function for scaling elements to [-0.5, 0.5]
  auto Scale = [&](std::vector<std::vector<float>>& placement,
                   int num_elements) {
    for (int dim = 0; dim < placement_dimensions_; dim++) {
      float min_value = std::numeric_limits<float>::max();
      float max_value = -1 * min_value;
      for (int i = 0; i < num_elements; i++) {
        max_value
            = placement[i][dim] > max_value ? placement[i][dim] : max_value;
        min_value
            = placement[i][dim] < min_value ? placement[i][dim] : min_value;
      }
      for (int i = 0; i < num_vertices_; i++) {
        placement[i][dim]
            = (placement[i][dim] - min_value) / (max_value - min_value) - 0.5;
      }
    }
  };

  // update the vertices first
  for (int dim = 0; dim < placement_dimensions_; dim++) {
    float min_value = std::numeric_limits<float>::max();
    float max_value = -1 * min_value;
    for (int i = 0; i < num_vertices_; i++) {
      max_value = placement_attr_[i][dim] > max_value ? placement_attr_[i][dim]
                                                      : max_value;
      min_value = placement_attr_[i][dim] < min_value ? placement_attr_[i][dim]
                                                      : min_value;
    }
    for (int i = 0; i < num_vertices_; i++) {
      v_embed[i][dim]
          = (placement_attr_[i][dim] - min_value) / (max_value - min_value)
            - 0.5;
    }
  }
  for (int i = 0; i < num_vertices_; i++) {
    v_weight.push_back(std::inner_product(vertex_weights_[i].begin(),
                                          vertex_weights_[i].end(),
                                          weight_factor.begin(),
                                          0.0));
  }

  // update the hyperedges
  std::vector<std::vector<float>> e_embed(
      num_hyperedges_, std::vector<float>(placement_dimensions_, 0));
  std::vector<float> e_weight;
  for (int e = 0; e < num_hyperedges_; e++) {
    const int e_size = eptr_[e + 1] - eptr_[e];
    e_weight.push_back(std::inner_product(hyperedge_weights_[e].begin(),
                                          hyperedge_weights_[e].end(),
                                          weight_factor.begin(),
                                          0.0)
                       / e_size);
    for (auto idx = eptr_[e]; idx < eptr_[e + 1]; idx++) {
      const int v = eind_[idx];
      std::vector<float> v_embed_w = MultiplyFactor(v_embed[v], 1.0 / e_size);
      e_embed[e] = e_embed[e] + v_embed_w;
    }
  }
  // Iterations
  for (auto iter = 0; iter < num_iters; iter++) {
    // perform embedding updating
    std::vector<std::vector<float>> e_embed_new(
        num_hyperedges_, std::vector<float>(placement_dimensions_, 0));
    std::vector<std::vector<float>> v_embed_new(
        num_vertices_, std::vector<float>(placement_dimensions_, 0));
    // update vertex
    for (int v = 0; v < num_vertices_; v++) {
      std::vector<float> temp_embed(placement_dimensions_, 0.0);
      float sum_e_weight = 0.0;
      for (auto idx = vptr_[v]; idx < vptr_[v + 1]; idx++) {
        const int e = vind_[idx];
        sum_e_weight += e_weight[e];
        temp_embed = temp_embed + MultiplyFactor(e_embed[e], e_weight[e]);
      }
      temp_embed = DivideFactor(temp_embed, sum_e_weight);
      v_embed_new[v] = MultiplyFactor(temp_embed, omega)
                       + MultiplyFactor(v_embed[v], 1.0 - omega);
    }
    // update hyperedge
    for (int e = 0; e < num_hyperedges_; e++) {
      std::vector<float> temp_embed(placement_dimensions_, 0.0);
      float sum_v_weight = 0.0;
      for (auto idx = eptr_[e]; idx < eptr_[e + 1]; idx++) {
        const int v = eind_[idx];
        sum_v_weight += v_weight[v];
        temp_embed = temp_embed + MultiplyFactor(v_embed[v], v_weight[v]);
      }
      temp_embed = DivideFactor(temp_embed, sum_v_weight);
      e_embed_new[e] = MultiplyFactor(temp_embed, omega)
                       + MultiplyFactor(e_embed[e], 1.0 - omega);
    }
    // rescale all the embedding to [-0.5, 0.5]
    Scale(v_embed_new, num_vertices_);
    Scale(e_embed_new, num_hyperedges_);
    v_embed = v_embed_new;
    e_embed = e_embed_new;
  }

  for (auto e = 0; e < num_hyperedges_; e++) {
    const int start_idx = eptr_[e];
    const int end_idx = eptr_[e + 1];
    std::vector<float> lower_bound(placement_dimensions_,
                                   std::numeric_limits<float>::max());
    std::vector<float> upper_bound(placement_dimensions_,
                                   -std::numeric_limits<float>::max());
    for (auto idx = start_idx; idx < end_idx; idx++) {
      const int v = eind_[idx];
      for (int i = 0; i < placement_dimensions_; i++) {
        lower_bound[i] = std::min(lower_bound[i], v_embed[v][i]);
        upper_bound[i] = std::max(upper_bound[i], v_embed[v][i]);
      }
    }
    // update the distance
    std::vector<float> dist;
    for (int i = 0; i < placement_dimensions_; i++)
      dist.push_back(upper_bound[i] - lower_bound[i]);
    algebraic_weight[e] = 1.0 / *std::max_element(dist.begin(), dist.end());
  }

  /*
  std::ofstream file_output;
  std::string file_name = std::string("algebraic_weight.txt.")
                         + std::to_string(num_hyperedges_);
  file_output.open(file_name);
  for (auto weight : algebraic_weight)
    file_output << weight << std::endl;
  file_output.close();
  */

  return algebraic_weight;
}

}  // namespace par
