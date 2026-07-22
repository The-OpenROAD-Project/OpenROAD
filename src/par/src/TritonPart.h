// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once
#include <string>
#include <vector>

#include "Coarsener.h"
#include "Hypergraph.h"
#include "Utilities.h"
#include "db_sta/dbReadVerilog.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "utl/Logger.h"

namespace par {

// The TritonPart Interface
// TritonPart is a state-of-the-art hypergraph and netlist partitioner that
// replaces previous engines such as hMETIS.
// The TritonPart is designed for VLSI CAD, thus it can
// understand all kinds of constraints and timing information.
// TritonPart can accept two types of input hypergraphs
// Type 1:  Generate the hypergraph by traversing the netlist.
//          Each bterm & inst is modeled as a vertex.
//          Each signal net is modeled as a hyperedge.
// Type 2:  Take the input hypergraph as an argument in the same manner as
//          hMetis.

class TritonPart
{
 public:
  TritonPart(sta::dbNetwork* network,
             odb::dbDatabase* db,
             sta::dbSta* sta,
             utl::Logger* logger);

  // Top level interface
  // The function for partitioning a hypergraph
  // This is the main API for TritonPart
  // Key supports:
  // (1) fixed vertices constraint in fixed_file
  // (2) community attributes in community_file (This can be used to guide the
  // partitioning process) (3) stay together attributes in group_file. (4)
  // timing-driven partitioning (5) fence-aware partitioning (6) placement-aware
  // partitioning, placement information is extracted from OpenDB
  void PartitionDesign(unsigned int num_parts_arg,
                       float balance_constraint_arg,
                       const std::vector<float>& base_balance_arg,
                       const std::vector<float>& scale_factor_arg,
                       unsigned int seed_arg,
                       bool timing_aware_flag_arg,
                       int top_n_arg,
                       bool placement_flag_arg,
                       bool fence_flag_arg,
                       float fence_lx_arg,
                       float fence_ly_arg,
                       float fence_ux_arg,
                       float fence_uy_arg,
                       const char* fixed_file_arg,
                       const char* community_file_arg,
                       const char* group_file_arg,
                       const char* solution_filename_arg);

  // Function to evaluate the hypergraph partitioning solution
  // This can be used to write the timing-weighted hypergraph
  // and evaluate the solution.
  // If the solution file is empty, then this function is to write the
  // solution. If the solution file is not empty, then this function is to
  // evaluate the solution without writing the hypergraph again This function is
  // only used for testing
  void EvaluatePartDesignSolution(unsigned int num_parts_arg,
                                  float balance_constraint_arg,
                                  const std::vector<float>& base_balance_arg,
                                  const std::vector<float>& scale_factor_arg,
                                  bool timing_aware_flag_arg,
                                  int top_n_arg,
                                  bool fence_flag_arg,
                                  float fence_lx_arg,
                                  float fence_ly_arg,
                                  float fence_ux_arg,
                                  float fence_uy_arg,
                                  const char* fixed_file_arg,
                                  const char* community_file_arg,
                                  const char* group_file_arg,
                                  const char* hypergraph_file,
                                  const char* hypergraph_int_weight_file,
                                  const char* solution_filename_arg);

  // The function for partitioning a hypergraph
  // This is used for replacing hMETIS
  // Key supports:
  // (1) fixed vertices constraint in fixed_file
  // (2) community attributes in community_file (This can be used to guide the
  // partitioning process) (3) stay together attributes in group_file. (4)
  // placement information is specified in placement file The format is that
  // each line cooresponds to a group fixed vertices, community and placement
  // attributes both follows the hMETIS format
  void PartitionHypergraph(unsigned int num_parts,
                           float balance_constraint,
                           const std::vector<float>& base_balance,
                           const std::vector<float>& scale_factor,
                           unsigned int seed,
                           int vertex_dimension,
                           int hyperedge_dimension,
                           int placement_dimension,
                           const char* hypergraph_file,
                           const char* fixed_file,
                           const char* community_file,
                           const char* group_file,
                           const char* placement_file);

  // Evaluate a given solution of a hypergraph
  // The fixed vertices should statisfy the fixed vertices constraint
  // The group of vertices should stay together in the solution
  // The vertex balance should be satisfied
  void EvaluateHypergraphSolution(unsigned int num_parts,
                                  float balance_constraint,
                                  const std::vector<float>& base_balance,
                                  const std::vector<float>& scale_factor,
                                  int vertex_dimension,
                                  int hyperedge_dimension,
                                  const char* hypergraph_file,
                                  const char* fixed_file,
                                  const char* group_file,
                                  const char* solution_file);

  // k-way partitioning used by Hier-RTLMP
  std::vector<int> PartitionKWaySimpleMode(
      unsigned int num_parts_arg,
      float balance_constraint_arg,
      unsigned int seed_arg,
      const std::vector<std::vector<int>>& hyperedges,
      const std::vector<float>& vertex_weights,
      const std::vector<float>& hyperedge_weights);

  // Main APIs
  void SetTimingParams(float net_timing_factor,
                       float path_timing_factor,
                       float path_snaking_factor,
                       float timing_exp_factor,
                       float extra_delay,
                       bool guardband_flag);

  // The cost introduced by a cut hyperedge e is e_wt_factors
  // dot_product hyperedge_weights_[e]. This parameter is used by
  // coarsening and partitioning
  void SetNetWeight(const std::vector<float>& e_wt_factors)
  {
    e_wt_factors_ = e_wt_factors;
  }

  // The ``weight'' of a vertex. For placement-driven coarsening, when
  // we merge two vertices, we need to update the location of merged
  // vertex based on the gravity center of these two vertices.  The
  // weight of vertex v is defined as v_wt_factors dot_product
  // vertex_weights_[v] this parameter is only used in coarsening
  void SetVertexWeight(const std::vector<float>& v_wt_factors)
  {
    v_wt_factors_ = v_wt_factors;
  }

  // The weight for placement information. For placement-driven
  // coarsening, when we calculate the score for best-choice
  // coarsening, the placement information also contributes to the
  // score function. we prefer to merge two vertices which are
  // adjacent physically the distance between u and v is defined as
  // norm2(placement_attr[u] - placement_attr[v],
  // placement_wt_factors_) this parameter is only used during
  // coarsening
  void SetPlacementWeight(const std::vector<float>& placement_wt_factors)
  {
    placement_wt_factors_ = placement_wt_factors;
  }

  // Set detailed parameters
  // There parameters only used by users who want to exploit the performance
  // limits of TritonPart
  void SetFineTuneParams(
      // coarsening related parameters
      int thr_coarsen_hyperedge_size_skip,
      int thr_coarsen_vertices,
      int thr_coarsen_hyperedges,
      float coarsening_ratio,
      int max_coarsen_iters,
      float adj_diff_ratio,
      int min_num_vertices_each_part,
      // initial partitioning related parameters
      int num_initial_solutions,
      int num_best_initial_solutions,
      // refinement related parameters
      int refiner_iters,
      int max_moves,
      float early_stop_ratio,
      int total_corking_passes,
      // vcycle related parameters
      bool v_cycle_flag,
      int max_num_vcycle,
      int num_coarsen_solutions,
      int num_vertices_threshold_ilp,
      int global_net_threshold);

 private:
  // Main partititon function
  void MultiLevelPartition();

  // read and build hypergraph
  void ReadHypergraph(const std::string& hypergraph,
                      const std::string& fixed_file,
                      const std::string& community_file,
                      const std::string& group_file,
                      const std::string& placement_file);

  // read and build netlist
  // placement information is extracted from the OpenDB database
  void ReadNetlist(const std::string& fixed_file,
                   const std::string& community_file,
                   const std::string& group_file);
  void BuildTimingPaths();  // Find all the critical timing paths

  void informFiles(const std::string& fixed_file,
                   const std::string& community_file,
                   const std::string& group_file,
                   const std::string& placement_file,
                   const std::string& hypergraph_file,
                   const std::string& hypergraph_int_weight_file,
                   const std::string& solution_file);

  float computeMicronArea(odb::dbInst* inst);

  // private member functions
  sta::dbNetwork* network_ = nullptr;
  odb::dbDatabase* db_ = nullptr;
  odb::dbBlock* block_ = nullptr;
  sta::dbSta* sta_ = nullptr;

  // user-specified parameters

  // constraints information
  float ub_factor_ = 1.0;            // balance constraint
  int num_parts_ = 2;                // number of partitions
  std::vector<float> base_balance_;  // the target balance for each block
  std::vector<float> scale_factor_;  // the scale factor for each block
                                     // (multi-technology-nodes)

  // random seed
  int seed_ = 0;

  // ---- support for partitioning design with placed information
  // ---- for example, pin-3D flow
  bool placement_flag_
      = false;  // if use placement information to guide partitioning
  int placement_dimensions_ = 2;  // by default, we are working on 2D canvas
  std::vector<std::vector<float>>
      placement_attr_;  // (internal representation, size is num_vertices_)

  bool fence_flag_ = false;  // if use fence constraint
  Rect fence_{0, 0, 0, 0};   // only consider the netlist within the fence

  // ---- timing information
  bool timing_aware_flag_ = true;  // Enable timing aware
  int top_n_ = 1000;               // top_n timing paths
  float maximum_clock_period_ = 0.0;
  std::vector<float> hyperedge_slacks_;   // normalized by clock period
  std::vector<VertexType> vertex_types_;  // the vertex type of each instances
  std::vector<TimingPath>
      timing_paths_;  // critical timing paths, extracted based OpenSTA
  bool guardband_flag_ = true;  // Turn on the timing guardband option

  // ---- community information
  // ---- all the vertices in the same community will stay together
  std::vector<int> community_attr_;  // the community id of vertices (internal
                                     // representation, size is num_vertices_)

  // ---- fixed vertex information
  std::vector<int> fixed_attr_;  // the block id of fixed vertices. (internal
                                 // representation, size is num_vertices_)

  // ---- vertex grouping information
  std::vector<std::vector<int>>
      group_attr_;  // each group cooresponds to a group

  // --- Global net threshold
  int global_net_threshold_
      = 1000;  // If the net is larger than global_net_threshold_,
               // Then it will be ignored by TritonPart

  // coarsening related parameters (stop conditions)
  int thr_coarsen_hyperedge_size_skip_
      = 50;  // if the size of a hyperedge is larger than
             // thr_coarsen_hyperedge_size_skip_, then we ignore this
             // hyperedge during coarsening
  int thr_coarsen_vertices_ = 200;   // the minimum threshold of number of
                                     // vertices in the coarsest hypergraph
  int thr_coarsen_hyperedges_ = 50;  // the minimum threshold of number of
                                     // hyperedges in the coarsest hypergraph
  float coarsening_ratio_
      = 1.5;  // the ratio of number of vertices of adjacent coarse hypergraphs
  int max_coarsen_iters_ = 20;  // maxinum number of coarsening iterations
  float adj_diff_ratio_
      = 0.0001;  // the ratio of number of vertices of adjacent coarse
                 // hypergraphs if the ratio is less than adj_diff_ratio_, then
                 // stop coarsening
  int min_num_vertices_each_part_
      = 4;  // minimum number of vertices in each block
            // for the partitioning solution of the coareset hypergraph
            // We achieve this by controlling the maximum vertex weight
            // during coarsening
  CoarsenOrder coarsen_order_ = CoarsenOrder::kRandom;
  // weight related parameter

  // cost related parameter
  std::vector<float>
      e_wt_factors_;  // the cost introduced by a cut hyperedge e is
                      // e_wt_factors dot_product hyperedge_weights_[e]
                      // this parameter is used by coarsening and partitioning

  std::vector<float>
      v_wt_factors_;  // the ``weight'' of a vertex. For placement-driven
                      // coarsening, when we merge two vertices, we need to
                      // update the location of merged vertex based on the
                      // gravity center of these two vertices.  The weight of
                      // vertex v is defined as v_wt_factors dot_product
                      // vertex_weights_[v] this parameter is only used in
                      // coarsening

  std::vector<float>
      placement_wt_factors_;  // the weight for placement information. For
                              // placement-driven coarsening, when we calculate
                              // the score for best-choice coarsening, the
                              // placement information also contributes to the
                              // score function. we prefer to merge two vertices
                              // which are adjacent physically the distance
                              // between u and v is defined as
                              // norm2(placement_attr[u] - placement_attr[v],
                              // placement_wt_factors_) this parameter is only
                              // used during coarsening

  float net_timing_factor_ = 1.0;  // the timing weight for cutting a hyperedge
  float path_timing_factor_
      = 1.0;  // the cost for cutting a critical timing path once. If a critical
              // path is cut by 3 times, the cost is defined as 3 *
              // path_timing_factor_
  float path_snaking_factor_
      = 1.0;  // the cost of introducing a snaking timing path, see our paper
              // for detailed explanation of snaking timing paths
  float timing_exp_factor_ = 2.0;  // exponential factor
  float extra_delay_ = 1;          // extra delay introduced by cut

  // initial partitioning related parameters
  int num_initial_solutions_
      = 50;  // number of initial random solutions generated
  int num_best_initial_solutions_
      = 10;  // number of best initial solutions used for stability

  // refinement related parameter
  int refiner_iters_ = 2;  // refinement iterations
  int max_moves_
      = 50;  // the allowed moves for each pass of FM or greedy refinement
             // or the number of vertices in an ILP instance
  float early_stop_ratio_
      = 0.5;  // if the number of moved vertices exceeds
              // num_vertices_of_a_hypergraph times early_stop_ratio_, then exit
              // current FM pass. This parameter is set based on the obersvation
              // that in a FM pass, most of the gains are achieved by first
              // several moves
  int total_corking_passes_ = 25;  // the maximum level of traversing the
                                   // buckets to solve the "corking effect"
  // V-cycle related parameters
  bool v_cycle_flag_ = true;
  int max_num_vcycle_ = 5;  // maximum number of vcycles
  int num_coarsen_solutions_
      = 3;  // number of coarsening solutions with different random seed

  // number of vertices used for ILP partitioning
  // If the number of vertices of a hypergraph is larger than
  // num_vertices_threshold_ilp_, then we will NOT use ILP-based partitioning
  int num_vertices_threshold_ilp_ = 50;

  // Hypergraph information
  // basic information
  std::vector<std::vector<int>> hyperedges_;
  int num_vertices_ = 0;
  int num_hyperedges_ = 0;
  int vertex_dimensions_ = 1;     // specified in the hypergraph
  int hyperedge_dimensions_ = 1;  // specified in the hypergraph
  std::vector<std::vector<float>> vertex_weights_;
  std::vector<std::vector<float>> hyperedge_weights_;
  // When we create the hypergraph, we ignore all the hyperedges with vertices
  // more than global_net_threshold_
  HGraphPtr hypergraph_
      = nullptr;  // the hypergraph after removing large hyperedges
  HGraphPtr original_hypergraph_
      = nullptr;  // the original hypergraph. In the timing-driven flow,
                  // the original hypergraph also serves as the timing graph

  // Final solution
  std::vector<int> solution_;  // store the part_id for each vertex

  // logger
  utl::Logger* logger_ = nullptr;
};

}  // namespace par
