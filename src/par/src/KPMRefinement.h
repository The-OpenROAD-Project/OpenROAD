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

#include <set>

#include "TPHypergraph.h"
#include "Utilities.h"
#include "utl/Logger.h"

namespace par {

// KPM = K-way Pair-wise FM (Fiduccia-Mattheyses algorithm)

using KPMpartition = std::pair<float, std::vector<std::vector<float>>>;

template <typename T>
using matrix = std::vector<std::vector<T>>;

class KPMRefinement
{
 public:
  KPMRefinement() = default;
  KPMRefinement(int num_parts,
                const std::vector<float>& e_wt_factors,
                float path_wt_factor,
                float snaking_wt_factor,
                float early_stop_ratio,
                int max_num_fm_pass,
                int seed,
                utl::Logger* logger)
  {
    num_parts_ = num_parts;
    e_wt_factors_ = e_wt_factors;
    path_wt_factor_ = path_wt_factor;
    snaking_wt_factor_ = snaking_wt_factor;
    early_stop_ratio_ = early_stop_ratio;
    max_num_fm_pass_ = max_num_fm_pass;
    seed_ = seed;
    logger_ = logger;
  }
  KPMRefinement(const KPMRefinement&) = default;
  KPMRefinement& operator=(const KPMRefinement&) = default;
  KPMRefinement(KPMRefinement&&) = default;
  KPMRefinement& operator=(KPMRefinement&&) = default;
  ~KPMRefinement() = default;

  // Golden Evaluator
  KPMpartition KPMevaluator(const HGraph hgraph,
                            std::vector<int>& solution,
                            bool print_flag = true);
  float KPMpass(const HGraph,
                std::vector<std::pair<int, int>>& partition_pairs,
                const matrix<float>& max_block_balance,
                std::vector<int>& solution);
  /*float KPMpass(const HGraph hgraph,
                const matrix<float>& max_block_balance,
                std::vector<int>& solution);*/
  void KPMrefinement(const HGraph hgraph,
                     const matrix<float>& max_block_balance,
                     std::vector<int>& solution);
  float KPMcalculateSpan(const HGraph hgraph,
                         int& from_pid,
                         int& to_pid,
                         std::vector<int>& solution);
  static std::vector<int> KPMfindBoundaryVertices(
      const HGraph hgraph,
      std::pair<int, int>& partition_pair,
      matrix<int>& net_degs);
  void SetKPMpartitionSeed(int seed) { seed_ = seed; }
  void SetKPMthresholdPasses(const int num_passes)
  {
    max_num_fm_pass_ = num_passes;
  }
  void SetKPMthresholdMoves(const int moves) { max_num_moves_ = moves; }
  int GetKPMPartitionSeed() const { return seed_; }
  void InitVisitFlags(int total_ele)
  {
    visit_.resize(total_ele);
    std::fill(visit_.begin(), visit_.end(), false);
  }
  void ResetVisitFlags(int total_ele)
  {
    std::fill(visit_.begin(), visit_.end(), false);
  }
  void InitBoundaryFlags(int total_ele)
  {
    boundary_.resize(total_ele);
    std::fill(boundary_.begin(), boundary_.end(), false);
  }
  void ResetBoundaryFlags(int total_ele)
  {
    std::fill(boundary_.begin(), boundary_.end(), false);
  }
  void ResetVisited(int v) { visit_[v] = false; }
  void MarkVisited(int v) { visit_[v] = true; }
  bool GetVisitStatus(int v) const { return visit_[v]; }
  void MarkBoundary(int v) { boundary_[v] = true; }
  bool GetBoundaryStatus(int v) const { return boundary_[v]; }

 private:
  class VertexGain
  {
   public:
    VertexGain()
    {
      status_ = false;
      vertex_ = -1;
      source_part_ = -1;
      potential_move_ = -1;
      gain_ = -1;
    }
    VertexGain(int arg_vertex, float arg_gain)
        : vertex_(arg_vertex), gain_(arg_gain)
    {
      potential_move_ = -1;
      status_ = true;
    }
    VertexGain(int arg_vertex, float arg_gain, int part)
        : vertex_(arg_vertex), source_part_(part), gain_(arg_gain)
    {
      potential_move_ = -1;
      status_ = true;
    }
    VertexGain(int arg_vertex,
               float arg_gain,
               std::map<int, float> arg_path_cost)
        : vertex_(arg_vertex), gain_(arg_gain), path_cost_(arg_path_cost)
    {
      source_part_ = -1;
      potential_move_ = -1;
      status_ = true;
    }
    VertexGain(int arg_vertex,
               float arg_gain,
               int part,
               std::map<int, float> arg_path_cost)
        : vertex_(arg_vertex),
          source_part_(part),
          gain_(arg_gain),
          path_cost_(arg_path_cost)
    {
      potential_move_ = -1;
      status_ = true;
    }
    VertexGain(const VertexGain&) = default;
    VertexGain& operator=(const VertexGain&) = default;
    VertexGain(VertexGain&&) = default;
    VertexGain& operator=(VertexGain&&) = default;
    bool operator<(const VertexGain& vertex_gain) const
    {
      if (gain_ > vertex_gain.gain_) {
        return true;
      } else if (gain_ == vertex_gain.gain_ && vertex_ < vertex_gain.vertex_) {
        return true;
      } else {
        return false;
      }
    }
    bool operator==(const VertexGain& vertex_gain) const
    {
      if (gain_ == vertex_gain.gain_ && path_cost_ == vertex_gain.path_cost_) {
        return true;
      } else {
        return false;
      }
    }
    int GetVertex() const { return vertex_; }
    int GetPotentialMove() const { return potential_move_; }
    float GetGain() const { return gain_; }
    bool GetStatus() const { return status_; }
    void SetVertex(int vertex) { vertex_ = vertex; }
    void SetGain(float gain) { gain_ = gain; }
    void SetActive() { status_ = true; }
    void SetDeactive() { status_ = false; }
    void SetPotentialMove(int move) { potential_move_ = move; }
    void SetPathCost(int path_id, float cost) { path_cost_[path_id] = cost; }
    float GetPathCost(int path_id) { return path_cost_[path_id]; }
    int GetTotalPaths() const { return path_cost_.size(); }
    int GetSourcePart() const { return source_part_; }

   private:
    int vertex_;
    int source_part_;
    int potential_move_;
    float gain_;
    bool status_;
    std::map<int, float>
        path_cost_;  // the updated path cost after moving vertex
  };

  // Priority queue implementation
  class PriorityQ
  {
   public:
    PriorityQ() = default;
    PriorityQ(int total_elements, HGraph hypergraph)
    {
      vertices_map_.resize(total_elements);
      std::fill(vertices_map_.begin(), vertices_map_.end(), -1);
      total_elements_ = 0;
      hypergraph_ = hypergraph;
      active_ = false;
    }
    PriorityQ(std::vector<std::shared_ptr<VertexGain>>& vertices,
              std::vector<int>& vertices_map)
        : vertices_(vertices), vertices_map_(vertices_map)
    {
    }
    PriorityQ(const PriorityQ&) = default;
    PriorityQ& operator=(const PriorityQ&) = default;
    PriorityQ(PriorityQ&&) = default;
    PriorityQ& operator=(PriorityQ&&) = default;
    ~PriorityQ() = default;
    inline void HeapifyUp(int index);
    void HeapifyDown(int index);
    void InsertIntoPQ(std::shared_ptr<VertexGain> element);
    std::shared_ptr<VertexGain> ExtractMax();
    void ChangePriority(int index, float priority);
    std::shared_ptr<VertexGain> GetMax() { return vertices_.front(); }
    void RemoveAt(int location);
    bool CheckIfEmpty() { return vertices_.empty(); }
    int GetTotalElements() const { return total_elements_; }
    int GetSizeOfPQ() const { return vertices_.size(); }
    int GetSizeOfMap() const { return vertices_map_.size(); }
    int GetLocationOfVertex(int v) const { return vertices_map_[v]; }
    std::shared_ptr<VertexGain> GetHeapVertex(int index)
    {
      return vertices_[index];
    }
    std::shared_ptr<VertexGain> GetHeapVertexFromId(int vertex_id)
    {
      assert(CheckIfVertexExists(vertex_id) == true);
      const int map_loc = GetLocationOfVertex(vertex_id);
      return vertices_[map_loc];
      // return &(vertices_[map_loc]);
    }
    void SetActive() { active_ = true; }
    void SetDeactive() { active_ = false; }
    bool GetStatus() const { return active_; }
    bool CheckIfVertexExists(int v)
    {
      return GetLocationOfVertex(v) > -1 ? true : false;
    }
    void Clear()
    {
      active_ = false;
      vertices_.clear();
      total_elements_ = 0;
      std::fill(vertices_map_.begin(), vertices_map_.end(), -1);
    }

   private:
    int Parent(int& element) { return floor((element - 1) / 2); }
    int LeftChild(int& element) { return ((2 * element) + 1); }
    int RightChild(int& element) { return ((2 * element) + 2); }

    bool active_;
    HGraph hypergraph_;
    std::vector<std::shared_ptr<VertexGain>> vertices_;
    std::vector<int> vertices_map_;
    int total_elements_;
  };

 public:
  // Alias for member classes PriorityQ, PQs and VertexGain
  using pq = PriorityQ;
  using vgain = VertexGain;
  using pqs = std::vector<std::shared_ptr<PriorityQ>>;
  // utility functions.
  // Get block balance
 private:
  inline matrix<float> KPMgetBlockBalance(const HGraph hgraph,
                                          std::vector<int>& solution);
  // update the net degree for existing solution
  // for each hyperedge, calculate the number of vertices in each part
  matrix<int> KPMgetNetDegrees(const HGraph hgraph, std::vector<int>& solution);
  // Calculate the cost for each timing path
  // In the default mode (v = -1, to_pid = -1),
  // we just calculate the cost for the timing path path_id
  // In the replacement mode, we replace the block id of v to to_pid
  float KPMcalculatePathCost(int path_id,
                             const HGraph hgraph,
                             const std::vector<int>& solution,
                             int v = -1,
                             int to_pid = -1);
  // Calculate the gain for a vertex v
  std::shared_ptr<VertexGain> KPMcalculateGain(
      int v,
      int from_pid,
      int to_pid,
      const HGraph hgraph,
      const std::vector<int>& solution,
      const std::vector<float>& cur_path_cost,
      const std::vector<std::vector<int>>& net_degs);
  // Initialize gain data structures between pairs of partitions
  void KPMinitialGainsBetweenPairs(const HGraph hgraph,
                                   const std::pair<int, int>& partition_pair,
                                   const std::vector<int>& boundary_vertices,
                                   const matrix<int>& net_degs,
                                   const std::vector<int>& solution,
                                   const std::vector<float>& cur_path_cost,
                                   pqs& gain_buckets);
  // Check if a move is legal or not
  bool KPMcheckLegality(HGraph hgraph,
                        int to,
                        std::shared_ptr<VertexGain> v,
                        matrix<float>& curr_block_balance,
                        const matrix<float>& max_block_balance);
  // Pick best vertex in a given move
  std::shared_ptr<VertexGain> KPMpickVertexToMove(
      HGraph hgraph,
      pqs& gain_buckets,
      matrix<float>& curr_block_balance,
      const matrix<float>& max_block_balance);
  // Accept the proposed move and make changes
  void KPMAcceptMove(std::shared_ptr<VertexGain> vertex_to_move,
                     HGraph hgraph,
                     std::vector<VertexGain>& moves_trace,
                     float& total_gain,
                     float& total_delta_gain,
                     std::pair<int, int>& partition_pair,
                     std::vector<int>& solution,
                     std::vector<float>& paths_cost,
                     matrix<float>& curr_block_balance,
                     pqs& gain_buckets,
                     matrix<int>& net_degs);
  // Function to return the pairwise combination of partitions that are tightly
  // connected
  std::vector<std::pair<int, int>> KPMfindPairs(
      const HGraph hgraph,
      std::vector<int>& solution,
      std::vector<float>& prev_scores);
  // Function to roll back moves post FM pass completion
  void KPMrollBackMoves(std::vector<VertexGain>& trace,
                        matrix<int>& net_degs,
                        int& best_move,
                        float& total_delta_gain,
                        pqs& gain_buckets,
                        HGraph hgraph,
                        matrix<float>& curr_block_balance,
                        std::vector<float>& cur_path_cost,
                        std::vector<int>& solution);
  // Find neighbors of a given vertex given a bipartition
  void KPMfindNeighbors(const HGraph hgraph,
                        int& vertex,
                        std::pair<int, int>& partition_pair,
                        std::vector<int>& solution,
                        std::set<int>& neighbors);
  void KPMupdateNeighbors(const HGraph hgraph,
                          const std::pair<int, int>& partition_pair,
                          const std::set<int>& neighbors,
                          const std::vector<int>& solution,
                          const std::vector<float>* cur_path_cost,
                          const matrix<int>& net_degs,
                          pqs& gain_buckets);
  // Function to chceck connectivity of a hyperedge
  inline int KPMgetConnectivity(const int& he,
                                const matrix<int>& net_degs) const;
  // Function to check if a vertex is lying on the boundary
  inline bool KPMcheckBoundaryVertex(const HGraph hgraph,
                                     const int& v,
                                     const std::pair<int, int> partition_pair,
                                     const matrix<int>& net_degs) const;
  int num_parts_ = 2;
  std::vector<float> e_wt_factors_;  // the cost weight for hyperedge weights
  float path_wt_factor_ = 0.0;       // the cost for cut of timing paths
  float snaking_wt_factor_ = 0.0;    // the cost for snaking timing paths
  float early_stop_ratio_
      = 0.01;  // The most improvements are from the earlier moves
  float max_num_moves_ = 50;  // The maximum number of moves in each pass
  int max_num_fm_pass_ = 10;  // number of fm pass
  int max_stagnation_ = 2;  // max stagnation allowed between successive passes
  int seed_ = 0;            // random seed
  utl::Logger* logger_ = nullptr;
  std::vector<bool> visit_;     // Vector to keep track of visited vertices
  std::vector<bool> boundary_;  // Vector to keep track of boundary vertices
};
// Alias for VertexGain and PQ classes so that we can define member functions
using kpm_heap = KPMRefinement::pq;
using vertex = KPMRefinement::vgain;
using kpm_heaps = KPMRefinement::pqs;
}  // namespace par
