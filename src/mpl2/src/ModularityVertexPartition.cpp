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

#include "ModularityVertexPartition.h"
#include "Optimizer.h"
#include <algorithm>
#include <iostream>
#include <queue>


namespace mpl2 {
/****************************************************************************
Create a new vertex partition.
*****************************************************************************/

/****************************************************************************
  Create a new vertex partition.

  Parameters:
    graph            -- The igraph.GraphForLeidenAlgorithm on which this partition is defined.
    membership=None  -- The membership std::vector of this partition, i.e. an
                        community number for each node. So membership[i] = c
                        implies that node i is in community c. If None, it is
                        initialised with each node in its own community.
    weight_attr=None -- What edge attribute should be used as a weight for the
                        edges? If None, the weight defaults to 1.
    size_attr=None   -- What node attribute should be used for keeping track
                        of the size of the node? In some methods (e.g. CPM or
                        Significance), we need to keep track of the total
                        size of the community. So when we aggregate/collapse
                        the graph, we should know how many nodes were in a
                        community. If None, the size of a node defaults to 1.
    self_weight_attr=None
                     -- What node attribute should be used for the self
                        weight? If None, the self_weight is
                        recalculated each time."""
*****************************************************************************/

ModularityVertexPartition::ModularityVertexPartition(
    GraphForLeidenAlgorithm* graph,
    std::vector<size_t> const& membership)
{
  this->destructor_delete_graph = false;
  this->graph_ = graph;
  if (membership.size() != graph->numVertices()) {
    throw Exception("Membership std::vector has incorrect size.");
  }
  this->_membership = membership;
  this->init_admin();
}

ModularityVertexPartition::ModularityVertexPartition(
    GraphForLeidenAlgorithm* graph)
{
  this->destructor_delete_graph = false;
  this->graph_ = graph;
  this->_membership = range(graph->numVertices());
  this->init_admin();
}

ModularityVertexPartition* ModularityVertexPartition::create(
    GraphForLeidenAlgorithm* graph)
{
  return new ModularityVertexPartition(graph);
}

ModularityVertexPartition* ModularityVertexPartition::create(
    GraphForLeidenAlgorithm* graph,
    std::vector<size_t> const& membership)
{
  return new ModularityVertexPartition(graph, membership);
}

ModularityVertexPartition::~ModularityVertexPartition()
{
  this->clean_mem();
  if (this->destructor_delete_graph)
    delete this->graph_;
}

void ModularityVertexPartition::clean_mem()
{
  // Clean up memory if necessary (not used in this simplified version)
}

double ModularityVertexPartition::csize(size_t comm)
{
  if (comm < this->_csize.size())
    return this->_csize[comm];
  else
    return 0;
}

size_t ModularityVertexPartition::cnodes(size_t comm)
{
  if (comm < this->_cnodes.size())
    return this->_cnodes[comm];
  else
    return 0;
}

std::vector<size_t> ModularityVertexPartition::get_community(size_t comm)
{
  std::vector<size_t> community;
  community.reserve(this->_cnodes[comm]);
  for (size_t i = 0; i < this->graph_->numVertices(); i++)
    if (this->_membership[i] == comm)
      community.push_back(i);
  return community;
}

std::vector<std::vector<size_t>> ModularityVertexPartition::get_communities()
{
  std::vector<std::vector<size_t>> communities(this->_n_communities);
  for (size_t i = 0; i < this->graph_->numVertices(); i++)
    communities[this->_membership[i]].push_back(i);
  return communities;
}

size_t ModularityVertexPartition::n_communities()
{
  return this->_n_communities;
}

void ModularityVertexPartition::relabel_communities(
    std::vector<size_t> const& new_comm_id)
{
  if (this->_n_communities != new_comm_id.size()) {
    throw Exception(
        "Problem swapping community labels. Mismatch between n_communities and "
        "new_comm_id std::vector.");
  }

  size_t n = this->graph_->numVertices();

  for (size_t i = 0; i < n; i++)
    this->_membership[i] = new_comm_id[this->_membership[i]];

  this->update_n_communities();
  size_t nbcomms = this->n_communities();

  std::vector<double> new_total_weight_in_comm(nbcomms, 0.0);
  std::vector<double> new_total_weight_from_comm(nbcomms, 0.0);
  std::vector<double> new_total_weight_to_comm(nbcomms, 0.0);
  std::vector<double> new_csize(nbcomms, 0);
  std::vector<size_t> new_cnodes(nbcomms, 0);

  // Relabel community admin
  for (size_t c = 0; c < new_comm_id.size(); c++) {
    size_t new_c = new_comm_id[c];
    if (this->_cnodes[c] > 0) {
      new_total_weight_in_comm[new_c] = this->_total_weight_in_comm[c];
      new_total_weight_from_comm[new_c] = this->_total_weight_from_comm[c];
      new_total_weight_to_comm[new_c] = this->_total_weight_to_comm[c];
      new_csize[new_c] = this->_csize[c];
      new_cnodes[new_c] = this->_cnodes[c];
    }
  }

  this->_total_weight_in_comm = new_total_weight_in_comm;
  this->_total_weight_from_comm = new_total_weight_from_comm;
  this->_total_weight_to_comm = new_total_weight_to_comm;
  this->_csize = new_csize;
  this->_cnodes = new_cnodes;

  this->_empty_communities.clear();
  for (size_t c = 0; c < nbcomms; c++) {
    if (this->_cnodes[c] == 0) {
      this->_empty_communities.push_back(c);
    }
  }

  // invalidate cached weight vectors
  for (size_t c : this->_cached_neigh_comms_from)
    this->_cached_weight_from_community[c] = 0;
  this->_cached_neigh_comms_from.clear();
  this->_cached_weight_from_community.resize(nbcomms, 0);
  this->_current_node_cache_community_from = n + 1;

  for (size_t c : this->_cached_neigh_comms_to)
    this->_cached_weight_to_community[c] = 0;
  this->_cached_neigh_comms_to.clear();
  this->_cached_weight_to_community.resize(nbcomms, 0);
  this->_current_node_cache_community_to = n + 1;

  for (size_t c : this->_cached_neigh_comms_all)
    this->_cached_weight_all_community[c] = 0;
  this->_cached_neigh_comms_all.clear();
  this->_cached_weight_all_community.resize(nbcomms, 0);
  this->_current_node_cache_community_all = n + 1;
}

/****************************************************************************
  Initialise the internal administration based on the membership std::vector.
*****************************************************************************/
void ModularityVertexPartition::init_admin()
{
  size_t n = this->graph_->numVertices();

  // Determine the number of communities
  this->update_n_communities();

  // Reset the administrative fields
  this->_total_weight_in_comm.clear();
  this->_total_weight_in_comm.resize(this->_n_communities);
  this->_total_weight_from_comm.clear();
  this->_total_weight_from_comm.resize(this->_n_communities);
  this->_total_weight_to_comm.clear();
  this->_total_weight_to_comm.resize(this->_n_communities);
  this->_csize.clear();
  this->_csize.resize(this->_n_communities);
  this->_cnodes.clear();
  this->_cnodes.resize(this->_n_communities);

  this->_current_node_cache_community_from = n + 1; this->_cached_weight_from_community.resize(this->_n_communities, 0);
  this->_current_node_cache_community_to = n + 1;   this->_cached_weight_to_community.resize(this->_n_communities, 0);
  this->_current_node_cache_community_all = n + 1;  this->_cached_weight_all_community.resize(this->_n_communities, 0);
  this->_cached_neigh_comms_all.resize(n);
  
  this->_empty_communities.clear();

  this->_total_weight_in_all_comms = 0.0;
  for (size_t v = 0; v < n; v++) {
    size_t v_comm = this->_membership[v];
    // Update community size
    this->_csize[v_comm] += this->graph_->getVertexWeight(v);
    this->_cnodes[v_comm] += 1;
  }

  size_t m = this->graph_->numVertices();
  for (size_t v = 0; v < m; v++) {
    auto neighbors = this->graph_->get_neighbours(v);
    for (int u : neighbors) {
      size_t v_comm = this->_membership[v];
      size_t u_comm = this->_membership[u];
      float w = this->graph_->getEdgeWeight(v, u);

      if (v_comm == u_comm) {
        this->_total_weight_in_comm[v_comm] += w;
        this->_total_weight_in_all_comms += w;
      }
    }
  }

  this->_total_possible_edges_in_all_comms = 0;
  for (size_t c = 0; c < this->_n_communities; c++) {
    double n_c = this->csize(c);
    double possible_edges = this->graph_->possible_edges(n_c);

    this->_total_possible_edges_in_all_comms += possible_edges;

    // It is possible that some community have a zero size (if the order
    // is for example not consecutive. We add those communities to the empty
    // communities std::vector for consistency.
    if (this->_cnodes[c] == 0)
      this->_empty_communities.push_back(c);
  }
}

void ModularityVertexPartition::update_n_communities()
{
  this->_n_communities = 0;
  for (size_t i = 0; i < this->graph_->numVertices(); i++)
    if (this->_membership[i] >= this->_n_communities)
      this->_n_communities = this->_membership[i] + 1;
}

/****************************************************************************
 Renumber the communities according to the new labels in new_comm_id.

 This adjusts the internal bookkeeping as required, avoiding the more costly
 setup required in init_admin(). In particular, this avoids recomputation of
 weights in/from/to each community by simply assigning the previously
 computed values to the new, relabeled communities.

 For instance, a new_comm_id of <1, 2, 0> will change the labels such that
 community 0 becomes 1, community 1 becomes 2, and community 2 becomes 0.
*****************************************************************************/

std::vector<size_t> ModularityVertexPartition::rank_order_communities(
    std::vector<ModularityVertexPartition*> partitions)
{
  size_t nb_layers = partitions.size();
  size_t nb_comms = partitions[0]->n_communities();

  // First sort the communities by size
  // Csizes
  // first - community
  // second - csize
  // third - number of nodes (may be aggregate nodes), to account for
  // communities with zero weight.
  std::vector<size_t*> csizes;
  for (size_t i = 0; i < nb_comms; i++) {
    double csize = 0;
    for (size_t layer = 0; layer < nb_layers; layer++)
      csize += partitions[layer]->csize(i);

    size_t* row = new size_t[3];
    row[0] = i;
    row[1] = csize;
    row[2] = partitions[0]->cnodes(i);
    csizes.push_back(row);
  }
  std::sort(csizes.begin(), csizes.end(), orderCSize);

  // Then use the sort order to assign new communities,
  // such that the largest community gets the lowest index.
  std::vector<size_t> new_comm_id(nb_comms, 0);
  for (size_t i = 0; i < nb_comms; i++) {
    size_t comm = csizes[i][0];
    new_comm_id[comm] = i;
    delete[] csizes[i];
  }

  return new_comm_id;
}

/****************************************************************************
  Renumber communities to make them consecutive and remove empty communities.
*****************************************************************************/
void ModularityVertexPartition::renumber_communities()
{
  std::vector<ModularityVertexPartition*> partitions(1);
  partitions[0] = this;
  std::vector<size_t> new_comm_id
      = ModularityVertexPartition::rank_order_communities(partitions);
  this->relabel_communities(new_comm_id);
}

/****************************************************************************
 Renumber the communities using the original fixed membership std::vector.
Notice that this doesn't ensure any property of the community numbers.
*****************************************************************************/
void ModularityVertexPartition::renumber_communities(
    std::vector<size_t> const& fixed_nodes,
    std::vector<size_t> const& fixed_membership)
{
  // Skip whole thing if there are no fixed nodes for efficiency
  if (fixed_nodes.size() == 0)
    return;

  // The number of communities does not depend on whether some are fixed
  size_t nb_comms = n_communities();

  // Fill the community map with the original communities
  std::vector<size_t> new_comm_id(nb_comms);
  std::vector<bool> comm_assigned_bool(nb_comms);
  std::priority_queue<size_t, std::vector<size_t>, std::greater<size_t>>
      new_comm_assigned;
  for (size_t v : fixed_nodes) {
    if (!comm_assigned_bool[_membership[v]]) {
      size_t fixed_comm_v = fixed_membership[v];
      new_comm_id[_membership[v]] = fixed_comm_v;
      comm_assigned_bool[_membership[v]] = true;
      new_comm_assigned.push(fixed_comm_v);
    }
  }

  // Index of the most recently added community
  size_t cc = 0;
  for (size_t c = 0; c != nb_comms; c++) {
    if (!comm_assigned_bool[c]) {
      // Look for the first free integer
      while (!new_comm_assigned.empty() && cc == new_comm_assigned.top()) {
        new_comm_assigned.pop();
        cc++;
      }
      // Assign the community
      new_comm_id[c] = cc++;
    }
  }

  this->relabel_communities(new_comm_id);
}

/****************************************************************************
  Move a node to a new community and update the administration.
*****************************************************************************/
void ModularityVertexPartition::move_node(size_t v, size_t new_comm)
{
  size_t old_comm = this->_membership[v];
  double node_size = this->graph_->getVertexWeight(v);

  if (new_comm != old_comm) {
    this->_cnodes[old_comm] -= 1;
    this->_csize[old_comm] -= node_size;
    if (this->_cnodes[old_comm] == 0) {
      this->_empty_communities.push_back(old_comm);
    }

    this->_cnodes[new_comm] += 1;
    this->_csize[new_comm] += node_size;

    this->_membership[v] = new_comm;
  }
}

/****************************************************************************
 Read new communities from coarser partition assuming that the community
 represents a node in the coarser partition (with the same index as the
 community number).
****************************************************************************/
void ModularityVertexPartition::from_coarse_partition(
    std::vector<size_t> const& coarse_partition_membership)
{
  this->from_coarse_partition(coarse_partition_membership, this->_membership);
}

void ModularityVertexPartition::from_coarse_partition(
    ModularityVertexPartition* coarse_partition)
{
  this->from_coarse_partition(coarse_partition, this->_membership);
}

void ModularityVertexPartition::from_coarse_partition(
    ModularityVertexPartition* coarse_partition,
    std::vector<size_t> const& coarse_node)
{
  this->from_coarse_partition(coarse_partition->membership(), coarse_node);
}

/****************************************************************************
 Set the current community of all nodes to the community specified in the
partition assuming that the coarser partition is created using the membership as
specified by coarser_membership. In other words node i becomes node
coarse_node[i] in the coarser partition and thus has community
coarse_partition_membership[coarse_node[i]].
****************************************************************************/
void ModularityVertexPartition::from_coarse_partition(
    std::vector<size_t> const& coarse_partition_membership,
    std::vector<size_t> const& coarse_node)
{
  // Read the coarser partition
  for (size_t v = 0; v < this->graph_->numVertices(); v++) {
    // In the coarser partition, the node should have the community id
    // as represented by the coarser_membership std::vector
    size_t v_level2 = coarse_node[v];

    // In the coarser partition, this node is represented by v_level2
    size_t v_comm_level2 = coarse_partition_membership[v_level2];

    // Set local membership to community found for node at second level
    this->_membership[v] = v_comm_level2;
  }

  this->clean_mem();
  this->init_admin();
}

/****************************************************************************
 Read new partition from another partition.
****************************************************************************/
void ModularityVertexPartition::from_partition(
    ModularityVertexPartition* partition)
{
  // Assign the membership of every node in the supplied partition
  // to the one in this partition
  for (size_t v = 0; v < this->graph_->numVertices(); v++)
    this->_membership[v] = partition->membership(v);
  this->clean_mem();
  this->init_admin();
}

/****************************************************************************
  Modularity calculation for the partition.
*****************************************************************************/
double ModularityVertexPartition::diff_move(size_t v, size_t new_comm)
{
  size_t old_comm = this->_membership[v];
  if (new_comm == old_comm) {
    return 0.0;
  }

  double w_to_old = this->graph_->getEdgeWeight(v, old_comm);
  double w_to_new = this->graph_->getEdgeWeight(v, new_comm);

  double improvement = (w_to_new - w_to_old);
  return improvement;
}

/****************************************************************************
  Calculate the modularity of the current partition.
*****************************************************************************/
double ModularityVertexPartition::quality()
{
  double mod = 0.0;
  double total_weight = this->graph_->total_weight();

  for (size_t c = 0; c < this->n_communities(); c++) {
    double internal_weight = this->total_weight_in_comm(c);
    double degree = this->_csize[c];
    mod += internal_weight - (degree * degree) / (2.0 * total_weight);
  }
  return mod / total_weight;
}

size_t ModularityVertexPartition::get_empty_community()
{
  if (this->_empty_communities.empty()) {
    // If there was no empty community yet,
    // we will create a new one.
    add_empty_community();
  }

  return this->_empty_communities.back();
}

void ModularityVertexPartition::set_membership(
    std::vector<size_t> const& membership)
{
  this->_membership = membership;

  this->clean_mem();
  this->init_admin();
}

size_t ModularityVertexPartition::add_empty_community()
{
  this->_n_communities = this->_n_communities + 1;

  if (this->_n_communities > this->graph_->numVertices())
    throw Exception(
        "There cannot be more communities than nodes, so there must already be "
        "an empty community.");

  size_t new_comm = this->_n_communities - 1;

  this->_csize.resize(this->_n_communities);
  this->_csize[new_comm] = 0;
  this->_cnodes.resize(this->_n_communities);
  this->_cnodes[new_comm] = 0;
  this->_total_weight_in_comm.resize(this->_n_communities);
  this->_total_weight_in_comm[new_comm] = 0;
  this->_total_weight_from_comm.resize(this->_n_communities);
  this->_total_weight_from_comm[new_comm] = 0;
  this->_total_weight_to_comm.resize(this->_n_communities);
  this->_total_weight_to_comm[new_comm] = 0;

  this->_cached_weight_all_community.resize(this->_n_communities);
  this->_cached_weight_from_community.resize(this->_n_communities);
  this->_cached_weight_to_community.resize(this->_n_communities);

  this->_empty_communities.push_back(new_comm);
  return new_comm;
}

void ModularityVertexPartition::cache_neigh_communities(size_t v)
{
  std::vector<double>* _cached_weight_tofrom_community = NULL;
  std::vector<size_t>* _cached_neighs_comms = NULL;
  _cached_weight_tofrom_community = &(this->_cached_weight_all_community);
  _cached_neighs_comms = &(this->_cached_neigh_comms_all);
  // Reset cached communities
  for (size_t c : *_cached_neighs_comms)
    (*_cached_weight_tofrom_community)[c] = 0;

  // Loop over all incident edges
  std::vector<size_t> const& neighbours = this->graph_->get_neighbours(v);
  std::vector<double> const& neighbour_edges
      = this->graph_->get_neighbour_edges(v);

  size_t degree = neighbours.size();

  // Reset cached neighbours
  _cached_neighs_comms->clear();
  for (size_t idx = 0; idx < degree; idx++) {
    size_t u = neighbours[idx];

    // If it is an edge to the requested community
    size_t comm = this->_membership[u];
    // Get the weight of the edge
    double w = neighbour_edges[idx];
    // Self loops appear twice here if the graph is undirected, so divide by 2.0
    // in that case.
    if (u == v)
      w /= 2.0;
    (*_cached_weight_tofrom_community)[comm] += w;
    // REMARK: Notice in the rare case of negative weights, being exactly equal
    // for a certain community, that this community may then potentially be
    // added multiple times to the _cached_neighs. However, I don' believe this
    // causes any further issue, so that's why I leave this here as is.
    if ((*_cached_weight_tofrom_community)[comm] != 0)
      _cached_neighs_comms->push_back(comm);
  }
}

std::vector<size_t> const& ModularityVertexPartition::get_neigh_comms(size_t v)
{
  if (this->_current_node_cache_community_all != v) {
    cache_neigh_communities(v);
    this->_current_node_cache_community_all = v;
  }
  return this->_cached_neigh_comms_all;
}

std::vector<size_t> ModularityVertexPartition::get_neigh_comms(
    size_t v,
    std::vector<size_t> const& constrained_membership)
{
  std::vector<size_t> neigh_comms;
  std::vector<bool> comm_added(this->n_communities(), false);
  for (size_t u : this->graph_->get_neighbours(v)) {
    if (constrained_membership[v] == constrained_membership[u]) {
      size_t comm = this->membership(u);
      if (!comm_added[comm]) {
        neigh_comms.push_back(comm);
        comm_added[comm];
      }
    }
  }
  return neigh_comms;
}

}  // namespace mpl2