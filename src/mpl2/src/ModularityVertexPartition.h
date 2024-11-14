///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, The Regents of the University of California
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
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////
#pragma once
#include <cfloat>
#include <deque>
#include <exception>
#include <limits>
#include <map>
#include <vector>

#include "leidenInterface.h"

namespace mpl2 {
class ModularityVertexPartition
{
 public:
  ModularityVertexPartition(GraphForLeidenAlgorithm* graph,
                            std::vector<size_t> const& membership);
  ModularityVertexPartition(GraphForLeidenAlgorithm* graph);
  ModularityVertexPartition* create(GraphForLeidenAlgorithm* graph);
  ModularityVertexPartition* create(GraphForLeidenAlgorithm* graph,
                                    std::vector<size_t> const& membership);

  ~ModularityVertexPartition();

  inline size_t membership(size_t v) { return this->_membership[v]; };
  inline std::vector<size_t> const& membership() const
  {
    return this->_membership;
  };

  double csize(size_t comm);
  size_t cnodes(size_t comm);
  std::vector<size_t> get_community(size_t comm);
  std::vector<std::vector<size_t>> get_communities();
  size_t n_communities();

  void move_node(size_t v, size_t new_comm);
  double diff_move(size_t v, size_t new_comm);
  double quality();

  inline GraphForLeidenAlgorithm* get_graph() { return this->graph_; };

  void renumber_communities();
  void renumber_communities(std::vector<size_t> const& fixed_nodes,
                            std::vector<size_t> const& fixed_membership);
  void renumber_communities(std::vector<size_t> const& new_membership);
  void set_membership(std::vector<size_t> const& new_membership);
  void relabel_communities(std::vector<size_t> const& new_comm_id);
  std::vector<size_t> static rank_order_communities(
      std::vector<ModularityVertexPartition*> partitions);
  size_t get_empty_community();
  size_t add_empty_community();
  void from_coarse_partition(
      std::vector<size_t> const& coarse_partition_membership);
  void from_coarse_partition(ModularityVertexPartition* partition);
  void from_coarse_partition(ModularityVertexPartition* partition,
                             std::vector<size_t> const& coarser_membership);
  void from_coarse_partition(
      std::vector<size_t> const& coarse_partition_membership,
      std::vector<size_t> const& coarse_node);

  void from_partition(ModularityVertexPartition* partition);

  inline double total_weight_in_comm(size_t comm)
  {
    return comm < _n_communities ? this->_total_weight_in_comm[comm] : 0.0;
  };
  inline double total_weight_from_comm(size_t comm)
  {
    return comm < _n_communities ? this->_total_weight_from_comm[comm] : 0.0;
  };
  inline double total_weight_to_comm(size_t comm)
  {
    return comm < _n_communities ? this->_total_weight_to_comm[comm] : 0.0;
  };

  inline double total_weight_in_all_comms()
  {
    return this->_total_weight_in_all_comms;
  };
  inline size_t total_possible_edges_in_all_comms()
  {
    return this->_total_possible_edges_in_all_comms;
  };

  inline double weight_to_comm(size_t v, size_t comm)
  {
    if (this->_current_node_cache_community_to != v) {
      this->cache_neigh_communities(v);
      this->_current_node_cache_community_to = v;
    }

    if (comm < this->_cached_weight_to_community.size())
      return this->_cached_weight_to_community[comm];
    else
      return 0.0;
  }

  inline double weight_from_comm(size_t v, size_t comm)
  {
    if (this->_current_node_cache_community_from != v) {
      this->cache_neigh_communities(v);
      this->_current_node_cache_community_from = v;
    }

    if (comm < this->_cached_weight_from_community.size())
      return this->_cached_weight_from_community[comm];
    else
      return 0.0;
  }

  std::vector<size_t> const& get_neigh_comms(size_t v);
  std::vector<size_t> get_neigh_comms(
      size_t v,
      std::vector<size_t> const& constrained_membership);

  // By delegating the responsibility for deleting the graph to the partition,
  // we no longer have to worry about deleting this graph.
  int destructor_delete_graph;

 protected:
  void init_admin();

  std::vector<size_t> _membership;  // Membership std::vector, i.e. \sigma_i = c
                                    // means that node i is in community c

  GraphForLeidenAlgorithm* graph_;

  // Community size
  std::vector<double> _csize;

  // Number of nodes in community
  std::vector<size_t> _cnodes;

  double weight_vertex_tofrom_comm(size_t v, size_t comm);

  void set_default_attrs();

 private:
  // Keep track of the internal weight of each community
  std::vector<double> _total_weight_in_comm;
  // Keep track of the total weight to a community
  std::vector<double> _total_weight_to_comm;
  // Keep track of the total weight from a community
  std::vector<double> _total_weight_from_comm;
  // Keep track of the total internal weight
  double _total_weight_in_all_comms;
  size_t _total_possible_edges_in_all_comms;
  size_t _n_communities;

  std::vector<size_t> _empty_communities;

  void cache_neigh_communities(size_t v);

  size_t _current_node_cache_community_from;
  std::vector<double> _cached_weight_from_community;
  std::vector<size_t> _cached_neigh_comms_from;
  size_t _current_node_cache_community_to;
  std::vector<double> _cached_weight_to_community;
  std::vector<size_t> _cached_neigh_comms_to;
  size_t _current_node_cache_community_all;
  std::vector<double> _cached_weight_all_community;
  std::vector<size_t> _cached_neigh_comms_all;

  void clean_mem();

  void update_n_communities();
};

}  // namespace mpl2