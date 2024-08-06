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

#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/odb.h"

namespace odb {
class Rect;
class Point;
class dbInst;
class dbModule;
class dbDatabase;
class dbITerm;
class dbTechLayer;
class dbBox;
class dbTrackGrid;
}  // namespace odb

namespace utl {
class Logger;
}

namespace mpl2 {
struct Rect;
class HardMacro;
class SoftMacro;

// ****************************************************************************
// This file includes the basic functions and basic classes for the HierRTLMP
//
//
// Explanation of logic module, cluster, hard macro, soft macro
// logic module : a module defined in the logical hierarchy tree
// cluster : the basic elements in the physical hierarchy tree.
//           When we map the logical hierarchy tree to physcial
//           hierarchy tree, we map one or more logical modules
//           to one cluster.
// hard macro : a macro used in the design, for example, a SRAM macro
//              We may generate some fake hard macros to model the
//              behaviors of pin access blockage
// soft macro : the physical abstraction for a cluster. We may inflate (bloat)
//              the area of the standard cells in the cluster when we
//              create soft macros from clusters. That means the bounding
//              box of a cluster is usually large than the actual size of the
//              cluster
//
//  Note in our framework, when we try to describe the postition of an object,
//  pos = (x, y) is the lower left corner of the object.
//
//
// Note in our framework, except the fence constraints, all the constraints
// (fixed position, preferred locations, and others) are on macros.  This means
// we do not accept pre-placed std cells as our inputs.
//*****************************************************************************

// Define the position of pin access blockage
// It can be {bottom, left, top, right} boundary of the cluster
// Each pin access blockage is modeled by a movable hard macro
// along the corresponding { B, L, T, R } boundary
// The size of the hard macro blockage is determined the by the
// size of that cluster
enum Boundary
{
  NONE,
  B,
  L,
  T,
  R
};

std::string toString(const Boundary& pin_access);
Boundary opposite(const Boundary& pin_access);

// Define the type for clusters
// StdCellCluster only has std cells. In the cluster type, it
// only has leaf_std_cells_ and dbModules_
// HardMacroCluster only has hard macros. In the cluster type,
// it only has leaf_macros_;
// MixedCluster has std cells and hard macros
enum ClusterType
{
  StdCellCluster,
  HardMacroCluster,
  MixedCluster
};

// Metrics class for logical modules and clusters
class Metrics
{
 public:
  Metrics() = default;
  Metrics(unsigned int num_std_cell,
          unsigned int num_macro,
          float std_cell_area,
          float macro_area);

  void addMetrics(const Metrics& metrics);
  const std::pair<unsigned int, unsigned int> getCountStats() const;
  const std::pair<float, float> getAreaStats() const;
  unsigned int getNumMacro() const;
  unsigned int getNumStdCell() const;
  float getStdCellArea() const;
  float getMacroArea() const;
  float getArea() const;

 private:
  // In the hierarchical autoclustering part,
  // we cluster the logical modules or clusters
  // based on num_std_cell and num_macro
  unsigned int num_std_cell_ = 0;
  unsigned int num_macro_ = 0;

  // In the macro placement part,
  // we need to know the sizes of clusters.
  // std_cell_area is the sum of areas of all std cells
  // macro_area is the sum of areas of all macros
  float std_cell_area_ = 0.0;
  float macro_area_ = 0.0;
};

// In this hierarchical autoclustering part,
// we convert the original gate-level netlist into a cluster-level netlist
class Cluster
{
 public:
  // constructors
  Cluster(int cluster_id, utl::Logger* logger);  // cluster name can be updated
  Cluster(int cluster_id, const std::string& cluster_name, utl::Logger* logger);

  // cluster id can not be changed
  int getId() const;
  // cluster name can be updated
  const std::string getName() const;
  void setName(const std::string& name);
  // cluster type (default type = MixedCluster)
  void setClusterType(const ClusterType& cluster_type);
  const ClusterType getClusterType() const;
  std::string getClusterTypeString() const;

  // Instances (Here we store dbModule to reduce memory)
  void addDbModule(odb::dbModule* db_module);
  void addLeafStdCell(odb::dbInst* leaf_std_cell);
  void addLeafMacro(odb::dbInst* leaf_macro);
  void addLeafInst(odb::dbInst* inst);
  void specifyHardMacros(std::vector<HardMacro*>& hard_macros);
  const std::vector<odb::dbModule*> getDbModules() const;
  const std::vector<odb::dbInst*> getLeafStdCells() const;
  const std::vector<odb::dbInst*> getLeafMacros() const;
  std::vector<HardMacro*> getHardMacros() const;
  void clearDbModules();
  void clearLeafStdCells();
  void clearLeafMacros();
  void clearHardMacros();
  void copyInstances(const Cluster& cluster);  // only based on cluster type

  // Position must be specified when setting an IO cluster
  void setAsIOCluster(const std::pair<float, float>& pos,
                      float width,
                      float height);
  bool isIOCluster() const;
  void setAsArrayOfInterconnectedMacros();
  bool isArrayOfInterconnectedMacros() const;
  bool isEmpty() const;
  bool correspondsToLogicalModule() const;

  // Metrics Support
  void setMetrics(const Metrics& metrics);
  const Metrics getMetrics() const;
  int getNumStdCell() const;
  int getNumMacro() const;
  float getArea() const;
  float getMacroArea() const;
  float getStdCellArea() const;

  // Physical location support
  float getWidth() const;
  float getHeight() const;
  float getX() const;
  float getY() const;
  void setX(float x);
  void setY(float y);
  const std::pair<float, float> getLocation() const;
  Rect getBBox() const;

  // Hierarchy Support
  void setParent(Cluster* parent);
  void addChild(Cluster* child);
  void removeChild(const Cluster* child);
  void addChildren(const std::vector<Cluster*>& children);
  void removeChildren();
  Cluster* getParent() const;
  std::vector<Cluster*> getChildren() const;

  bool isLeaf() const;  // if the cluster is a leaf cluster
  std::string getIsLeafString() const;
  bool mergeCluster(Cluster& cluster,
                    bool& delete_flag);  // return true if succeed

  // Connection signature support
  void initConnection();
  void addConnection(int cluster_id, float weight);
  const std::map<int, float> getConnection() const;
  bool isSameConnSignature(const Cluster& cluster, float net_threshold);
  bool hasMacroConnectionWith(const Cluster& cluster, float net_threshold);
  // Get closely-connected cluster if such cluster exists
  // For example, if a small cluster A is closely connected to a
  // well-formed cluster B, (there are also other well-formed clusters
  // C, D), A is only connected to B and A has no connection with C, D
  int getCloseCluster(const std::vector<int>& candidate_clusters,
                      float net_threshold);

  // Path synthesis support
  // After path synthesis, the children cluster of current cluster will
  // not have any connections outsize the parent cluster
  // All the outside connections have been converted to the connections
  // related to pin access
  void setPinAccess(int cluster_id, Boundary pin_access, float weight);
  void addBoundaryConnection(Boundary pin_a, Boundary pin_b, float num_net);
  const std::pair<Boundary, float> getPinAccess(int cluster_id);
  const std::map<int, std::pair<Boundary, float>> getPinAccessMap() const;
  const std::map<Boundary, std::map<Boundary, float>> getBoundaryConnection()
      const;

  // virtual connections
  const std::vector<std::pair<int, int>> getVirtualConnections() const;
  void addVirtualConnection(int src, int target);

  // Print Basic Information
  void printBasicInformation(utl::Logger* logger) const;

  // Macro Placement Support
  void setSoftMacro(SoftMacro* soft_macro);
  SoftMacro* getSoftMacro() const;

  void setMacroTilings(const std::vector<std::pair<float, float>>& tilings);
  const std::vector<std::pair<float, float>> getMacroTilings() const;

 private:
  // Private Variables
  int id_ = -1;  // cluster id (a valid cluster id should be nonnegative)
  std::string name_ = "";            // cluster name
  ClusterType type_ = MixedCluster;  // cluster type

  // Instances in the cluster
  // the logical module included in the cluster
  // dbModule is a object representing logical module in the OpenDB
  std::vector<odb::dbModule*> db_modules_;
  // the std cell instances in the cluster (leaf std cell instances)
  std::vector<odb::dbInst*> leaf_std_cells_;
  // the macros in the cluster (leaf macros)
  std::vector<odb::dbInst*> leaf_macros_;
  // all the macros in the cluster
  std::vector<HardMacro*> hard_macros_;

  // We model bundled IOS (Pads) as a cluster with no area
  // The position be the center of IOs
  bool is_io_cluster_ = false;
  bool is_array_of_interconnected_macros = false;

  // Each cluster uses metrics to store its statistics
  Metrics metrics_;

  // Each cluster cooresponding to a SoftMacro in the placement engine
  // which will concludes the information about real pos, width, height, area
  std::unique_ptr<SoftMacro> soft_macro_;

  // Each cluster is a node in the physical hierarchy tree
  // Thus we need to define related to parent and children pointers
  Cluster* parent_ = nullptr;       // parent of current cluster
  std::vector<Cluster*> children_;  // children of current cluster

  // macro tilings for hard macros
  std::vector<std::pair<float, float>> macro_tilings_;  // <width, height>

  // To support grouping small clusters based connection signature,
  // we define connection_map_
  // Here we do not differentiate the input and output connections
  std::map<int, float> connection_map_;  // cluster_id, number of connections

  // store the virtual connection between children
  // the virtual connection is used to tie the std cell part and the
  // corresponding macro part together
  std::vector<std::pair<int, int>> virtual_connections_;

  // pin access for each bundled connection
  std::map<int, std::pair<Boundary, float>> pin_access_map_;
  std::map<Boundary, std::map<Boundary, float>> boundary_connection_map_;
  utl::Logger* logger_;
};

// A hard macro have fixed width and height
// User can specify a halo width for each macro
// We specify the position of macros in terms (x, y, width, height)
// Except for the fake hard macros (pin access blockage or other blockage),
// each HardMacro cooresponds to one macro
class HardMacro
{
 public:
  // Create a macro with specified size
  // Model fixed terminals
  HardMacro(std::pair<float, float> loc, const std::string& name);

  // In this case, we model the pin position at the center of the macro
  HardMacro(float width, float height, const std::string& name);

  // create a macro from dbInst
  HardMacro(odb::dbInst* inst, float halo_width = 0.0, float halo_height = 0.0);

  // overload the comparison operators
  // based on area, width, height order
  bool operator<(const HardMacro& macro) const;
  bool operator==(const HardMacro& macro) const;

  // Get Physical Information
  // Note that the default X and Y include halo_width
  void setLocation(const std::pair<float, float>& location);
  void setX(float x);
  void setY(float y);
  const std::pair<float, float> getLocation() const;
  float getX() const { return x_; }
  float getY() const { return y_; }
  // The position of pins relative to the lower left of the instance
  float getPinX() const { return x_ + pin_x_; }
  float getPinY() const { return y_ + pin_y_; }
  // The position of pins relative to the origin of the canvas;
  float getAbsPinX() const { return pin_x_; }
  float getAbsPinY() const { return pin_y_; }
  // width, height (include halo_width)
  float getWidth() const { return width_; }
  float getHeight() const { return height_; }

  // Note that the real X and Y does NOT include halo_width
  void setRealLocation(const std::pair<float, float>& location);
  void setRealX(float x);
  void setRealY(float y);
  const std::pair<float, float> getRealLocation() const;
  float getRealX() const;
  float getRealY() const;
  float getRealWidth() const;
  float getRealHeight() const;

  // Orientation support
  odb::dbOrientType getOrientation() const;
  // We do not allow rotation of macros
  // This may violate the direction of metal layers
  // flip about X or Y axis
  void flip(bool flip_horizontal);

  // Interfaces with OpenDB
  odb::dbInst* getInst() const;
  const std::string getName() const;
  const std::string getMasterName() const;

  int getXDBU() const { return block_->micronsToDbu(getX()); }

  int getYDBU() const { return block_->micronsToDbu(getY()); }

  int getRealXDBU() const { return block_->micronsToDbu(getRealX()); }

  int getRealYDBU() const { return block_->micronsToDbu(getRealY()); }

  int getWidthDBU() const { return block_->micronsToDbu(getWidth()); }

  int getHeightDBU() const { return block_->micronsToDbu(getHeight()); }

  int getRealWidthDBU() const { return block_->micronsToDbu(getRealWidth()); }

  int getRealHeightDBU() const { return block_->micronsToDbu(getRealHeight()); }

  int getUXDBU() const { return getXDBU() + getWidthDBU(); }

  int getUYDBU() const { return getYDBU() + getHeightDBU(); }

  int getRealUXDBU() const { return getRealXDBU() + getRealWidthDBU(); }

  int getRealUYDBU() const { return getRealYDBU() + getRealHeightDBU(); }

  void setXDBU(int x) { setX(block_->dbuToMicrons(x)); }

  void setYDBU(int y) { setY(block_->dbuToMicrons(y)); }

 private:
  // We define x_, y_ and orientation_ here
  // to avoid keep updating OpenDB during simulated annealing
  // Also enable the multi-threading
  float x_ = 0.0;            // lower left corner
  float y_ = 0.0;            // lower left corner
  float halo_width_ = 0.0;   // halo width
  float halo_height_ = 0.0;  // halo height
  float width_ = 0.0;        // width_ = macro_width + 2 * halo_width
  float height_ = 0.0;       // height_ = macro_height + 2 * halo_width
  std::string name_ = "";    // macro name
  odb::dbOrientType orientation_ = odb::dbOrientType::R0;

  // we assume all the pins locate at the center of all pins
  // related to the lower left corner
  float pin_x_ = 0.0;
  float pin_y_ = 0.0;

  odb::dbInst* inst_ = nullptr;
  odb::dbBlock* block_ = nullptr;
};

// We have three types of SoftMacros
// Type 1:  a SoftMacro corresponding to a Cluster (MixedCluster,
// StdCellCluster, HardMacroCluster)
// Type 2:  a SoftMacro corresponding to a IO cluster
// Type 3:  a SoftMacro corresponding to a all kinds of blockages
// Here (x, y) is the lower left corner of the soft macro
// For all the soft macros, we model the bundled pin at the center
// of the soft macro. So we do not need private variables for pin positions
// SoftMacro is a physical abstraction for Cluster.
// Note that constrast to classical soft macro definition,
// we allow the soft macro to change its area.
// For the SoftMacro corresponding to different types of clusters,
// we allow different shape constraints:
// For SoftMacro corresponding to MixedCluster and StdCellCluster,
// the macro must have fixed area
// For SoftMacro corresponding to HardMacroCluster,
// the macro can have different sizes. In this case, the width_list and
// height_list is not sorted. Generally speaking we can have following types of
// SoftMacro (1) SoftMacro : MixedCluster (2) SoftMacro : StdCellCluster (3)
// SoftMacro : HardMacroCluster (4) SoftMacro : Fixed Hard Macro (or blockage)
// (5) SoftMacro : Hard Macro (or pin access blockage)
// (6) SoftMacro : Fixed Terminals

class SoftMacro
{
 public:
  // Create a SoftMacro with specified size
  // Create a SoftMacro representing the blockage
  SoftMacro(float width, float height, const std::string& name);
  // Create a SoftMacro representing fixed hard macro or blockage
  SoftMacro(float width,
            float height,
            const std::string& name,
            float lx,
            float ly);
  // Create a SoftMacro representing the IO cluster
  SoftMacro(const std::pair<float, float>& pos,
            const std::string& name,
            float width = 0.0,
            float height = 0.0,
            Cluster* cluster = nullptr);

  // create a SoftMacro from a cluster
  SoftMacro(Cluster* cluster);

  // name
  const std::string getName() const;

  void setX(float x);
  void setY(float y);
  void setLocation(const std::pair<float, float>& location);
  void setWidth(float width);      // only for StdCellCluster and MixedCluster
  void setHeight(float height);    // only for StdCellCluster and MixedCluster
  void shrinkArea(float percent);  // only for StdCellCluster
  void setArea(float area);        // only for StdCellCluster and MixedCluster
  void resizeRandomly(std::uniform_real_distribution<float>& distribution,
                      std::mt19937& generator);
  // This function for discrete shape curves, HardMacroCluster
  // If force_flag_ = true, it will force the update of width_list_ and
  // height_list_
  void setShapes(const std::vector<std::pair<float, float>>& shapes,
                 bool force_flag = false);  // < <width, height>
  // This function for specify shape curves (piecewise function),
  // for StdCellCluster and MixedCluster
  void setShapes(const std::vector<std::pair<float, float>>& width_list,
                 float area);
  float getX() const { return x_; }
  float getY() const { return y_; }

  // The position of pins relative to the lower left of the instance
  float getPinX() const { return x_ + 0.5f * width_; }
  float getPinY() const { return y_ + 0.5f * height_; }

  std::pair<float, float> getLocation() const
  {
    return std::pair<float, float>(x_, y_);
  }
  float getWidth() const { return width_; }
  float getHeight() const { return height_; }
  float getArea() const;
  Rect getBBox() const;
  // Num Macros
  bool isMacroCluster() const;
  bool isStdCellCluster() const;
  bool isMixedCluster() const;
  void setLocationF(float x, float y);
  void setShapeF(float width, float height);
  int getNumMacro() const;
  // Align Flag support
  void setAlignFlag(bool flag);
  bool getAlignFlag() const;
  // cluster
  Cluster* getCluster() const;
  // calculate macro utilization
  float getMacroUtil() const;

 private:
  // utility function
  int findPos(std::vector<std::pair<float, float>>& list,
              float& value,
              bool increase_order);

  // We define x_, y_ and orientation_ here
  // Also enable the multi-threading
  float x_ = 0.0;          // lower left corner
  float y_ = 0.0;          // lower left corner
  float width_ = 0.0;      // width_
  float height_ = 0.0;     // height_
  float area_ = 0.0;       // area of the standard cell cluster
  std::string name_ = "";  // macro name
  // variables to describe shape curves (discrete or piecewise curves)
  std::vector<std::pair<float, float>> width_list_;   // nondecreasing order
  std::vector<std::pair<float, float>> height_list_;  // nonincreasing order

  // Interfaces with hard macro
  Cluster* cluster_ = nullptr;
  bool fixed_ = false;  // if the macro is fixed

  // Alignment support
  // if the cluster has been aligned related to other macro_cluster or
  // boundaries
  bool align_flag_ = false;
};

// In our netlist model, we only have two-pin nets
struct BundledNet
{
  BundledNet(int src, int target, float weight)
  {
    this->terminals = std::pair<int, int>(src, target);
    this->weight = weight;
    hpwl = 0;
  }

  BundledNet(const std::pair<int, int>& terminals, float weight)
  {
    this->terminals = terminals;
    this->weight = weight;
    hpwl = 0;
  }

  bool operator==(const BundledNet& net)
  {
    return (terminals.first == net.terminals.first)
           && (terminals.second == net.terminals.second);
  }

  // id for terminals, the id can be the id of hard macro or soft
  // macro
  std::pair<int, int> terminals;
  // Number of bundled connections (can be timing-related weight)
  float weight;
  // support for bus synthsis
  float hpwl;  // HPWL of the Net (in terms of path length)
  // shortest paths:  to minimize timing
  // store all the shortest paths between two soft macros
  std::vector<std::vector<int>> edge_paths;
  // store all the shortest paths between two soft macros in terms of
  // boundary edges.  All the internal edges are removed
  std::vector<std::vector<int>> boundary_edge_paths;

  // In our framework, we only bundled connections between clusters.
  // Thus each net must have both src_cluster_id and target_cluster_id
  int src_cluster_id = -1;
  int target_cluster_id = -1;
};

// Here we redefine the Rect class
// odb::Rect use database unit
// Rect class use float type for Micron unit
struct Rect
{
  Rect() = default;
  Rect(const float lx,
       const float ly,
       const float ux,
       const float uy,
       bool fixed_flag = false)
      : lx(lx), ly(ly), ux(ux), uy(uy), fixed_flag(fixed_flag)
  {
  }

  float xMin() const { return lx; }
  float yMin() const { return ly; }
  float xMax() const { return ux; }
  float yMax() const { return uy; }

  float getX() const { return (lx + ux) / 2.0; }

  float getY() const { return (ly + uy) / 2.0; }

  float getWidth() const { return ux - lx; }

  float getHeight() const { return uy - ly; }

  void setLoc(float x,
              float y,
              float core_lx,
              float core_ly,
              float core_ux,
              float core_uy)
  {
    if (fixed_flag == true) {
      return;
    }

    const float width = ux - lx;
    const float height = uy - ly;
    lx = x - width / 2.0;
    ly = y - height / 2.0;
    ux = x + width / 2.0;
    uy = y + height / 2.0;
    if (lx < core_lx) {
      lx = core_lx;
      ux = lx + width;
    }

    if (ly < core_ly) {
      ly = core_ly;
      uy = ly + height;
    }

    if (ux > core_ux) {
      lx = core_ux - width;
      ux = core_ux;
    }

    if (uy > core_uy) {
      ly = core_uy - height;
      uy = core_uy;
    }
  }

  void moveHor(float dist)
  {
    lx = lx + dist;
    ux = ux + dist;
  }

  void moveVer(float dist)
  {
    ly = ly + dist;
    uy = uy + dist;
  }

  void move(float x_dist,
            float y_dist,
            float core_lx,
            float core_ly,
            float core_ux,
            float core_uy)
  {
    if (fixed_flag == true) {
      return;
    }
    moveHor(x_dist);
    moveVer(y_dist);
    const float width = getWidth();
    const float height = getHeight();
    if (lx < core_lx) {
      lx = core_lx;
      ux = lx + width;
    }

    if (ly < core_ly) {
      ly = core_ly;
      uy = ly + height;
    }

    if (ux > core_ux) {
      lx = core_ux - width;
      ux = core_ux;
    }

    if (uy > core_uy) {
      ly = core_uy - height;
      uy = core_uy;
    }

    if (lx < core_lx - 1.0 || ly < core_ly - 1.0 || ux > core_ux + 1.0
        || uy > core_uy + 1.0) {
      std::cout << "Error !!!\n"
                << "core_lx =  " << core_lx << "  "
                << "core_ly =  " << core_ly << "  "
                << "core_ux =  " << core_ux << "  "
                << "core_uy =  " << core_uy << std::endl;
    }
  }

  void resetForce()
  {
    f_x_a = 0.0;
    f_y_a = 0.0;
    f_x_r = 0.0;
    f_y_r = 0.0;
    f_x = 0.0;
    f_y = 0.0;
  }

  void makeSquare(float ar = 1.0)
  {
    if (fixed_flag == true) {
      return;
    }
    const float x = getX();
    const float y = getY();
    const float height = std::sqrt(getWidth() * getHeight() * ar);
    const float width = getWidth() * getHeight() / height;
    lx = x - width / 2.0;
    ly = y - height / 2.0;
    ux = x + width / 2.0;
    uy = y + height / 2.0;
  }

  void addAttractiveForce(float f_x, float f_y)
  {
    f_x_a += f_x;
    f_y_a += f_y;
  }

  void addRepulsiveForce(float f_x, float f_y)
  {
    f_x_r += f_x;
    f_y_r += f_y;
  }

  void setForce(float f_x_, float f_y_)
  {
    f_x = f_x_;
    f_y = f_y_;
  }

  bool isValid() const
  {
    return (lx > 0.0) && (ly > 0.0) && (ux > 0.0) && (uy > 0.0);
  }

  void merge(const Rect& rect)
  {
    if (!isValid()) {
      return;
    }

    lx = std::min(lx, rect.lx);
    ly = std::min(ly, rect.ly);
    ux = std::max(ux, rect.ux);
    uy = std::max(uy, rect.uy);
  }

  void relocate(float outline_lx,
                float outline_ly,
                float outline_ux,
                float outline_uy)
  {
    if (!isValid()) {
      return;
    }

    lx = std::max(lx, outline_lx);
    ly = std::max(ly, outline_ly);
    ux = std::min(ux, outline_ux);
    uy = std::min(uy, outline_uy);
    lx -= outline_lx;
    ly -= outline_ly;
    ux -= outline_lx;
    uy -= outline_ly;
  }

  float lx = 0.0;
  float ly = 0.0;
  float ux = 0.0;
  float uy = 0.0;

  // for force-directed placement
  // attractive force
  float f_x_a = 0.0;
  float f_y_a = 0.0;

  // repulsive force
  float f_x_r = 0.0;
  float f_y_r = 0.0;

  // total force
  float f_x = 0.0;
  float f_y = 0.0;

  bool fixed_flag = false;
};

struct SequencePair
{
  std::vector<int> pos_sequence;
  std::vector<int> neg_sequence;
};

}  // namespace mpl2
