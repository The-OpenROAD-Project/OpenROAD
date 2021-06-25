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

namespace utl {
class Logger;
}

using utl::Logger;

namespace par {
// * L1, L2, L3  : all the BTerms on the left boundary
// * T1, T2, T3  : all the BTerms on the top boundary
// * R1, R2, R3  : all the BTerms on the right boundary
// * B1, B2, B3  : all the BTerms on the bottom boundary
class BundledIO
{
 public:
  BundledIO() {}
  BundledIO(int id, std::string name) : _id(id), _name(name) {}

  // accessor
  unsigned int getNumBTerms() { return _bterm_vec.size(); }
  int getId() { return _id; }
  std::string getName() { return _name; }
  std::vector<std::string> getBTerms() { return _bterm_vec; }
  bool findBTerm(std::string bterm_name)
  {
    std::vector<std::string>::iterator iter;
    iter = std::find(_bterm_vec.begin(), _bterm_vec.end(), bterm_name);
    if (iter == _bterm_vec.end())
      return false;
    else
      return true;
  }

  // operations
  void addBTerm(std::string bterm_name)
  {
    if (findBTerm(bterm_name) == false)
      _bterm_vec.push_back(bterm_name);
  }

 private:
  int _id = 0;
  std::string _name = "";
  std::vector<std::string> _bterm_vec;
};

class Cluster
{
 public:
  Cluster() {}
  Cluster(int id, bool type, std::string name)
      : _id(id), _type(type), _name(name)
  {
  }

  // Accessor
  int getId() { return _id; }
  bool getType() { return _type; }
  sta::Instance* getTopInstance() { return _top_inst; }
  std::string getName() { return _name; }
  std::vector<std::string> getLogicalModuleVec() { return _logical_module_vec; }
  std::vector<sta::Instance*> getInstVec() { return _inst_vec; }
  std::vector<sta::Instance*> getMacroVec() { return _macro_vec; }
  unsigned int getNumMacro() { return _macro_vec.size(); }
  unsigned int getNumInst() { return _inst_vec.size(); }
  std::unordered_map<int, unsigned int> getInputConnectionMap()
  {
    return _input_connection_map;
  }

  std::unordered_map<int, unsigned int> getOutputConnectionMap()
  {
    return _output_connection_map;
  }

  unsigned int getInputConnection(int cluster_id)
  {
    std::unordered_map<int, unsigned int>::iterator map_iter;
    map_iter = _input_connection_map.find(cluster_id);
    if (map_iter == _input_connection_map.end())
      return 0;
    else
      return map_iter->second;
  }

  unsigned int getOutputConnection(int cluster_id)
  {
    std::unordered_map<int, unsigned int>::iterator map_iter;
    map_iter = _output_connection_map.find(cluster_id);
    if (map_iter == _output_connection_map.end())
      return 0;
    else
      return map_iter->second;
  }

  // operations
  void removeMacro() { _macro_vec.clear(); }

  float calculateArea(ord::dbVerilogNetwork* network);

  void addInst(sta::Instance* inst) { _inst_vec.push_back(inst); }
  void addMacro(sta::Instance* inst) { _macro_vec.push_back(inst); }
  void specifyTopInst(sta::Instance* inst) { _top_inst = inst; }
  void specifyName(std::string name) { _name = name; }
  void addLogicalModule(std::string module_name)
  {
    _logical_module_vec.push_back(module_name);
  }

  void addLogicalModuleVec(std::vector<std::string> module_vec)
  {
    for (auto& module : module_vec)
      _logical_module_vec.push_back(module);
  }

  void initConnection()
  {
    _input_connection_map.clear();
    _output_connection_map.clear();
  }

  void addInputConnection(int cluster_id, unsigned int weight = 1)
  {
    std::unordered_map<int, unsigned int>::iterator map_iter;
    map_iter = _input_connection_map.find(cluster_id);
    if (map_iter == _input_connection_map.end())
      _input_connection_map[cluster_id] = weight;
    else
      map_iter->second += weight;
  }

  void addOutputConnection(int cluster_id, unsigned int weight = 1)
  {
    std::unordered_map<int, unsigned int>::iterator map_iter;
    map_iter = _output_connection_map.find(cluster_id);
    if (map_iter == _output_connection_map.end())
      _output_connection_map[cluster_id] = weight;
    else
      map_iter->second += weight;
  }

  // These functions only for test
  void printInputConnection()
  {
    std::unordered_map<int, unsigned int>::iterator map_iter;
    map_iter = _input_connection_map.begin();
    while (map_iter != _input_connection_map.end()) {
      std::cout << "cluster_id:   " << map_iter->first << "   ";
      std::cout << "num_connections:   " << map_iter->second << "   ";
      std::cout << std::endl;
      map_iter++;
    }
  }

  void printOutputConnection()
  {
    std::unordered_map<int, unsigned int>::iterator map_iter;
    map_iter = _output_connection_map.begin();
    while (map_iter != _output_connection_map.end()) {
      std::cout << "cluster_id:   " << map_iter->first << "   ";
      std::cout << "num_connections:   " << map_iter->second << "   ";
      std::cout << std::endl;
      map_iter++;
    }
  }

 private:
  int _id = 0;
  bool _type = true;  // false for glue logic
  sta::Instance* _top_inst = nullptr;
  std::string _name = "";
  std::vector<std::string> _logical_module_vec;
  std::vector<sta::Instance*> _inst_vec;
  std::vector<sta::Instance*> _macro_vec;
  std::unordered_map<int, unsigned int> _input_connection_map;
  std::unordered_map<int, unsigned int> _output_connection_map;
};

class Metric
{
 public:
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

class autoclusterMgr
{
 public:
  autoclusterMgr(ord::dbVerilogNetwork* network,
                 odb::dbDatabase* db,
                 Logger* logger)
      : _network(network), _db(db), _logger(logger)
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
  ord::dbVerilogNetwork* _network = nullptr;
  odb::dbDatabase* _db = nullptr;
  odb::dbBlock* _block = nullptr;
  Logger* _logger;
  unsigned int _max_num_macro = 0;
  unsigned int _min_num_macro = 0;
  unsigned int _max_num_inst = 0;
  unsigned int _min_num_inst = 0;
  unsigned int _net_threshold = 0;
  unsigned int _virtual_weight = 10000;
  unsigned int _num_buffer = 0;
  float _area_buffer = 0;

  float _dbu = 0.0;

  int _floorplan_lx = 0;
  int _floorplan_ly = 0;
  int _floorplan_ux = 0;
  int _floorplan_uy = 0;

  // Map all the BTerms to one of "L", "R", "B" and "T"
  std::unordered_map<std::string, std::string> _bterm_map;
  std::unordered_map<std::string, int> _bundled_io_map;
  std::unordered_map<sta::Instance*, Metric> _logical_cluster_map;
  std::unordered_map<int, Cluster*> _cluster_map;
  std::unordered_map<sta::Instance*, int> _inst_map;

  std::unordered_map<int, int> _virtual_map;

  std::map<sta::Instance*, int> _buffer_map;
  int _buffer_id = -1;
  std::vector<std::vector<sta::Net*>> _buffer_net_vec;
  std::vector<sta::Net*> _buffer_net_list;

  std::vector<Cluster*> _cluster_list;
  std::vector<Cluster*> _merge_cluster_list;
  std::queue<Cluster*> _break_cluster_list;
  std::queue<Cluster*> _mlpart_cluster_list;

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
                     int& src_id,
                     int& count,
                     std::vector<int>& col_idx,
                     std::vector<int>& row_idx,
                     std::vector<double>& edge_weight,
                     std::unordered_map<Cluster*, int>& node_map,
                     std::unordered_map<int, sta::Instance*>& idx_to_inst,
                     std::unordered_map<sta::Instance*, int>& inst_to_idx);
  void MLPartBufferNetUtil(
      int& src_id,
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
                       Logger* logger);

}
