#include "opendb/db.h"
#include "autocluster.h"
#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
#ifdef PARTITIONERS
#include "MLPart.h"
#endif
#include <sys/stat.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <queue>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "utl/Logger.h"

using utl::PAR;

namespace par {
using std::ceil;
using std::cout;
using std::endl;
using std::find;
using std::floor;
using std::map;
using std::max;
using std::min;
using std::ofstream;
using std::pair;
using std::queue;
using std::string;
using std::to_string;
using std::tuple;
using std::unordered_map;
using std::vector;

using odb::dbBlock;
using odb::dbBox;
using odb::dbDatabase;
using odb::dbInst;
using odb::dbMaster;
using odb::dbMPin;
using odb::dbMTerm;
using odb::dbSet;
using odb::dbSigType;
using odb::dbStringProperty;
using odb::Rect;

using sta::Cell;
using sta::Instance;
using sta::InstanceChildIterator;
using sta::InstancePinIterator;
using sta::LeafInstanceIterator;
using sta::LibertyCell;
using sta::LibertyCellPortIterator;
using sta::LibertyPort;
using sta::Net;
using sta::NetConnectedPinIterator;
using sta::NetIterator;
using sta::NetPinIterator;
using sta::NetTermIterator;
using sta::Pin;
using sta::PinSeq;
using sta::PortDirection;
using sta::Term;

// ******************************************************************************
// Class Cluster
// ******************************************************************************
float Cluster::calculateArea(ord::dbVerilogNetwork* network) const
{
  float area = 0.0;
  for (auto* inst : inst_vec_) {
    LibertyCell* liberty_cell = network->libertyCell(inst);
    area += liberty_cell->area();
  }

  for (auto* macro : macro_vec_) {
    LibertyCell* liberty_cell = network->libertyCell(macro);
    area += liberty_cell->area();
  }

  return area;
}

// ******************************************************************************
// Class AutoClusterMgr
// ******************************************************************************
//
//  traverseLogicalHierarchy
//  Recursive function to collect the design metrics (number of instances, hard
//  macros, area) in the logical hierarchy
//
Metric AutoClusterMgr::computeMetrics(sta::Instance* inst)
{
  float area = 0.0;
  unsigned int num_inst = 0;
  unsigned int num_macro = 0;

  InstanceChildIterator* child_iter = network_->childIterator(inst);
  while (child_iter->hasNext()) {
    Instance* child = child_iter->next();
    if (network_->isHierarchical(child)) {
      Metric metric = computeMetrics(child);
      area += metric.area;
      num_inst += metric.num_inst;
      num_macro += metric.num_macro;
    } else {
      LibertyCell* liberty_cell = network_->libertyCell(child);
      area += liberty_cell->area();
      if (liberty_cell->isBuffer()) {
        num_buffer_ += 1;
        area_buffer_ += liberty_cell->area();
        buffer_map_[child] = ++buffer_id_;
      }

      Cell* cell = network_->cell(child);
      const char* cell_name = network_->name(cell);
      dbMaster* master = db_->findMaster(cell_name);
      if (master->isBlock())
        num_macro += 1;
      else
        num_inst += 1;
    }
  }
  Metric metric = Metric(area, num_macro, num_inst);
  logical_cluster_map_[inst] = metric;
  return metric;
}

static bool isConnectedNet(const pair<Net*, Net*>& p1,
                           const pair<Net*, Net*>& p2)
{
  if (p1.first != nullptr) {
    if (p1.first == p2.first || p1.first == p2.second)
      return true;
  }

  if (p1.second != nullptr) {
    if (p1.second == p2.first || p1.second == p2.second)
      return true;
  }

  return false;
}

static void appendNet(vector<Net*>& vec, pair<Net*, Net*>& p)
{
  if (p.first != nullptr) {
    if (find(vec.begin(), vec.end(), p.first) == vec.end())
      vec.push_back(p.first);
  }

  if (p.second != nullptr) {
    if (find(vec.begin(), vec.end(), p.second) == vec.end())
      vec.push_back(p.second);
  }
}

//
// Handle Buffer transparency for handling net connection across buffers
//  RV? Is this handling chain of buffers?
//  TODO: handle inverter pairs
//
void AutoClusterMgr::getBufferNet()
{
  vector<pair<Net*, Net*>> buffer_net;
  for (int i = 0; i <= buffer_id_; i++) {
    buffer_net.push_back({nullptr, nullptr});
  }
  getBufferNetUtil(network_->topInstance(), buffer_net);

  vector<int> class_array(buffer_id_ + 1);
  for (int i = 0; i <= buffer_id_; i++)
    class_array[i] = i;

  int unique_id = 0;

  for (int i = 0; i <= buffer_id_; i++) {
    if (class_array[i] == i)
      class_array[i] = unique_id++;

    for (int j = i + 1; j <= buffer_id_; j++) {
      if (isConnectedNet(buffer_net[i], buffer_net[j]))
        class_array[j] = class_array[i];
    }
  }

  for (int i = 0; i < unique_id; i++) {
    vector<Net*> vec_net;
    buffer_net_vec_.push_back(vec_net);
  }

  for (int i = 0; i <= buffer_id_; i++) {
    appendNet(buffer_net_vec_[class_array[i]], buffer_net[i]);
  }
}

void AutoClusterMgr::getBufferNetUtil(Instance* inst,
                                      vector<pair<Net*, Net*>>& buffer_net)
{
  const bool is_top = (inst == network_->topInstance());
  NetIterator* net_iter = network_->netIterator(inst);
  while (net_iter->hasNext()) {
    Net* net = net_iter->next();
    bool with_buffer = false;
    if (is_top || !hasTerminals(net)) {
      NetConnectedPinIterator* pin_iter = network_->connectedPinIterator(net);
      while (pin_iter->hasNext()) {
        Pin* pin = pin_iter->next();
        if (network_->isLeaf(pin)) {
          Instance* child_inst = network_->instance(pin);
          LibertyCell* liberty_cell = network_->libertyCell(child_inst);
          if (liberty_cell->isBuffer()) {
            with_buffer = true;
            const int buffer_id = buffer_map_[child_inst];

            if (buffer_net[buffer_id].first == nullptr)
              buffer_net[buffer_id].first = net;
            else if (buffer_net[buffer_id].second == nullptr)
              buffer_net[buffer_id].second = net;
            else
              logger_->error(
                  PAR, 401, "Buffer Net has more than two net connection...");
          }
        }
      }

      if (with_buffer == true) {
        buffer_net_list_.push_back(net);
      }
    }
  }

  delete net_iter;

  InstanceChildIterator* child_iter = network_->childIterator(inst);
  while (child_iter->hasNext()) {
    Instance* child = child_iter->next();
    getBufferNetUtil(child, buffer_net);
  }

  delete child_iter;
}

//
//  Create a bundled model for external pins.  Group boundary pins into bundles
//  Currently creates 3 groups per side
//  TODO
//     1) Make it customizable  number of bins per side
//     2) Use area to determine bundles
//     3) handle rectilinear boundaries and area pins (3D connections)
//
void AutoClusterMgr::createBundledIO()
{
  // Get the floorplan information

  Rect die_box;
  block_->getDieArea(die_box);

  floorplan_lx_ = die_box.xMin();
  floorplan_ly_ = die_box.yMin();
  floorplan_ux_ = die_box.xMax();
  floorplan_uy_ = die_box.yMax();

  // Map all the BTerms to IORegions
  for (auto term : block_->getBTerms()) {
    std::string bterm_name = term->getName();
    int lx = INT_MAX;
    int ly = INT_MAX;
    int ux = 0;
    int uy = 0;
    for (auto pin : term->getBPins()) {
      for (auto box : pin->getBoxes()) {
        lx = min(lx, box->xMin());
        ly = min(ly, box->yMin());
        ux = max(ux, box->xMax());
        uy = max(uy, box->yMax());
      }
    }

    int x_third = floorplan_ux_ / 3;
    int y_third = floorplan_uy_ / 3;

    if (lx == floorplan_lx_) {  // Left
      if (uy <= y_third)
        bterm_map_[bterm_name] = LeftLower;
      else if (ly >= 2 * y_third)
        bterm_map_[bterm_name] = LeftUpper;
      else
        bterm_map_[bterm_name] = LeftMiddle;
    } else if (ux == floorplan_ux_) {  // Right
      if (uy <= y_third)
        bterm_map_[bterm_name] = RightLower;
      else if (ly >= 2 * y_third)
        bterm_map_[bterm_name] = RightUpper;
      else
        bterm_map_[bterm_name] = RightMiddle;
    } else if (ly == floorplan_ly_) {  // Bottom
      if (ux <= x_third)
        bterm_map_[bterm_name] = BottomLower;
      else if (lx >= 2 * x_third)
        bterm_map_[bterm_name] = BottomUpper;
      else
        bterm_map_[bterm_name] = BottomMiddle;
    } else if (uy == floorplan_uy_) {  // Top
      if (ux <= x_third)
        bterm_map_[bterm_name] = TopLower;
      else if (lx >= 2 * x_third)
        bterm_map_[bterm_name] = TopUpper;
      else
        bterm_map_[bterm_name] = TopMiddle;
    } else {
      logger_->error(
          PAR, 400, "Floorplan has not been initialized? Pin location error.");
    }
  }
}

void AutoClusterMgr::createCluster(int& cluster_id)
{
  // This function will only be called by top instance
  Instance* inst = network_->topInstance();
  const Metric metric = logical_cluster_map_[inst];
  bool is_hier = false;
  if (metric.num_macro > max_num_macro_ || metric.num_inst > max_num_inst_) {
    InstanceChildIterator* child_iter = network_->childIterator(inst);
    vector<Instance*> glue_inst_vec;
    while (child_iter->hasNext()) {
      Instance* child = child_iter->next();
      if (network_->isHierarchical(child)) {
        createClusterUtil(child, cluster_id);
        is_hier = true;
      } else
        glue_inst_vec.push_back(child);
    }

    // Create cluster for glue logic
    if (glue_inst_vec.size() >= 1) {
      string name = "top";
      if (!is_hier)
        name += "_glue_logic";
      Cluster* cluster = new Cluster(++cluster_id, !is_hier, name);
      vector<Instance*>::iterator vec_iter;
      for (vec_iter = glue_inst_vec.begin(); vec_iter != glue_inst_vec.end();
           vec_iter++) {
        LibertyCell* liberty_cell = network_->libertyCell(*vec_iter);
        if (liberty_cell->isBuffer() == true)
          continue;

        Cell* cell = network_->cell(*vec_iter);
        const char* cell_name = network_->name(cell);
        dbMaster* master = db_->findMaster(cell_name);
        if (master->isBlock())
          cluster->addMacro(*vec_iter);
        else
          cluster->addInst(*vec_iter);
        inst_map_[*vec_iter] = cluster_id;
      }
      cluster_map_[cluster_id] = cluster;

      if (cluster->getNumInst() >= min_num_inst_
          || cluster->getNumMacro() >= min_num_macro_) {
        cluster_list_.push_back(cluster);
      } else {
        merge_cluster_list_.push_back(cluster);
      }
    }
  } else {
    // This no need to do any clustering
    Cluster* cluster = new Cluster(++cluster_id, true, string("top_instance"));
    cluster_map_[cluster_id] = cluster;
    cluster->setTopInst(inst);
    cluster->addLogicalModule(string("top_instance"));
    LeafInstanceIterator* leaf_iter = network_->leafInstanceIterator(inst);
    while (leaf_iter->hasNext()) {
      Instance* leaf_inst = leaf_iter->next();
      LibertyCell* liberty_cell = network_->libertyCell(leaf_inst);
      if (liberty_cell->isBuffer() == false) {
        Cell* cell = network_->cell(leaf_inst);
        const char* cell_name = network_->name(cell);
        dbMaster* master = db_->findMaster(cell_name);
        if (master->isBlock())
          cluster->addMacro(leaf_inst);
        else
          cluster->addInst(leaf_inst);

        inst_map_[leaf_inst] = cluster_id;
      }
    }
    cluster_list_.push_back(cluster);
  }
}

void AutoClusterMgr::createClusterUtil(Instance* inst, int& cluster_id)
{
  Cluster* cluster
      = new Cluster(++cluster_id, true, string(network_->pathName(inst)));
  cluster->setTopInst(inst);
  cluster->addLogicalModule(string(network_->pathName(inst)));
  cluster_map_[cluster_id] = cluster;
  LeafInstanceIterator* leaf_iter = network_->leafInstanceIterator(inst);
  while (leaf_iter->hasNext()) {
    Instance* leaf_inst = leaf_iter->next();
    LibertyCell* liberty_cell = network_->libertyCell(leaf_inst);
    if (liberty_cell->isBuffer() == false) {
      Cell* cell = network_->cell(leaf_inst);
      const char* cell_name = network_->name(cell);
      dbMaster* master = db_->findMaster(cell_name);
      if (master->isBlock())
        cluster->addMacro(leaf_inst);
      else
        cluster->addInst(leaf_inst);

      inst_map_[leaf_inst] = cluster_id;
    }
  }

  if (cluster->getNumMacro() >= max_num_macro_
      || cluster->getNumInst() >= max_num_inst_) {
    cluster_list_.push_back(cluster);
    break_cluster_list_.push(cluster);
  } else if (cluster->getNumMacro() >= min_num_macro_
             || cluster->getNumInst() >= min_num_inst_) {
    cluster_list_.push_back(cluster);
  } else {
    merge_cluster_list_.push_back(cluster);
  }
}

void AutoClusterMgr::updateConnection()
{
  unordered_map<int, Cluster*>::iterator map_it;
  for (map_it = cluster_map_.begin(); map_it != cluster_map_.end(); map_it++) {
    map_it->second->initConnection();
  }
  calculateConnection(network_->topInstance());
  calculateBufferNetConnection();
}

bool AutoClusterMgr::hasTerminals(Net* net)
{
  NetTermIterator* term_iter = network_->termIterator(net);
  const bool has_terms = term_iter->hasNext();
  delete term_iter;
  return has_terms;
}

void AutoClusterMgr::calculateBufferNetConnection()
{
  for (int i = 0; i < buffer_net_vec_.size(); i++) {
    int driver_id = 0;
    vector<int> loads_id;
    vector<int>::iterator vec_iter;
    for (int j = 0; j < buffer_net_vec_[i].size(); j++) {
      Net* net = buffer_net_vec_[i][j];
      const bool is_top = network_->instance(net) == network_->topInstance();
      if (is_top || !hasTerminals(net)) {
        NetConnectedPinIterator* pin_iter = network_->connectedPinIterator(net);
        while (pin_iter->hasNext()) {
          Pin* pin = pin_iter->next();
          if (network_->isTopLevelPort(pin)) {
            const char* port_name = network_->portName(pin);
            const int id = bundled_io_map_[bterm_map_[string(port_name)]];
            const PortDirection* port_dir = network_->direction(pin);
            if (port_dir == PortDirection::input()) {
              driver_id = id;
            } else {
              vec_iter = find(loads_id.begin(), loads_id.end(), id);
              if (vec_iter == loads_id.end())
                loads_id.push_back(id);
            }
          } else if (network_->isLeaf(pin)) {
            Instance* inst = network_->instance(pin);
            LibertyCell* liberty_cell = network_->libertyCell(inst);
            if (liberty_cell->isBuffer() == false) {
              const PortDirection* port_dir = network_->direction(pin);
              const int id = inst_map_[inst];
              if (port_dir == PortDirection::output()) {
                driver_id = id;
              } else {
                vec_iter = find(loads_id.begin(), loads_id.end(), id);
                if (vec_iter == loads_id.end())
                  loads_id.push_back(id);
              }
            }
          }
        }
      }
    }

    if (driver_id != 0 && loads_id.size() > 0) {
      Cluster* driver_cluster = cluster_map_[driver_id];
      for (int i = 0; i < loads_id.size(); i++) {
        cluster_map_[driver_id]->addOutputConnection(loads_id[i]);
        cluster_map_[loads_id[i]]->addInputConnection(driver_id);
      }
    }
  }
}

void AutoClusterMgr::calculateConnection(Instance* inst)
{
  const bool is_top = (inst == network_->topInstance());
  NetIterator* net_iter = network_->netIterator(inst);
  while (net_iter->hasNext()) {
    Net* net = net_iter->next();
    int driver_id = 0;
    vector<int> loads_id;
    vector<int>::iterator vec_iter;
    bool buffer_flag = false;
    if (find(buffer_net_list_.begin(), buffer_net_list_.end(), net)
        != buffer_net_list_.end())
      buffer_flag = true;

    if ((buffer_flag == false) && (is_top || !hasTerminals(net))) {
      NetConnectedPinIterator* pin_iter = network_->connectedPinIterator(net);
      while (pin_iter->hasNext()) {
        const Pin* pin = pin_iter->next();
        if (network_->isTopLevelPort(pin)) {
          const char* port_name = network_->portName(pin);
          const int id = bundled_io_map_[bterm_map_[string(port_name)]];
          const PortDirection* port_dir = network_->direction(pin);
          if (port_dir == PortDirection::input()) {
            driver_id = id;
          } else {
            vec_iter = find(loads_id.begin(), loads_id.end(), id);
            if (vec_iter == loads_id.end())
              loads_id.push_back(id);
          }
        } else if (network_->isLeaf(pin)) {
          Instance* inst = network_->instance(pin);
          const PortDirection* port_dir = network_->direction(pin);
          const int id = inst_map_[inst];
          if (port_dir == PortDirection::output()) {
            driver_id = id;
          } else {
            vec_iter = find(loads_id.begin(), loads_id.end(), id);
            if (vec_iter == loads_id.end())
              loads_id.push_back(id);
          }
        }
      }

      if (driver_id != 0 && loads_id.size() > 0) {
        Cluster* driver_cluster = cluster_map_[driver_id];
        for (int i = 0; i < loads_id.size(); i++) {
          cluster_map_[driver_id]->addOutputConnection(loads_id[i]);
          cluster_map_[loads_id[i]]->addInputConnection(driver_id);
        }
      }
    }
  }

  delete net_iter;

  InstanceChildIterator* child_iter = network_->childIterator(inst);
  while (child_iter->hasNext()) {
    Instance* child = child_iter->next();
    calculateConnection(child);
  }

  delete child_iter;
}

void AutoClusterMgr::merge(string parent_name)
{
  if (merge_cluster_list_.size() == 0)
    return;

  if (merge_cluster_list_.size() == 1) {
    cluster_list_.push_back(merge_cluster_list_[0]);
    merge_cluster_list_.clear();
    updateConnection();
    return;
  }

  unsigned int num_inst = calculateClusterNumInst(merge_cluster_list_);
  unsigned int num_macro = calculateClusterNumMacro(merge_cluster_list_);
  int iteration = 0;
  int merge_index = 0;
  while (num_inst > max_num_inst_ || num_macro > max_num_macro_) {
    int num_merge_cluster = merge_cluster_list_.size();
    mergeUtil(parent_name, merge_index);
    if (num_merge_cluster == merge_cluster_list_.size())
      break;

    num_inst = calculateClusterNumInst(merge_cluster_list_);
    num_macro = calculateClusterNumMacro(merge_cluster_list_);
  }

  if (merge_cluster_list_.size() > 1)
    for (int i = 1; i < merge_cluster_list_.size(); i++) {
      mergeCluster(merge_cluster_list_[0], merge_cluster_list_[i]);
      delete merge_cluster_list_[i];
    }

  if (merge_cluster_list_.size() > 0) {
    merge_cluster_list_[0]->setName(parent_name + string("_cluster_")
                                    + to_string(merge_index++));
    cluster_list_.push_back(merge_cluster_list_[0]);
    merge_cluster_list_.clear();
  }
  updateConnection();
}

unsigned int AutoClusterMgr::calculateClusterNumMacro(
    vector<Cluster*>& cluster_vec)
{
  unsigned int num_macro = 0;
  for (auto cluster : cluster_vec)
    num_macro += cluster->getNumMacro();

  return num_macro;
}

unsigned int AutoClusterMgr::calculateClusterNumInst(
    vector<Cluster*>& cluster_vec)
{
  unsigned int num_inst = 0;
  for (auto cluster : cluster_vec)
    num_inst += cluster->getNumInst();

  return num_inst;
}

//
// Merge target cluster into src
// RV -- del targe cluster?
//
void AutoClusterMgr::mergeCluster(Cluster* src, Cluster* target)
{
  const int src_id = src->getId();
  const int target_id = target->getId();
  cluster_map_.erase(target_id);
  src->addLogicalModuleVec(target->getLogicalModuleVec());

  for (auto inst : target->getInsts()) {
    src->addInst(inst);
    inst_map_[inst] = src_id;
  }

  for (auto macro : target->getMacros()) {
    src->addMacro(macro);
    inst_map_[macro] = src_id;
  }
}

void AutoClusterMgr::mergeUtil(string parent_name, int& merge_index)
{
  vector<int> outside_vec;
  vector<int> merge_vec;

  for (auto cluster : cluster_list_)
    outside_vec.push_back(cluster->getId());

  for (auto cluster : merge_cluster_list_)
    merge_vec.push_back(cluster->getId());

  const int M = merge_vec.size();
  const int N = outside_vec.size();
  vector<bool> internal_flag(M);
  vector<int> class_id(M);
  vector<vector<bool>> graph(M);

  for (int i = 0; i < M; i++) {
    graph[i].resize(N);
    internal_flag[i] = true;
    class_id[i] = i;
  }

  for (int i = 0; i < M; i++) {
    for (int j = 0; j < N; j++) {
      const unsigned int input
          = merge_cluster_list_[i]->getInputConnection(outside_vec[j]);
      const unsigned int output
          = merge_cluster_list_[i]->getOutputConnection(outside_vec[j]);
      if (input + output > net_threshold_) {
        graph[i][j] = true;
        internal_flag[i] = false;
      }
    }
  }

  for (int i = 0; i < M; i++) {
    if (internal_flag[i] == false && class_id[i] == i) {
      for (int j = i + 1; j < M; j++) {
        bool flag = true;
        for (int k = 0; k < N; k++) {
          if (flag == false)
            break;
          flag = flag && (graph[i][k] == graph[j][k]);
        }
        if (flag == true)
          class_id[j] = i;
      }
    }
  }

  // Merge clusters with same connection topology
  for (int i = 0; i < M; i++) {
    if (internal_flag[i] == false && class_id[i] == i) {
      for (int j = i + 1; j < M; j++) {
        if (class_id[j] == i) {
          mergeCluster(merge_cluster_list_[i], merge_cluster_list_[j]);
        }
      }
    }
  }

  vector<Cluster*> temp_cluster_vec;
  for (int i = 0; i < M; i++) {
    if (class_id[i] == i) {
      const int num_inst = merge_cluster_list_[i]->getNumInst();
      const int num_macro = merge_cluster_list_[i]->getNumMacro();
      if (num_inst >= min_num_inst_ || num_macro >= min_num_macro_) {
        cluster_list_.push_back(merge_cluster_list_[i]);
        merge_cluster_list_[i]->setName(parent_name + string("_cluster_")
                                        + to_string(merge_index++));
      } else {
        temp_cluster_vec.push_back(merge_cluster_list_[i]);
      }
    } else {
      delete merge_cluster_list_[i];
    }
  }

  merge_cluster_list_.clear();
  merge_cluster_list_ = temp_cluster_vec;

  updateConnection();
}

//
// Break a cluser (logical module) into its child modules and create a cluster
// each of the child modules
//
void AutoClusterMgr::breakCluster(Cluster* cluster_old, int& cluster_id)
{
  Instance* inst = cluster_old->getTopInstance();
  InstanceChildIterator* child_iter = network_->childIterator(inst);
  vector<Instance*> glue_inst_vec;
  bool is_hier = false;
  while (child_iter->hasNext()) {
    Instance* child = child_iter->next();
    if (network_->isHierarchical(child)) {
      is_hier = true;
      createClusterUtil(child, cluster_id);
    } else
      glue_inst_vec.push_back(child);
  }

  if (!is_hier) {
    return;
  }

  // Create cluster for glue logic
  if (glue_inst_vec.size() >= 1) {
    const string name = network_->pathName(inst) + string("_glue_logic");
    Cluster* cluster = new Cluster(++cluster_id, false, name);
    for (auto inst : glue_inst_vec) {
      LibertyCell* liberty_cell = network_->libertyCell(inst);
      if (liberty_cell->isBuffer() == false) {
        Cell* cell = network_->cell(inst);
        const char* cell_name = network_->name(cell);
        const dbMaster* master = db_->findMaster(cell_name);
        if (master->isBlock())
          cluster->addMacro(inst);
        else
          cluster->addInst(inst);

        inst_map_[inst] = cluster_id;
      }
    }
    cluster_map_[cluster_id] = cluster;

    //
    // Check cluster size. If it is smaller than min_inst threshold, add it to
    // merge_cluster list
    //
    if (cluster->getNumInst() >= min_num_inst_
        || cluster->getNumMacro() >= min_num_macro_) {
      cluster_list_.push_back(cluster);
    } else {
      merge_cluster_list_.push_back(cluster);
    }
  }

  cluster_map_.erase(cluster_old->getId());
  auto vec_it = find(cluster_list_.begin(), cluster_list_.end(), cluster_old);
  cluster_list_.erase(vec_it);
  delete cluster_old;
  updateConnection();
  merge(string(network_->pathName(inst)));
}

//
// For clusters that are greater than max_inst threshold, use MLPart to break
// the cluster into smaller clusters
//
void AutoClusterMgr::MLPart(Cluster* cluster, int& cluster_id)
{
  const int num_inst = cluster->getNumInst();
  if (num_inst < 2 * min_num_inst_)
    return;

  cluster_map_.erase(cluster->getId());
  auto vec_it = find(cluster_list_.begin(), cluster_list_.end(), cluster);
  cluster_list_.erase(vec_it);

  const int src_id = cluster->getId();
  unordered_map<int, Instance*> idx_to_inst;
  unordered_map<Instance*, int> inst_to_idx;
  vector<double> vertex_weight;
  vector<double> edge_weight;
  vector<int> col_idx;  // edges represented by vertex indices
  vector<int> row_ptr;  // pointers for edges
  int inst_id = 0;
  unordered_map<Cluster*, int> node_map;
  // we also consider outside world
  for (int i = 0; i < cluster_list_.size(); i++) {
    vertex_weight.push_back(1.0);
    node_map[cluster_list_[i]] = inst_id++;
  }

  vector<Instance*> inst_vec = cluster->getInsts();
  for (int i = 0; i < inst_vec.size(); i++) {
    idx_to_inst[inst_id++] = inst_vec[i];
    inst_to_idx[inst_vec[i]] = inst_id;
    vertex_weight.push_back(1.0);
  }

  int count = 0;
  MLPartNetUtil(network_->topInstance(),
                src_id,
                count,
                col_idx,
                row_ptr,
                edge_weight,
                node_map,
                idx_to_inst,
                inst_to_idx);
  MLPartBufferNetUtil(src_id,
                      count,
                      col_idx,
                      row_ptr,
                      edge_weight,
                      node_map,
                      idx_to_inst,
                      inst_to_idx);

  row_ptr.push_back(count);

  // Convert it to MLPart Format
  const int num_vertice = vertex_weight.size();
  const int num_edge = row_ptr.size() - 1;
  const int num_col_idx = col_idx.size();

  double* vertexWeight
      = (double*) malloc((unsigned) num_vertice * sizeof(double));
  int* rowPtr = (int*) malloc((unsigned) (num_edge + 1) * sizeof(int));
  int* colIdx = (int*) malloc((unsigned) (num_col_idx) * sizeof(int));
  double* edgeWeight = (double*) malloc((unsigned) num_edge * sizeof(double));
  int* part = (int*) malloc((unsigned) num_vertice * sizeof(int));

  for (int i = 0; i < num_vertice; i++) {
    part[i] = -1;
    vertexWeight[i] = 1.0;
  }

  for (int i = 0; i < num_edge; i++) {
    edgeWeight[i] = 1.0;
    rowPtr[i] = row_ptr[i];
  }

  rowPtr[num_edge] = row_ptr[num_edge];

  for (int i = 0; i < num_col_idx; i++)
    colIdx[i] = col_idx[i];

  // MLPart only support 2-way partition
  const int npart = 2;
  double balanceArray[2] = {0.5, 0.5};
  double tolerance = 0.05;
  unsigned int seed = 0;

#ifdef PARTITIONERS
  UMpack_mlpart(num_vertice,
                num_edge,
                vertexWeight,
                rowPtr,
                colIdx,
                edgeWeight,
                npart,  // Number of Partitions
                balanceArray,
                tolerance,
                part,
                1,  // Starts Per Run #TODO: add a tcl command
                1,  // Number of Runs
                0,  // Debug Level
                seed);
#endif

  const string name_part0 = cluster->getName() + string("_cluster_0");
  const string name_part1 = cluster->getName() + string("_cluster_1");
  Cluster* cluster_part0 = new Cluster(++cluster_id, true, name_part0);
  const int id_part0 = cluster_id;
  cluster_map_[id_part0] = cluster_part0;
  cluster_list_.push_back(cluster_part0);
  Cluster* cluster_part1 = new Cluster(++cluster_id, true, name_part1);
  const int id_part1 = cluster_id;
  cluster_map_[id_part1] = cluster_part1;
  cluster_list_.push_back(cluster_part1);
  cluster_part0->addLogicalModuleVec(cluster->getLogicalModuleVec());
  cluster_part1->addLogicalModuleVec(cluster->getLogicalModuleVec());

  for (int i = cluster_list_.size() - 2; i < num_vertice; i++) {
    if (part[i] == 0) {
      cluster_part0->addInst(idx_to_inst[i]);
      inst_map_[idx_to_inst[i]] = id_part0;
    } else {
      cluster_part1->addInst(idx_to_inst[i]);
      inst_map_[idx_to_inst[i]] = id_part1;
    }
  }

  if (cluster_part0->getNumInst() > max_num_inst_)
    mlpart_cluster_list_.push(cluster_part0);

  if (cluster_part1->getNumInst() > max_num_inst_)
    mlpart_cluster_list_.push(cluster_part1);

  delete cluster;
}

void AutoClusterMgr::MLPartNetUtil(Instance* inst,
                                   const int src_id,
                                   int& count,
                                   vector<int>& col_idx,
                                   vector<int>& row_ptr,
                                   vector<double>& edge_weight,
                                   unordered_map<Cluster*, int>& node_map,
                                   unordered_map<int, Instance*>& idx_to_inst,
                                   unordered_map<Instance*, int>& inst_to_idx)
{
  const bool is_top = (inst == network_->topInstance());
  NetIterator* net_iter = network_->netIterator(inst);
  while (net_iter->hasNext()) {
    Net* net = net_iter->next();
    int driver_id = -1;
    vector<int> loads_id;
    bool buffer_flag = false;
    if (find(buffer_net_list_.begin(), buffer_net_list_.end(), net)
        != buffer_net_list_.end())
      buffer_flag = true;

    if ((buffer_flag == false) && (is_top || !hasTerminals(net))) {
      NetConnectedPinIterator* pin_iter = network_->connectedPinIterator(net);
      while (pin_iter->hasNext()) {
        Pin* pin = pin_iter->next();
        if (network_->isTopLevelPort(pin)) {
          const char* port_name = network_->portName(pin);
          int id = bundled_io_map_[bterm_map_[string(port_name)]];
          id = node_map[cluster_map_[id]];
          PortDirection* port_dir = network_->direction(pin);
          if (port_dir == PortDirection::input()) {
            driver_id = id;
          } else {
            auto vec_iter = find(loads_id.begin(), loads_id.end(), id);
            if (vec_iter == loads_id.end())
              loads_id.push_back(id);
          }
        } else if (network_->isLeaf(pin)) {
          Instance* inst = network_->instance(pin);
          PortDirection* port_dir = network_->direction(pin);
          int id = inst_map_[inst];
          if (id == src_id)
            id = inst_to_idx[inst];
          else
            id = node_map[cluster_map_[id]];
          if (port_dir == PortDirection::output()) {
            driver_id = id;
          } else {
            auto vec_iter = find(loads_id.begin(), loads_id.end(), id);
            if (vec_iter == loads_id.end())
              loads_id.push_back(id);
          }
        }
      }

      if (driver_id != -1 && loads_id.size() > 0) {
        row_ptr.push_back(count);
        edge_weight.push_back(1.0);
        col_idx.push_back(driver_id);
        count++;
        for (int i = 0; i < loads_id.size(); i++) {
          col_idx.push_back(loads_id[i]);
          count++;
        }
      }
    }
  }

  delete net_iter;

  InstanceChildIterator* child_iter = network_->childIterator(inst);
  while (child_iter->hasNext()) {
    Instance* child = child_iter->next();
    MLPartNetUtil(child,
                  src_id,
                  count,
                  col_idx,
                  row_ptr,
                  edge_weight,
                  node_map,
                  idx_to_inst,
                  inst_to_idx);
  }

  delete child_iter;
}

void AutoClusterMgr::MLPartBufferNetUtil(
    const int src_id,
    int& count,
    vector<int>& col_idx,
    vector<int>& row_ptr,
    vector<double>& edge_weight,
    unordered_map<Cluster*, int>& node_map,
    unordered_map<int, Instance*>& idx_to_inst,
    unordered_map<Instance*, int>& inst_to_idx)
{
  for (int i = 0; i < buffer_net_vec_.size(); i++) {
    int driver_id = -1;
    vector<int> loads_id;
    for (int j = 0; j < buffer_net_vec_[i].size(); j++) {
      Net* net = buffer_net_vec_[i][j];
      const bool is_top = network_->instance(net) == network_->topInstance();
      if (is_top || !hasTerminals(net)) {
        NetConnectedPinIterator* pin_iter = network_->connectedPinIterator(net);
        while (pin_iter->hasNext()) {
          Pin* pin = pin_iter->next();
          if (network_->isTopLevelPort(pin)) {
            const char* port_name = network_->portName(pin);
            int id = bundled_io_map_[bterm_map_[string(port_name)]];
            id = node_map[cluster_map_[id]];
            const PortDirection* port_dir = network_->direction(pin);
            if (port_dir == PortDirection::input()) {
              driver_id = id;
            } else {
              auto vec_iter = find(loads_id.begin(), loads_id.end(), id);
              if (vec_iter == loads_id.end())
                loads_id.push_back(id);
            }
          } else if (network_->isLeaf(pin)) {
            Instance* inst = network_->instance(pin);
            LibertyCell* liberty_cell = network_->libertyCell(inst);
            if (liberty_cell->isBuffer() == false) {
              const PortDirection* port_dir = network_->direction(pin);
              int id = inst_map_[inst];
              if (id == src_id)
                id = inst_to_idx[inst];
              else
                id = node_map[cluster_map_[id]];

              if (port_dir == PortDirection::output()) {
                driver_id = id;
              } else {
                auto vec_iter = find(loads_id.begin(), loads_id.end(), id);
                if (vec_iter == loads_id.end())
                  loads_id.push_back(id);
              }
            }
          }
        }
      }
    }

    if (driver_id != -1 && loads_id.size() > 0) {
      row_ptr.push_back(count);
      edge_weight.push_back(1.0);
      col_idx.push_back(driver_id);
      count++;
      for (int i = 0; i < loads_id.size(); i++) {
        col_idx.push_back(loads_id[i]);
        count++;
      }
    }
  }
}

//
//  For a cluster that contains macros, further split groups based on macro
//  size. Identical size macros are grouped together
//
void AutoClusterMgr::MacroPart(Cluster* cluster_old, int& cluster_id)
{
  // cout << "Enter macro part:  " << endl;
  vector<Instance*> macro_vec = cluster_old->getMacros();
  map<int, vector<Instance*>> macro_map;
  for (auto macro : macro_vec) {
    Cell* cell = network_->cell(macro);
    const char* cell_name = network_->name(cell);
    const dbMaster* master = db_->findMaster(cell_name);
    const int area = master->getWidth() * master->getHeight();
    if (macro_map.find(area) != macro_map.end()) {
      macro_map[area].push_back(macro);
    } else {
      vector<Instance*> temp_vec;
      temp_vec.push_back(macro);
      macro_map[area] = temp_vec;
    }
  }

  const string parent_name = cluster_old->getName();
  int part_id = 0;

  vector<int> cluster_id_list;
  for (auto& [area, macros] : macro_map) {
    vector<Instance*> temp_vec = macros;
    string name = parent_name + "_part_" + to_string(part_id++);
    Cluster* cluster = new Cluster(++cluster_id, true, name);
    cluster_id_list.push_back(cluster->getId());
    cluster->addLogicalModule(parent_name);
    cluster_list_.push_back(cluster);
    cluster_map_[cluster_id] = cluster;
    virtual_map_[cluster->getId()] = virtual_map_[cluster_old->getId()];
    for (int i = 0; i < temp_vec.size(); i++) {
      inst_map_[temp_vec[i]] = cluster_id;
      cluster->addMacro(temp_vec[i]);
    }
  }

  for (int i = 0; i < cluster_id_list.size(); i++)
    for (int j = i + 1; j < cluster_id_list.size(); j++) {
      virtual_map_[cluster_id_list[i]] = cluster_id_list[j];
    }

  cluster_map_.erase(cluster_old->getId());
  virtual_map_.erase(cluster_old->getId());
  auto vec_it = find(cluster_list_.begin(), cluster_list_.end(), cluster_old);
  cluster_list_.erase(vec_it);
  delete cluster_old;
}

void AutoClusterMgr::printMacroCluster(Cluster* cluster_old, int& cluster_id)
{
  queue<Cluster*> temp_cluster_queue;
  vector<Instance*> macro_vec = cluster_old->getMacros();
  string module_name = cluster_old->getName();
  for (int i = 0; i < module_name.size(); i++) {
    if (module_name[i] == '/')
      module_name[i] = '*';
  }

  const string block_file_name
      = string("./rtl_mp/") + module_name + string(".txt.block");
  const string net_file_name
      = string("./rtl_mp/") + module_name + string(".txt.net");

  ofstream output_file;
  output_file.open(block_file_name.c_str());
  for (int i = 0; i < macro_vec.size(); i++) {
    const pair<float, float> pin_pos = printPinPos(macro_vec[i]);
    const Cell* cell = network_->cell(macro_vec[i]);
    const char* cell_name = network_->name(cell);
    const dbMaster* master = db_->findMaster(cell_name);
    const float width = master->getWidth() / dbu_;
    const float height = master->getHeight() / dbu_;
    output_file << network_->pathName(macro_vec[i]) << "  ";
    output_file << width << "   " << height << "    ";
    output_file << pin_pos.first << "   " << pin_pos.second << "  ";
    output_file << endl;
    Cluster* cluster
        = new Cluster(++cluster_id, true, network_->pathName(macro_vec[i]));
    cluster_map_[cluster_id] = cluster;
    inst_map_[macro_vec[i]] = cluster_id;
    cluster->addMacro(macro_vec[i]);
    temp_cluster_queue.push(cluster);
    cluster_list_.push_back(cluster);
  }

  output_file.close();
  updateConnection();

  output_file.open(net_file_name.c_str());
  int net_id = 0;
  for (auto [src_id, cluster] : cluster_map_) {
    unordered_map<int, unsigned int> connection_map
        = cluster->getOutputConnections();
    unordered_map<int, unsigned int>::iterator iter = connection_map.begin();
    bool flag = true;
    while (iter != connection_map.end()) {
      if (iter->first != src_id) {
        if (flag == true) {
          output_file << endl;
          output_file << "Net_" << ++net_id << ":  " << endl;
          output_file << "source: " << cluster->getName() << "   ";
          flag = false;
        }
        output_file << cluster->getName() << "   " << iter->second << "   ";
      }
      iter++;
    }
  }
  output_file << endl;
  output_file.close();

  while (!temp_cluster_queue.empty()) {
    Cluster* cluster = temp_cluster_queue.front();
    temp_cluster_queue.pop();
    cluster_map_.erase(cluster->getId());
    vector<Cluster*>::iterator vec_it
        = find(cluster_list_.begin(), cluster_list_.end(), cluster);
    cluster_list_.erase(vec_it);
    delete cluster;
  }

  for (auto macro : macro_vec) {
    inst_map_[macro] = cluster_old->getId();
  }
}

pair<float, float> AutoClusterMgr::printPinPos(Instance* macro_inst)
{
  const float dbu = db_->getTech()->getDbUnitsPerMicron();
  int lx = 100000000;
  int ly = 100000000;
  int ux = 0;
  int uy = 0;
  const Cell* cell = network_->cell(macro_inst);
  const char* cell_name = network_->name(cell);
  dbMaster* master = db_->findMaster(cell_name);
  for (dbMTerm* mterm : master->getMTerms()) {
    if (mterm->getSigType() == 0) {
      for (dbMPin* mpin : mterm->getMPins()) {
        for (dbBox* box : mpin->getGeometry()) {
          lx = min(lx, box->xMin());
          ly = min(ly, box->yMin());
          ux = max(ux, box->xMax());
          uy = max(uy, box->yMax());
        }
      }
    }
  }
  const float x_center = (lx + ux) / (2 * dbu);
  const float y_center = (ly + uy) / (2 * dbu);
  return std::pair<float, float>(x_center, y_center);
}

void AutoClusterMgr::mergeMacro(string parent_name, int std_cell_id)
{
  if (merge_cluster_list_.size() == 0)
    return;

  if (merge_cluster_list_.size() == 1) {
    virtual_map_[merge_cluster_list_[0]->getId()] = std_cell_id;
    cluster_list_.push_back(merge_cluster_list_[0]);
    merge_cluster_list_.clear();
    return;
  }

  int merge_index = 0;
  mergeMacroUtil(parent_name, merge_index, std_cell_id);
}

void AutoClusterMgr::mergeMacroUtil(string parent_name,
                                    int& merge_index,
                                    int std_cell_id)
{
  vector<int> outside_vec;
  vector<int> merge_vec;

  for (auto cluster : cluster_list_)
    outside_vec.push_back(cluster->getId());

  for (auto cluster : merge_cluster_list_)
    merge_vec.push_back(cluster->getId());

  const int M = merge_vec.size();
  const int N = outside_vec.size();
  vector<int> class_id(M);
  vector<vector<bool>> graph(M);

  for (int i = 0; i < M; i++) {
    graph[i].resize(N);
    class_id[i] = i;
  }

  for (int i = 0; i < M; i++) {
    for (int j = 0; j < N; j++) {
      const unsigned int input
          = merge_cluster_list_[i]->getInputConnection(outside_vec[j]);
      const unsigned int output
          = merge_cluster_list_[i]->getOutputConnection(outside_vec[j]);
      if (input + output > net_threshold_) {
        graph[i][j] = true;
      } else {
        graph[i][j] = false;
      }
    }
  }

  for (int i = 0; i < M; i++) {
    if (class_id[i] == i) {
      for (int j = i + 1; j < M; j++) {
        bool flag = true;
        for (int k = 0; k < N; k++) {
          if (flag == false)
            break;
          flag = flag && (graph[i][k] == graph[j][k]);
        }
        if (flag == true)
          class_id[j] = i;
      }
    }
  }

  // Merge clusters with same connection topology
  for (int i = 0; i < M; i++) {
    if (class_id[i] == i) {
      for (int j = i + 1; j < M; j++) {
        if (class_id[j] == i) {
          mergeCluster(merge_cluster_list_[i], merge_cluster_list_[j]);
        }
      }
    }
  }

  vector<int> cluster_id_list;
  for (int i = 0; i < M; i++) {
    if (class_id[i] == i) {
      cluster_list_.push_back(merge_cluster_list_[i]);
      virtual_map_[merge_cluster_list_[i]->getId()] = std_cell_id;
      merge_cluster_list_[i]->setName(parent_name + string("_cluster_")
                                      + to_string(merge_index++));
      cluster_id_list.push_back(merge_cluster_list_[i]->getId());
    } else {
      delete merge_cluster_list_[i];
    }
  }

  // for(int i = 0; i < cluster_id_list.size(); i++)
  //     for(int j = i + 1; j < cluster_id_list.size(); j++)
  //         virtual_map_[cluster_id_list[i]] = cluster_id_list[j];

  merge_cluster_list_.clear();
}

//
//  Auto clustering by traversing the design hierarchy
//
//  Parameters:
//     max_num_macro, min_num_macro:   max and min number of marcos in a macro
//     cluster. max_num_inst min_num_inst:  max and min number of std cell
//     instances in a soft cluster. If a logical module has greater than the max
//     threshold of instances, we descend down the hierarchy to examine the
//     children. If multiple clusters are created for child modules that are
//     smaller than the min threshold value, we merge them based on connectivity
//     signatures
//
void AutoClusterMgr::partitionDesign(unsigned int max_num_macro,
                                     unsigned int min_num_macro,
                                     unsigned int max_num_inst,
                                     unsigned int min_num_inst,
                                     unsigned int net_threshold,
                                     unsigned int virtual_weight,
                                     unsigned int ignore_net_threshold,
                                     const char* report_directory,
                                     const char* file_name)
{
  logger_->report("Running Partition Design...");

  block_ = db_->getChip()->getBlock();
  dbu_ = db_->getTech()->getDbUnitsPerMicron();
  max_num_macro_ = max_num_macro;
  min_num_macro_ = min_num_macro;
  max_num_inst_ = max_num_inst;
  min_num_inst_ = min_num_inst;
  net_threshold_ = net_threshold;
  virtual_weight_ = virtual_weight;

  createBundledIO();
  int cluster_id = 0;

  //
  // Map each bundled IO to cluster with zero area
  // Create a cluster for each bundled io
  //
  for (auto [io, name] : {pair(LeftMiddle, "LM"),
                          pair(RightMiddle, "RM"),
                          pair(TopMiddle, "TM"),
                          pair(BottomMiddle, "BM"),

                          pair(LeftLower, "LL"),
                          pair(RightLower, "RL"),
                          pair(TopLower, "TL"),
                          pair(BottomLower, "BL"),

                          pair(LeftUpper, "LU"),
                          pair(RightUpper, "RU"),
                          pair(TopUpper, "TU"),
                          pair(BottomUpper, "BU")}) {
    Cluster* cluster = new Cluster(++cluster_id, true, name);
    bundled_io_map_[io] = cluster_id;
    cluster_map_[cluster_id] = cluster;
    cluster_list_.push_back(cluster);
  }

  Metric metric = computeMetrics(network_->topInstance());
  logger_->info(
      PAR,
      402,
      "Traversed Logical Hierarchy \n\t Number of std cell instances: {}\n\t "
      "Total Area: {}\n\t Number of Hard Macros: {}\n\t ",
      metric.num_inst,
      metric.area,
      metric.num_macro);

  // get all the nets with buffers
  getBufferNet();

  // Break down the top-level instance
  createCluster(cluster_id);
  updateConnection();
  merge("top");

  //
  // Break down clusters
  // Walk down the tree and create clusters for logical modules
  // Stop when the clusters are smaller than the max size threshold
  //
  while (!break_cluster_list_.empty()) {
    Cluster* cluster = break_cluster_list_.front();
    break_cluster_list_.pop();
    breakCluster(cluster, cluster_id);
  }

  //
  // Use MLPart to partition large clusters
  // For clusters that are larger than max threshold size (flat insts) break
  // down the cluster by netlist partitioning using MLPart
  //
  for (int i = 0; i < cluster_list_.size(); i++) {
    if (cluster_list_[i]->getNumInst() > max_num_inst_) {
      mlpart_cluster_list_.push(cluster_list_[i]);
    }
  }

  while (!mlpart_cluster_list_.empty()) {
    Cluster* cluster = mlpart_cluster_list_.front();
    mlpart_cluster_list_.pop();
    MLPart(cluster, cluster_id);
  }

  //
  // split the macros and std cells
  // For clusters that contains HM and std cell -- split the cluster into two
  // a HM part and a std cell part
  //
  vector<Cluster*> par_cluster_vec;
  for (auto cluster : cluster_list_) {
    if (cluster->getNumMacro() > 0) {
      par_cluster_vec.push_back(cluster);
    }
  }

  for (int i = 0; i < par_cluster_vec.size(); i++) {
    Cluster* cluster_old = par_cluster_vec[i];
    int id = (-1) * cluster_old->getId();
    virtual_map_[id] = cluster_old->getId();
    string name = cluster_old->getName() + string("_macro");
    Cluster* cluster = new Cluster(id, true, name);
    cluster->addLogicalModule(name);
    cluster_map_[id] = cluster;
    vector<Instance*> macro_vec = cluster_old->getMacros();
    for (int j = 0; j < macro_vec.size(); j++) {
      inst_map_[macro_vec[j]] = id;
      cluster->addMacro(macro_vec[j]);
    }
    cluster_list_.push_back(cluster);
    name = cluster_old->getName() + string("_std_cell");
    cluster_old->setName(name);
    cluster_old->removeMacro();
  }
  par_cluster_vec.clear();
  updateConnection();

  //
  // group macros based on connection signature
  // Use connection signatures to group and split macros
  //
  queue<Cluster*> par_cluster_queue;
  for (int i = 0; i < cluster_list_.size(); i++)
    if (cluster_list_[i]->getNumMacro() > 0)
      par_cluster_queue.push(cluster_list_[i]);

  while (!par_cluster_queue.empty()) {
    Cluster* cluster_old = par_cluster_queue.front();
    par_cluster_queue.pop();
    vector<Instance*> macro_vec = cluster_old->getMacros();
    string name = cluster_old->getName();
    for (int i = 0; i < macro_vec.size(); i++) {
      Cluster* cluster
          = new Cluster(++cluster_id, true, network_->pathName(macro_vec[i]));
      cluster->addLogicalModule(network_->pathName(macro_vec[i]));
      cluster_map_[cluster_id] = cluster;
      inst_map_[macro_vec[i]] = cluster_id;
      cluster->addMacro(macro_vec[i]);
      merge_cluster_list_.push_back(cluster);
    }
    int std_cell_id = virtual_map_[cluster_old->getId()];
    virtual_map_.erase(cluster_old->getId());
    cluster_map_.erase(cluster_old->getId());
    vector<Cluster*>::iterator vec_it
        = find(cluster_list_.begin(), cluster_list_.end(), cluster_old);
    cluster_list_.erase(vec_it);
    delete cluster_old;
    updateConnection();
    mergeMacro(name, std_cell_id);
  }

  //
  // group macros based on area footprint, This will allow for more efficient
  // tiling with limited wasted space between the macros
  //
  for (int i = 0; i < cluster_list_.size(); i++)
    if (cluster_list_[i]->getNumMacro() > min_num_macro_)
      par_cluster_queue.push(cluster_list_[i]);

  while (!par_cluster_queue.empty()) {
    Cluster* cluster = par_cluster_queue.front();
    par_cluster_queue.pop();
    MacroPart(cluster, cluster_id);
  }

  updateConnection();

  //
  // add virtual weights between std cell and hard macro portion of the cluster
  //
  unordered_map<int, int>::iterator weight_it = virtual_map_.begin();
  while (weight_it != virtual_map_.end()) {
    int id = weight_it->second;
    int target_id = weight_it->first;
    int num_macro_id = cluster_map_[id]->getNumMacro();
    int num_macro_target_id = cluster_map_[target_id]->getNumMacro();
    if (num_macro_id > 0 && num_macro_target_id > 0) {
      cluster_map_[id]->addOutputConnection(target_id, virtual_weight_);
    }

    weight_it++;
  }

  Rect die_box;
  block_->getCoreArea(die_box);
  floorplan_lx_ = die_box.xMin();
  floorplan_ly_ = die_box.yMin();
  floorplan_ux_ = die_box.xMax();
  floorplan_uy_ = die_box.yMax();

  //
  // generate block file
  // Generates the output files needed by the macro placer
  //
  unordered_map<int, Cluster*>::iterator map_iter = cluster_map_.begin();

  string block_file
      = string(report_directory) + '/' + file_name + ".block";
  ofstream output_file;
  output_file.open(block_file);
  output_file << "[INFO] Num clusters: " << cluster_list_.size() << endl;
  output_file << "[INFO] Floorplan width: "
              << (floorplan_ux_ - floorplan_lx_) / dbu_ << endl;
  output_file << "[INFO] Floorplan height:  "
              << (floorplan_uy_ - floorplan_ly_) / dbu_ << endl;
  output_file << "[INFO] Floorplan_lx: " << floorplan_lx_ / dbu_ << endl;
  output_file << "[INFO] Floorplan_ly: " << floorplan_ly_ / dbu_ << endl;
  output_file << "[INFO] Num std cells: "
              << logical_cluster_map_[network_->topInstance()].num_inst << endl;
  output_file << "[INFO] Num macros: "
              << logical_cluster_map_[network_->topInstance()].num_macro
              << endl;
  output_file << "[INFO] Total area: "
              << logical_cluster_map_[network_->topInstance()].area << endl;
  output_file << "[INFO] Num buffers:  " << num_buffer_ << endl;
  output_file << "[INFO] Buffer area:  " << area_buffer_ << endl;
  output_file << endl;
  logger_->info(
      PAR, 403, "Number of Clusters created: {}", cluster_list_.size());
  map_iter = cluster_map_.begin();
  float dbu = db_->getTech()->getDbUnitsPerMicron();
  while (map_iter != cluster_map_.end()) {
    int id = map_iter->first;
    float area = map_iter->second->calculateArea(network_);
    if (area != 0.0) {
      output_file << "cluster: " << map_iter->second->getName() << endl;
      output_file << "area:  " << area << endl;
      if (map_iter->second->getNumMacro() > 0) {
        vector<Instance*> macro_vec = map_iter->second->getMacros();
        for (int i = 0; i < macro_vec.size(); i++) {
          Cell* cell = network_->cell(macro_vec[i]);
          const char* cell_name = network_->name(cell);
          dbMaster* master = db_->findMaster(cell_name);
          float width = master->getWidth() / dbu;
          float height = master->getHeight() / dbu;
          output_file << network_->pathName(macro_vec[i]) << "  ";
          output_file << width << "   " << height << endl;
        }
      }
      output_file << endl;
    }
    map_iter++;
  }

  output_file.close();

  // generate net file
  string net_file = string(report_directory) + '/' + file_name + ".net";
  output_file.open(net_file);
  int net_id = 0;
  map_iter = cluster_map_.begin();
  while (map_iter != cluster_map_.end()) {
    int src_id = map_iter->first;
    unordered_map<int, unsigned int> connection_map
        = map_iter->second->getOutputConnections();
    unordered_map<int, unsigned int>::iterator iter = connection_map.begin();

    if (!(connection_map.size() == 0
          || (connection_map.size() == 1 && iter->first == src_id))) {
      //bool flag = (src_id >= 1 && src_id <= 12)
      //            || (cluster_map_[src_id]->getNumMacro() > 0);
      output_file << "Net_" << ++net_id << ":  " << endl;
      output_file << "source: " << map_iter->second->getName() << "   ";
      while (iter != connection_map.end()) {
        if (iter->first != src_id) {
          int weight = iter->second;
          //if (flag || (iter->first >= 1 && iter->first <= 12)
          //    || cluster_map_[iter->first]->getNumMacro() > 0) {
          //  weight += virtual_weight_;
          //}
            
          if(weight < ignore_net_threshold) {
              weight = 0;
          }

          output_file << cluster_map_[iter->first]->getName() << "   " << weight
                      << "   ";
        }

        iter++;
      }

      output_file << endl;
    }

    // output_file << endl;
    map_iter++;
  }
  output_file << endl;
  output_file.close();

  // print connections for each hard macro cluster
  for (int i = 0; i < cluster_list_.size(); i++)
    if (cluster_list_[i]->getNumMacro() > 0)
      par_cluster_queue.push(cluster_list_[i]);

  while (!par_cluster_queue.empty()) {
    Cluster* cluster_old = par_cluster_queue.front();
    par_cluster_queue.pop();
    printMacroCluster(cluster_old, cluster_id);
  }

  // delete all the clusters
  for (int i = 0; i < cluster_list_.size(); i++) {
    delete cluster_list_[i];
  }
}

}  // namespace par
