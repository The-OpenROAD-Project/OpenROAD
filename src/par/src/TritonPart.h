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
// The TritonPart is an open-source version of hMETIS,
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
  void tritonPartDesign(unsigned int num_parts,
                        float balance_constraint,
                        unsigned int seed,
                        const std::string& solution_filename,
                        const std::string& paths_filename,
                        const std::string& hypergraph_filename);
  // This is only used for replacing hMETIS
  void tritonPartHypergraph(const char* hypergraph_file,
                            const char* fixed_file,
                            unsigned int num_parts,
                            float balance_constraint,
                            int vertex_dimension,
                            int hyperedge_dimension,
                            unsigned int seed);

  // 2-way partition c++ interface for Hier-RTLMP
  std::vector<int> TritonPart2Way(
      int num_vertices,
      int num_hyperedges,
      const std::vector<std::vector<int>>& hyperedges,
      const std::vector<float>& vertex_weights,
      float balance_constraints,
      int seed = 0);

  /*void tritonPartHypergraph(const char* hypergraph_file,
                            const char* fixed_file,
                            unsigned int num_parts,
                            float balance_constraint,
                            int vertex_dimension,
                            int hyperedge_dimension,
                            unsigned int seed);*/

 private:
  // Pre process the hypergraph by skipping large hyperedges
  HGraph preProcessHypergraph();
  // Generate timing report
  void GenerateTimingReport(std::vector<int>& partition, bool design);
  void WritePathsToFile(const std::string& paths_filename);
  std::vector<int> TritonPart_design_PartTwoWay(unsigned int num_parts_,
                                                float ub_factor_,
                                                int vertex_dimensions_,
                                                int hyperedge_dimensions_,
                                                unsigned int seed_);
  std::vector<int> TritonPart_design_PartKWay(unsigned int num_parts_,
                                              float ub_factor_,
                                              int vertex_dimensions_,
                                              int hyperedge_dimensions_,
                                              unsigned int seed_);
  std::vector<int> TritonPart_hypergraph_PartTwoWay(const char* hypergraph_file,
                                                    const char* fixed_file,
                                                    unsigned int num_parts,
                                                    float balance_constraint,
                                                    int vertex_dimension,
                                                    int hyperedge_dimension,
                                                    unsigned int seed);
  std::vector<int> TritonPart_hypergraph_PartKWay(const char* hypergraph_file,
                                                  const char* fixed_file,
                                                  unsigned int num_parts,
                                                  float balance_constraint,
                                                  int vertex_dimension,
                                                  int hyperedge_dimension,
                                                  unsigned int seed);
  void TritonPart_PartRecursive(const char* hypergraph_file,
                                const char* fixed_file,
                                unsigned int num_parts,
                                float balance_constraint,
                                int vertex_dimension,
                                int hyperedge_dimension,
                                unsigned int seed);
  ord::dbNetwork* network_ = nullptr;
  odb::dbDatabase* db_ = nullptr;
  odb::dbBlock* block_ = nullptr;
  sta::dbSta* sta_ = nullptr;

  // Other parameters
  int global_net_threshold_ = 100000;  //

  // Hypergraph information
  // basic information
  std::vector<std::vector<int>> hyperedges_;
  int num_vertices_ = 0;
  int num_hyperedges_ = 0;
  int vertex_dimensions_ = 1;
  int hyperedge_dimensions_ = 1;
  std::vector<std::vector<float>> vertex_weights_;
  std::vector<std::vector<float>> hyperedge_weights_;
  std::vector<std::vector<float>> nonscaled_hyperedge_weights_;

  // placement information
  int placement_dimensions_ = 0;
  std::vector<std::vector<float>>
      placement_attr_;  // the embedding for vertices

  // community information
  std::vector<int> community_attr_;  // the community id of vertices

  // type
  std::vector<std::string> vtx_type_;

  // fixed vertices
  bool fixed_vertex_flag_ = false;
  std::vector<int> fixed_attr_;  // the block id of fixed vertices.

  // timing information
  std::vector<TimingPath> timing_paths_;  // critical timing paths
  std::vector<float> timing_attr_;

  // constraints information
  float ub_factor_ = 1.0;  // balance constraint
  int num_parts_ = 2;      // number of partitions

  // threshold_hyperedges_to_skip
  int he_size_threshold_ = 50;

  // random seed
  int seed_ = 0;

  // Utility functions for reading hypergraph
  void ReadNetlistWithTypes();
  void ReadNetlist();  // read hypergraph from netlist
  void ReadHypergraph(std::string hypergraph, std::string fixed_file);
  // Read hypergraph from input files
  void BuildHypergraph();
  // Write the hypergraph to file
  void WriteHypergraph(const std::string& hypergraph_filename);
  // When we create the hypergraph, we ignore all the hyperedges with vertices
  // more than global_net_threshold_
  HGraph hypergraph_ = nullptr;  // the original hypergraph

  // Final solution
  std::vector<int> parts_;  // store the part_id for each vertex

  // Timing related functions and variables
  bool timing_aware_flag_ = true;  // Enable timing aware
  int top_n_ = 1000;               // top_n timing paths
  void BuildTimingPaths();         // Find all the critical timing paths

  // logger
  utl::Logger* logger_ = nullptr;
};

}  // namespace par
