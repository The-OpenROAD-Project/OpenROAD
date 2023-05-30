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
// This file includes the basic data structure for hypergraph,
// vertex, hyperedge and timing paths. We also explain our basic
// conventions.
// Rule1 : num_vertices, num_hyperedges, vertex_dimensions,
//         hyperedge_dimensions, placement_dimension,
//         cluster_id (c), vertex_id (v), hyperedge_id (e)
//         are all in int type.
// Rule2 : Each hyperedge can include a vertex at most once.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <functional>
#include <set>
#include <vector>

#include "Utilities.h"
#include "utl/Logger.h"

// The basic function related to hypergraph should be listed here

namespace par {

struct TPHypergraph;
using HGraphPtr = std::shared_ptr<TPHypergraph>;

// The data structure for critical timing path
// A timing path is a sequence of vertices, for example, a -> b -> c -> d
// A timing path can also be viewed a sequence of hypereges,
// for example, e1 -> e2 -> e3
// In our formulation, we lay the timing graph over the hypergraph
struct TimingPath
{
  std::vector<int> path;  // a list of vertex id -> path-based method
  std::vector<int> arcs;  // a list of hyperedge id -> net-based method
  float slack = 0.0;      // slack for this critical timing paths (normalized to
                          // clock period)

  TimingPath() = default;

  TimingPath(const std::vector<int>& path_arg,
             const std::vector<int>& arcs_arg,
             float slack_arg)
  {
    path = path_arg;
    arcs = arcs_arg;
    slack = slack_arg;
  }
};

// Here we use TPHypergraph class because the Hypegraph class
// has been used by other programs.
struct TPHypergraph
{
  TPHypergraph(
      int vertex_dimensions,
      int hyperedge_dimensions,
      int placement_dimensions,
      const std::vector<std::vector<int>>& hyperedges,
      const std::vector<std::vector<float>>& vertex_weights,
      const std::vector<std::vector<float>>& hyperedge_weights,
      // fixed vertices
      const std::vector<int>& fixed_attr,  // the block id of fixed vertices.
      // community attribute
      const std::vector<int>& community_attr,
      // placement information
      const std::vector<std::vector<float>>& placement_attr,
      utl::Logger* logger);

  TPHypergraph(
      int vertex_dimensions,
      int hyperedge_dimensions,
      int placement_dimensions,
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
          vertex_types,  // except the original timing graph, users do not need
                         // to specify this
      // slack information
      const std::vector<float>& hyperedges_slack,
      const std::vector<std::set<int>>& hyperedges_arc_set,
      const std::vector<TimingPath>& timing_paths,
      utl::Logger* logger);

  int GetNumVertices() const { return num_vertices_; }
  int GetNumHyperedges() const { return num_hyperedges_; }
  int GetNumTimingPaths() const { return num_timing_paths_; }

  std::vector<float> GetTotalVertexWeights() const;

  // get balance constraints
  // TODO:  RePlace the Vertex Balance with UpperVertexBalance
  std::vector<std::vector<float>> GetVertexBalance(int num_parts,
                                                   float ub_factor) const;

  std::vector<std::vector<float>> GetUpperVertexBalance(int num_parts,
                                                        float ub_factor) const;

  std::vector<std::vector<float>> GetLowerVertexBalance(int num_parts,
                                                        float ub_factor) const;

  // basic hypergraph
  int num_vertices_ = 0;
  int num_hyperedges_ = 0;
  int vertex_dimensions_ = 1;
  int hyperedge_dimensions_ = 1;
  std::vector<std::vector<float>> vertex_weights_;
  std::vector<std::vector<float>>
      hyperedge_weights_;  // hyperedge weights can be negative
  // slack for hyperedge
  std::vector<float> hyperedge_timing_attr_;  // slack of each hyperedge
  std::vector<float>
      hyperedge_timing_cost_;  // translate the slack of hyperedge into cost
  std::vector<std::set<int>>
      hyperedge_arc_set_;  // map current hyperedge into arcs in timing graph
                           // the slack of each hyperedge e is the
                           // minimum_slack_hyperedge_arc_set_[e]
  // hyperedges: each hyperedge is a set of vertices
  std::vector<int> eind_;
  std::vector<int> eptr_;
  // vertices: each vertex is a set of hyperedges
  std::vector<int> vind_;
  std::vector<int> vptr_;

  // Fill vertex_c_attr which maps the vertex to its corresponding cluster
  // To simpify the implementation, the vertex_c_attr maps the original larger
  // hypergraph vertex_c_attr has hgraph->num_vertices_ elements. This is used
  // during coarsening phase similar to hyperedge_arc_set_
  std::vector<std::vector<int>> vertex_c_attr_;

  // fixed vertices.  If fixed_vertex_flag_ = false, fixed_attr_ is empty
  bool fixed_vertex_flag_ = false;  // If there are fixed vertices
  std::vector<int> fixed_attr_;     // the block id of fixed vertices

  // vertex types
  std::vector<VertexType> vertex_types_;  // the type of each vertex

  // community structure. If community_flag_ = false, community_ is empty
  // Note that fixed_attr_ and community_attr_ can be different
  // For example, a is fixed to block 1 and a can belong to community 10
  bool community_flag_ = false;      // If there is community structure
  std::vector<int> community_attr_;  // the community id of vertices

  // placement information.
  // The embedding information for the hypergraph
  // This embedding can be real placement from chip layout
  // It can also be spectral embedding
  // If placement_flag = false, placement_attr_ is empty
  bool placement_flag_ = false;
  int placement_dimensions_ = 0;
  std::vector<std::vector<float>>
      placement_attr_;  // the embedding for vertices

  // Timing information
  bool timing_flag_ = false;  // timing flag
  int num_timing_paths_ = 0;
  // All the timing paths connected to the vertex
  std::vector<int> pind_v_;
  std::vector<int> pptr_v_;
  // view a timing path as a sequence of vertices
  std::vector<int> vind_p_;
  std::vector<int> vptr_p_;
  // view a timing path as a sequence of arcs
  std::vector<int> eind_p_;
  std::vector<int> eptr_p_;
  // slack for each timing paths
  std::vector<float> path_timing_attr_;
  std::vector<float>
      path_timing_cost_;  // translate the slack of timing path into weight
  // logger information
  utl::Logger* logger_ = nullptr;
};

}  // namespace par
