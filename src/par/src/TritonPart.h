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
#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <string>
#include <vector>

#include "TPHypergraph.h"
#include "db_sta/dbReadVerilog.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "sta/Bfs.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/Sta.hh"
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
template <typename T>
using matrix = std::vector<std::vector<T> >;

class TritonPart
{
 public:
  TritonPart(ord::dbNetwork* network,
             odb::dbDatabase* db,
             sta::dbSta* sta,
             utl::Logger* logger)
      : network_(network), db_(db), sta_(sta), logger_(logger)
  {
  }

  // Top level interface
  // The function for partitioning a hypergraph
  // This is the main API for TritonPart
  // Key supports:
  // (1) fixed vertices constraint in fixed_file
  // (2) community attributes in community_file (This can be used to guide the partitioning process)
  // (3) stay together attributes in group_file.
  // (4) timing-driven partitioning
  // (5) fence-aware partitioning
  void PartitionDesign(unsigned int num_parts_arg,
                       float balance_constraint_arg,
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

  // The function for partitioning a hypergraph
  // This is used for replacing hMETIS
  // Key supports: 
  // (1) fixed vertices constraint in fixed_file
  // (2) community attributes in community_file (This can be used to guide the partitioning process)
  // (3) stay together attributes in group_file.
  // The format is that each line cooresponds to a group
  // fixed vertices and community attributes both follows the hMETIS format
  void PartitionHypergraph(unsigned int num_parts,
                           float balance_constraint,
                           unsigned int seed,
                           int vertex_dimension,
                           int hyperedge_dimension,
                           int placement_dimension,
                           const char* hypergraph_file,
                           const char* fixed_file,
                           const char* community_file,
                           const char* group_file,
                           const char* placement_file);


  // k-way partitioning used by Hier-RTLMP
  std::vector<int> PartitionKWaySimpleMode(unsigned int num_parts_arg,
                                           float balance_constraint_arg,
                                           unsigned int seed_arg,
                                           const std::vector<std::vector<int> >& hyperedges,
                                           const std::vector<float>& vertex_weights,
                                           const std::vector<float>& hyperedge_weights);
    
  // Main APIs
  void SetTimingParams(float net_timing_factor, 
                       float path_timing_factor,
                       float path_snaking_factor,
                       float timing_exp_factor,
                       float extra_delay) {
    net_timing_factor_ = net_timing_factor;   // the timing weight for cutting a hyperedge
    path_timing_factor_ = path_timing_factor; // the cost for cutting a critical timing path once. If a critical path is cut by 3 times,
                                              // the cost is defined as 3 * path_timing_factor_
    path_snaking_factor_ = path_snaking_factor; // the cost of introducing a snaking timing path, see our paper for detailed explanation
                                                // of snaking timing paths
    timing_exp_factor_ = timing_exp_factor; // exponential factor 
    extra_delay_ = extra_delay; // extra delay introduced by cut
  }
                         
  void SetNetWeight(std::vector<float> e_wt_factors) {
    e_wt_factors_ = e_wt_factors;  // the cost introduced by a cut hyperedge e is
                                   // e_wt_factors dot_product hyperedge_weights_[e]
                                   // this parameter is used by coarsening and partitioning
  }

  void SetVertexWeight(std::vector<float> v_wt_factors) {
    v_wt_factors_ = v_wt_factors;    // the ``weight'' of a vertex. For placement-driven coarsening,
                                     // when we merge two vertices, we need to update the location
                                     // of merged vertex based on the gravity center of these two 
                                     // vertices.  The weight of vertex v is defined
                                     // as v_wt_factors dot_product vertex_weights_[v]
                                     // this parameter is only used in coarsening
  }

  void SetPlacementWeight(std::vector<float> placement_wt_factors) {
    placement_wt_factors_ = placement_wt_factors; // the weight for placement information. For placement-driven 
                                                  // coarsening, when we calculate the score for best-choice coarsening,
                                                  // the placement information also contributes to the score function.
                                                  // we prefer to merge two vertices which are adjacent physically
                                                  // the distance between u and v is defined as 
                                                  // norm2(placement_attr[u] - placement_attr[v], placement_wt_factors_)
                                                  // this parameter is only used during coarsening       
  }

  // Set detailed parameters
  // There parameters only used by users who want to exploit the performance 
  // limits of TritonPart
  void SetFineTuneParams(// coarsening related parameters
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
                         int max_num_fm_pass,
                         float early_stop_ratio,
                         int total_corking_passes,
                         // vcycle related parameters
                         bool v_cycle_flag,
                         int max_num_vcycle,
                         int num_ubfactor_delta)
  {
    // coarsening related parameters (stop conditions)
    thr_coarsen_hyperedge_size_skip_ = thr_coarsen_hyperedge_size_skip; // if the size of a hyperedge is larger than
                                                                        // thr_coarsen_hyperedge_size_skip_, then we ignore this
                                                                        // hyperedge during coarsening
    thr_coarsen_vertices_ = thr_coarsen_vertices; // the minimum threshold of number of vertices in the coarsest 
                                                  // hypergraph 
    thr_coarsen_hyperedges_ = thr_coarsen_hyperedges; // the minimum threshold of number of hyperedges in the 
                                                      // coarsest hypergraph
    coarsening_ratio_ = coarsening_ratio; // the ratio of number of vertices of adjacent coarse hypergraphs
    max_coarsen_iters_ = max_coarsen_iters; // maxinum number of coarsening iterations
    adj_diff_ratio_ = adj_diff_ratio; // the ratio of number of vertices of adjacent coarse hypergraphs
                                      // if the ratio is less than adj_diff_ratio_, then stop coarsening
    min_num_vertices_each_part_ = min_num_vertices_each_part; // minimum number of vertices in each block 
                                                              // for the partitioning solution of the coareset hypergraph
                                                              // We achieve this by controlling the maximum vertex weight
                                                              // during coarsening
    // initial partitioning related parameters
    num_initial_solutions_ = num_initial_solutions;       // number of initial random solutions generated
    num_best_initial_solutions_ = num_best_initial_solutions;  // number of best initial solutions used for stability
    // refinement related parameter
    refiner_iters_ = refiner_iters; // refinement iterations
    max_moves_ = max_moves;  // the allowed moves for each pass of FM or greedy refinement
                             // or the number of vertices in an ILP instance
    max_num_fm_pass_ = max_num_fm_pass;  // the allowed number of FM pass in each refinement iteration
    early_stop_ratio_ = early_stop_ratio; // if the number of moved vertices exceeds num_vertices_of_a_hypergraph times
                                          // early_stop_ratio_, then exit current FM pass.
                                          // This parameter is set based on the obersvation that
                                          // in a FM pass, most of the gains are achieved by first several moves
    total_corking_passes_ = total_corking_passes; // the maximum level of traversing the buckets to solve the "corking effect"
    // V-cycle related parameters
    v_cycle_flag_ = v_cycle_flag;
    max_num_vcycle_ = max_num_vcycle;     // maximum number of vcycles

    // This parameter is set to avoid early-stuck of FM
    num_ubfactor_delta_ = num_ubfactor_delta;  // allowing marginal imbalance to improve QoR
  }

 private:
  // Main partititon function
  void MultiLevelPartition();

  // read and build hypergraph
  void ReadHypergraph(std::string hypergraph, 
                      std::string fixed_file,
                      std::string community_file,
                      std::string group_file,
                      std::string placement_file);

  // read and build netlist
  void ReadNetlist(std::string fixed_file,
                   std::string community_file,
                   std::string group_file);    
  void BuildTimingPaths();  // Find all the critical timing paths

  // private member functions
  ord::dbNetwork* network_ = nullptr;
  odb::dbDatabase* db_ = nullptr;
  odb::dbBlock* block_ = nullptr;
  sta::dbSta* sta_ = nullptr;

  // user-specified parameters  

  // constraints information
  float ub_factor_ = 1.0;  // balance constraint
  int num_parts_ = 2;      // number of partitions

  // random seed
  int seed_ = 0;

  // ---- support for partitioning design with placed information
  // ---- for example, pin-3D flow
  bool placement_flag_ = false; // if use placement information to guide partitioning
  int placement_dimensions_ = 2;  // by default, we are working on 2D canvas
  std::vector<std::vector<float> > placement_attr_; // (internal representation, size is num_vertices_)
  
  bool fence_flag_ = false; // if use fence constraint
  Rect fence_ {0, 0, 0, 0}; // only consider the netlist within the fence
  
  // ---- timing information
  bool timing_aware_flag_ = true;  // Enable timing aware
  int top_n_ = 1000;               // top_n timing paths
  float maximum_clock_period_ = 0.0;
  std::vector<float> hyperedge_slacks_; // normalized by clock period
  std::vector<VertexType> vertex_types_; // the vertex type of each instances
  std::vector<TimingPath> timing_paths_;  // critical timing paths, extracted based OpenSTA

  // ---- community information
  // ---- all the vertices in the same community will stay together
  std::vector<int> community_attr_;  // the community id of vertices (internal representation, size is num_vertices_)

  // ---- fixed vertex information
  std::vector<int> fixed_attr_;  // the block id of fixed vertices. (internal representation, size is num_vertices_)

  // ---- vertex grouping information
  std::vector<int> group_attr_; // map vertex to its group id

  // coarsening related parameters (stop conditions)
  int thr_coarsen_hyperedge_size_skip_ = 50; // if the size of a hyperedge is larger than
                                   // thr_coarsen_hyperedge_size_skip_, then we ignore this
                                   // hyperedge during coarsening
  int thr_coarsen_vertices_ = 200; // the minimum threshold of number of vertices in the coarsest 
                                   // hypergraph 
  int thr_coarsen_hyperedges_ = 50; // the minimum threshold of number of hyperedges in the 
                                    // coarsest hypergraph
  float coarsening_ratio_ = 1.5; // the ratio of number of vertices of adjacent coarse hypergraphs
  int max_coarsen_iters_ = 20; // maxinum number of coarsening iterations
  float adj_diff_ratio_ = 0.0001; // the ratio of number of vertices of adjacent coarse hypergraphs
                                  // if the ratio is less than adj_diff_ratio_, then stop coarsening
  int min_num_vertices_each_part_ = 4; // minimum number of vertices in each block 
                                       // for the partitioning solution of the coareset hypergraph
                                       // We achieve this by controlling the maximum vertex weight
                                       // during coarsening
  CoarsenOrder coarsen_order_ = CoarsenOrder::RANDOM;
  // weight related parameter

  // cost related parameter
  std::vector<float> e_wt_factors_;  // the cost introduced by a cut hyperedge e is
                                     // e_wt_factors dot_product hyperedge_weights_[e]
                                     // this parameter is used by coarsening and partitioning

  std::vector<float> v_wt_factors_;  // the ``weight'' of a vertex. For placement-driven coarsening,
                                     // when we merge two vertices, we need to update the location
                                     // of merged vertex based on the gravity center of these two 
                                     // vertices.  The weight of vertex v is defined
                                     // as v_wt_factors dot_product vertex_weights_[v]
                                     // this parameter is only used in coarsening
                                   
  std::vector<float> placement_wt_factors_; // the weight for placement information. For placement-driven 
                                            // coarsening, when we calculate the score for best-choice coarsening,
                                            // the placement information also contributes to the score function.
                                            // we prefer to merge two vertices which are adjacent physically
                                            // the distance between u and v is defined as 
                                            // norm2(placement_attr[u] - placement_attr[v], placement_wt_factors_)
                                            // this parameter is only used during coarsening        

  float net_timing_factor_ = 1.0;  // the timing weight for cutting a hyperedge
  float path_timing_factor_ = 1.0; // the cost for cutting a critical timing path once. If a critical path is cut by 3 times,
                                   // the cost is defined as 3 * path_timing_factor_
  float path_snaking_factor_ = 1.0; // the cost of introducing a snaking timing path, see our paper for detailed explanation
                                    // of snaking timing paths
  float timing_exp_factor_ = 2.0;   // exponential factor 
  float extra_delay_ = 1;           // extra delay introduced by cut

  // initial partitioning related parameters
  int num_initial_solutions_ = 50;       // number of initial random solutions generated
  int num_best_initial_solutions_ = 10;  // number of best initial solutions used for stability

  // refinement related parameter
  int refiner_iters_ = 2; // refinement iterations
  int max_moves_ = 50; // the allowed moves for each pass of FM or greedy refinement
                       // or the number of vertices in an ILP instance
  int max_num_fm_pass_ = 10; // the allowed number of FM pass in each refinement iteration
  float early_stop_ratio_ = 0.5; // if the number of moved vertices exceeds num_vertices_of_a_hypergraph times
                                 // early_stop_ratio_, then exit current FM pass.
                                 // This parameter is set based on the obersvation that
                                 // in a FM pass, most of the gains are achieved by first several moves
  int total_corking_passes_ = 25; // the maximum level of traversing the buckets to solve the "corking effect"
  // V-cycle related parameters
  bool v_cycle_flag_ = true;
  int max_num_vcycle_ = 5;      // maximum number of vcycles

  // This parameter is set to avoid early-stuck of FM
  // TODO (20230401) : we need to more experiments to tune this, not used now
  int num_ubfactor_delta_ = 5;  // allowing marginal imbalance to improve QoR
                                                                 
  // Hypergraph information
  // basic information
  std::vector<std::vector<int> > hyperedges_;
  int num_vertices_ = 0;
  int num_hyperedges_ = 0;
  int vertex_dimensions_ = 1; // specified in the hypergraph
  int hyperedge_dimensions_ = 1; // specified in the hypergraph
  std::vector<std::vector<float> > vertex_weights_;
  std::vector<std::vector<float> > hyperedge_weights_;
  // When we create the hypergraph, we ignore all the hyperedges with vertices
  // more than global_net_threshold_
  HGraph hypergraph_ = nullptr;  // the hypergraph after removing large hyperedges
  HGraph original_hypergraph_ = nullptr; // the original hypergraph. In the timing-driven flow,
                                         // the original hypergraph also serves as the timing graph

  // Final solution
  std::vector<int> solution_;  // store the part_id for each vertex

  // logger
  utl::Logger* logger_ = nullptr;
};

}  // namespace par
