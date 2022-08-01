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

#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <string>
#include <vector>
#include <algorithm>
#include <random>

#include "object.h"
#include "db_sta/dbReadVerilog.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "sta/Bfs.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/Sta.hh"
#include "utl/Logger.h"

namespace mpl {
using utl::MPL;

///////////////////////////////////////////////////////////////////////
// Basic utility functions

// conversion between dbu and micro
float Dbu2Micro(int metric, float dbu) 
{
  return metric / dbu;
}

int Micro2Dbu(float metric, float dbu) 
{
  return std::round(metric * std::round(dbu));
}

// Sort shapes
bool SortShape(const std::pair<float, float>& shape1,
               const std::pair<float, float>& shape2)
{
  // first sort based on area, then based on aspect ratios
  if (shape1.first * shape1.second == shape2.first * shape2.second) 
    return shape1.second / shape1.first < shape2.second / shape2.first;
  else
    return shape1.first * shape2.second < shape2.first * shape2.second;
}


std::string to_string(const PinAccess& pin_access)
{
  if (pin_access == L)
    return std::string("L");
  else if (pin_access == T)
    return std::string("T");
  else if (pin_access == R)
    return std::string("R");
  else if (pin_access == B)
    return std::string("B");
  else
    return std::string("NONE");
}

PinAccess Opposite(const PinAccess& pin_access) 
{
  if (pin_access == L)
    return R;
  else if (pin_access == T)
    return B;
  else if (pin_access == R)
    return L;
  else if (pin_access == B)
    return T;
  else
    return NONE;
}

// Compare two intervals according to starting points
bool ComparePairFirst(std::pair<float, float> p1, std::pair<float, float> p2)
{
  return p1.first < p2.first;
}

// Compare two intervals according to the product
bool ComparePairProduct(std::pair<float, float> p1, std::pair<float, float> p2)
{
  return p1.first * p1.second < p2.first * p2.second;
}

///////////////////////////////////////////////////////////////////////
// Metric Class
Metric::Metric(unsigned int num_std_cell, 
               unsigned int num_macro,
               float std_cell_area,
               float macro_area) 
{
  this->num_std_cell_ = num_std_cell;
  this->num_macro_ = num_macro;
  this->std_cell_area_ = std_cell_area;
  this->macro_area_ = macro_area;
}

void Metric::AddMetric(const Metric& metric) 
{
  this->num_std_cell_ += metric.num_std_cell_;
  this->num_macro_ += metric.num_macro_;
  this->std_cell_area_ += metric.std_cell_area_;
  this->macro_area_ += metric.macro_area_;
  this->inflat_std_cell_area_ += metric.inflat_std_cell_area_;
  this->inflat_macro_area_ += metric.inflat_macro_area_;
}

void Metric::InflatStdCellArea(float std_cell_util) 
{
  if ((std_cell_util > 0.0) && (std_cell_util < 1.0))
    this->inflat_std_cell_area_ /= std_cell_util;
}

const std::pair<unsigned int, unsigned int> 
  Metric::GetCountStats() const 
{
  return std::pair<unsigned int, unsigned int>(this->num_std_cell_, 
                                               this->num_macro_);  
}

const std::pair<float, float> Metric::GetAreaStats() const 
{
  return std::pair<float, float>(this->std_cell_area_, this->macro_area_);
}

const std::pair<float, float> Metric::GetInflatAreaStats() const 
{
  return std::pair<float, float>(this->inflat_std_cell_area_, 
                                 this->inflat_macro_area_);
}

unsigned int Metric::GetNumMacro() const 
{
  return num_macro_;
}

unsigned int Metric::GetNumStdCell() const 
{
  return num_std_cell_;
}

float Metric::GetStdCellArea() const 
{
  return std_cell_area_;  
}

float Metric::GetMacroArea() const 
{
  return macro_area_;
}

float Metric::GetArea() const 
{
  return std_cell_area_ + macro_area_;
}

float Metric::GetInflatStdCellArea() const 
{
  return inflat_std_cell_area_;
}

float Metric::GetInflatMacroArea() const 
{
  return inflat_macro_area_;
}

float Metric::GetInflatArea() const 
{
  return inflat_std_cell_area_ + inflat_macro_area_;
}


///////////////////////////////////////////////////////////////////////
// Cluster Class
// Constructors and Destructors
Cluster::Cluster(int cluster_id) 
{
  this->id_ = cluster_id;  
}

Cluster::Cluster(int cluster_id, std::string cluster_name) 
{
  this->id_ = cluster_id;
  this->name_ = cluster_name;
}

Cluster::~Cluster() 
{
  delete soft_macro_;  // delete related soft macro object  
}

// cluster id
int Cluster::GetId() const
{
  return id_;
}

// cluster name
const std::string Cluster::GetName() const
{
  return name_;
}

void Cluster::SetName(const std::string name) 
{
  name_ = name;  
}

// cluster type
void Cluster::SetClusterType(const ClusterType& cluster_type)
{
  type_ = cluster_type;
}

const ClusterType Cluster::GetClusterType() const
{
  return type_;
}

// Instances (Here we store dbModule to reduce memory)
void Cluster::AddDbModule(odb::dbModule* dbModule) 
{
  dbModules_.push_back(dbModule);
}

void Cluster::AddLeafStdCell(odb::dbInst* leaf_std_cell) 
{
  leaf_std_cells_.push_back(leaf_std_cell);  
}

void Cluster::AddLeafMacro(odb::dbInst* leaf_macro) 
{
  leaf_macros_.push_back(leaf_macro);
}

void Cluster::SpecifyHardMacros(std::vector<HardMacro*>& hard_macros)
{
  hard_macros_ = hard_macros;  
}

const std::vector<odb::dbModule*> Cluster::GetDbModules() const
{
  return dbModules_;
}

const std::vector<odb::dbInst*> Cluster::GetLeafStdCells() const 
{
  return leaf_std_cells_;
}

const std::vector<odb::dbInst*> Cluster::GetLeafMacros() const 
{
  return leaf_macros_;
}

const std::vector<HardMacro*> Cluster::GetHardMacros() const
{
  return hard_macros_;
}

void Cluster::ClearDbModules()
{
  dbModules_.clear();
}

void Cluster::ClearLeafStdCells()
{
  leaf_std_cells_.clear();
}

void Cluster::ClearLeafMacros()
{
  leaf_macros_.clear();
}

void Cluster::ClearHardMacros()
{
  hard_macros_.clear();
}

// copy instances based on cluster Type
void Cluster::CopyInstances(const Cluster& cluster)
{
  // clear firstly
  dbModules_.clear();
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
    dbModules_.insert(dbModules_.end(),
            cluster.dbModules_.begin(),
              cluster.dbModules_.end());
  } else { // type_ == MixedCluster
    leaf_macros_.insert(leaf_macros_.end(),
                    cluster.leaf_macros_.begin(),
                    cluster.leaf_macros_.end());
    leaf_std_cells_.insert(leaf_std_cells_.end(),
                 cluster.leaf_std_cells_.begin(),
                   cluster.leaf_std_cells_.end());
    dbModules_.insert(dbModules_.end(),
            cluster.dbModules_.begin(),
              cluster.dbModules_.end());
  }
}

// Bundled IO (Pads) cluster support
// The position is the center of IO pads in the cluster
void Cluster::SetIOClusterFlag(const std::pair<float, float> pos, 
                               const float width, const float height) 
{
  io_cluster_flag_ = true;
  // call the constructor to create a SoftMacro
  // representing the IO cluster
  delete soft_macro_;
  soft_macro_ = new SoftMacro(pos, name_, width, height);
}

bool Cluster::GetIOClusterFlag() const 
{
  return io_cluster_flag_;
}

// Metric Support and Statistics
void Cluster::SetMetric(const Metric& metric)
{
  metric_ = metric;  
}

const Metric Cluster::GetMetric() const 
{
  return metric_;
}

int Cluster::GetNumStdCell() const 
{
  if (type_ == HardMacroCluster)
    return 0;
  else
    return metric_.GetNumStdCell();
}

int Cluster::GetNumMacro() const
{
  if (type_ == StdCellCluster)
    return 0;
  else
    return metric_.GetNumMacro();
}

float Cluster::GetArea() const
{
  return this->GetStdCellArea() + this->GetMacroArea();
}

float Cluster::GetStdCellArea() const
{
  if (type_ == HardMacroCluster)
    return 0.0;
  else
    return metric_.GetStdCellArea();
}

float Cluster::GetMacroArea() const
{
  if (type_ == StdCellCluster)
    return 0.0;
  else
    return metric_.GetMacroArea();
}

// Physical location support
float Cluster::GetWidth() const 
{
  if (soft_macro_ == nullptr)
    return 0.0;
  else
    return soft_macro_->GetWidth();
}

float Cluster::GetHeight() const 
{
  if (soft_macro_ == nullptr)
    return 0.0;
  else
    return soft_macro_->GetHeight();
}

float Cluster::GetX() const 
{
  if (soft_macro_ == nullptr)
    return 0.0;
  else
    return soft_macro_->GetX();
}

float Cluster::GetY() const 
{
  if (soft_macro_ == nullptr)
    return 0.0;
  else
    return soft_macro_->GetY();
}

void Cluster::SetX(float x)  
{
  if (soft_macro_ != nullptr)
    soft_macro_->SetX(x);
}

void Cluster::SetY(float y)  
{
  if (soft_macro_ != nullptr)
    soft_macro_->SetY(y);
}

const std::pair<float, float> Cluster::GetLocation() const 
{
  if (soft_macro_ == nullptr)
    return std::pair<float, float>(0.0, 0.0);
  else
    return soft_macro_->GetLocation();
}

// Hierarchy Support
void Cluster::SetParent(Cluster* parent) 
{
  parent_ = parent;
}

void Cluster::AddChild(Cluster* child) 
{
  children_.insert(child);
}
    
void Cluster::RemoveChild(const Cluster* child) 
{
  children_.erase(std::find(children_.begin(), children_.end(), child));
}

void Cluster::AddChildren(const std::vector<Cluster*>& children) 
{
  children_.insert(children.begin(), children.end());
}
    
void Cluster::RemoveChildren() 
{
  children_.clear();
}
    
Cluster* Cluster::GetParent() const 
{
  return parent_;
}
    
std::set<Cluster*> Cluster::GetChildren() const 
{
  return children_;  
}
 
bool Cluster::IsLeaf() const 
{
  return (children_.size() == 0);   
}

// We only merge clusters with the same parent cluster
bool Cluster::MergeCluster(const Cluster& cluster) 
{
  if (parent_ != cluster.parent_)
    return false;

  // modify name
  name_ += "||" + cluster.name_;
  // add instances (std cells and macros)
  //this->CopyInstances(cluster); 
  leaf_macros_.insert(leaf_macros_.end(),
                      cluster.leaf_macros_.begin(),
                      cluster.leaf_macros_.end());
  leaf_std_cells_.insert(leaf_std_cells_.end(),
                cluster.leaf_std_cells_.begin(),
                cluster.leaf_std_cells_.end());
  dbModules_.insert(dbModules_.end(),
                cluster.dbModules_.begin(),
                cluster.dbModules_.end());
  // Add Metric
  metric_.AddMetric(cluster.metric_);
  // Remove this cluster from the children list of the parent
  parent_->RemoveChild(&cluster);
  return true;
}

// Connection signature support
void Cluster::InitConnection() 
{
  connection_map_.clear();   
}

void Cluster::AddConnection(int cluster_id, float weight) 
{
  if (connection_map_.find(cluster_id) == connection_map_.end())
    connection_map_[cluster_id] = weight;
  else
    connection_map_[cluster_id] += weight;
}
    
const std::map<int, float> Cluster::GetConnection() const 
{
  return connection_map_;
}


// The connection signature is based on connection topology
// if the number of connnections between two clusters is larger than the net_threshold
// we think the two clusters are connected, otherwise they are disconnected.     
bool Cluster::IsSameConnSignature(const Cluster& cluster, 
                                  float net_threshold) 
{
  std::vector<int> neighbors;  // neighbors of current cluster
  std::vector<int> cluster_neighbors; // neighbors of the input cluster
  for (auto& [cluster_id , weight] : connection_map_)
    if ((cluster_id != id_) &&
        (cluster_id != cluster.id_) &&
        (weight >= net_threshold)) 
      neighbors.push_back(cluster_id);

  for (auto& [cluster_id , weight] : cluster.connection_map_)
    if ((cluster_id != id_) &&
        (cluster_id != cluster.id_) &&
        (weight >= net_threshold)) 
      cluster_neighbors.push_back(cluster_id);
 
  if (neighbors.size() != cluster_neighbors.size()) {
    return false;
  } else {
    std::sort(neighbors.begin(), neighbors.end());
    std::sort(cluster_neighbors.begin(), cluster_neighbors.end());
    for (int i = 0; i < neighbors.size(); i++)
      if (neighbors[i] != cluster_neighbors[i])
        return false;
  }
 
  return true;  
}

// Get closely-connected cluster if such cluster exists
// For example, if a small cluster A is closely connected to a
// well-formed cluster B, (there are also other well-formed clusters
// C, D), A is only connected to B and A has no connection with C, D
// candidate_clusters are small clusters, 
// any cluster not in candidate_clusters is a well-formed cluster
int Cluster::GetCloseCluster(const std::vector<int>& candidate_clusters,
                         float net_threshold)
{
  int closely_cluster = -1;
  int num_closely_clusters = 0;
  for (auto& [cluster_id, num_nets] : connection_map_) {
    if (num_nets > net_threshold &&
        std::find(candidate_clusters.begin(), candidate_clusters.end(), cluster_id) 
        == candidate_clusters.end()) {
      num_closely_clusters++;
      closely_cluster = cluster_id;
    }
  }

  if (num_closely_clusters == 1)
    return closely_cluster;
  else
    return -1;
}

// Pin Access Support
void Cluster::SetPinAccess(int cluster_id, PinAccess pin_access, float net_weight) 
{
  if (cluster_id < 0)
    std::cout << "Error !!!\n"
              << "Cluster id is less than 0 in SetPinAccess"
              << std::endl;
  pin_access_map_[cluster_id] = std::pair<PinAccess, float>(pin_access, net_weight);
}

const std::pair<PinAccess, float> Cluster::GetPinAccess(int cluster_id) 
{
  return pin_access_map_[cluster_id];
}

const std::map<int, std::pair<PinAccess, float> > Cluster::GetPinAccessMap() const 
{
  return pin_access_map_;
}

const std::map<PinAccess, std::map<PinAccess, float> > Cluster::GetBoundaryConnection() const 
{
  return boundary_connection_map_;
}

void Cluster::AddBoundaryConnection(PinAccess pin_a, PinAccess pin_b, float num_net)
{
  if (boundary_connection_map_.find(pin_a) == boundary_connection_map_.end()) {
    std::map<PinAccess, float> pin_map;
    pin_map[pin_b] = num_net;
    boundary_connection_map_[pin_a] = pin_map;
  } else {
    if (boundary_connection_map_[pin_a].find(pin_b) == boundary_connection_map_[pin_a].end())
      boundary_connection_map_[pin_a][pin_b] = num_net;
    else
      boundary_connection_map_[pin_a][pin_b] += num_net;
  }
}


// Print Basic Information
// Normally we call this after macro placement is done
void Cluster::PrintBasicInformation(utl::Logger* logger) const {
  std::string line = "\n";
  line += std::string(80, '*') + "\n";
  line += "[INFO] cluster_name :  " + name_ + "  ";
  line += "cluster_id : " + std::to_string(id_) + "  \n";
  line += "num_std_cell : " + std::to_string(this->GetNumStdCell()) + "  ";
  line += "num_macro : " + std::to_string(this->GetNumMacro()) + "\n";
  line += "width : " + std::to_string(this->GetWidth()) + "  ";
  line += "height : " + std::to_string(this->GetHeight()) + "  ";
  line += "location :  ( " + std::to_string((this->GetLocation()).first) +  " , ";
  line += std::to_string((this->GetLocation()).second) +  " )\n";
  for (const auto& hard_macro : hard_macros_) {
    line += "\t macro_name : " + hard_macro->GetName();
    line += "\t width : " + std::to_string(hard_macro->GetRealWidth());
    line += "\t height : " + std::to_string(hard_macro->GetRealHeight());
    line += "\t lx : " + std::to_string(hard_macro->GetRealX());
    line += "\t ly : " + std::to_string(hard_macro->GetRealY());
    line += "\n";
  }

  logger->info(MPL, 2022, line);
}


// Macro Placement Support
void Cluster::SetSoftMacro(SoftMacro* soft_macro)
{
  delete soft_macro_;
  soft_macro_ = soft_macro;
}

SoftMacro* Cluster::GetSoftMacro() const
{
  return soft_macro_;
}

void Cluster::SetMacroTilings(const std::vector<std::pair<float, float> >& tilings)
{
  macro_tilings_ = tilings;
}

const std::vector<std::pair<float, float> > Cluster::GetMacroTilings() const
{
  return macro_tilings_;
}

// Virtual Connections
const std::vector<std::pair<int, int> > Cluster::GetVirtualConnections() const
{
  return virtual_connections_;
}

void Cluster::AddVirtualConnection(int src, int target)
{
  virtual_connections_.push_back(std::pair<int, int>(src, target));
}

///////////////////////////////////////////////////////////////////////
// Metric HardMacro
HardMacro::HardMacro(std::pair<float, float> loc, const std::string name) 
{
  width_ = 0.0;
  height_ = 0.0;
  name_ = name;
  pin_x_ = 0.0;
  pin_y_ = 0.0;
  x_ = loc.first;
  y_ = loc.second;
}

HardMacro::HardMacro(float width, float height, const std::string name) 
{
  width_ = width;
  height_ = height;
  name_ = name;
  pin_x_ = width / 2.0;
  pin_y_ = height / 2.0;
}


HardMacro::HardMacro(odb::dbInst* inst, float dbu, float halo_width)
{
  inst_ = inst;
  dbu_ = dbu;
  halo_width_ = halo_width;

  // set name
  name_ = inst->getName();
  odb::dbMaster* master = inst->getMaster();
  // set the width and height
  width_ = Dbu2Micro(master->getWidth(), dbu) + 2 * halo_width;
  height_ = Dbu2Micro(master->getHeight(), dbu) + 2 * halo_width;
  // Set the position of virtual pins
  // Here we only consider signal pins
  odb::Rect bbox;
  bbox.mergeInit();
  for (odb::dbMTerm* mterm : master->getMTerms()) 
    if (mterm->getSigType() == odb::dbSigType::SIGNAL) 
      for (odb::dbMPin* mpin : mterm->getMPins()) 
        for (odb::dbBox* box : mpin->getGeometry()) {
          odb::Rect rect = box->getBox();
          bbox.merge(rect);
        }
  pin_x_ = Dbu2Micro((bbox.xMin() + bbox.xMax()) / 2.0, dbu) + halo_width_;
  pin_y_ = Dbu2Micro((bbox.yMin() + bbox.yMax()) / 2.0, dbu) + halo_width_;
  std::cout << "pin_x : " << pin_x_ << std::endl;
  std::cout << "pin_y : " << pin_y_ << std::endl;
}


// overload the comparison operators
// based on area, width, height order
// When we compare, we also consider the effect of halo_width
bool HardMacro::operator<(const HardMacro& macro) const 
{
  if (width_ * height_  != macro.width_ * macro.height_)
    return width_ * height_ < macro.width_ * macro.height_;
  else if (width_ != macro.width_)
    return width_ < macro.width_;
  else
    return height_ < macro.height_;
}

bool HardMacro::operator==(const HardMacro& macro) const 
{
  
  std::cout << "width_ :  " << width_ << "  macro.width_ : " << macro.width_ << "  "
            << "height_ :  " << height_ << "  macro.height_ : " << macro.height_ << "   "
            << ((width_ == macro.width_) && (height_ == macro.height_)) << std::endl;
  return (width_ == macro.width_) && (height_ == macro.height_);
}


// Get Physical Information
// Note that the default X and Y include halo_width
void HardMacro::SetLocation(const std::pair<float, float>& location) 
{
  if (width_ * height_ <= 0.0)
    return;
  x_ = location.first;
  y_ = location.second;
}

void HardMacro::SetX(float x) 
{
  if (width_ * height_ <= 0.0)
    return;
  x_ = x;  
}
  
void HardMacro::SetY(float y) 
{
  if (width_ * height_ <= 0.0)
    return;
  y_ = y; 
}
 
const std::pair<float, float> HardMacro::GetLocation() const 
{
  return std::pair<float, float>(x_, y_);
}
    
float HardMacro::GetX() const 
{
  return x_;  
}

float HardMacro::GetY() const 
{
  return y_;  
}

// The position of pins relative to the lower left of the instance
float HardMacro::GetPinX() const 
{
  return x_ + pin_x_;  
}

float HardMacro::GetPinY() const 
{
  return y_ + pin_y_;
}

// The position of pins relative to the origin of the canvas;
float HardMacro::GetAbsPinX() const 
{
  return pin_x_;
}

float HardMacro::GetAbsPinY() const 
{
  return pin_y_;
}

// width and height
float HardMacro::GetWidth() const
{
  return width_;
}

float HardMacro::GetHeight() const
{
  return height_;
}



// Note that the real X and Y does NOT include halo_width
void HardMacro::SetRealLocation(const std::pair<float, float>& location) 
{
  if (width_ * height_ <= 0.0)
    return;
  
  x_ = location.first - halo_width_;
  y_ = location.second - halo_width_;
}
     
void HardMacro::SetRealX(float x) 
{
  if (width_ * height_ <= 0.0)
    return;
  x_ = x - halo_width_;  
}
     
void HardMacro::SetRealY(float y) 
{
  if (width_ * height_ <= 0.0)
    return;
  y_ = y - halo_width_;  
}
 
const std::pair<float, float> HardMacro::GetRealLocation() const 
{
  return std::pair<float, float>(x_ + halo_width_, y_ + halo_width_);  
}
     
float HardMacro::GetRealX() const 
{
  return x_ + halo_width_;  
}

float HardMacro::GetRealY() const 
{
  return y_ + halo_width_; 
}

float HardMacro::GetRealWidth() const
{
  return width_ - 2 * halo_width_;
}

float HardMacro::GetRealHeight() const
{
  return height_ - 2 * halo_width_;
}


// Orientation support
std::string HardMacro::GetOrientation() const 
{  
  return orientation_.getString();
}
     
// We do not allow rotation of macros
// This may violate the direction of metal layers
// axis = true, flip horizontally
// axis = false, flip vertically
void HardMacro::Flip(bool axis) 
{
  //if (orientation_.getString() == std::string("MY"))
  //  return;
  if (axis == true) {
    orientation_ = orientation_.flipX();
    pin_y_ = height_ - pin_y_; 
  } else {
    orientation_ = orientation_.flipY();
    pin_x_ = width_ - pin_x_;
    //std::cout << "action :  " << orientation_.getString() << std::endl;
  }
}

// Interfaces with OpenDB
odb::dbInst* HardMacro::GetInst() const 
{
  return inst_;  
}

const std::string HardMacro::GetName() const 
{
  return name_;  
}

const std::string HardMacro::GetMasterName() const 
{
  if (inst_ == nullptr)
    return name_;
  else
    return inst_->getMaster()->getName();
}

// update the location and orientation of the macro inst in OpenDB
void HardMacro::UpdateDb(float pitch_x, float pitch_y) 
{
  if ((inst_ == nullptr) || (dbu_ <= 0.0)) 
    return;
  float lx = this->GetRealX();
  float ly = this->GetRealY();
  float ux = lx + this->GetRealWidth();
  float uy = ly + this->GetRealHeight();
  lx = std::round(lx / pitch_x) * pitch_x;
  ux = std::round(ux / pitch_x) * pitch_x;
  ly = std::round(ly / pitch_y) * pitch_y;
  uy = std::round(uy / pitch_y) * pitch_y;
  std::cout << "Update macro " << this->GetName() << std::endl;
  std::cout << "lx :  " << lx << "  "
            << "ly :  " << ly << "  "
            << "ux :  " << ux << "  "
            << "uy :  " << uy << "  "
            << "orientation : " << orientation_.getString()
            << std::endl;
  if (orientation_.getString() == std::string("MX")) 
    inst_->setLocation(Micro2Dbu(lx, dbu_), Micro2Dbu(uy, dbu_));
  else if (orientation_.getString() == std::string("MY"))
    inst_->setLocation(Micro2Dbu(ux, dbu_), Micro2Dbu(ly, dbu_));
  else if (orientation_.getString() == std::string("R180"))
    inst_->setLocation(Micro2Dbu(ux, dbu_), Micro2Dbu(uy, dbu_));
  else
    inst_->setLocation(Micro2Dbu(lx, dbu_), Micro2Dbu(ly, dbu_));
  inst_->setOrient(orientation_);
  inst_->setPlacementStatus(odb::dbPlacementStatus::LOCKED);
}
    
///////////////////////////////////////////////////////////////////////
// SoftMacro Class
// Create a SoftMacro with specified size
// In this case, we think the cluster is a macro cluster with only one macro
// SoftMacro : Hard Macro (or pin access blockage)
SoftMacro::SoftMacro(float width, float height, const std::string name) 
{
  name_ = name;
  width_ = width;
  height_  = height;
  area_ = width * height;
  cluster_ = nullptr;
}

// SoftMacro : Fixed Hard Macro (or blockage)
SoftMacro::SoftMacro(float width, float height, const std::string name,
                     float lx, float ly) 
{
  name_    = name;
  width_   = width;
  height_  = height;
  area_    = width * height;
  x_       = lx;
  y_       = ly;
  cluster_ = nullptr;
  fixed_   = true;
}

// Create a SoftMacro representing the IO cluster or fixed terminals
SoftMacro::SoftMacro(const std::pair<float, float>& pos, const std::string name,
                     float width, float height) 
{ 
  name_ = name;
  x_ = pos.first;
  y_ = pos.second;
  width_  = width;
  height_ = height;
  area_   = 0.0; // width_ * height_ = 0.0 for this case
  cluster_ = nullptr;
  fixed_  = true;
}

// create a SoftMacro from a cluster
SoftMacro::SoftMacro(Cluster* cluster) 
{
  name_ = cluster->GetName();
  cluster_ = cluster;
}

// name
const std::string SoftMacro::GetName() const
{
  return name_;
}

// Physical Information
void SoftMacro::SetX(float x) 
{
  if (refer_lx_ > 0.0 && refer_ly_ > 0.0) {
    if (x > refer_lx_)
      x_ = x;
    else
      x_ = refer_lx_;
    return;
  }
    
  if (fixed_ == false)
    x_ = x;
}
 
void SoftMacro::SetY(float y)
{
  if (refer_lx_ > 0.0 && refer_ly_ > 0.0) {
    if (y > refer_ly_)
      y_ = y;
    else
      y_ = refer_ly_;
    return;
  }
  
  if (fixed_ == false)
    y_ = y;
}

void SoftMacro::SetLocation(const std::pair<float, float>& location)
{
  if (fixed_ == true)
    return;  
  x_ = location.first;
  y_ = location.second;
}

// This is a utility function called by SetWidth, SetHeight
int SoftMacro::FindPos(std::vector<std::pair<float, float> >& list, 
                       float& value,  bool increase_order)
{
  // We assume the value is within the range of list
  int idx = 0;
  if (increase_order == true) {
    while ((idx < list.size()) && (list[idx].second < value))
      idx++;
    if (list[idx].first > value) 
      value = list[idx].first;    
  } else {
    while ((idx < list.size()) && (list[idx].second > value))
      idx++;
    if (list[idx].first < value) 
      value = list[idx].first;    
  }
  return idx;
}

void SoftMacro::SetWidth(float width)
{
  if (width <= 0.0 || area_ == 0.0 || 
      width_list_.size() != height_list_.size() ||
      width_list_.size() == 0 ||
      cluster_ == nullptr ||
      cluster_->GetClusterType() == HardMacroCluster ||
      cluster_->GetIOClusterFlag() == true)
    return;

  // the width_list_ is sorted in nondecreasing order
  if (width <= width_list_[0].first) {
    width_  = width_list_[0].first;
    height_ = height_list_[0].first;
    area_   = width_ * height_;
  } else if (width  >= width_list_[width_list_.size() - 1].second) {
    width_   = width_list_[width_list_.size() - 1].second;
    height_  = height_list_[height_list_.size() - 1].second;
    area_    = width_ * height_;
  } else {
    width_ = width;
    int idx = FindPos(width_list_, width_, true);
    area_ = width_list_[idx].second * height_list_[idx].second;
    height_ = area_ / width_;
  }
}

void SoftMacro::SetHeight(float height)
{  
  if (height <= 0.0 || area_ == 0.0 || 
      width_list_.size() != height_list_.size() ||
      width_list_.size() == 0 ||
      cluster_ == nullptr ||
      cluster_->GetClusterType() == HardMacroCluster ||
      cluster_->GetIOClusterFlag() == true)
    return;
  
  // the height_list_ is sorted in nonincreasing order
  if (height >= height_list_[0].first) {
    height_ = height_list_[0].first;
    width_  = width_list_[0].first;
    area_   = width_ * height_;
  } else if (height <= height_list_[height_list_.size() - 1].second) {
    height_  = height_list_[height_list_.size() - 1].second;
    width_   = width_list_[width_list_.size() - 1].second;
    area_ = width_ * height_;
  } else {
    height_ = height;
    int idx = FindPos(height_list_, height_, false);
    area_ = width_list_[idx].second * height_list_[idx].second;
    width_ = area_ / height_;
  }
}



void SoftMacro::ShrinkArea(float percent)
{
  if (percent < 0.0)
    percent = 0.0;

  if (percent > 1.0)
    percent = 1.0;

  if (area_ == 0.0 || 
      width_list_.size() != height_list_.size() ||
      width_list_.size() == 0 ||
      cluster_ == nullptr ||
      cluster_->GetClusterType() != StdCellCluster ||
      cluster_->GetIOClusterFlag() == true)
    return;
  
  width_ = width_ * percent;
  height_ = height_ * percent;
  area_ = width_ * height_;
}


void SoftMacro::SetArea(float area)
{
  if (area_ == 0.0 || 
      width_list_.size() != height_list_.size() ||
      width_list_.size() == 0 ||
      cluster_ == nullptr ||
      cluster_->GetClusterType() == HardMacroCluster ||
      cluster_->GetIOClusterFlag() == true ||
      area <= width_list_[0].first * height_list_[0].first)
    return;
  
  // area must be larger than area_
  std::vector<std::pair<float, float> > width_list;
  std::vector<std::pair<float, float> > height_list;
  for (int i = 0; i < width_list_.size(); i++) {
    const float min_width = width_list_[i].first;
    const float min_height = height_list_[i].second;
    const float max_width = area / min_height;
    const float max_height = area / min_width;
    if (width_list.size() == 0 || 
        min_width > width_list[width_list.size() - 1].second) {
      width_list.push_back(std::pair<float, float>(min_width, max_width));
      height_list.push_back(std::pair<float, float>(max_height, min_height));
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
void SoftMacro::SetShapes(const std::vector<std::pair<float, float> >& shapes, bool force_flag)
{
  if (force_flag == false && (shapes.size() == 0 || cluster_ == nullptr || 
      cluster_->GetClusterType() != HardMacroCluster))
    return;

  // Here we do not need to sort width_list_, height_list_
  for (auto& shape : shapes) {
    width_list_.push_back(std::pair<float, float>(shape.first, shape.first));
    height_list_.push_back(std::pair<float, float>(shape.second, shape.second));
  }
  width_ = shapes[0].first;
  height_ = shapes[0].second;
  area_ = shapes[0].first * shapes[0].second;
}
    

// This function for specify shape curves (piecewise function),
// for StdCellCluster and MixedCluster
void SoftMacro::SetShapes(const std::vector<std::pair<float, float> >& width_list, float area)
{ 
  if (width_list.size() == 0 || area <= 0.0 ||
      cluster_ == nullptr || cluster_->GetIOClusterFlag() == true ||
      cluster_->GetClusterType() == HardMacroCluster)
    return;
  area_ = area;
  // sort width list based 
  height_list_ = width_list;
  std::sort(height_list_.begin(), height_list_.end(), ComparePairFirst);
  for (auto& shape : height_list_) {
    const float min_width = shape.first;
    const float max_width = shape.second;
    if (width_list_.size() == 0 || 
        min_width > width_list_[width_list_.size() - 1].second) 
      width_list_.push_back(std::pair<float, float>(min_width, max_width));
    else 
      width_list_[width_list_.size() - 1].second = max_width;
  }
  height_list_.clear();
  for (auto& shape : width_list_) 
    height_list_.push_back(std::pair<float, float>(area / shape.first, area / shape.second));
  width_ = width_list_[0].first;
  height_ = height_list_[0].first;
  std::cout << this->GetName() << std::endl;
  std::cout << "width_list : ";
  for (auto& width : width_list_)
    std::cout << width.first << "  -  " << width.second << "   ";
  std::cout << std::endl;
  std::cout << "height_list : ";
  for (auto& height : height_list_)
    std::cout << height.first << " -  " << height.second << "   ";
  std::cout << std::endl;
}


float SoftMacro::GetX() const
{
  return x_;
}

float SoftMacro::GetY() const
{
  return y_;
}

float SoftMacro::GetPinX() const
{
  return x_ + width_ / 2.0;
}

float SoftMacro::GetPinY() const
{
  return y_ + height_ / 2.0;
}

const std::pair<float, float> SoftMacro::GetLocation() const 
{
  return std::pair<float, float>(x_, y_);
}

float SoftMacro::GetWidth() const
{
  return width_;   
}

float SoftMacro::GetHeight() const
{
  return height_;
}

float SoftMacro::GetArea() const
{
  return area_;
}

// Num Macros
bool SoftMacro::IsMacroCluster() const
{
  if (cluster_ == nullptr)
    return false;
  else
    return (cluster_->GetClusterType() == HardMacroCluster);
}

bool SoftMacro::IsStdCellCluster() const
{
  if (cluster_ == nullptr)
    return false;
  else
    return (cluster_->GetClusterType() == StdCellCluster);
}

int SoftMacro::GetNumMacro() const
{
  if (cluster_ == nullptr)
    return 0;
  else
    return cluster_->GetNumMacro();
}

void SoftMacro::ResizeRandomly(std::uniform_real_distribution<float>& distribution,
                               std::mt19937& generator) 
{
  if (width_list_.size() == 0)
    return;
  const int idx = static_cast<int>(std::floor(
                    (distribution)(generator) * width_list_.size()));
  const float min_width = width_list_[idx].first;
  const float max_width = width_list_[idx].second;
  width_ = min_width + (distribution)(generator) * (max_width - min_width);
  area_ = width_list_[idx].first * height_list_[idx].first;
  height_ = area_ / width_;
}

// Align Flag support
void SoftMacro::SetAlignFlag(bool flag) 
{
  align_flag_ = flag;
}

bool SoftMacro::GetAlignFlag() const
{
  return align_flag_;
}

// cluster
Cluster* SoftMacro::GetCluster() const
{
  return cluster_;
}

// Calculate macro utilization
float SoftMacro::GetMacroUtil() const 
{ 
  if (cluster_ == nullptr || area_ == 0.0)
    return 0.0;

  if (cluster_->GetClusterType() == HardMacroCluster)
    return 1.0;

  if (cluster_->GetClusterType() == MixedCluster)
    return cluster_->GetMacroArea() / area_;

  return 0.0;
}

bool SoftMacro::IsMixedCluster() const
{
  if (cluster_ == nullptr)
    return false;
  else
    return (cluster_->GetClusterType() == MixedCluster);
}

void SoftMacro::SetLocationF(float x, float y)
{
  x_ = x;
  y_ = y;
}

void SoftMacro::SetShapeF(float width, float height)
{
  width_ = width;
  height_ = height;
  area_ = width * height;
}

void SoftMacro::PrintShape()
{
  std::cout << "width_list : ";
  for (auto& width : width_list_)
    std::cout << " <" << width.first << " , " << width.second << " >  ";
  std::cout << std::endl;
  std::cout << "width_list : ";
  for (auto& width : height_list_)
    std::cout << " <" << width.first << " , " << width.second << " >  ";
  std::cout << std::endl;
}


}  // namespace mpl
