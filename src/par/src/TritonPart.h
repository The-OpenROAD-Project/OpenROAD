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
using matrix = std::vector<std::vector<T>>;

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
  void PartitionDesign(unsigned int num_parts,
                       float balance_constraint,
                       unsigned int seed,
                       const std::string& solution_filename,
                       const std::string& paths_filename,
                       const std::string& hypergraph_filename);
  // This is only used for replacing hMETIS
  void PartitionHypergraph(const char* hypergraph_file,
                           const char* fixed_file,
                           unsigned int num_parts,
                           float balance_constraint,
                           int vertex_dimension,
                           int hyperedge_dimension,
                           unsigned int seed);

  // 2-way partition c++ interface for Hier-RTLMP
  std::vector<int> Partition2Way(
      int num_vertices,
      int num_hyperedges,
      const std::vector<std::vector<int>>& hyperedges,
      const std::vector<float>& vertex_weights,
      float balance_constraints,
      int seed = 0);

  // APIs
  void SetFenceFlag(bool fence_flag) { fence_flag_ = fence_flag; }

  void SetFence(float fence_lx, float fence_ly, float fence_ux, float fence_uy)
  {
    const int dbu = db_->getTech()->getDbUnitsPerMicron();
    fence_
        = Rect(fence_lx * dbu, fence_ly * dbu, fence_ux * dbu, fence_uy * dbu);
  }

 private:
  // Pre process the hypergraph by skipping large hyperedges
  HGraph PreProcessHypergraph();
  // Generate timing report
  void GenerateTimingReport(std::vector<int>& partition, bool design);
  void WritePathsToFile(const std::string& paths_filename);
  std::vector<int> DesignPartTwoWay(unsigned int num_parts_,
                                    float ub_factor_,
                                    int vertex_dimensions_,
                                    int hyperedge_dimensions_,
                                    unsigned int seed_);
  std::vector<int> DesignPartKWay(unsigned int num_parts_,
                                  float ub_factor_,
                                  int vertex_dimensions_,
                                  int hyperedge_dimensions_,
                                  unsigned int seed_);
  std::vector<int> HypergraphPartTwoWay(const char* hypergraph_file,
                                        const char* fixed_file,
                                        unsigned int num_parts,
                                        float balance_constraint,
                                        int vertex_dimension,
                                        int hyperedge_dimension,
                                        unsigned int seed);
  std::vector<int> HypergraphPartKWay(const char* hypergraph_file,
                                      const char* fixed_file,
                                      unsigned int num_parts,
                                      float balance_constraint,
                                      int vertex_dimension,
                                      int hyperedge_dimension,
                                      unsigned int seed);
  void PartRecursive(const char* hypergraph_file,
                     const char* fixed_file,
                     unsigned int num_parts,
                     float balance_constraint,
                     int vertex_dimension,
                     int hyperedge_dimension,
                     unsigned int seed);
  // Utility functions for reading hypergraph
  void ReadNetlistWithTypes();
  void ReadNetlist();  // read hypergraph from netlist
  void ReadHypergraph(std::string hypergraph, std::string fixed_file);
  // Read hypergraph from input files
  void BuildHypergraph();
  // Write the hypergraph to file
  void WriteHypergraph(const std::string& hypergraph_filename);

  void BuildTimingPaths();  // Find all the critical timing paths
  void MultiLevelPartition(std::vector<int>& solution);

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
  bool placement_flag_
      = false;  // if use placement information to guide partitioning
  std::map<std::string, std::vector<float>>
      placement_info_;            // vertex_name, location of vertex
  int placement_dimensions_ = 2;  // by default, we are working on 2D canvas
  std::vector<std::vector<float>>
      placement_attr_;  // (internal representation, size is num_vertices_)

  bool fence_flag_ = false;  // if use fence constraint
  Rect fence_{0, 0, 0, 0};   // only consider the netlist within the fence

  // ---- timing information
  bool timing_aware_flag_ = true;  // Enable timing aware
  int top_n_ = 1000;               // top_n timing paths
  float maximum_clock_period_ = 0.0;
  std::vector<float> hyperedge_slacks_;  // normalized by clock period
  std::vector<TimingPath>
      timing_paths_;  // critical timing paths, extracted based OpenSTA
  std::vector<float>
      timing_attr_;  // (internal representation, size is num_hyperedges)

  // ---- community information
  // ---- all the vertices in the same community will stay together
  bool community_flag_ = false;  // if community information is used
  std::map<std::string, int>
      community_info_;               // vertex_name, community of vertex
  std::vector<int> community_attr_;  // the community id of vertices (internal
                                     // representation, size is num_vertices_)

  // ---- fixed vertex information
  bool fixed_vertex_flag_ = false;  // if fixed vertices are specified
  std::map<std::string, int> fixed_vertex_info_;  // vertex name, block id
  std::vector<int> fixed_attr_;  // the block id of fixed vertices. (internal
                                 // representation, size is num_vertices_)

  // other parameters
  int he_size_threshold_ = 50;  // if the size of a hyperedge is larger than
                                // he_size_threshold, then ignore this hyperedge

  // coarsening related parameters (stop conditions)
  int thr_coarsen_hyperedge_size_skip_
      = 50;  // if the size of a hyperedge is larger than
             // thr_coarsen_hyperedge_size_skip_, then we ignore this
             // hyperedge during coarsening
  int thr_coarsen_vertices_ = 200;  // the minimum threshold of number of
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
  // weight related parameter

  int path_traverse_step_
      = 2;  // during coarsening, use this parameter to check the neighbors
            // in the critial timing paths

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

  float path_wt_factor_ = 1.0;  // the cost for cutting a critical timing path
                                // once. If a critical path is cut by 3 times,
                                // the cost is defined as 3 * path_wt_factor_
  float net_cut_factor_
      = 1.0;  // the cost for cutting a net, without timing information
  float timing_factor_ = 1.0;  // the weight for cutting a path
  float snaking_wt_factor_
      = 1.0;  // the cost of introducing a snaking timing path, see our paper
              // for detailed explanation of snaking timing paths
  float timing_exp_factor_ = 2.0;  // exponential factor

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
  int max_num_fm_pass_
      = 10;  // the allowed number of FM pass in each refinement iteration
  float early_stop_ratio_
      = 0.5;  // if the number of moved vertices exceeds
              // num_vertices_of_a_hypergraph times early_stop_ratio_, then exit
              // current FM pass. This parameter is set based on the obersvation
              // that in a FM pass, most of the gains are achieved by first
              // several moves

  // V-cycle related parameters
  bool v_cycle_flag_ = true;
  int max_num_vcycle_ = 5;  // maximum number of vcycles

  // This parameter is set to avoid early-stuck of FM
  int num_ubfactor_delta_ = 5;  // allowing marginal imbalance to improve QoR

  // Hypergraph information
  // basic information
  std::vector<std::vector<int>> hyperedges_;
  int num_vertices_ = 0;
  int num_hyperedges_ = 0;
  int vertex_dimensions_ = 1;     // specified in the hypergraph
  int hyperedge_dimensions_ = 1;  // specified in the hypergraph
  std::vector<std::vector<float>> vertex_weights_;
  std::vector<VertexType> vertex_types_;  // the vertex type of each instances
  std::vector<std::vector<float>> hyperedge_weights_;
  std::vector<std::vector<float>> nonscaled_hyperedge_weights_;

  // When we create the hypergraph, we ignore all the hyperedges with vertices
  // more than global_net_threshold_
  HGraph hypergraph_ = nullptr;  // the original hypergraph

  // Final solution
  std::vector<int> parts_;  // store the part_id for each vertex

  // logger
  utl::Logger* logger_ = nullptr;
};

}  // namespace par
