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
// hyperedge_dimensions,
//         placement_dimension, cluster_id (c), vertex_id (v), hyperedge_id (e)
//         are all in int type.
// Rule2 : We assume T (int, float) is the type for vertex weight, U (int,
// float)
//         is the type for hyperedge weight.
// Rule3 : Each hyperedge can include a vertex at most once.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <functional>

#include "Utilities.h"
#include "utl/Logger.h"

namespace par {
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
};

// T is the type for vertex properties (float or int)
// U is the type for hyperedge properties (float or int)
// Here we use TPHypergraph class because the Hypegraph class
// has been used by other programs.
class TPHypergraph
{
 public:
  explicit TPHypergraph() {}  // Use the default value for each parameter

  // Multiple constructors
  // Basic form
  TPHypergraph(int vertex_dimensions,
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
               utl::Logger* logger);

  TPHypergraph(
      int num_vertices,
      int num_hyperedges,
      int vertex_dimensions,
      int hyperedge_dimensions,
      // hyperedges: each hyperedge is a set of vertices
      const std::vector<int>& eind,
      const std::vector<int>& eptr,
      // vertices: each vertex is a set of hyperedges
      const std::vector<int>& vind,
      const std::vector<int>& vptr,
      // weights
      const std::vector<std::vector<float>>& vertex_weights,
      const std::vector<std::vector<float>>& hyperedge_weights,
      // fixed vertices
      const std::vector<int>& fixed_attr,  // the block id of fixed vertices.
      // community structure
      const std::vector<int>& community_attr,
      // placement information
      int placement_dimensions,
      const std::vector<std::vector<float>>& placement_attr,
      // timing flag
      // view a timing path as a sequence of hyperedges
      const std::vector<int>& vind_p,
      const std::vector<int>& vptr_p,
      const std::vector<int>& pind_v,
      const std::vector<int>& pptr_v,
      // slack for each timing paths
      const std::vector<float> timing_attr,
      utl::Logger* logger)
  {
    num_vertices_ = num_vertices;
    num_hyperedges_ = num_hyperedges;
    vertex_dimensions_ = vertex_dimensions;
    hyperedge_dimensions_ = hyperedge_dimensions;
    eind_ = eind;
    eptr_ = eptr;
    vind_ = vind;
    vptr_ = vptr;
    vertex_weights_ = vertex_weights;
    hyperedge_weights_ = hyperedge_weights;
    // fixed vertices
    fixed_vertex_flag_ = (fixed_attr.size() > 0);
    fixed_attr_ = fixed_attr;
    // community structure
    community_flag_ = (community_attr.size() > 0);
    community_attr_ = community_attr;
    // placement information
    placement_flag_ = (placement_dimensions > 0);
    placement_dimensions_ = placement_dimensions;
    placement_attr_ = placement_attr;
    // timing flag
    num_timing_paths_ = static_cast<int>(timing_attr.size());
    vind_p_ = vind_p;
    vptr_p_ = vptr_p;
    pind_v_ = pind_v;
    pptr_v_ = pptr_v;
    timing_attr_ = timing_attr;
    // logger
    logger_ = logger;
  }

  TPHypergraph(
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
      utl::Logger* logger);

  TPHypergraph(
      int num_vertices,
      int num_hyperedges,
      int vertex_dimensions,
      int hyperedge_dimensions,
      // hyperedges: each hyperedge is a set of vertices
      const std::vector<int>& eind,
      const std::vector<int>& eptr,
      // vertices: each vertex is a set of hyperedges
      const std::vector<int>& vind,
      const std::vector<int>& vptr,
      // weights
      const std::vector<std::vector<float>>& vertex_weights,
      const std::vector<std::vector<float>>& hyperedge_weights,
      // Necessary for constraints driven
      const std::vector<std::vector<float>>& nonscaled_hyperedge_weights,
      // fixed vertices
      const std::vector<int>& fixed_attr,  // the block id of fixed vertices.
      // community structure
      const std::vector<int>& community_attr,
      // placement information
      int placement_dimensions,
      const std::vector<std::vector<float>>& placement_attr,
      // timing flag
      // view a timing path as a sequence of hyperedges
      const std::vector<int>& vind_p,
      const std::vector<int>& vptr_p,
      const std::vector<int>& pind_v,
      const std::vector<int>& pptr_v,
      // slack for each timing paths
      const std::vector<float> timing_attr,
      utl::Logger* logger)
  {
    num_vertices_ = num_vertices;
    num_hyperedges_ = num_hyperedges;
    vertex_dimensions_ = vertex_dimensions;
    hyperedge_dimensions_ = hyperedge_dimensions;
    eind_ = eind;
    eptr_ = eptr;
    vind_ = vind;
    vptr_ = vptr;
    vertex_weights_ = vertex_weights;
    hyperedge_weights_ = hyperedge_weights;
    nonscaled_hyperedge_weights_ = nonscaled_hyperedge_weights;
    // fixed vertices
    fixed_vertex_flag_ = (fixed_attr.size() > 0);
    fixed_attr_ = fixed_attr;
    // community structure
    community_flag_ = (community_attr.size() > 0);
    community_attr_ = community_attr;
    // placement information
    placement_flag_ = (placement_dimensions > 0);
    placement_dimensions_ = placement_dimensions;
    placement_attr_ = placement_attr;
    // timing flag
    num_timing_paths_ = static_cast<int>(timing_attr.size());
    vind_p_ = vind_p;
    vptr_p_ = vptr_p;
    pind_v_ = pind_v;
    pptr_v_ = pptr_v;
    timing_attr_ = timing_attr;
    // logger
    logger_ = logger;
  }

  // copy constructor
  TPHypergraph(const TPHypergraph& hgraph)
  {
    num_vertices_ = hgraph.num_vertices_;
    num_hyperedges_ = hgraph.num_hyperedges_;
    vertex_dimensions_ = hgraph.vertex_dimensions_;
    hyperedge_dimensions_ = hgraph.hyperedge_dimensions_;
    eind_ = hgraph.eind_;
    eptr_ = hgraph.eptr_;
    vind_ = hgraph.vind_;
    vptr_ = hgraph.vptr_;
    vertex_weights_ = hgraph.vertex_weights_;
    hyperedge_weights_ = hgraph.hyperedge_weights_;
    nonscaled_hyperedge_weights_ = hgraph.nonscaled_hyperedge_weights_;
    // vertex_c_attr
    vertex_c_attr_ = hgraph.vertex_c_attr_;
    // fixed vertices
    fixed_vertex_flag_ = hgraph.fixed_vertex_flag_;
    fixed_attr_ = hgraph.fixed_attr_;
    // community structure
    community_flag_ = hgraph.community_flag_;
    community_attr_ = hgraph.community_attr_;
    // placement information
    placement_flag_ = hgraph.placement_flag_;
    placement_dimensions_ = hgraph.placement_dimensions_;
    placement_attr_ = hgraph.placement_attr_;
    // timing flag
    num_timing_paths_ = hgraph.num_timing_paths_;
    vind_p_ = hgraph.vind_p_;
    vptr_p_ = hgraph.vptr_p_;
    pind_v_ = hgraph.pind_v_;
    pptr_v_ = hgraph.pptr_v_;
    timing_attr_ = hgraph.timing_attr_;
    // logger
    logger_ = hgraph.logger_;
  }

  // assignment operator
  TPHypergraph& operator=(const TPHypergraph& hgraph)
  {
    num_vertices_ = hgraph.num_vertices_;
    num_hyperedges_ = hgraph.num_hyperedges_;
    vertex_dimensions_ = hgraph.vertex_dimensions_;
    hyperedge_dimensions_ = hgraph.hyperedge_dimensions_;
    eind_ = hgraph.eind_;
    eptr_ = hgraph.eptr_;
    vind_ = hgraph.vind_;
    vptr_ = hgraph.vptr_;
    vertex_weights_ = hgraph.vertex_weights_;
    hyperedge_weights_ = hgraph.hyperedge_weights_;
    nonscaled_hyperedge_weights_ = hgraph.nonscaled_hyperedge_weights_;
    // vertex_c_attr
    vertex_c_attr_ = hgraph.vertex_c_attr_;
    // fixed vertices
    fixed_vertex_flag_ = hgraph.fixed_vertex_flag_;
    fixed_attr_ = hgraph.fixed_attr_;
    // community structure
    community_flag_ = hgraph.community_flag_;
    community_attr_ = hgraph.community_attr_;
    // placement information
    placement_flag_ = hgraph.placement_flag_;
    placement_dimensions_ = hgraph.placement_dimensions_;
    placement_attr_ = hgraph.placement_attr_;
    // timing flag
    num_timing_paths_ = hgraph.num_timing_paths_;
    vind_p_ = hgraph.vind_p_;
    vptr_p_ = hgraph.vptr_p_;
    pind_v_ = hgraph.pind_v_;
    pptr_v_ = hgraph.pptr_v_;
    timing_attr_ = hgraph.timing_attr_;
    // logger
    logger_ = hgraph.logger_;
    return *this;
  }

  // destructor
  ~TPHypergraph() = default;

  int GetNumVertices() const { return num_vertices_; }
  int GetNumHyperedges() const { return num_hyperedges_; }
  int GetNumTimingPaths() const { return num_timing_paths_; }
  std::vector<float> GetTotalVertexWeights() const;
  std::vector<float> GetTotalHyperedgeWeights() const;
  // write the hypergraph out in general hmetis format
  void WriteHypergraph(std::string hypergraph_file) const;
  void WriteReducedHypergraph(
      const std::string reduced_file,
      const std::vector<float> vertex_w_factor,
      const std::vector<float> hyperedge_w_factor) const;
  // get balance constraints
  std::vector<std::vector<float>> GetVertexBalance(int num_parts,
                                                   float ubfactor);
  std::vector<float> ComputeAlgebraicWights() const;

 private:
  // basic hypergraph
  int num_vertices_ = 0;
  int num_hyperedges_ = 0;
  int vertex_dimensions_ = 1;
  int hyperedge_dimensions_ = 1;
  std::vector<std::vector<float>> vertex_weights_;
  std::vector<std::vector<float>>
      hyperedge_weights_;  // hyperedge weights can be negative
  std::vector<std::vector<float>> nonscaled_hyperedge_weights_;
  // hyperedges: each hyperedge is a set of vertices
  std::vector<int> eind_;
  std::vector<int> eptr_;
  // vertices: each vertex is a set of hyperedges
  std::vector<int> vind_;
  std::vector<int> vptr_;
  // Fill vertex_c_attr which maps the vertex to its corresponding cluster
  // vertex_c_attr has hgraph->num_vertices_ elements
  std::vector<int> vertex_c_attr_;
  // fixed vertices.  If fixed_vertex_flag_ = false, fixed_attr_ is empty
  bool fixed_vertex_flag_ = false;  // If there are fixed vertices
  std::vector<int> fixed_attr_;     // the block id of fixed vertices.
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
  // if timing_flag_ = false, paths_ is empty
  int num_timing_paths_ = 0;
  // view a timing path as a sequence of vertices
  std::vector<int> vind_p_;  // each timing path is a sequence of vertices
  std::vector<int> vptr_p_;
  std::vector<int> pind_v_;  // All the timing paths connected to the vertex
  std::vector<int> pptr_v_;
  // slack for each timing paths
  std::vector<float> timing_attr_;

  // logger information
  utl::Logger* logger_ = nullptr;

  friend class TritonPart;               // define friend class for TritonPart
  friend class Coarsening;               // define friend class for Coarsening
  friend class TPcoarsener;              // define friend class for TPcoarsening
  friend class TPmultilevelPartitioner;  // define friend class for
                                         // TPmultilevelPartitioner
  friend class TPpartitioner;        // define friend class for TPpartitioner
  friend class TPrefiner;            // define friend class for TPrefiner
  friend class TPtwoWayFM;           // define friend class for TPtwoWayFM
  friend class TPkWayFM;             // define friend class for TPkWayFM
  friend class TPgreedyRefine;       // define friend class for TPgreedyRefine
  friend class TPilpRefine;          // define friend class for TPilpRefine
  friend class TPpriorityQueue;      // define friend class for TPpriorityQueue
  friend class Partitioners;         // define friend class for Partitioners
  friend class MultiLevelHierarchy;  // define friend class for
                                     // MultiLevelHierarchy
  friend class KPMRefinement;        // define friend class for KPMFM
  friend class IlpRefiner;  // define friend class for ILP based refinement
  friend class Obfuscator;  // define friend class for netlist obfuscator
};

// We should use shared_ptr because we enable multi-thread feature during
// partitioning
using HGraph = std::shared_ptr<TPHypergraph>;
}  // namespace par
