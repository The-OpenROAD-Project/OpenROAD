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
#include "TPEvaluator.h"
#include "Utilities.h"
#include "utl/Logger.h"

namespace par {

// MATRIX is a two-dimensional vectors
template <typename T>
using MATRIX = std::vector<std::vector<T> >;
// TP_partition is the partitioning solution
using TP_partition = std::vector<int>; //
// TP_partition_token is the metrics of a given partition
// it consists of two part:  cost (cutsize), balance for each block
// for example, TP_partition_token.second[0] is the balance of 
// block_0
using TP_partition_token = std::pair<float, MATRIX<float> >;

// Vertex Gain is the basic elements of FM
// We do not use the classical gain-bucket data structure
// We design our own priority-queue based gain-bucket data structure
// to support float gain
class VertexGain;
using TP_gain_cell = std::shared_ptr<VertexGain>; // for abbreviation

class HyperedgeGain;
using TP_gain_hyperedge = std::shared_ptr<HyperedgeGain>;

// Priority-queue based gain bucket
class TPpriorityQueue;
using TP_gain_buckets = std::vector<std::shared_ptr<TPpriorityQueue>>;
using TP_gain_bucket = std::shared_ptr<TPpriorityQueue>;

class GoldenEvaluator;
using TP_evaluator_ptr = std::shared_ptr<GoldenEvaluator>;

// The algorithm we supported
enum class RefinerChoice
{
  GREEDY,  // greedy refinement. try to one entire hyperedge each time
  FLAT_K_WAY_FM, // direct k-way FM
  KPM_FM, // K-way pair-wise FM
  ILP_REFINE // ILP-based partitioning (only for two-way since k-way ILP partitioning is too timing-consuming)
};

class TPkWayFMRefine;
using TP_k_way_fm_refiner_ptr = std::shared_ptr<TPkWayFMRefine>;

class TPkWayPMRefine;
using TP_k_way_pm_refiner_ptr = std::shared_ptr<TPkWayPMRefine>;

class TPgreedyRefine;
using TP_greedy_refiner_ptr = std::shared_ptr<TPgreedyRefine>;

class TPilpRefine;
using TP_ilp_refiner_ptr = std::shared_ptr<TPilpRefine>;


// Vertex Gain is the basic elements of FM
// We do not use the classical gain-bucket data structure
// We design our own priority-queue based gain-bucket data structure
// to support float gain
class VertexGain
{
 public:
  
  // constructors
  VertexGain() { }

  VertexGain(const int vertex, 
             const int src_block_id,
             const int destination_block_id,
             const float gain,
             const std::map<int, float> path_cost) 
    : vertex_(vertex),
      source_part_(src_block_id),
      destination_part_(destination_block_id),
      gain_(gain),
      path_cost_(path_cost) 
  {
  }
      
  // accessor functions
  int GetVertex() const { return vertex_; }
  void SetVertex(int vertex) { vertex_ = vertex; }

  float GetGain() const { return gain_; }
  void SetGain(float gain) { gain_ = gain; }

  // get the delta path cost
  std::map<int, float> GetPathCost() const { return path_cost_; }      
  
  int GetSourcePart() const { return source_part_; }
  int GetDestinationPart() const { return destination_part_; }

 private:
  int vertex_ = -1; // vertex id
  int source_part_ = -1; // the source block id 
  int destination_part_ = -1; // the destination block id 
  float gain_ = -std::numeric_limits<float>::max(); // gain value of moving this vertex
  std::map<int, float> path_cost_;  // the updated DELTA path cost after moving vertex
                                    // the path_cost will change because we will dynamically update the
                                    // the weight of the path based on the number of the cut on the path
};


// Hyperedge Gain.
// Compared to VertexGain, there is no source_part_
// Because this hyperedge spans multiple blocks
class HyperedgeGain {
  public:
    HyperedgeGain() { }
    HyperedgeGain(const int hyperedge_id, 
                  const int destination_part,
                  const float gain,
                  const std::map<int, float>& path_cost) 
    {
      hyperedge_id_ = hyperedge_id;
      destination_part_ = destination_part;
      gain_ = gain;
      path_cost_ = path_cost;
    }

    
    float GetGain() const { return gain_; }
    void SetGain(float gain) { gain_ = gain; }
   
    int GetHyperedge() const { return hyperedge_id_; }

    int GetDestinationPart() const { return destination_part_; }
    
    const std::map<int, float> GetPathCost() const { return path_cost_; }      

  private:  
    int hyperedge_id_ = -1;  // hyperedge id
    int destination_part_ = -1; // the destination block id
    float gain_ = 0.0; // the initialization should be 0.0
    std::map<int, float> path_cost_;  // the updated DELTA path cost after moving vertex 
                                      // the path_cost will change because we will dynamically update the
                                      // the weight of the path based on the number of the cut on the path
};


// ------------------------------------------------------------
// Priority-queue based gain bucket (Only for VertexGain)
// Actually we implement the priority queue with Max Heap
// We did not use the STL priority queue becuase we need
// to record the location of each element (vertex gain)
// -------------------------------------------------------------
class TPpriorityQueue
{
 public:
  // constructors
  TPpriorityQueue() {  };

  TPpriorityQueue(int total_elements, int maximum_traverse_level, HGraphPtr hypergraph)
    : maximum_traverse_level_(maximum_traverse_level)
  {
    vertices_map_.resize(total_elements);
    std::fill(vertices_map_.begin(), vertices_map_.end(), -1);
    total_elements_ = 0;
    hypergraph_ = hypergraph;
    active_ = false;
  }

  // insert one element (std::shared_ptr<VertexGain>) into the priority queue
  void InsertIntoPQ(std::shared_ptr<VertexGain> element);

  // extract the largest element, i.e.,
  // get the largest element and remove it from the heap
  std::shared_ptr<VertexGain> ExtractMax();

  // get the largest element without removing it from the heap
  std::shared_ptr<VertexGain> GetMax() { return vertices_.front(); }

  // find the vertex gain which can satisfy the balance constraint
  std::shared_ptr<VertexGain> GetBestCandidate(
      const MATRIX<float>& curr_block_balance,
      const MATRIX<float>& upper_block_balance,
      const MATRIX<float>& lower_block_balance,
      const HGraphPtr hgraph);     

  // update the priority (gain) for the specified vertex
  void ChangePriority(int vertex_id, std::shared_ptr<VertexGain> new_element);

  // Remove the specified vertex 
  void Remove(int vertex_id);

  // Basic accessors
  bool CheckIfEmpty() const  { return vertices_.empty(); }
  int GetTotalElements() const { return total_elements_; }
  // the size of the max heap
  int GetSizeOfMap() const { return vertices_map_.size(); }
  // check if the vertex exists
  bool CheckIfVertexExists(int v) const
  {
    return vertices_map_[v] > -1 ? true : false;
  }
  // check the status of the heap
  void SetActive() { active_ = true; }
  void SetDeactive() { active_ = false; }
  bool GetStatus() const { return active_; }
  // clear the heap
  void Clear()
  {
    active_ = false;
    vertices_.clear();
    total_elements_ = 0;
    std::fill(vertices_map_.begin(), vertices_map_.end(), -1);
  }

 private:
  // The max heap (priority queue) is organized as a binary tree
  // Get parent, left child and right child index
  // Generic functions of max heap
  inline int Parent(int element) const {
    return std::floor((element - 1) / 2);
  }  
  
  inline int LeftChild(int& element) const { 
    return 2 * element + 1; 
  }

  inline int RightChild(int& element) const { 
    return 2 * element + 2; 
  }

  // This is called when we add a new element
  void HeapifyUp(int index);
  
  // This is called when we remove an existing element
  void HeapifyDown(int index);

  // Compare the two elements
  // If the gains are equal then pick the vertex with the smaller weight
  // The hope is doing this will incentivize in preventing corking effect
  bool CompareElementLargeThan(int index_a, int index_b);
  
  // private variables
  bool active_;
  HGraphPtr hypergraph_;
  std::vector<std::shared_ptr<VertexGain> > vertices_; // elements
  std::vector<int> vertices_map_; // store the location of vertex_gain for each vertex
                                  // vertices_map_ always has the size of hypergraph_->num_vertices_
  int total_elements_; // number of elements in the priority queue
  int maximum_traverse_level_ = 25; // the maximum level of traversing the buckets to solve the "corking effect"
};

// ------------------------------------------------------------------------
// The base class for refinement TPrefiner
// It implements the most basic functions for refinement
// and provides the basic parameters.
// Note that the TPrefiner is an operator class
// It should not modify the hypergraph itself
// ------------------------------------------------------------------------
class TPrefiner {
  public:
    TPrefiner(const int num_parts,
              const int refiner_iters, 
              const float path_wt_factor, // weight for cutting a critical timing path
              const float snaking_wt_factor, // weight for snaking timing paths
              const int max_move, // the maximum number of vertices or hyperedges can be moved in each pass
              TP_evaluator_ptr evaluator, // evaluator
              utl::Logger* logger)
        : num_parts_(num_parts),
          refiner_iters_(refiner_iters),
          path_wt_factor_(path_wt_factor),
          snaking_wt_factor_(snaking_wt_factor),
          max_move_(max_move),
          max_move_default_(max_move),
          refiner_iters_default_(refiner_iters)
    {
      evaluator_ = evaluator;
      logger_ = logger;
    }
    
    TPrefiner(const TPrefiner&) = delete;
    TPrefiner(TPrefiner&) = delete;
    virtual ~TPrefiner() = default;

    // The main function 
    void Refine(const HGraphPtr hgraph,
                const MATRIX<float>& upper_block_balance,
                const MATRIX<float>& lower_block_balance,
                TP_partition& solution);

    // accessor
    void SetMaxMove(int max_move) { 
      logger_->report("[INFO] Set the max_move to {}", max_move);
      max_move_ = max_move; 
    }

    void SetRefineIters(int refiner_iters) {
      logger_->report("[INFO] Set the refiner_iter to {}", refiner_iters);
      refiner_iters_ = refiner_iters; 
    }

    void RestoreDefaultParameters() {
      max_move_ = max_move_default_;
      refiner_iters_ = refiner_iters_default_;
      logger_->report("[INFO] Reset the max_move to {}", max_move_);
      logger_->report("[INFO] Reset the refiner_iters to {}", refiner_iters_);
    }

  protected:
    // protected functions
    virtual float Pass(const HGraphPtr hgraph,
                       const MATRIX<float>& upper_block_balance,
                       const MATRIX<float>& lower_block_balance,
                       MATRIX<float>& block_balance, // the current block balance
                       MATRIX<int>& net_degs, // the current net degree
                       std::vector<float>& paths_cost, // the current path cost
                       TP_partition& solution,
                       std::vector<bool>& visited_vertices_flag) { 
                        logger_->report("Warning: Call from the Refiner class");
                        return 0.0; }
 
    // By default, v = -1 and to_pid = -1
    // if to_pid == -1, we are calculate the current cost
    // of the path;
    // else if to_pid != -1, we are culculate the cost of the path
    // after moving v to block to_pid
    float CalculatePathCost(int path_id,
                            const HGraphPtr hgraph,
                            const TP_partition& solution,
                            int v = -1, // v = -1 by default
                            int to_pid = -1 // to_pid = -1 by default
                            ) const;

    // Find all the boundary vertices. The boundary vertices will not include any fixed vertices
    std::vector<int> FindBoundaryVertices(const HGraphPtr hgraph,
                                          const MATRIX<int>& net_degs,
                                          const std::vector<bool>& visited_vertices_flag
                                          ) const;

    std::vector<int> FindBoundaryVertices(const HGraphPtr hgraph,
                                          const MATRIX<int>& net_degs,
                                          const std::vector<bool>& visited_vertices_flag,
                                          const std::vector<int>& solution,
                                          const std::pair<int,int>& partition_pair) const;

    std::vector<int> FindNeighbors(const HGraphPtr hgraph, 
                                   const int vertex_id,
                                   const std::vector<bool>& visited_vertices_flag) const;
  
    std::vector<int> FindNeighbors(const HGraphPtr hgraph, 
                                   const int vertex_id, 
                                   const std::vector<bool>& visited_vertices_flag,
                                   const std::vector<int>& solution,
                                   const std::pair<int,int>& partition_pair) const;
                                                                                                         
    // Functions related to move a vertex and hyperedge
    // -----------------------------------------------------------
    // The most important function for refinent
    // If we want to update the score function for other purposes
    // we should update this function.
    // -----------------------------------------------------------
    // calculate the possible gain of moving a vertex
    // we need following arguments:
    // from_pid : from block id
    // to_pid : to block id
    // solution : the current solution
    // cur_path_cost : current path cost
    // net_degs : current net degrees
    TP_gain_cell CalculateVertexGain(int v,
                                     int from_pid,
                                     int to_pid,
                                     const HGraphPtr hgraph,
                                     const std::vector<int>& solution,
                                     const std::vector<float>& cur_path_cost,
                                     const MATRIX<int>& net_degs) const;

    // accept the vertex gain
    void AcceptVertexGain(TP_gain_cell gain_cell,
                          const HGraphPtr hgraph,
                          float& total_delta_gain,
                          std::vector<bool>& visited_vertices_flag,
                          std::vector<int>& solution,
                          std::vector<float>& cur_paths_cost,
                          MATRIX<float>& curr_block_balance,
                          MATRIX<int>& net_degs) const;


    // restore the vertex gain            
    void RollBackVertexGain(TP_gain_cell gain_cell,
                            const HGraphPtr hgraph,
                            std::vector<bool>& visited_vertices_flag,
                            std::vector<int>& solution,
                            std::vector<float>& cur_path_cost,
                            MATRIX<float>& curr_block_balance,
                            MATRIX<int>& net_degs) const;

    
    // check if we can move the vertex to some block
    bool CheckVertexMoveLegality(int v, // vertex_id
                                 int to_pid, // to block id
                                 int from_pid, // from block id
                                 const HGraphPtr hgraph, 
                                 const MATRIX<float>& curr_block_balance,
                                 const MATRIX<float>& upper_block_balance,
                                 const MATRIX<float>& lower_block_balance) const;
    
    // calculate the possible gain of moving a entire hyperedge
    // We can view the process of moving the vertices in hyperege
    // one by one, then restore the moving sequence to make sure that
    // the current status is not changed. Solution should not be const
    // calculate the possible gain of moving a hyperedge
    TP_gain_hyperedge CalculateHyperedgeGain(int hyperedge_id, 
                                             int to_pid,
                                             const HGraphPtr hgraph,
                                             std::vector<int>& solution,
                                             const std::vector<float>& cur_paths_cost,
                                             const MATRIX<int>& net_degs) const;

    // check if we can move the hyperegde into some block
    bool CheckHyperedgeMoveLegality(int e, // hyperedge id 
                                    int to_pid, // to block id
                                    const HGraphPtr hgraph,
                                    const std::vector<int>& solution,
                                    const MATRIX<float>& curr_block_balance,
                                    const MATRIX<float>& upper_block_balance,
                                    const MATRIX<float>& lower_block_balance) const;

    // accpet the hyperedge gain
    void AcceptHyperedgeGain(TP_gain_hyperedge hyperedge_gain,
                             const HGraphPtr hgraph,
                             float& total_delta_gain,
                             std::vector<int>& solution,
                             std::vector<float>& cur_paths_cost,
                             MATRIX<float>& curr_block_balance,
                             MATRIX<int>& net_degs) const;

    // Note that there is no RollBackHyperedgeGain
    // Because we only use greedy hyperedge refinement

    // user specified parameters
    const int num_parts_ = 2; // number of blocks in the partitioning
    int refiner_iters_ = 2; // number of refinement iterations
    const float path_wt_factor_ = 1.0; // the cost for cutting a critical timing path once. 
                                       // If a critical path is cut by 3 times,
                                       // the cost is defined as 3 * path_wt_factor_ * weight_of_path
    const float snaking_wt_factor_ = 1.0; // the cost of introducing a snaking timing path, see our paper for detailed explanation
                                          // of snaking timing paths
    int max_move_ = 50; // the maxinum number of vertices can be moved in each pass    
    // defualt parameters
    // during partitioning, we may need to update the value
    // of refiner_iters_ and max_move_ for the coarsest hypergraphs
    const int refiner_iters_default_ = 2;
    const int max_move_default_ = 50;
    
    utl::Logger* logger_ = nullptr;    
    TP_evaluator_ptr evaluator_ = nullptr;
};

// --------------------------------------------------------------------------
// FM-based direct k-way refinement
// --------------------------------------------------------------------------
class TPkWayFMRefine : public TPrefiner
{
  public:
    // We need the constructor here.  We have one more parameter related to "corking effect"
    TPkWayFMRefine(const int num_parts,
                   const int refiner_iters, 
                   const float path_wt_factor, // weight for cutting a critical timing path
                   const float snaking_wt_factor, // weight for snaking timing paths
                   const int max_move, // the maximum number of vertices or hyperedges can be moved in each pass
                   const int total_corking_passes,
                   TP_evaluator_ptr evaluator, // evaluator
                   utl::Logger* logger) 
      : total_corking_passes_(total_corking_passes),
        TPrefiner(num_parts, refiner_iters, path_wt_factor, snaking_wt_factor,
                  max_move, evaluator, logger)
      { }
    


    // Mark these two functions as public.
    // Because they will be called by multi-threading 
     // Initialize the single bucket
    void InitializeSingleGainBucket(
        TP_gain_buckets& buckets,
        int to_pid, // move the vertex into this block (block_id = to_pid)
        const HGraphPtr hgraph,
        const std::vector<int>& boundary_vertices,
        const MATRIX<int>& net_degs,
        const std::vector<float>& cur_paths_cost,
        const TP_partition& solution) const;


    // After moving one vertex, the gain of its neighbors will also need
    // to be updated. This function is used to update the gain of neighbor vertices
    // notices that the neighbors has been calculated based on solution, visited status,
    // boundary vertices status
    void UpdateSingleGainBucket(int part,
                                TP_gain_buckets& buckets,
                                const HGraphPtr hgraph,
                                const std::vector<int>& neighbors,
                                const MATRIX<int>& net_degs,
                                const std::vector<float>& cur_paths_cost,
                                const TP_partition& solution) const;



    protected:  
    // The main function for the FM-based refinement
    // In each pass, we only move the boundary vertices
    float Pass(const HGraphPtr hgraph,
               const MATRIX<float>& upper_block_balance,
               const MATRIX<float>& lower_block_balance,
               MATRIX<float>& block_balance, // the current block balance
               MATRIX<int>& net_degs, // the current net degree
               std::vector<float>& cur_paths_cost, // the current path cost
               TP_partition& solution,
               std::vector<bool>& visited_vertices_flag) override;

    // gain bucket related functions
    // Initialize the gain buckets in parallel
    void InitializeGainBucketsKWay(
        TP_gain_buckets& buckets,
        const HGraphPtr hgraph,
        const std::vector<int>& boundary_vertices,
        const MATRIX<int>& net_degs,
        const std::vector<float>& cur_paths_cost,
        const TP_partition& solution) const;
     
    // Determine which vertex gain to be picked
    std::shared_ptr<VertexGain> PickMoveKWay(
        TP_gain_buckets& buckets,
        const HGraphPtr hgraph,
        const MATRIX<float>& curr_block_balance,
        const MATRIX<float>& upper_block_balance,
        const MATRIX<float>& lower_block_balance) const;
  
    // move one vertex based on the calculated gain_cell
    void AcceptKWayMove(std::shared_ptr<VertexGain> gain_cell,
                        TP_gain_buckets& gain_buckets,
                        std::vector<TP_gain_cell>& moves_trace,
                        float& total_delta_gain,
                        std::vector<bool>& visited_vertices_flag,
                        HGraphPtr hgraph,
                        MATRIX<float>& curr_block_balance,
                        MATRIX<int>& net_degs,
                        std::vector<float>& cur_paths_cost,
                        std::vector<int>& solution) const;

    // Remove vertex from a heap
    // Remove the vertex id related vertex gain
    void HeapEleDeletion(int vertex_id,
                         int part,
                         TP_gain_buckets& buckets) const;

    // variables
    int total_corking_passes_ = 25; // the maximum level of traversing the buckets to solve the "corking effect"
};


// ------------------------------------------------------------------------------
// K-way pair-wise FM refinement
// ------------------------------------------------------------------------------
// The motivation is that FM can achieve better performance for 2-way partitioning
// than k-way partitioning. So we decompose the k-way partitioning into multiple 
// 2-way partitioning through maximum matching.
// In the first iteration, the maximum matching is based on connectivity between each
// blocks.  In the remaing iterations, the maximum matching is based on the delta gain
// for each block. Based on the paper of pair-wise PM, we can not keep using connectivity
// based maximum matching.  Otherwise, we may easily got stuck in local minimum.
// We use multiple multiple member functions of TPkWayFMRefine
// especially the functions related to gain buckets
class TPkWayPMRefine : public TPkWayFMRefine
{
  public:
    TPkWayPMRefine(const int num_parts,
                   const int refiner_iters, 
                   const float path_wt_factor, // weight for cutting a critical timing path
                   const float snaking_wt_factor, // weight for snaking timing paths
                   const int max_move, // the maximum number of vertices or hyperedges can be moved in each pass
                   const int total_corking_passes,
                   TP_evaluator_ptr evaluator, // evaluator
                   utl::Logger* logger) 
      : TPkWayFMRefine(num_parts, refiner_iters,
                       path_wt_factor, snaking_wt_factor,
                       max_move, total_corking_passes, 
                       evaluator, logger)
      { }
  
  private:
    // In each pass, we only move the boundary vertices
    // here we pass block_balance and net_degrees as reference
    // because we only move a few vertices during each pass
    // i.e., block_balance and net_degs will not change too much
    // so we precompute the block_balance and net_degs
    // the return value is the gain improvement
    float Pass(const HGraphPtr hgraph,
               const MATRIX<float>& upper_block_balance,
               const MATRIX<float>& lower_block_balance,
               MATRIX<float>& block_balance, // the current block balance
               MATRIX<int>& net_degs, // the current net degree
               std::vector<float>& cur_paths_cost, // the current path cost
               TP_partition& solution,
               std::vector<bool>& visited_vertices_flag) override;


    // The function to calculate the matching_scores
    void CalculateMaximumMatch(std::vector<std::pair<int, int> >& maximum_matches,
                               const std::map<std::pair<int, int>, float>& matching_scores) const;
    

    // Perform 2-way FM between blocks in partition pair
    float PerformPairFM(const HGraphPtr hgraph,
                        const MATRIX<float>& upper_block_balance,
                        const MATRIX<float>& lower_block_balance,
                        MATRIX<float>& block_balance, // the current block balance
                        MATRIX<int>& net_degs, // the current net degree
                        std::vector<float>& paths_cost, // the current path cost
                        TP_partition& solution,
                        TP_gain_buckets& buckets,
                        std::vector<bool>& visited_vertices_flag,
                        const std::pair<int, int>& partition_pair) const;

    // gain bucket related functions
    // Initialize the gain buckets in parallel
    void InitializeGainBucketsPM(TP_gain_buckets& buckets,
                                 const HGraphPtr hgraph,
                                 const std::vector<int>& boundary_vertices,
                                 const MATRIX<int>& net_degs,
                                 const std::vector<float>& cur_paths_cost,
                                 const TP_partition& solution,
                                 const std::pair<int, int>& partition_pair) const;
  

    // variables 
    // the connectivity between different blocks.
    // (block_a, block_b, score) where block_a < block_b
    std::map<std::pair<int, int>, float> pre_matching_connectivity_;
};

// ------------------------------------------------------------------------------
// K-way hyperedge greedy refinement
// Basically during hyperedge greedy refinement, we try to move the straddle 
// hyperedge into some block, to minimize the cost. 
// Moving the entire hyperedge can help to escape the local minimum caused 
// by moving vertex one by one
// ------------------------------------------------------------------------------
class TPgreedyRefine : public TPrefiner
{
  public:
  TPgreedyRefine(const int num_parts,
              const int refiner_iters, 
              const float path_wt_factor, // weight for cutting a critical timing path
              const float snaking_wt_factor, // weight for snaking timing paths
              const int max_move, // the maximum number of vertices or hyperedges can be moved in each pass
              TP_evaluator_ptr evaluator, // evaluator
              utl::Logger* logger) 
      : TPrefiner(num_parts, refiner_iters, 
                  path_wt_factor, snaking_wt_factor,
                  max_move, evaluator, logger)
      { }
  
  private:
    // In each pass, we only move the boundary vertices
    // here we pass block_balance and net_degrees as reference
    // because we only move a few vertices during each pass
    // i.e., block_balance and net_degs will not change too much
    // so we precompute the block_balance and net_degs
    // the return value is the gain improvement
    float Pass(const HGraphPtr hgraph,
               const MATRIX<float>& upper_block_balance,
               const MATRIX<float>& lower_block_balance,
               MATRIX<float>& block_balance, // the current block balance
               MATRIX<int>& net_degs, // the current net degree
               std::vector<float>& cur_paths_cost, // the current path cost
               TP_partition& solution,
               std::vector<bool>& visited_vertices_flag) override;
};


// ------------------------------------------------------------------------------
// K-way ILP Based Refinement
// ILP Based Refinement is usually very slow and cannot handle path related
// cost.  Please try to avoid using ILP-Based Refinement for K-way partitioning
// and path-related partitioning.  But ILP Based Refinement is good for 2=way
// min-cut problem
// --------------------------------------------------------------------------------
class TPilpRefine : public TPrefiner
{
  public:
  TPilpRefine(const int num_parts,
              const int refiner_iters, 
              const float path_wt_factor, // weight for cutting a critical timing path
              const float snaking_wt_factor, // weight for snaking timing paths
              const int max_move, // the maximum number of vertices or hyperedges can be moved in each pass
              TP_evaluator_ptr evaluator, // evaluator
              utl::Logger* logger) 
      : TPrefiner(num_parts, refiner_iters, 
                  path_wt_factor, snaking_wt_factor,
                  max_move, evaluator, logger)
      { }
  
  private:
    // In each pass, we only move the boundary vertices
    // here we pass block_balance and net_degrees as reference
    // because we only move a few vertices during each pass
    // i.e., block_balance and net_degs will not change too much
    // so we precompute the block_balance and net_degs
    // the return value is the gain improvement
    float Pass(const HGraphPtr hgraph,
               const MATRIX<float>& upper_block_balance,
               const MATRIX<float>& lower_block_balance,
               MATRIX<float>& block_balance, // the current block balance
               MATRIX<int>& net_degs, // the current net degree
               std::vector<float>& cur_paths_cost, // the current path cost
               TP_partition& solution,
               std::vector<bool>& visited_vertices_flag);
};

}  // namespace par
