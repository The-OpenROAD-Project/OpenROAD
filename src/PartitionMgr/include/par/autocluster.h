#pragma once

#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include "db_sta/dbReadVerilog.hh"
#include "opendb/db.h"
#include "sta/Liberty.hh"
#include "utl/Logger.h"

namespace par {

class Cluster
{
 public:
  Cluster() {}
  Cluster(int id, bool type, std::string name)
      : id_(id), type_(type), name_(name)
  {
  }

  // Accessor
  int getId() const { return id_; }
  bool getType() const { return type_; }
  sta::Instance* getTopInstance() const { return top_inst_; }
  const std::string& getName() const { return name_; }
  const std::vector<std::string>& getLogicalModuleVec() const
  {
    return logical_module_vec_;
  }
  const std::vector<sta::Instance*>& getInsts() const { return inst_vec_; }
  const std::vector<sta::Instance*>& getMacros() const { return macro_vec_; }
  unsigned int getNumMacro() const { return macro_vec_.size(); }
  unsigned int getNumInst() const { return inst_vec_.size(); }
  const std::unordered_map<int, unsigned int>& getInputConnections() const
  {
    return input_connection_map_;
  }

  const std::unordered_map<int, unsigned int>& getOutputConnections() const
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

  float calculateArea(ord::dbVerilogNetwork* network) const;

  void addInst(sta::Instance* inst) { inst_vec_.push_back(inst); }
  void addMacro(sta::Instance* inst) { macro_vec_.push_back(inst); }
  void setTopInst(sta::Instance* inst) { top_inst_ = inst; }
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
  void printInputConnections()
  {
    for (auto [cluster_id, num_conn] : input_connection_map_) {
      std::cout << "cluster_id:   " << cluster_id << "   ";
      std::cout << "num_connections:   " << num_conn << "   ";
      std::cout << std::endl;
    }
  }

  void printOutputConnections()
  {
    for (auto [cluster_id, num_conn] : output_connection_map_) {
      std::cout << "cluster_id:   " << cluster_id << "   ";
      std::cout << "num_connections:   " << num_conn << "   ";
      std::cout << std::endl;
    }
  }

 private:
  int id_ = 0;
  bool type_ = true;  // false for glue logic
  sta::Instance* top_inst_ = nullptr;
  std::string name_ = "";
  std::vector<std::string> logical_module_vec_;
  std::vector<sta::Instance*> inst_vec_;
  std::vector<sta::Instance*> macro_vec_;
  std::unordered_map<int, unsigned int> input_connection_map_;
  std::unordered_map<int, unsigned int> output_connection_map_;
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
  AutoClusterMgr(ord::dbVerilogNetwork* network,
                 odb::dbDatabase* db,
                 utl::Logger* logger)
      : network_(network), db_(db), logger_(logger)
  {
  }

  void partitionDesign(unsigned int max_num_macro,
                       unsigned int min_num_macro,
                       unsigned int max_mum_inst,
                       unsigned int min_num_inst,
                       unsigned int net_threshold,
                       unsigned int virtual_weight,
                       const char* file_name);

 private:
  ord::dbVerilogNetwork* network_ = nullptr;
  odb::dbDatabase* db_ = nullptr;
  odb::dbBlock* block_ = nullptr;
  utl::Logger* logger_;
  unsigned int max_num_macro_ = 0;
  unsigned int min_num_macro_ = 0;
  unsigned int max_num_inst_ = 0;
  unsigned int min_num_inst_ = 0;
  unsigned int net_threshold_ = 0;
  unsigned int virtual_weight_ = 10000;
  unsigned int num_buffer_ = 0;
  float area_buffer_ = 0;

  float dbu_ = 0.0;

  int floorplan_lx_ = 0;
  int floorplan_ly_ = 0;
  int floorplan_ux_ = 0;
  int floorplan_uy_ = 0;

  // Map all the BTerms to one of "L", "R", "B" and "T"
  std::unordered_map<std::string, std::string> bterm_map_;
  std::unordered_map<std::string, int> bundled_io_map_;
  std::unordered_map<sta::Instance*, Metric> logical_cluster_map_;
  std::unordered_map<int, Cluster*> cluster_map_;
  std::unordered_map<sta::Instance*, int> inst_map_;

  std::unordered_map<int, int> virtual_map_;

  std::map<sta::Instance*, int> buffer_map_;
  int buffer_id_ = -1;
  std::vector<std::vector<sta::Net*>> buffer_net_vec_;
  std::vector<sta::Net*> buffer_net_list_;

  std::vector<Cluster*> cluster_list_;
  std::vector<Cluster*> merge_cluster_list_;
  std::queue<Cluster*> break_cluster_list_;
  std::queue<Cluster*> mlpart_cluster_list_;

  void createBundledIO();
  Metric traverseLogicalHierarchy(sta::Instance* inst);
  void getBufferNet();
  void getBufferNetUtil(
      sta::Instance* inst,
      std::vector<std::pair<sta::Net*, sta::Net*>>& buffer_net);

  void createCluster(int& cluster_id);
  void createClusterUtil(sta::Instance* inst, int& cluster_id);
  void updateConnection();
  void calculateConnection(sta::Instance* inst);
  void calculateBufferNetConnection();
  bool hasTerminals(sta::Net* net);
  unsigned int calculateClusterNumInst(std::vector<Cluster*>& cluster_vec);
  unsigned int calculateClusterNumMacro(std::vector<Cluster*>& cluster_vec);
  void mergeCluster(Cluster* src, Cluster* target);
  void merge(std::string parent_name);
  void mergeMacro(std::string parent_name, int std_cell_id);
  void mergeMacroUtil(std::string parent_name,
                      int& merge_index,
                      int std_cell_id);
  void mergeUtil(std::string parent_name, int& merge_index);
  void breakCluster(Cluster* cluster_old, int& cluster_id);
  void MLPart(Cluster* cluster, int& cluster_id);
  void MacroPart(Cluster* cluster, int& cluster_id);
  void printMacroCluster(Cluster* cluster, int& cluster_id);
  std::pair<float, float> printPinPos(sta::Instance* inst);
  void MLPartNetUtil(sta::Instance* inst,
                     const int src_id,
                     int& count,
                     std::vector<int>& col_idx,
                     std::vector<int>& row_idx,
                     std::vector<double>& edge_weight,
                     std::unordered_map<Cluster*, int>& node_map,
                     std::unordered_map<int, sta::Instance*>& idx_to_inst,
                     std::unordered_map<sta::Instance*, int>& inst_to_idx);
  void MLPartBufferNetUtil(
      const int src_id,
      int& count,
      std::vector<int>& col_idx,
      std::vector<int>& row_idx,
      std::vector<double>& edge_weight,
      std::unordered_map<Cluster*, int>& node_map,
      std::unordered_map<int, sta::Instance*>& idx_to_inst,
      std::unordered_map<sta::Instance*, int>& inst_to_idx);
};
}  // namespace par

namespace ord {

void dbPartitionDesign(ord::dbVerilogNetwork* network,
                       odb::dbDatabase* db,
                       unsigned int max_num_macro,
                       unsigned int min_num_macro,
                       unsigned int max_num_inst,
                       unsigned int min_num_inst,
                       unsigned int net_threshold,
                       unsigned int virtual_weight,
                       const char* file_name,
                       utl::Logger* logger);

}
