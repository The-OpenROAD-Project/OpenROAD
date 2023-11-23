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

#include "object.h"

#include <iostream>

#include "odb/db.h"
#include "utl/Logger.h"

namespace mpl2 {
using utl::MPL;

///////////////////////////////////////////////////////////////////////
// Basic utility functions

// conversion between dbu and microns
float dbuToMicron(int metric, float dbu)
{
  return metric / dbu;
}

int micronToDbu(float metric, float dbu)
{
  return std::round(metric * dbu);
}

std::string toString(const PinAccess& pin_access)
{
  switch (pin_access) {
    case L:
      return std::string("L");
    case T:
      return std::string("T");
    case R:
      return std::string("R");
    case B:
      return std::string("B");
    default:
      return std::string("NONE");
  }
}

PinAccess opposite(const PinAccess& pin_access)
{
  switch (pin_access) {
    case L:
      return R;
    case T:
      return B;
    case R:
      return L;
    case B:
      return T;
    default:
      return NONE;
  }
}

// Compare two intervals according to starting points
static bool comparePairFirst(const std::pair<float, float>& p1,
                             const std::pair<float, float>& p2)
{
  return p1.first < p2.first;
}

///////////////////////////////////////////////////////////////////////
// Metrics Class
Metrics::Metrics(unsigned int num_std_cell,
                 unsigned int num_macro,
                 float std_cell_area,
                 float macro_area)
{
  num_std_cell_ = num_std_cell;
  num_macro_ = num_macro;
  std_cell_area_ = std_cell_area;
  macro_area_ = macro_area;
}

void Metrics::addMetrics(const Metrics& metrics)
{
  num_std_cell_ += metrics.num_std_cell_;
  num_macro_ += metrics.num_macro_;
  std_cell_area_ += metrics.std_cell_area_;
  macro_area_ += metrics.macro_area_;
}

const std::pair<unsigned int, unsigned int> Metrics::getCountStats() const
{
  return std::pair<unsigned int, unsigned int>(num_std_cell_, num_macro_);
}

const std::pair<float, float> Metrics::getAreaStats() const
{
  return std::pair<float, float>(std_cell_area_, macro_area_);
}

unsigned int Metrics::getNumMacro() const
{
  return num_macro_;
}

unsigned int Metrics::getNumStdCell() const
{
  return num_std_cell_;
}

float Metrics::getStdCellArea() const
{
  return std_cell_area_;
}

float Metrics::getMacroArea() const
{
  return macro_area_;
}

float Metrics::getArea() const
{
  return std_cell_area_ + macro_area_;
}

///////////////////////////////////////////////////////////////////////
// Cluster Class
// Constructors and Destructors
Cluster::Cluster(int cluster_id, utl::Logger* logger)
{
  id_ = cluster_id;
  logger_ = logger;
}

Cluster::Cluster(int cluster_id,
                 const std::string& cluster_name,
                 utl::Logger* logger)
{
  id_ = cluster_id;
  name_ = cluster_name;
  logger_ = logger;
}

// cluster id
int Cluster::getId() const
{
  return id_;
}

const std::string Cluster::getName() const
{
  return name_;
}

void Cluster::setName(const std::string& name)
{
  name_ = name;
}

// cluster type
void Cluster::setClusterType(const ClusterType& cluster_type)
{
  type_ = cluster_type;
}

const ClusterType Cluster::getClusterType() const
{
  return type_;
}

// Instances (Here we store dbModule to reduce memory)
void Cluster::addDbModule(odb::dbModule* db_module)
{
  db_modules_.push_back(db_module);
}

void Cluster::addLeafStdCell(odb::dbInst* leaf_std_cell)
{
  leaf_std_cells_.push_back(leaf_std_cell);
}

void Cluster::addLeafMacro(odb::dbInst* leaf_macro)
{
  leaf_macros_.push_back(leaf_macro);
}

void Cluster::specifyHardMacros(std::vector<HardMacro*>& hard_macros)
{
  hard_macros_ = hard_macros;
}

const std::vector<odb::dbModule*> Cluster::getDbModules() const
{
  return db_modules_;
}

const std::vector<odb::dbInst*> Cluster::getLeafStdCells() const
{
  return leaf_std_cells_;
}

const std::vector<odb::dbInst*> Cluster::getLeafMacros() const
{
  return leaf_macros_;
}

std::vector<HardMacro*> Cluster::getHardMacros() const
{
  return hard_macros_;
}

void Cluster::clearDbModules()
{
  db_modules_.clear();
}

void Cluster::clearLeafStdCells()
{
  leaf_std_cells_.clear();
}

void Cluster::clearLeafMacros()
{
  leaf_macros_.clear();
}

void Cluster::clearHardMacros()
{
  hard_macros_.clear();
}

std::string Cluster::getClusterTypeString() const
{
  std::string cluster_type;

  if (is_io_cluster_) {
    return "BundledIO";
  }

  switch (type_) {
    case StdCellCluster:
      cluster_type = "StdCell";
      break;
    case MixedCluster:
      cluster_type = "Mixed";
      break;
    case HardMacroCluster:
      cluster_type = "Macro";
      break;
  }

  return cluster_type;
}

std::string Cluster::getIsLeafString() const
{
  std::string is_leaf_string;

  if (!is_io_cluster_ && children_.empty()) {
    is_leaf_string = "Leaf";
  }

  return is_leaf_string;
}

// copy instances based on cluster Type
void Cluster::copyInstances(const Cluster& cluster)
{
  // clear firstly
  db_modules_.clear();
  leaf_std_cells_.clear();
  leaf_macros_.clear();
  hard_macros_.clear();
  // insert new elements
  if (type_ == HardMacroCluster) {
    leaf_macros_.insert(leaf_macros_.end(),
                        cluster.leaf_macros_.begin(),
                        cluster.leaf_macros_.end());
  } else if (type_ == StdCellCluster) {
    leaf_std_cells_.insert(leaf_std_cells_.end(),
                           cluster.leaf_std_cells_.begin(),
                           cluster.leaf_std_cells_.end());
    db_modules_.insert(db_modules_.end(),
                       cluster.db_modules_.begin(),
                       cluster.db_modules_.end());
  } else {  // type_ == MixedCluster
    leaf_macros_.insert(leaf_macros_.end(),
                        cluster.leaf_macros_.begin(),
                        cluster.leaf_macros_.end());
    leaf_std_cells_.insert(leaf_std_cells_.end(),
                           cluster.leaf_std_cells_.begin(),
                           cluster.leaf_std_cells_.end());
    db_modules_.insert(db_modules_.end(),
                       cluster.db_modules_.begin(),
                       cluster.db_modules_.end());
  }
}

// Bundled IO (Pads) cluster support
// The position is the center of IO pads in the cluster
void Cluster::setAsIOCluster(const std::pair<float, float>& pos,
                             const float width,
                             const float height)
{
  is_io_cluster_ = true;
  // Create a SoftMacro representing the IO cluster
  soft_macro_ = std::make_unique<SoftMacro>(pos, name_, width, height, this);
}

bool Cluster::isIOCluster() const
{
  return is_io_cluster_;
}

// Metrics Support and Statistics
void Cluster::setMetrics(const Metrics& metrics)
{
  metrics_ = metrics;
}

const Metrics Cluster::getMetrics() const
{
  return metrics_;
}

int Cluster::getNumStdCell() const
{
  if (type_ == HardMacroCluster) {
    return 0;
  }
  return metrics_.getNumStdCell();
}

int Cluster::getNumMacro() const
{
  if (type_ == StdCellCluster) {
    return 0;
  }
  return metrics_.getNumMacro();
}

float Cluster::getArea() const
{
  return getStdCellArea() + getMacroArea();
}

float Cluster::getStdCellArea() const
{
  if (type_ == HardMacroCluster) {
    return 0.0;
  }

  return metrics_.getStdCellArea();
}

float Cluster::getMacroArea() const
{
  if (type_ == StdCellCluster) {
    return 0.0;
  }

  return metrics_.getMacroArea();
}

// Physical location support
float Cluster::getWidth() const
{
  if (!soft_macro_) {
    return 0.0;
  }

  return soft_macro_->getWidth();
}

float Cluster::getHeight() const
{
  if (!soft_macro_) {
    return 0.0;
  }

  return soft_macro_->getHeight();
}

float Cluster::getX() const
{
  if (!soft_macro_) {
    return 0.0;
  }

  return soft_macro_->getX();
}

float Cluster::getY() const
{
  if (!soft_macro_) {
    return 0.0;
  }

  return soft_macro_->getY();
}

void Cluster::setX(float x)
{
  if (soft_macro_) {
    soft_macro_->setX(x);
  }
}

void Cluster::setY(float y)
{
  if (soft_macro_) {
    soft_macro_->setY(y);
  }
}

const std::pair<float, float> Cluster::getLocation() const
{
  if (!soft_macro_) {
    return {0, 0};
  }

  return soft_macro_->getLocation();
}

// Hierarchy Support
void Cluster::setParent(Cluster* parent)
{
  parent_ = parent;
}

void Cluster::addChild(Cluster* child)
{
  children_.push_back(child);
}

void Cluster::removeChild(const Cluster* child)
{
  children_.erase(std::find(children_.begin(), children_.end(), child));
}

void Cluster::addChildren(const std::vector<Cluster*>& children)
{
  std::copy(children.begin(), children.end(), std::back_inserter(children_));
}

void Cluster::removeChildren()
{
  children_.clear();
}

Cluster* Cluster::getParent() const
{
  return parent_;
}

std::vector<Cluster*> Cluster::getChildren() const
{
  return children_;
}

bool Cluster::isLeaf() const
{
  return children_.empty();
}

// We only merge clusters with the same parent cluster
// We only merge clusters with the same parent cluster
bool Cluster::mergeCluster(Cluster& cluster, bool& delete_flag)
{
  if (parent_ != cluster.parent_) {
    return false;
  }

  parent_->removeChild(&cluster);
  metrics_.addMetrics(cluster.metrics_);
  // modify name
  name_ += "||" + cluster.name_;
  // if current cluster is a leaf cluster
  leaf_macros_.insert(leaf_macros_.end(),
                      cluster.leaf_macros_.begin(),
                      cluster.leaf_macros_.end());
  leaf_std_cells_.insert(leaf_std_cells_.end(),
                         cluster.leaf_std_cells_.begin(),
                         cluster.leaf_std_cells_.end());
  db_modules_.insert(db_modules_.end(),
                     cluster.db_modules_.begin(),
                     cluster.db_modules_.end());
  delete_flag = true;
  // if current cluster is not a leaf cluster
  if (!children_.empty()) {
    children_.push_back(&cluster);
    cluster.setParent(this);
    delete_flag = false;
  }
  return true;
}

// Connection signature support
void Cluster::initConnection()
{
  connection_map_.clear();
}

void Cluster::addConnection(int cluster_id, float weight)
{
  if (connection_map_.find(cluster_id) == connection_map_.end()) {
    connection_map_[cluster_id] = weight;
  } else {
    connection_map_[cluster_id] += weight;
  }
}

const std::map<int, float> Cluster::getConnection() const
{
  return connection_map_;
}

// The connection signature is based on connection topology
// if the number of connnections between two clusters is larger than the
// net_threshold we think the two clusters are connected, otherwise they are
// disconnected.
bool Cluster::isSameConnSignature(const Cluster& cluster, float net_threshold)
{
  std::vector<int> neighbors;          // neighbors of current cluster
  std::vector<int> cluster_neighbors;  // neighbors of the input cluster
  for (auto& [cluster_id, weight] : connection_map_) {
    if ((cluster_id != id_) && (cluster_id != cluster.id_)
        && (weight >= net_threshold)) {
      neighbors.push_back(cluster_id);
    }
  }

  for (auto& [cluster_id, weight] : cluster.connection_map_) {
    if ((cluster_id != id_) && (cluster_id != cluster.id_)
        && (weight >= net_threshold)) {
      cluster_neighbors.push_back(cluster_id);
    }
  }

  if (neighbors.size() != cluster_neighbors.size()) {
    return false;
  }
  std::sort(neighbors.begin(), neighbors.end());
  std::sort(cluster_neighbors.begin(), cluster_neighbors.end());
  for (int i = 0; i < neighbors.size(); i++) {
    if (neighbors[i] != cluster_neighbors[i]) {
      return false;
    }
  }

  return true;
}

//
// Get closely-connected cluster if such cluster exists
// For example, if a small cluster A is closely connected to a
// well-formed cluster B, (there are also other well-formed clusters
// C, D), A is only connected to B and A has no connection with C, D
//
// candidate_clusters are small clusters that need to be merged,
// any cluster not in candidate_clusters is a well-formed cluster
//
int Cluster::getCloseCluster(const std::vector<int>& candidate_clusters,
                             float net_threshold)
{
  int closely_cluster = -1;
  int num_closely_clusters = 0;
  for (auto& [cluster_id, num_nets] : connection_map_) {
    debugPrint(logger_,
               MPL,
               "multilevel_autoclustering",
               2,
               "cluster_id: {}, nets: {}",
               cluster_id,
               num_nets);
    if (num_nets > net_threshold
        && std::find(
               candidate_clusters.begin(), candidate_clusters.end(), cluster_id)
               == candidate_clusters.end()) {
      num_closely_clusters++;
      closely_cluster = cluster_id;
    }
  }

  if (num_closely_clusters == 1) {
    return closely_cluster;
  }
  return -1;
}

// Pin Access Support
void Cluster::setPinAccess(int cluster_id,
                           PinAccess pin_access,
                           float net_weight)
{
  if (cluster_id < 0) {
    logger_->error(MPL,
                   38,
                   "Cannot set pin access for {} boundary.",
                   toString(pin_access));
  }
  pin_access_map_[cluster_id]
      = std::pair<PinAccess, float>(pin_access, net_weight);
}

const std::pair<PinAccess, float> Cluster::getPinAccess(int cluster_id)
{
  return pin_access_map_[cluster_id];
}

const std::map<int, std::pair<PinAccess, float>> Cluster::getPinAccessMap()
    const
{
  return pin_access_map_;
}

const std::map<PinAccess, std::map<PinAccess, float>>
Cluster::getBoundaryConnection() const
{
  return boundary_connection_map_;
}

void Cluster::addBoundaryConnection(PinAccess pin_a,
                                    PinAccess pin_b,
                                    float num_net)
{
  if (boundary_connection_map_.find(pin_a) == boundary_connection_map_.end()) {
    std::map<PinAccess, float> pin_map;
    pin_map[pin_b] = num_net;
    boundary_connection_map_[pin_a] = pin_map;
  } else {
    if (boundary_connection_map_[pin_a].find(pin_b)
        == boundary_connection_map_[pin_a].end()) {
      boundary_connection_map_[pin_a][pin_b] = num_net;
    } else {
      boundary_connection_map_[pin_a][pin_b] += num_net;
    }
  }
}

// Print Basic Information
// Normally we call this after macro placement is done
void Cluster::printBasicInformation(utl::Logger* logger) const
{
  std::string line = "\n";
  line += std::string(80, '*') + "\n";
  line += "[INFO] cluster_name :  " + name_ + "  ";
  line += "cluster_id : " + std::to_string(id_) + "  \n";
  line += "num_std_cell : " + std::to_string(getNumStdCell()) + "  ";
  line += "num_macro : " + std::to_string(getNumMacro()) + "\n";
  line += "width : " + std::to_string(getWidth()) + "  ";
  line += "height : " + std::to_string(getHeight()) + "  ";
  line += "location :  ( " + std::to_string((getLocation()).first) + " , ";
  line += std::to_string((getLocation()).second) + " )\n";
  for (const auto& hard_macro : hard_macros_) {
    line += "\t macro_name : " + hard_macro->getName();
    line += "\t width : " + std::to_string(hard_macro->getRealWidth());
    line += "\t height : " + std::to_string(hard_macro->getRealHeight());
    line += "\t lx : " + std::to_string(hard_macro->getRealX());
    line += "\t ly : " + std::to_string(hard_macro->getRealY());
    line += "\n";
  }

  logger->report(line);
}

// Macro Placement Support
void Cluster::setSoftMacro(SoftMacro* soft_macro)
{
  soft_macro_.reset(soft_macro);
}

SoftMacro* Cluster::getSoftMacro() const
{
  return soft_macro_.get();
}

void Cluster::setMacroTilings(
    const std::vector<std::pair<float, float>>& tilings)
{
  macro_tilings_ = tilings;
}

const std::vector<std::pair<float, float>> Cluster::getMacroTilings() const
{
  return macro_tilings_;
}

// Virtual Connections
const std::vector<std::pair<int, int>> Cluster::getVirtualConnections() const
{
  return virtual_connections_;
}

void Cluster::addVirtualConnection(int src, int target)
{
  virtual_connections_.emplace_back(src, target);
}

///////////////////////////////////////////////////////////////////////
// HardMacro
HardMacro::HardMacro(std::pair<float, float> loc, const std::string& name)
{
  width_ = 0.0;
  height_ = 0.0;
  name_ = name;
  pin_x_ = 0.0;
  pin_y_ = 0.0;
  x_ = loc.first;
  y_ = loc.second;
}

HardMacro::HardMacro(float width, float height, const std::string& name)
{
  width_ = width;
  height_ = height;
  name_ = name;
  pin_x_ = width / 2.0;
  pin_y_ = height / 2.0;
}

HardMacro::HardMacro(odb::dbInst* inst,
                     float dbu,
                     int manufacturing_grid,
                     float halo_width,
                     float halo_height)
{
  inst_ = inst;
  dbu_ = dbu;
  halo_width_ = halo_width;
  halo_height_ = halo_height;
  manufacturing_grid_ = manufacturing_grid;

  // set name
  name_ = inst->getName();
  odb::dbMaster* master = inst->getMaster();
  // set the width and height
  width_ = dbuToMicron(master->getWidth(), dbu) + 2 * halo_width;
  height_ = dbuToMicron(master->getHeight(), dbu) + 2 * halo_height;
  // Set the position of virtual pins
  // Here we only consider signal pins
  odb::Rect bbox;
  bbox.mergeInit();
  for (odb::dbMTerm* mterm : master->getMTerms()) {
    if (mterm->getSigType() == odb::dbSigType::SIGNAL) {
      for (odb::dbMPin* mpin : mterm->getMPins()) {
        for (odb::dbBox* box : mpin->getGeometry()) {
          odb::Rect rect = box->getBox();
          bbox.merge(rect);
        }
      }
    }
  }
  pin_x_ = dbuToMicron((bbox.xMin() + bbox.xMax()) / 2.0, dbu) + halo_width_;
  pin_y_ = dbuToMicron((bbox.yMin() + bbox.yMax()) / 2.0, dbu) + halo_height_;
}

// overload the comparison operators
// based on area, width, height order
// When we compare, we also consider the effect of halo_width
bool HardMacro::operator<(const HardMacro& macro) const
{
  if (width_ * height_ != macro.width_ * macro.height_) {
    return width_ * height_ < macro.width_ * macro.height_;
  }
  if (width_ != macro.width_) {
    return width_ < macro.width_;
  }
  return height_ < macro.height_;
}

bool HardMacro::operator==(const HardMacro& macro) const
{
  return (width_ == macro.width_) && (height_ == macro.height_);
}

// Get Physical Information
// Note that the default X and Y include halo_width
void HardMacro::setLocation(const std::pair<float, float>& location)
{
  if (width_ * height_ <= 0.0) {
    return;
  }
  x_ = location.first;
  y_ = location.second;
}

void HardMacro::setX(float x)
{
  if (width_ * height_ <= 0.0) {
    return;
  }
  x_ = x;
}

void HardMacro::setY(float y)
{
  if (width_ * height_ <= 0.0) {
    return;
  }
  y_ = y;
}

const std::pair<float, float> HardMacro::getLocation() const
{
  return std::pair<float, float>(x_, y_);
}

float HardMacro::getX() const
{
  return x_;
}

float HardMacro::getY() const
{
  return y_;
}

// The position of pins relative to the lower left of the instance
float HardMacro::getPinX() const
{
  return x_ + pin_x_;
}

float HardMacro::getPinY() const
{
  return y_ + pin_y_;
}

// The position of pins relative to the origin of the canvas;
float HardMacro::getAbsPinX() const
{
  return pin_x_;
}

float HardMacro::getAbsPinY() const
{
  return pin_y_;
}

// width and height
float HardMacro::getWidth() const
{
  return width_;
}

float HardMacro::getHeight() const
{
  return height_;
}

// Note that the real X and Y does NOT include halo_width
void HardMacro::setRealLocation(const std::pair<float, float>& location)
{
  if (width_ * height_ <= 0.0) {
    return;
  }

  x_ = location.first - halo_width_;
  y_ = location.second - halo_height_;
}

void HardMacro::setRealX(float x)
{
  if (width_ * height_ <= 0.0) {
    return;
  }

  x_ = x - halo_width_;
}

void HardMacro::setRealY(float y)
{
  if (width_ * height_ <= 0.0) {
    return;
  }

  y_ = y - halo_height_;
}

const std::pair<float, float> HardMacro::getRealLocation() const
{
  return std::pair<float, float>(x_ + halo_width_, y_ + halo_height_);
}

float HardMacro::getRealX() const
{
  return x_ + halo_width_;
}

float HardMacro::getRealY() const
{
  return y_ + halo_height_;
}

float HardMacro::getRealWidth() const
{
  return width_ - 2 * halo_width_;
}

float HardMacro::getRealHeight() const
{
  return height_ - 2 * halo_height_;
}

// Orientation support
odb::dbOrientType HardMacro::getOrientation() const
{
  return orientation_;
}

// We do not allow rotation of macros
// This may violate the direction of metal layers
void HardMacro::flip(bool flip_horizontal)
{
  if (flip_horizontal) {
    orientation_ = orientation_.flipX();
    pin_y_ = height_ - pin_y_;
  } else {
    orientation_ = orientation_.flipY();
    pin_x_ = width_ - pin_x_;
  }
}

// Interfaces with OpenDB
odb::dbInst* HardMacro::getInst() const
{
  return inst_;
}

const std::string HardMacro::getName() const
{
  return name_;
}

const std::string HardMacro::getMasterName() const
{
  if (inst_ == nullptr) {
    return name_;
  }
  return inst_->getMaster()->getName();
}

// update the location and orientation of the macro inst in OpenDB
void HardMacro::updateDb(float pitch_x, float pitch_y, odb::dbBlock* block)
{
  if ((inst_ == nullptr) || (dbu_ <= 0.0)) {
    return;
  }

  const float lower_x = getRealX();
  const float lower_y = getRealY();
  const float upper_x = lower_x + getRealWidth();
  const float upper_y = lower_y + getRealHeight();

  const Rect macro_with_halo_box(lower_x, lower_y, upper_x, upper_y);
  const odb::dbOrientType hard_macro_orientation = this->getOrientation();

  const odb::Point snap_origin = computeSnapOrigin(
      macro_with_halo_box, hard_macro_orientation, pitch_x, pitch_y, block);

  inst_->setOrigin(snap_origin.x(), snap_origin.y());
  inst_->setOrient(orientation_);
  inst_->setPlacementStatus(odb::dbPlacementStatus::PLACED);
}

odb::Point HardMacro::computeSnapOrigin(const Rect& macro_box,
                                        const odb::dbOrientType& orientation,
                                        float& pitch_x,
                                        float& pitch_y,
                                        odb::dbBlock* block)
{
  float offset_x = 0.0;
  float offset_y = 0.0;
  float pin_width_x = 0.0;
  float pin_width_y = 0.0;

  // get the offset and pitch of related routing layers
  odb::dbMaster* master = inst_->getMaster();
  for (odb::dbMTerm* mterm : master->getMTerms()) {
    if (mterm->getSigType() == odb::dbSigType::SIGNAL) {
      for (odb::dbMPin* mpin : mterm->getMPins()) {
        for (odb::dbBox* box : mpin->getGeometry()) {
          odb::dbTechLayer* layer = box->getTechLayer();

          if (layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
            computeDirectionSpacingParameters(
                block, layer, box, offset_y, pitch_y, pin_width_y, false);
          } else {
            computeDirectionSpacingParameters(
                block, layer, box, offset_x, pitch_x, pin_width_x, true);
          }
        }
      }
    }
  }

  // Defaults for R0
  float origin_x = macro_box.xMin();
  float origin_y = macro_box.yMin();
  float pin_offset_x = pin_width_x / 2;
  float pin_offset_y = pin_width_y / 2;

  if (orientation == odb::dbOrientType::MX) {
    origin_y = macro_box.yMax();
    pin_offset_y = -pin_offset_y;
  } else if (orientation == odb::dbOrientType::MY) {
    origin_x = macro_box.xMax();
    pin_offset_x = -pin_offset_x;
  } else if (orientation == odb::dbOrientType::R180) {
    origin_x = macro_box.xMax();
    origin_y = macro_box.yMax();
    pin_offset_x = -pin_offset_x;
    pin_offset_y = -pin_offset_y;
  }

  // Compute trackgrid alignment only if there are pins in the grid's direction.
  if (pin_width_x != 0) {
    origin_x = std::round((origin_x - offset_x) / pitch_x) * pitch_x + offset_x
               - pin_offset_x;
  }

  if (pin_width_y != 0) {
    origin_y = std::round((origin_y - offset_y) / pitch_y) * pitch_y + offset_y
               - pin_offset_y;
  }

  const int snap_origin_x
      = std::round(float(micronToDbu(origin_x, dbu_)) / manufacturing_grid_)
        * manufacturing_grid_;
  const int snap_origin_y
      = std::round(float(micronToDbu(origin_y, dbu_)) / manufacturing_grid_)
        * manufacturing_grid_;

  const odb::Point snap_origin(snap_origin_x, snap_origin_y);

  return snap_origin;
}

// Compute for each pin: layer offset, pitch and pin width
void HardMacro::computeDirectionSpacingParameters(
    odb::dbBlock* block,
    odb::dbTechLayer* layer,
    odb::dbBox* box,
    float& offset,
    float& pitch,
    float& pin_width,
    const bool& is_vertical_direction)
{
  odb::dbTrackGrid* track_grid = block->findTrackGrid(layer);

  pin_width
      = dbuToMicron(getDirectionPinWidth(box, is_vertical_direction), dbu_);

  if (track_grid != nullptr) {
    std::vector<int> coordinate_grid;

    getDirectionTrackGrid(track_grid, coordinate_grid, is_vertical_direction);

    offset = dbuToMicron(static_cast<float>(coordinate_grid[0]), dbu_);
    pitch = dbuToMicron(
        static_cast<float>(coordinate_grid[1] - coordinate_grid[0]), dbu_);
  } else {
    pitch = dbuToMicron(getDirectionPitch(layer, is_vertical_direction), dbu_);
    offset
        = dbuToMicron(getDirectionOffset(layer, is_vertical_direction), dbu_);
  }
}

float HardMacro::getDirectionPitch(odb::dbTechLayer* layer,
                                   const bool& is_vertical_direction)
{
  float pitch = 0.0;

  if (is_vertical_direction) {
    pitch = layer->getPitchX();
  } else {
    pitch = layer->getPitchY();
  }

  return pitch;
}

float HardMacro::getDirectionOffset(odb::dbTechLayer* layer,
                                    const bool& is_vertical_direction)
{
  float offset = 0.0;

  if (is_vertical_direction) {
    offset = layer->getOffsetX();
  } else {
    offset = layer->getOffsetY();
  }

  return offset;
}

float HardMacro::getDirectionPinWidth(odb::dbBox* box,
                                      const bool& is_vertical_direction)
{
  float pin_width = 0.0;

  if (is_vertical_direction) {
    pin_width = box->getDX();
  } else {
    pin_width = box->getDY();
  }

  return pin_width;
}

void HardMacro::getDirectionTrackGrid(odb::dbTrackGrid* track_grid,
                                      std::vector<int>& coordinate_grid,
                                      const bool& is_vertical_direction)
{
  if (is_vertical_direction) {
    track_grid->getGridX(coordinate_grid);
  } else {
    track_grid->getGridY(coordinate_grid);
  }
}

///////////////////////////////////////////////////////////////////////
// SoftMacro Class
// Create a SoftMacro with specified size
// In this case, we think the cluster is a macro cluster with only one macro
// SoftMacro : Hard Macro (or pin access blockage)
SoftMacro::SoftMacro(float width, float height, const std::string& name)
{
  name_ = name;
  width_ = width;
  height_ = height;
  area_ = width * height;
  cluster_ = nullptr;
}

// SoftMacro : Fixed Hard Macro (or blockage)
SoftMacro::SoftMacro(float width,
                     float height,
                     const std::string& name,
                     float lx,
                     float ly)
{
  name_ = name;
  width_ = width;
  height_ = height;
  area_ = width * height;
  x_ = lx;
  y_ = ly;
  cluster_ = nullptr;
  fixed_ = true;
}

// Create a SoftMacro representing the IO cluster or fixed terminals
SoftMacro::SoftMacro(const std::pair<float, float>& pos,
                     const std::string& name,
                     float width,
                     float height,
                     Cluster* cluster)
{
  name_ = name;
  x_ = pos.first;
  y_ = pos.second;
  width_ = width;
  height_ = height;
  area_ = 0.0;  // width_ * height_ = 0.0 for this case
  cluster_ = cluster;
  fixed_ = true;
}

// create a SoftMacro from a cluster
SoftMacro::SoftMacro(Cluster* cluster)
{
  name_ = cluster->getName();
  cluster_ = cluster;
}

// name
const std::string SoftMacro::getName() const
{
  return name_;
}

// Physical Information
void SoftMacro::setX(float x)
{
  if (refer_lx_ > 0.0 && refer_ly_ > 0.0) {
    if (x > refer_lx_) {
      x_ = x;
    } else {
      x_ = refer_lx_;
    }
    return;
  }

  if (!fixed_) {
    x_ = x;
  }
}

void SoftMacro::setY(float y)
{
  if (refer_lx_ > 0.0 && refer_ly_ > 0.0) {
    if (y > refer_ly_) {
      y_ = y;
    } else {
      y_ = refer_ly_;
    }
    return;
  }

  if (!fixed_) {
    y_ = y;
  }
}

void SoftMacro::setLocation(const std::pair<float, float>& location)
{
  if (fixed_) {
    return;
  }
  x_ = location.first;
  y_ = location.second;
}

// This is a utility function called by SetWidth, SetHeight
int SoftMacro::findPos(std::vector<std::pair<float, float>>& list,
                       float& value,
                       bool increase_order)
{
  // We assume the value is within the range of list
  int idx = 0;
  if (increase_order) {
    while ((idx < list.size()) && (list[idx].second < value)) {
      idx++;
    }
    if (list[idx].first > value) {
      value = list[idx].first;
    }
  } else {
    while ((idx < list.size()) && (list[idx].second > value)) {
      idx++;
    }
    if (list[idx].first < value) {
      value = list[idx].first;
    }
  }
  return idx;
}

void SoftMacro::setWidth(float width)
{
  if (width <= 0.0 || area_ == 0.0 || width_list_.size() != height_list_.size()
      || width_list_.empty() || cluster_ == nullptr
      || cluster_->getClusterType() == HardMacroCluster
      || cluster_->isIOCluster()) {
    return;
  }

  // the width_list_ is sorted in nondecreasing order
  if (width <= width_list_[0].first) {
    width_ = width_list_[0].first;
    height_ = height_list_[0].first;
    area_ = width_ * height_;
  } else if (width >= width_list_[width_list_.size() - 1].second) {
    width_ = width_list_[width_list_.size() - 1].second;
    height_ = height_list_[height_list_.size() - 1].second;
    area_ = width_ * height_;
  } else {
    width_ = width;
    int idx = findPos(width_list_, width_, true);
    area_ = width_list_[idx].second * height_list_[idx].second;
    height_ = area_ / width_;
  }
}

void SoftMacro::setHeight(float height)
{
  if (height <= 0.0 || area_ == 0.0 || width_list_.size() != height_list_.size()
      || width_list_.empty() || cluster_ == nullptr
      || cluster_->getClusterType() == HardMacroCluster
      || cluster_->isIOCluster()) {
    return;
  }

  // the height_list_ is sorted in nonincreasing order
  if (height >= height_list_[0].first) {
    height_ = height_list_[0].first;
    width_ = width_list_[0].first;
    area_ = width_ * height_;
  } else if (height <= height_list_[height_list_.size() - 1].second) {
    height_ = height_list_[height_list_.size() - 1].second;
    width_ = width_list_[width_list_.size() - 1].second;
    area_ = width_ * height_;
  } else {
    height_ = height;
    int idx = findPos(height_list_, height_, false);
    area_ = width_list_[idx].second * height_list_[idx].second;
    width_ = area_ / height_;
  }
}

void SoftMacro::shrinkArea(float percent)
{
  if (percent < 0.0) {
    percent = 0.0;
  }

  if (percent > 1.0) {
    percent = 1.0;
  }

  if (area_ == 0.0 || width_list_.size() != height_list_.size()
      || width_list_.empty() || cluster_ == nullptr
      || cluster_->getClusterType() != StdCellCluster
      || cluster_->isIOCluster()) {
    return;
  }

  width_ = width_ * percent;
  height_ = height_ * percent;
  area_ = width_ * height_;
}

void SoftMacro::setArea(float area)
{
  if (area_ == 0.0 || width_list_.size() != height_list_.size()
      || width_list_.empty() || cluster_ == nullptr
      || cluster_->getClusterType() == HardMacroCluster
      || cluster_->isIOCluster()
      || area <= width_list_[0].first * height_list_[0].first) {
    return;
  }

  // area must be larger than area_
  std::vector<std::pair<float, float>> width_list;
  std::vector<std::pair<float, float>> height_list;
  for (int i = 0; i < width_list_.size(); i++) {
    const float min_width = width_list_[i].first;
    const float min_height = height_list_[i].second;
    const float max_width = area / min_height;
    const float max_height = area / min_width;
    if (width_list.empty()
        || min_width > width_list[width_list.size() - 1].second) {
      width_list.emplace_back(min_width, max_width);
      height_list.emplace_back(max_height, min_height);
    } else {
      width_list[width_list.size() - 1].second = max_width;
      height_list[height_list.size() - 1].second = min_height;
    }
  }

  width_list_ = width_list;
  height_list_ = height_list;
  area_ = area;
  width_ = width_list_[0].first;
  height_ = height_list_[0].first;
}

// This function for discrete shape curves, HardMacroCluster
void SoftMacro::setShapes(const std::vector<std::pair<float, float>>& shapes,
                          bool force_flag)
{
  if (!force_flag
      && (shapes.empty() || cluster_ == nullptr
          || cluster_->getClusterType() != HardMacroCluster)) {
    return;
  }

  // Here we do not need to sort width_list_, height_list_
  for (auto& shape : shapes) {
    width_list_.emplace_back(shape.first, shape.first);
    height_list_.emplace_back(shape.second, shape.second);
  }
  width_ = shapes[0].first;
  height_ = shapes[0].second;
  area_ = shapes[0].first * shapes[0].second;
}

// This function for specify shape curves (piecewise function),
// for StdCellCluster and MixedCluster
void SoftMacro::setShapes(
    const std::vector<std::pair<float, float>>& width_list,
    float area)
{
  if (width_list.empty() || area <= 0.0 || cluster_ == nullptr
      || cluster_->isIOCluster()
      || cluster_->getClusterType() == HardMacroCluster) {
    return;
  }
  area_ = area;
  width_list_.clear();
  height_list_.clear();
  // sort width list based
  height_list_ = width_list;
  std::sort(height_list_.begin(), height_list_.end(), comparePairFirst);
  for (auto& shape : height_list_) {
    if (width_list_.empty()
        || shape.first > width_list_[width_list_.size() - 1].second) {
      width_list_.push_back(shape);
    } else if (shape.second > width_list_[width_list_.size() - 1].second) {
      width_list_[width_list_.size() - 1].second = shape.second;
    }
  }
  height_list_.clear();
  for (auto& shape : width_list_) {
    height_list_.emplace_back(area / shape.first, area / shape.second);
  }
  width_ = width_list_[0].first;
  height_ = height_list_[0].first;
}

float SoftMacro::getX() const
{
  return x_;
}

float SoftMacro::getY() const
{
  return y_;
}

float SoftMacro::getPinX() const
{
  return x_ + width_ / 2.0;
}

float SoftMacro::getPinY() const
{
  return y_ + height_ / 2.0;
}

const std::pair<float, float> SoftMacro::getLocation() const
{
  return std::pair<float, float>(x_, y_);
}

float SoftMacro::getWidth() const
{
  return width_;
}

float SoftMacro::getHeight() const
{
  return height_;
}

float SoftMacro::getArea() const
{
  return area_ > 0.01 ? area_ : 0.0;
}

// Num Macros
bool SoftMacro::isMacroCluster() const
{
  if (cluster_ == nullptr) {
    return false;
  }
  return (cluster_->getClusterType() == HardMacroCluster);
}

bool SoftMacro::isStdCellCluster() const
{
  if (cluster_ == nullptr) {
    return false;
  }

  return (cluster_->getClusterType() == StdCellCluster);
}

int SoftMacro::getNumMacro() const
{
  if (cluster_ == nullptr) {
    return 0;
  }

  return cluster_->getNumMacro();
}

void SoftMacro::resizeRandomly(
    std::uniform_real_distribution<float>& distribution,
    std::mt19937& generator)
{
  if (width_list_.empty()) {
    return;
  }
  const int idx = static_cast<int>(
      std::floor(distribution(generator) * width_list_.size()));
  const float min_width = width_list_[idx].first;
  const float max_width = width_list_[idx].second;
  width_ = min_width + distribution(generator) * (max_width - min_width);
  area_ = width_list_[idx].first * height_list_[idx].first;
  height_ = area_ / width_;
}

// Align Flag support
void SoftMacro::setAlignFlag(bool flag)
{
  align_flag_ = flag;
}

bool SoftMacro::getAlignFlag() const
{
  return align_flag_;
}

// cluster
Cluster* SoftMacro::getCluster() const
{
  return cluster_;
}

// Calculate macro utilization
float SoftMacro::getMacroUtil() const
{
  if (cluster_ == nullptr || area_ == 0.0) {
    return 0.0;
  }

  if (cluster_->getClusterType() == HardMacroCluster) {
    return 1.0;
  }

  if (cluster_->getClusterType() == MixedCluster) {
    return cluster_->getMacroArea() / area_;
  }

  return 0.0;
}

bool SoftMacro::isMixedCluster() const
{
  if (cluster_ == nullptr) {
    return false;
  }

  return (cluster_->getClusterType() == MixedCluster);
}

void SoftMacro::setLocationF(float x, float y)
{
  x_ = x;
  y_ = y;
}

void SoftMacro::setShapeF(float width, float height)
{
  width_ = width;
  height_ = height;
  area_ = width * height;
}

}  // namespace mpl2
