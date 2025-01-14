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

#include "Hypergraph.h"

#include <algorithm>
#include <limits>
#include <set>
#include <vector>

#include "Utilities.h"
#include "utl/Logger.h"

namespace par {

Hypergraph::Hypergraph(
    const int vertex_dimensions,
    const int hyperedge_dimensions,
    const int placement_dimensions,
    const std::vector<std::vector<int>>& hyperedges,
    const std::vector<std::vector<float>>& vertex_weights,
    const std::vector<std::vector<float>>& hyperedge_weights,
    // fixed vertices
    const std::vector<int>& fixed_attr,  // the block id of fixed vertices.
    // community attribute
    const std::vector<int>& community_attr,
    // placement information
    const std::vector<std::vector<float>>& placement_attr,
    utl::Logger* logger)
    : num_vertices_(static_cast<int>(vertex_weights.size())),
      num_hyperedges_(static_cast<int>(hyperedge_weights.size())),
      vertex_dimensions_(vertex_dimensions),
      hyperedge_dimensions_(hyperedge_dimensions),
      vertex_weights_(vertex_weights),
      hyperedge_weights_(hyperedge_weights)
{
  // add hyperedge
  // hyperedges: each hyperedge is a set of vertices
  eptr_.push_back(0);
  for (const auto& hyperedge : hyperedges) {
    eind_.insert(eind_.end(), hyperedge.begin(), hyperedge.end());
    eptr_.push_back(static_cast<int>(eind_.size()));
  }

  // add vertex
  // create vertices from hyperedges
  std::vector<std::vector<int>> vertices(num_vertices_);
  for (int e = 0; e < num_hyperedges_; e++) {
    for (auto v : hyperedges[e]) {
      vertices[v].push_back(e);  // e is the hyperedge id
    }
  }

  vptr_.push_back(0);
  for (const auto& vertex : vertices) {
    vind_.insert(vind_.end(), vertex.begin(), vertex.end());
    vptr_.push_back(static_cast<int>(vind_.size()));
  }

  // fixed vertices
  fixed_vertex_flag_ = (fixed_attr.size() == num_vertices_);
  if (fixed_vertex_flag_) {
    fixed_attr_ = fixed_attr;
  }

  // community information
  community_flag_ = (community_attr.size() == num_vertices_);
  if (community_flag_) {
    community_attr_ = community_attr;
  }

  // placement information
  placement_flag_
      = (placement_dimensions > 0 && placement_attr.size() == num_vertices_);
  if (placement_flag_) {
    placement_dimensions_ = placement_dimensions;
    placement_attr_ = placement_attr;
  } else {
    placement_dimensions_ = 0;
  }

  logger_ = logger;
}

Hypergraph::Hypergraph(
    const int vertex_dimensions,
    const int hyperedge_dimensions,
    const int placement_dimensions,
    const std::vector<std::vector<int>>& hyperedges,
    const std::vector<std::vector<float>>& vertex_weights,
    const std::vector<std::vector<float>>& hyperedge_weights,
    // fixed vertices
    const std::vector<int>& fixed_attr,  // the block id of fixed vertices.
    // community attribute
    const std::vector<int>& community_attr,
    // placement information
    const std::vector<std::vector<float>>& placement_attr,
    // the type of each vertex
    const std::vector<VertexType>&
        vertex_types,  // except the original timing graph,
                       // users do not need to specify this
    // slack information
    const std::vector<float>& hyperedges_slack,
    const std::vector<std::set<int>>& hyperedges_arc_set,
    const std::vector<TimingPath>& timing_paths,
    utl::Logger* logger)
    : Hypergraph(vertex_dimensions,
                 hyperedge_dimensions,
                 placement_dimensions,
                 hyperedges,
                 vertex_weights,
                 hyperedge_weights,
                 fixed_attr,
                 community_attr,
                 placement_attr,
                 logger)
{
  // add vertex types
  vertex_types_ = vertex_types;

  // slack information
  if (hyperedges_slack.size() == num_hyperedges_
      && hyperedges_arc_set.size() == num_hyperedges_) {
    timing_flag_ = true;
    num_timing_paths_ = static_cast<int>(timing_paths.size());
    hyperedge_timing_attr_ = hyperedges_slack;
    hyperedge_arc_set_ = hyperedges_arc_set;
    // create the vertex Matrix which stores the paths incident to vertex
    std::vector<std::vector<int>> incident_paths(num_vertices_);
    vptr_p_.push_back(0);
    eptr_p_.push_back(0);
    for (int path_id = 0; path_id < num_timing_paths_; path_id++) {
      // view each path as a sequence of vertices
      const auto& timing_path = timing_paths[path_id].path;
      vind_p_.insert(vind_p_.end(), timing_path.begin(), timing_path.end());
      vptr_p_.push_back(static_cast<int>(vind_p_.size()));
      for (const int v : timing_path) {
        incident_paths[v].push_back(path_id);
      }
      // view each path as a sequence of hyperedge
      const auto& timing_arc = timing_paths[path_id].arcs;
      eind_p_.insert(eind_p_.end(), timing_arc.begin(), timing_arc.end());
      eptr_p_.push_back(static_cast<int>(eind_p_.size()));
      // add the timing attribute
      path_timing_attr_.push_back(timing_paths[path_id].slack);
    }
    pptr_v_.push_back(0);
    for (auto& paths : incident_paths) {
      pind_v_.insert(pind_v_.end(), paths.begin(), paths.end());
      pptr_v_.push_back(static_cast<int>(pind_v_.size()));
    }
  }
}

std::vector<float> Hypergraph::GetTotalVertexWeights() const
{
  std::vector<float> total_weight(vertex_dimensions_, 0.0);
  for (auto& weight : vertex_weights_) {
    total_weight = total_weight + weight;
  }
  return total_weight;
}

std::vector<std::vector<float>> Hypergraph::GetUpperVertexBalance(
    int num_parts,
    float ub_factor,
    std::vector<float> base_balance) const
{
  std::vector<float> vertex_balance = GetTotalVertexWeights();
  for (auto& value : base_balance) {
    value += ub_factor * 0.01;
  }
  std::vector<std::vector<float>> upper_block_balance(num_parts,
                                                      vertex_balance);
  for (int i = 0; i < num_parts; i++) {
    upper_block_balance[i]
        = MultiplyFactor(upper_block_balance[i], base_balance[i]);
  }
  return upper_block_balance;
}

std::vector<std::vector<float>> Hypergraph::GetLowerVertexBalance(
    int num_parts,
    float ub_factor,
    std::vector<float> base_balance) const
{
  std::vector<float> vertex_balance = GetTotalVertexWeights();
  for (auto& value : base_balance) {
    value -= ub_factor * 0.01;
    if (value <= 0.0) {
      value = 0.0;
    }
  }
  std::vector<std::vector<float>> lower_block_balance(num_parts,
                                                      vertex_balance);
  for (int i = 0; i < num_parts; i++) {
    lower_block_balance[i]
        = MultiplyFactor(lower_block_balance[i], base_balance[i]);
  }
  return lower_block_balance;
}

void Hypergraph::ResetVertexCAttr()
{
  vertex_c_attr_.clear();
  vertex_c_attr_.resize(GetNumVertices());
}

void Hypergraph::ResetHyperedgeTimingAttr()
{
  std::fill(hyperedge_timing_attr_.begin(),
            hyperedge_timing_attr_.end(),
            std::numeric_limits<float>::max());
}

void Hypergraph::ResetPathTimingCost()
{
  path_timing_cost_.clear();
  path_timing_cost_.resize(GetNumTimingPaths());
}

void Hypergraph::ResetPathTimingSlack()
{
  path_timing_attr_.clear();
  path_timing_attr_.resize(GetNumTimingPaths());
}

}  // namespace par
