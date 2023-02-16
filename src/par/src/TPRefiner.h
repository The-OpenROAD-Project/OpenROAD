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

template <typename T>
using matrix = std::vector<std::vector<T>>;
using TP_partition = std::vector<int>;
using TP_partition_token = std::pair<float, matrix<float>>;

enum RefinerChoice
{
  TWO_WAY_FM,
  GREEDY,
  FLAT_K_WAY_FM,
  KPM_FM,
  ILP_REFINE
};

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
  VertexGain(int arg_vertex, float arg_gain, std::map<int, float> arg_path_cost)
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
  std::map<int, float> path_cost_;  // the updated path cost after moving vertex
};

using TP_gain_cell = std::shared_ptr<VertexGain>;

class TPrefiner
{
 public:
  TPrefiner() = default;
  TPrefiner(const int num_parts,
            const int refiner_iters,
            const int refiner_choice,
            const int seed,
            const std::vector<float> e_wt_factors,
            const float path_wt_factor,
            const float snaking_wt_factor,
            utl::Logger* logger)
      : num_parts_(num_parts),
        refiner_iters_(refiner_iters),
        refiner_choice_(refiner_choice),
        seed_(seed),
        e_wt_factors_(e_wt_factors),
        path_wt_factor_(path_wt_factor),
        snaking_wt_factor_(snaking_wt_factor),
        tolerance_(0),
        logger_(logger)
  {
  }
  TPrefiner(const TPrefiner&) = default;
  TPrefiner(TPrefiner&&) = default;
  TPrefiner& operator=(const TPrefiner&) = default;
  TPrefiner& operator=(TPrefiner&&) = default;
  virtual ~TPrefiner() = default;
  virtual void BalancePartition(const HGraph hgraph,
                                const matrix<float>& max_block_balance,
                                std::vector<int>& solution)
      = 0;
  virtual void Refine(const HGraph hgraph,
                      const matrix<float>& max_block_balance,
                      TP_partition& solution)
      = 0;
  // virtual void BalancePartition(HGraph hgraph, TP_partition& solution);
  matrix<int> GetNetDegrees(const HGraph hgraph, std::vector<int>& solution);
  void FindNeighbors(const HGraph hgraph,
                     int& vertex,
                     std::pair<int, int>& partition_pair,
                     TP_partition& solution,
                     std::set<int>& neighbors,
                     bool k_flag);
  bool CheckBoundaryVertex(const HGraph hgraph,
                           const int& v,
                           const std::pair<int, int> partition_pair,
                           const matrix<int>& net_degs) const;
  std::vector<int> FindBoundaryVertices(const HGraph hgraph,
                                        matrix<int>& net_degs,
                                        std::pair<int, int>& partition_pair);
  TP_partition_token CutEvaluator(const HGraph hgraph,
                                  std::vector<int>& solution);
  float CalculatePathCost(int path_id,
                          const HGraph hgraph,
                          const std::vector<int>& solution,
                          int v = -1,
                          int to_pid = -1);
  void UpdateNeighboringPaths(const std::pair<int, int>& partition_pair,
                              const std::set<int>& neighbors,
                              const HGraph hgraph,
                              const TP_partition& solution,
                              std::vector<float>& paths_cost);
  matrix<float> GetBlockBalance(const HGraph hgraph,
                                std::vector<int>& solution);
  TP_gain_cell CalculateGain(int v,
                             int from_pid,
                             int to_pid,
                             const HGraph hgraph,
                             const std::vector<int>& solution,
                             const std::vector<float>& cur_path_cost,
                             const matrix<int>& net_degs);
  bool CheckLegality(const HGraph hgraph,
                     int to,
                     std::shared_ptr<VertexGain> v,
                     const matrix<float>& curr_block_balance,
                     const matrix<float>& max_block_balance);
  virtual void RollbackMoves(std::vector<VertexGain>& trace,
                             matrix<int>& net_degs,
                             int& best_move,
                             float& total_delta_gain,
                             HGraph hgraph,
                             matrix<float>& curr_block_balance,
                             std::vector<float>& cur_path_cost,
                             std::vector<int>& solution)
      = 0;
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
  void SetHeSizeSkip(const int he_size_skip)
  {
    thr_he_size_skip_ = he_size_skip;
  }
  int GetHeSizeSkip() const { return thr_he_size_skip_; }
  void SetSeed(const int seed) { seed_ = seed; }
  int GetSeed() const { return seed_; }
  int GetIters() const { return refiner_iters_; }
  void SetTolerance(float tol) { tolerance_ = tol; }
  float GetTolerance() const { return tolerance_; }
  int GetNumParts() const { return num_parts_; }
  int GetRefinerIters() const { return refiner_iters_; }
  int GetRefinerChoice() const { return refiner_choice_; }
  std::vector<float> GetEdgeWtFactors() const { return e_wt_factors_; }
  float GetPathWtFactor() const { return path_wt_factor_; }
  float GetSnakingWtFactor() const { return snaking_wt_factor_; }
  void GeneratePathsAsEdges(const HGraph hgraph);
  void ReweighTimingWeights(int path,
                            const HGraph hgraph,
                            std::vector<int>& solution,
                            std::vector<float>& path_cost);
  std::pair<int, int> GetTimingCuts(const HGraph hgraph,
                                    std::vector<int>& solution);
  inline void InitPathCuts(const HGraph hgraph,
                           std::vector<int>& path_cuts,
                           std::vector<int>& solution);
  void SetPathCuts(const int val)
  {
    path_cuts_.resize(val);
    std::fill(path_cuts_.begin(), path_cuts_.end(), 0);
  }
  int GetPathCuts(int pathid, const HGraph hgraph, std::vector<int>& solution);
  inline void InitPaths(const HGraph hgraph,
                        std::vector<float>& path_cost,
                        std::vector<int>& solution);
  utl::Logger* GetLogger() const { return logger_; }

 protected:
  int num_parts_;
  int refiner_iters_;
  int refiner_choice_;
  int seed_;
  int thr_he_size_skip_;
  std::vector<float> e_wt_factors_;
  std::vector<bool> boundary_;
  std::vector<bool> visit_;
  matrix<int> paths_;  // paths represented as set of hyperedges
  std::vector<int> path_cuts_;
  float path_wt_factor_;
  float snaking_wt_factor_;
  float tolerance_;
  utl::Logger* logger_ = nullptr;
};

// Priority queue implementation
class TPpriorityQueue
{
 public:
  TPpriorityQueue() = default;
  TPpriorityQueue(int total_elements, HGraph hypergraph)
  {
    vertices_map_.resize(total_elements);
    std::fill(vertices_map_.begin(), vertices_map_.end(), -1);
    total_elements_ = 0;
    hypergraph_ = hypergraph;
    active_ = false;
  }
  TPpriorityQueue(std::vector<std::shared_ptr<VertexGain>>& vertices,
                  std::vector<int>& vertices_map)
      : vertices_(vertices), vertices_map_(vertices_map)
  {
  }
  TPpriorityQueue(const TPpriorityQueue&) = default;
  TPpriorityQueue& operator=(const TPpriorityQueue&) = default;
  TPpriorityQueue(TPpriorityQueue&&) = default;
  TPpriorityQueue& operator=(TPpriorityQueue&&) = default;
  ~TPpriorityQueue() = default;
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

using TP_gain_buckets = std::vector<std::shared_ptr<TPpriorityQueue>>;
using TP_gain_bucket = std::shared_ptr<TPpriorityQueue>;

class TPtwoWayFM : public TPrefiner
{
 public:
  TPtwoWayFM() = default;
  TPtwoWayFM(const int num_parts,
             const int refiner_iters,
             const int max_moves,
             const int refiner_choice,
             const int seed,
             const std::vector<float> e_wt_factors,
             const float path_wt_factor,
             const float snaking_wt_factor,
             utl::Logger* logger)
      : TPrefiner(num_parts,
                  refiner_iters,
                  refiner_choice,
                  seed,
                  e_wt_factors,
                  path_wt_factor,
                  snaking_wt_factor,
                  logger)
  {
    max_moves_ = max_moves;
  }
  TPtwoWayFM(const TPtwoWayFM&) = default;
  TPtwoWayFM(TPtwoWayFM&&) = default;
  TPtwoWayFM& operator=(const TPtwoWayFM&) = default;
  TPtwoWayFM& operator=(TPtwoWayFM&&) = default;
  ~TPtwoWayFM() = default;
  void SetMaxPasses(const int passes) { refiner_iters_ = passes; }
  void BalancePartition(const HGraph hgraph,
                        const matrix<float>& max_block_balance,
                        std::vector<int>& solution) override;
  void Refine(const HGraph hgraph,
              const matrix<float>& max_block_balance,
              TP_partition& solution) override;
  void SetMaxMoves(const int moves) { max_moves_ = moves; }
  int GetMaxMoves() const { return max_moves_; }
  // void BalancePartition(HGraph hgraph, TP_partition& solution) override;

 private:
  std::shared_ptr<VertexGain> FindMovableVertex(
      const int corking_part,
      const HGraph hgraph,
      TP_gain_buckets& buckets,
      const matrix<float>& curr_block_balance,
      const matrix<float>& max_block_balance);
  std::shared_ptr<VertexGain> SolveCorkingEffect(
      const int corking_part,
      const HGraph hgraph,
      TP_gain_buckets& buckets,
      const matrix<float>& curr_block_balance,
      const matrix<float>& max_block_balance);
  std::shared_ptr<VertexGain> PickMoveTwoWay(
      const HGraph hgraph,
      TP_gain_buckets& buckets,
      const matrix<float>& curr_block_balance,
      const matrix<float>& max_block_balance);
  void RollbackMoves(std::vector<VertexGain>& trace,
                     matrix<int>& net_degs,
                     int& best_move,
                     float& total_delta_gain,
                     HGraph hgraph,
                     matrix<float>& curr_block_balance,
                     std::vector<float>& cur_path_cost,
                     std::vector<int>& solution) override;
  void UpdateNeighbors(const HGraph hgraph,
                       const std::pair<int, int>& partition_pair,
                       const std::set<int>& neighbors,
                       const TP_partition& solution,
                       const std::vector<float>& cur_path_cost,
                       const matrix<int>& net_degs,
                       TP_gain_buckets& gain_buckets);
  void AcceptTwoWayMove(std::shared_ptr<VertexGain> gain_cell,
                        HGraph hgraph,
                        std::vector<VertexGain>& moves_trace,
                        float& total_gain,
                        float& total_delta_gain,
                        std::pair<int, int>& partition_pair,
                        std::vector<int>& solution,
                        std::vector<float>& paths_cost,
                        matrix<float>& curr_block_balance,
                        TP_gain_buckets& gain_buckets,
                        matrix<int>& net_degs);
  void InitGainBucketsTwoWay(const HGraph hgraph,
                             TP_partition& solution,
                             const matrix<int> net_degs,
                             const std::vector<int>& boundary_vertices,
                             const std::vector<float>& cur_path_cost,
                             TP_gain_buckets& buckets);
  float Pass(const HGraph hgraph,
             const matrix<float>& max_block_balance,
             TP_partition& solution,
             std::vector<float>& paths_cost);
  int max_moves_;
};

using TP_two_way_refining_ptr = std::shared_ptr<TPtwoWayFM>;

class TPkWayFM : public TPrefiner
{
 public:
  TPkWayFM() = default;
  TPkWayFM(const int num_parts,
           const int refiner_iters,
           const int max_moves,
           const int refiner_choice,
           const int seed,
           const std::vector<float> e_wt_factors,
           const float path_wt_factor,
           const float snaking_wt_factor,
           utl::Logger* logger)
      : TPrefiner(num_parts,
                  refiner_iters,
                  refiner_choice,
                  seed,
                  e_wt_factors,
                  path_wt_factor,
                  snaking_wt_factor,
                  logger)
  {
    max_moves_ = max_moves;
  }
  TPkWayFM(const TPkWayFM&) = default;
  TPkWayFM(TPkWayFM&&) = default;
  TPkWayFM& operator=(const TPkWayFM&) = default;
  TPkWayFM& operator=(TPkWayFM&&) = default;
  ~TPkWayFM() = default;
  void Refine(const HGraph hgraph,
              const matrix<float>& max_block_balance,
              TP_partition& solution) override;
  void SetMaxMoves(const int moves) { max_moves_ = moves; }
  int GetMaxMoves() const { return max_moves_; }
  void BalancePartition(const HGraph hgraph,
                        const matrix<float>& max_block_balance,
                        TP_partition& solution) override
  {
  }

 private:
  float Pass(const HGraph hgraph,
             const matrix<float>& max_block_balance,
             TP_partition& solution,
             std::vector<float>& paths_cost);
  void HeapEleDeletion(int vertex_id, int part, TP_gain_buckets& buckets);
  void InitializeSingleGainBucket(TP_gain_buckets& buckets,
                                  int to_pid,
                                  const std::vector<int>& boundary_vertices,
                                  const HGraph hgraph,
                                  const TP_partition& solution,
                                  const std::vector<float>& cur_path_cost,
                                  const matrix<int>& net_degs);
  void UpdateSingleGainBucket(int part,
                              const std::set<int>& neighbors,
                              TP_gain_buckets& buckets,
                              const HGraph hgraph,
                              const TP_partition& solution,
                              const std::vector<float>& cur_path_cost,
                              const matrix<int>& net_degs);
  void InitializeGainBucketsKWay(const HGraph hgraph,
                                 const TP_partition& solution,
                                 const matrix<int>& net_degs,
                                 const std::vector<int>& boundary_vertices,
                                 const std::vector<float>& cur_path_cost,
                                 TP_gain_buckets& buckets);
  void AcceptKWayMove(std::shared_ptr<VertexGain> gain_cell,
                      HGraph hgraph,
                      std::vector<VertexGain>& moves_trace,
                      float& total_gain,
                      float& total_delta_gain,
                      std::pair<int, int>& partition_pair,
                      std::vector<int>& solution,
                      std::vector<float>& paths_cost,
                      matrix<float>& curr_block_balance,
                      TP_gain_buckets& gain_buckets,
                      matrix<int>& net_degs);
  void RollbackMoves(std::vector<VertexGain>& trace,
                     matrix<int>& net_degs,
                     int& best_move,
                     float& total_delta_gain,
                     HGraph hgraph,
                     matrix<float>& curr_block_balance,
                     std::vector<float>& cur_path_cost,
                     std::vector<int>& solution) override
  {
  }
  void RollbackMovesKWay(std::vector<VertexGain>& trace,
                         matrix<int>& net_degs,
                         int& best_move,
                         float& total_delta_gain,
                         HGraph hgraph,
                         matrix<float>& curr_block_balance,
                         std::vector<float>& cur_path_cost,
                         std::vector<int>& solution,
                         std::vector<int>& partition_trace);
  std::shared_ptr<VertexGain> PickMoveKWay(
      const HGraph hgraph,
      TP_gain_buckets& buckets,
      const matrix<float>& curr_block_balance,
      const matrix<float>& max_block_balance);
  int max_moves_;
};

using TP_k_way_refining_ptr = std::shared_ptr<TPkWayFM>;

class TPgreedyRefine : public TPrefiner
{
 public:
  TPgreedyRefine() = default;
  TPgreedyRefine(const int num_parts,
                 const int refiner_iters,
                 const int max_moves,
                 const int refiner_choice,
                 const int seed,
                 const std::vector<float> e_wt_factors,
                 const float path_wt_factor,
                 const float snaking_wt_factor,
                 utl::Logger* logger)
      : TPrefiner(num_parts,
                  refiner_iters,
                  refiner_choice,
                  seed,
                  e_wt_factors,
                  path_wt_factor,
                  snaking_wt_factor,
                  logger),
        max_moves_(0)
  {
  }
  TPgreedyRefine(const TPgreedyRefine&) = default;
  TPgreedyRefine(TPgreedyRefine&&) = default;
  TPgreedyRefine& operator=(const TPgreedyRefine&) = default;
  TPgreedyRefine& operator=(TPgreedyRefine&&) = default;
  ~TPgreedyRefine() = default;
  void SetMaxMoves(const int moves) { max_moves_ = moves; }
  int GetMaxMoves() const { return max_moves_; }
  void Refine(const HGraph hgraph,
              const matrix<float>& max_vertex_balance,
              TP_partition& solution) override;
  void BalancePartition(const HGraph hgraph,
                        const matrix<float>& max_block_balance,
                        std::vector<int>& solution) override
  {
  }
  void RollbackMoves(std::vector<VertexGain>& trace,
                     matrix<int>& net_degs,
                     int& best_move,
                     float& total_delta_gain,
                     HGraph hgraph,
                     matrix<float>& curr_block_balance,
                     std::vector<float>& cur_path_cost,
                     std::vector<int>& solution) override
  {
  }

 private:
  float CalculateGain(HGraph hgraph,
                      int straddle_he,
                      int from,
                      int to,
                      std::vector<int>& straddle,
                      matrix<int>& net_degs);
  void CommitStraddle(int to,
                      HGraph hgraph,
                      std::vector<int>& straddle,
                      TP_partition& solution,
                      matrix<int>& net_degs,
                      matrix<float>& block_balance);
  float MoveStraddle(int he,
                     std::vector<int>& straddle_0,
                     std::vector<int>& straddle_1,
                     HGraph hgraph,
                     TP_partition& solution,
                     matrix<int>& net_degs,
                     matrix<float>& block_balance,
                     const matrix<float>& max_block_balance);
  float Pass(HGraph hgraph,
             TP_partition& solution,
             const matrix<float>& max_vertex_balance);
  int max_moves_;
};

using TP_greedy_refiner_ptr = std::shared_ptr<TPgreedyRefine>;

class TPilpGraph
{
 public:
  TPilpGraph() = default;
  TPilpGraph(const TPilpGraph&) = default;
  TPilpGraph(TPilpGraph&&) = default;
  TPilpGraph& operator=(const TPilpGraph&) = default;
  TPilpGraph& operator=(TPilpGraph&&) = default;
  ~TPilpGraph() = default;
  TPilpGraph(const int vertex_dimensions,
             const int hyperedge_dimensions,
             const bool fixed_flag,
             std::vector<int> fixed,
             const matrix<int> hyperedges,
             const matrix<float> vertex_weights,
             const matrix<float> hyperedge_weights)
  {
    num_vertices_ = static_cast<int>(vertex_weights.size());
    num_hyperedges_ = static_cast<int>(hyperedge_weights.size());
    vertex_dimensions_ = vertex_dimensions;
    hyperedge_dimensions_ = hyperedge_dimensions;
    vertex_weights_ = vertex_weights;
    hyperedge_weights_ = hyperedge_weights;
    fixed_flag_ = fixed_flag;
    fixed_ = fixed;
    // hyperedges
    eind_.clear();
    eptr_.clear();
    eptr_.push_back(static_cast<int>(eind_.size()));
    for (const auto& hyperedge : hyperedges) {
      eind_.insert(eind_.end(), hyperedge.begin(), hyperedge.end());
      eptr_.push_back(static_cast<int>(eind_.size()));
    }
  }
  inline const int GetVertexDimensions() const { return vertex_dimensions_; }
  inline const int GetHyperedgeDimensions() const
  {
    return hyperedge_dimensions_;
  }
  inline const int GetNumVertices() const { return num_vertices_; }
  inline const int GetNumHyperedges() const { return num_hyperedges_; }
  inline std::pair<int, int> GetEdgeIndices(const int he) const
  {
    return std::make_pair(eptr_[he], eptr_[he + 1]);
  }
  inline std::vector<float> const& GetVertexWeight(const int& v) const
  {
    return vertex_weights_[v];
  }
  inline std::vector<float> const& GetHyperedgeWeight(const int& e) const
  {
    return hyperedge_weights_[e];
  }
  inline bool CheckFixedStatus(const int v) const { return fixed_[v] > -1; }
  inline int GetFixedPart(const int v) const { return fixed_[v]; }
  inline bool CheckFixedFlag() const { return fixed_flag_; }
  inline std::vector<float> GetTotalVertexWeights()
  {
    std::vector<float> total_wt(GetVertexDimensions(), 0.0);
    for (auto vWt : vertex_weights_) {
      total_wt = total_wt + vWt;
    }
    return total_wt;
  }
  std::vector<int> eind_;

 private:
  int hyperedge_dimensions_;
  int vertex_dimensions_;
  int num_vertices_;
  int num_hyperedges_;
  bool fixed_flag_;
  std::vector<int> fixed_;
  std::vector<int> eptr_;
  matrix<float> vertex_weights_;
  matrix<float> hyperedge_weights_;
};

class TPilpRefine : public TPrefiner
{
 public:
  TPilpRefine() = default;
  TPilpRefine(const TPilpRefine&) = default;
  TPilpRefine(TPilpRefine&&) = default;
  TPilpRefine& operator=(const TPilpRefine&) = default;
  TPilpRefine& operator=(TPilpRefine&&) = default;
  ~TPilpRefine() = default;
  TPilpRefine(const int num_parts,
              const int refiner_iters,
              const int max_moves,
              const int refiner_choice,
              const int seed,
              const std::vector<float> e_wt_factors,
              const float path_wt_factor,
              const float snaking_wt_factor,
              utl::Logger* logger,
              int wavefront)
      : TPrefiner(num_parts,
                  refiner_iters,
                  refiner_choice,
                  seed,
                  e_wt_factors,
                  path_wt_factor,
                  snaking_wt_factor,
                  logger)
  {
    wavefront_ = wavefront;
  }
  void SolveIlpInstanceOR(std::shared_ptr<TPilpGraph> hgraph,
                          TP_partition& refined_partition,
                          const matrix<float>& max_block_balance);
  void SolveIlpInstance(std::shared_ptr<TPilpGraph> hgraph,
                        TP_partition& refined_partition,
                        const matrix<float>& max_block_balance);
  void Refine(const HGraph hgraph,
              const matrix<float>& max_vertex_balance,
              TP_partition& solution) override;
  void BalancePartition(const HGraph hgraph,
                        const matrix<float>& max_block_balance,
                        std::vector<int>& solution) override
  {
  }
  void RollbackMoves(std::vector<VertexGain>& trace,
                     matrix<int>& net_degs,
                     int& best_move,
                     float& total_delta_gain,
                     HGraph hgraph,
                     matrix<float>& curr_block_balance,
                     std::vector<float>& cur_path_cost,
                     std::vector<int>& solution) override
  {
  }
  void SetHyperedgeThr(const int thr) { he_thr_ = thr; }
  int GetHyperedgeThr() const { return he_thr_; }
  int GetWavefront() const { return wavefront_; }

 private:
  inline void Remap(std::vector<int>& partition,
                    std::vector<int>& refined_partition);
  std::shared_ptr<TPilpGraph> ContractHypergraph(HGraph hgraph,
                                                 TP_partition& solution,
                                                 int wavefront);
  void OrderVertexSet(HGraph hgraph,
                      std::vector<int>& vertices,
                      TP_partition& solution,
                      matrix<int>& net_degs,
                      matrix<float>& block_balance,
                      const matrix<float>& max_block_balance);
  void ContractNonBoundary(HGraph hgraph,
                           std::vector<int>& vertices,
                           TP_partition& solution);
  float CalculateGain(int vertex,
                      TP_partition& solution,
                      HGraph hgraph,
                      matrix<int>& net_degs);
  int wavefront_;
  int he_thr_;
  std::vector<int> cluster_map_;
};

using TP_ilp_refiner_ptr = std::shared_ptr<TPilpRefine>;

// structure to save partition pair for pairwise FM
/*
class TPpartitionPair
{
 public:
  TPpartitionPair() = default;
  TPpartitionPair(const i, const int x, const int y, const float xcon)
      : pair_id_(i), pair_x_(x), pair_y_(y), connectivity_(xcon)
  {
  }
  TPpartitionPair(const TPpartitionPair&) = default;
  TPpartitionPair(TPpartitionPair&&) = default;
  TPpartitionPair& operator=(const TPpartitionPair&) = default;
  TPpartitionPair& operator=(TPpartitionPair&&) = default;
  ~TPpartitionPair() = default;\
  void SetId(const int i) { pair_id_ = i; }
  int GetId() const { return pair_id_; }
  void SetPairX(const int x) { pair_x_ = x; }
  void SetPairY(const int y) { pair_y_ = y; }
  int GetPairX() const { return pair_x_; }
  int GetPairY() const { return pair_y_; }
  void SetConnectivity(const float connection) { connectivity_ = connection; }
  float GetConnectivity() const { return connectivity_; }

 private:
  int pair_id_;
  int pair_x_;
  int pair_y_;
  float connectivity_;
};

using TP_partition_pair_ptr = std::shared_ptr<TPpartitionPair>;

class TPkpm : public TPrefiner, public TPtwoWayFM
{
 public:
  TPkpm() = default;
  TPkpm(const int num_parts,
        const int refiner_iters,
        const int max_moves,
        const int refiner_choice,
        const int seed,
        const std::vector<float> e_wt_factors,
        const float path_wt_factor,
        const float snaking_wt_factor,
        utl::Logger* logger)
      : TPrefiner(num_parts,
                  refiner_iters,
                  refiner_choice,
                  seed,
                  e_wt_factors,
                  path_wt_factor,
                  snaking_wt_factor,
                  logger)
  {
    max_moves_ = max_moves;
  }
  TPkpm(const TPkpm&) = default;
  TPkpm(TPkpm&&) = default;
  TPkpm& operator=(const TPkpm&) = default;
  TPkpm& operator=(TPkpm&&) = default;
  ~TPkpm() = default;
  void Refine(const HGraph hgraph,
              const matrix<float>& max_vertex_balance,
              TP_partition& solution) override;
  void BalancePartition(const HGraph hgraph,
                        const matrix<float>& max_block_balance,
                        std::vector<int>& solution) override;

 private:
  void FindTightlyConnectedPairs(const HGraph hgraph,
                                 std::vector<int>& solution);
  float CalculateSpan(const HGraph hgraph,
                      int& from_pid,
                      int& to_pid,
                      std::vector<int>& solution);
  int max_moves_;
};*/
}  // namespace par
