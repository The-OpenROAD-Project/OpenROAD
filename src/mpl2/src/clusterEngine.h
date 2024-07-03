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

#include <map>
#include <set>
#include <unordered_map>
#include <vector>

namespace utl {
class Logger;
}

namespace sta {
class dbNetwork;
}

namespace odb {
class dbBlock;
class dbInst;
class dbBTerm;
class dbMaster;
class dbITerm;
}  // namespace odb

namespace mpl2 {
class Metrics;
class Cluster;
class HardMacro;

struct DataFlow
{
  const int register_dist = 5;
  const float factor = 2.0;
  const float weight = 1;

  std::vector<std::pair<odb::dbITerm*, std::vector<std::set<odb::dbInst*>>>>
      macro_pin_to_regs;
  std::vector<std::pair<odb::dbBTerm*, std::vector<std::set<odb::dbInst*>>>>
      io_to_regs;
  std::vector<std::pair<odb::dbITerm*, std::vector<std::set<odb::dbInst*>>>>
      macro_pin_to_macros;
};

struct SizeThresholds
{
  SizeThresholds()
      : max_macro(0), min_macro(0), max_std_cell(0), min_std_cell(0)
  {
  }

  int max_macro;
  int min_macro;
  int max_std_cell;
  int min_std_cell;
};

struct PhysicalHierarchyMaps
{
  std::map<int, Cluster*> id_to_cluster;
  std::map<odb::dbInst*, HardMacro*> inst_to_hard;
  std::unordered_map<odb::dbInst*, int> inst_to_cluster_id;
  std::unordered_map<odb::dbBTerm*, int> bterm_to_cluster_id;

  // Only for designs with IO Pads
  std::map<odb::dbBTerm*, odb::dbInst*> bterm_to_inst;
};

struct PhysicalHierarchy
{
  PhysicalHierarchy()
      : root(nullptr),
        coarsening_ratio(0),
        max_level(0),
        bundled_ios_per_edge(0),
        large_net_threshold(0),
        has_io_clusters(true)
  {
  }

  Cluster* root;
  PhysicalHierarchyMaps maps;

  SizeThresholds base_thresholds;
  float coarsening_ratio;
  int max_level;
  int bundled_ios_per_edge;
  int large_net_threshold;  // used to ignore global nets

  bool has_io_clusters;
};

class ClusteringEngine
{
 public:
  ClusteringEngine(odb::dbBlock* block,
                   sta::dbNetwork* network,
                   utl::Logger* logger);

  void setDesignMetrics(Metrics* design_metrics);
  void setTargetStructure(PhysicalHierarchy* tree);

  void buildPhysicalHierarchy();

 private:
  void initTree();
  void setDefaultThresholds();
  void createIOClusters();
  void mapIOPads();

  // Methods for data flow
  void createDataFlow();
  void dataFlowDFSIOPin(int parent,
                        int idx,
                        std::vector<std::set<odb::dbInst*>>& insts,
                        std::map<int, odb::dbBTerm*>& io_pin_vertex,
                        std::map<int, odb::dbInst*>& std_cell_vertex,
                        std::map<int, odb::dbITerm*>& macro_pin_vertex,
                        std::vector<bool>& stop_flag_vec,
                        std::vector<bool>& visited,
                        std::vector<std::vector<int>>& vertices,
                        std::vector<std::vector<int>>& hyperedges,
                        bool backward_search);
  void dataFlowDFSMacroPin(int parent,
                           int idx,
                           std::vector<std::set<odb::dbInst*>>& std_cells,
                           std::vector<std::set<odb::dbInst*>>& macros,
                           std::map<int, odb::dbBTerm*>& io_pin_vertex,
                           std::map<int, odb::dbInst*>& std_cell_vertex,
                           std::map<int, odb::dbITerm*>& macro_pin_vertex,
                           std::vector<bool>& stop_flag_vec,
                           std::vector<bool>& visited,
                           std::vector<std::vector<int>>& vertices,
                           std::vector<std::vector<int>>& hyperedges,
                           bool backward_search);

  static bool isIgnoredMaster(odb::dbMaster* master);

  odb::dbBlock* block_;
  sta::dbNetwork* network_;
  utl::Logger* logger_;

  Metrics* design_metrics_;
  PhysicalHierarchy* tree_;

  int id_;
  SizeThresholds level_thresholds_;
  DataFlow data_flow;
};

}  // namespace mpl2