///////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2020, The Regents of the University of California
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

#include "db_sta/dbReadVerilog.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "sta/Bfs.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/Sta.hh"
#include "utl/Logger.h"

namespace par {

// The four edges (left, right, bottom, top) are divided into
// thirds (lower, middle, upper).  The cross-product produces
// twelve io regions.  IOs (bterms) are mapped to these regions.
enum IORegion
{
  LeftLower,
  LeftMiddle,
  LeftUpper,
  RightLower,
  RightMiddle,
  RightUpper,
  TopLower,
  TopMiddle,
  TopUpper,
  BottomLower,
  BottomMiddle,
  BottomUpper
};

class Cluster
{
 public:
  Cluster() {}
  Cluster(int id, const std::string& name) : id_(id), name_(name) {}

  // Accessor
  int getId() const { return id_; }
  odb::dbModule* getTopModule() const { return top_module_; }
  std::string getName() const { return name_; }
  std::vector<std::string> getLogicalModuleVec() const
  {
    return logical_module_vec_;
  }
  std::vector<odb::dbInst*> getInsts() const { return inst_vec_; }
  std::vector<odb::dbInst*> getMacros() const { return macro_vec_; }
  unsigned int getNumMacro() const { return macro_vec_.size(); }
  unsigned int getNumInst() const { return inst_vec_.size(); }
  std::map<int, unsigned int> getInputConnections() const
  {
    return input_connection_map_;
  }

  std::map<int, unsigned int> getOutputConnections() const
  {
    return output_connection_map_;
  }

  unsigned int getInputConnection(int cluster_id) const
  {
    auto map_iter = input_connection_map_.find(cluster_id);
    if (map_iter == input_connection_map_.end())
      return 0;
    else
      return map_iter->second;
  }

  unsigned int getOutputConnection(int cluster_id) const
  {
    auto map_iter = output_connection_map_.find(cluster_id);
    if (map_iter == output_connection_map_.end())
      return 0;
    else
      return map_iter->second;
  }

  // operations
  void removeMacro() { macro_vec_.clear(); }

  float calculateArea(ord::dbNetwork* network) const;
  void calculateNumSeq(ord::dbNetwork* network);
  int getNumSeq() const { return num_seq_; }

  void addInst(odb::dbInst* inst) { inst_vec_.push_back(inst); }
  void addMacro(odb::dbInst* inst) { macro_vec_.push_back(inst); }
  void setTopModule(odb::dbModule* module) { top_module_ = module; }
  void setName(const std::string& name) { name_ = name; }
  void addLogicalModule(const std::string& module_name)
  {
    logical_module_vec_.push_back(module_name);
  }

  void addLogicalModuleVec(const std::vector<std::string>& module_vec)
  {
    for (auto& module : module_vec)
      logical_module_vec_.push_back(module);
  }

  void initConnection()
  {
    input_connection_map_.clear();
    output_connection_map_.clear();
  }

  void addInputConnection(int cluster_id, unsigned int weight = 1)
  {
    auto map_iter = input_connection_map_.find(cluster_id);
    if (map_iter == input_connection_map_.end())
      input_connection_map_[cluster_id] = weight;
    else
      map_iter->second += weight;
  }

  void addOutputConnection(int cluster_id, unsigned int weight = 1)
  {
    auto map_iter = output_connection_map_.find(cluster_id);
    if (map_iter == output_connection_map_.end())
      output_connection_map_[cluster_id] = weight;
    else
      map_iter->second += weight;
  }

  // These functions only for test
  void printInputConnections() const
  {
    for (auto [cluster_id, num_conn] : input_connection_map_) {
      std::cout << "cluster_id:   " << cluster_id << "   ";
      std::cout << "num_connections:   " << num_conn << "   ";
      std::cout << std::endl;
    }
  }

  void printOutputConnections() const
  {
    for (auto [cluster_id, num_conn] : output_connection_map_) {
      std::cout << "cluster_id:   " << cluster_id << "   ";
      std::cout << "num_connections:   " << num_conn << "   ";
      std::cout << std::endl;
    }
  }

 private:
  int id_ = 0;
  int num_seq_ = 0;
  odb::dbModule* top_module_ = nullptr;
  std::string name_ = "";
  std::vector<std::string> logical_module_vec_;
  std::vector<odb::dbInst*> inst_vec_;
  std::vector<odb::dbInst*> macro_vec_;
  std::map<int, unsigned int> input_connection_map_;
  std::map<int, unsigned int> output_connection_map_;
};

struct Metric
{
  float area = 0.0;
  unsigned int num_macro = 0;
  unsigned int num_inst = 0;

  Metric() {}

  Metric(float area, unsigned int num_macro, unsigned int num_inst)
  {
    this->area = area;
    this->num_macro = num_macro;
    this->num_inst = num_inst;
  }
};

class AutoClusterMgr
{
 public:
  AutoClusterMgr(ord::dbNetwork* network,
                 odb::dbDatabase* db,
                 sta::dbSta* sta,
                 utl::Logger* logger)
      : network_(network), db_(db), sta_(sta), logger_(logger)
  {
  }

  void partitionDesign(unsigned int max_num_macro,
                       unsigned int min_num_macro,
                       unsigned int max_mum_inst,
                       unsigned int min_num_inst,
                       unsigned int net_threshold,
                       unsigned int virtual_weight,
                       unsigned int ignore_net_threshold,
                       unsigned int num_hops,
                       unsigned int timing_weight,
                       bool std_cell_timing_flag,
                       const char* report_directory,
                       const char* file_name,
                       float keepin_lx,
                       float keepin_ly,
                       float keepin_ux,
                       float keepin_uy);

 private:
  ord::dbNetwork* network_ = nullptr;
  odb::dbDatabase* db_ = nullptr;
  odb::dbBlock* block_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  utl::Logger* logger_ = nullptr;
  unsigned int max_num_macro_ = 0;
  unsigned int min_num_macro_ = 0;
  unsigned int max_num_inst_ = 0;
  unsigned int min_num_inst_ = 0;
  unsigned int net_threshold_ = 0;
  unsigned int virtual_weight_ = 10000;
  unsigned int num_buffer_ = 0;
  bool std_cell_timing_flag_ = false;
  float area_buffer_ = 0;

  float dbu_ = 0.0;

  int floorplan_lx_ = 0;
  int floorplan_ly_ = 0;
  int floorplan_ux_ = 0;
  int floorplan_uy_ = 0;

  // IOs
  std::vector<float> B_pin_;
  std::vector<float> T_pin_;
  std::vector<float> L_pin_;
  std::vector<float> R_pin_;

  // This is for Pads
  void PrintPadPos(odb::dbModule* module, std::ostream& out);
  void PrintIOPadNet(std::ostream& out);
  std::map<odb::dbBTerm*, odb::dbInst*> io_pad_map_;
  std::map<odb::dbInst*, odb::dbBTerm*> pad_io_map_;

  // Map all the BTerms to an IORegion
  std::map<const odb::dbBTerm*, IORegion> bterm_map_;
  std::map<IORegion, int> bundled_io_map_;
  std::map<const odb::dbModule*, Metric> logical_cluster_map_;
  std::map<int, Cluster*> cluster_map_;
  std::map<const odb::dbInst*, int> inst_map_;

  std::map<int, int> virtual_map_;

  std::map<const odb::dbInst*, int> buffer_map_;
  int buffer_id_ = -1;
  std::vector<std::vector<odb::dbNet*>> buffer_net_vec_;
  std::set<odb::dbNet*> buffer_nets_;

  // timing-driven related function
  unsigned int num_hops_ = 5;
  unsigned int timing_weight_ = 1;

  std::vector<odb::dbInst*> macros_;
  std::vector<odb::dbInst*> seeds_;
  std::map<sta::Vertex*, std::map<sta::Pin*, int>> vertex_fanins_;
  std::map<int, std::map<sta::Pin*, int>> virtual_vertex_map_;
  std::map<int, std::map<int, int>> virtual_timing_map_;
  std::map<sta::Pin*, odb::dbInst*> pin_inst_map_;

  std::vector<Cluster*> cluster_list_;
  std::vector<Cluster*> merge_cluster_list_;
  std::queue<Cluster*> break_cluster_list_;
  std::queue<Cluster*> mlpart_cluster_list_;

  void findAdjacencies();

  void seedFaninBfs(sta::BfsFwdIterator& bfs);
  void findFanins(sta::BfsFwdIterator& bfs);
  sta::Pin* findSeqOutPin(sta::Instance* inst, sta::LibertyPort* out_port);
  void copyFaninsAcrossRegisters(sta::BfsFwdIterator& bfs);
  void addTimingWeight(float weight);
  void addFanin(sta::Vertex*, sta::Pin*, int num_bit);
  void addWeight(int src_id, int target_id, int weight);
  void calculateSeed();

  void createBundledIO();
  Metric computeMetrics(odb::dbModule* module);
  void getBufferNet();
  void getBufferNetUtil(
      odb::dbBlock* block,
      std::vector<std::pair<odb::dbNet*, odb::dbNet*>>& buffer_net);

  void createCluster(int& cluster_id);
  void createClusterUtil(odb::dbModule* module, int& cluster_id);
  void updateConnection();
  void calculateConnection();
  void calculateBufferNetConnection();
  bool hasTerminals(const sta::Net* net);
  unsigned int calculateClusterNumInst(
      const std::vector<Cluster*>& cluster_vec);
  unsigned int calculateClusterNumMacro(
      const std::vector<Cluster*>& cluster_vec);
  void mergeCluster(Cluster* src, const Cluster* target);
  void merge(const std::string& parent_name);
  void mergeMacro(const std::string& parent_name, int std_cell_id);
  void mergeMacroUtil(const std::string& parent_name,
                      int& merge_index,
                      int std_cell_id);
  void mergeUtil(const std::string& parent_name, int& merge_index);
  void breakCluster(Cluster* cluster_old, int& cluster_id);
  void MLPart(Cluster* cluster, int& cluster_id);
  void MacroPart(Cluster* cluster, int& cluster_id);
  void printMacroCluster(Cluster* cluster, int& cluster_id);
  std::pair<float, float> printPinPos(odb::dbInst* inst);
  void MLPartNetUtil(odb::dbModule* module,
                     const int src_id,
                     int& count,
                     std::vector<int>& col_idx,
                     std::vector<int>& row_idx,
                     std::vector<double>& edge_weight,
                     std::map<Cluster*, int>& node_map,
                     std::map<int, odb::dbInst*>& idx_to_inst,
                     std::map<odb::dbInst*, int>& inst_to_idx);
  void MLPartBufferNetUtil(const int src_id,
                           int& count,
                           std::vector<int>& col_idx,
                           std::vector<int>& row_idx,
                           std::vector<double>& edge_weight,
                           std::map<Cluster*, int>& node_map,
                           std::map<int, odb::dbInst*>& idx_to_inst,
                           std::map<odb::dbInst*, int>& inst_to_idx);
};

}  // namespace par
