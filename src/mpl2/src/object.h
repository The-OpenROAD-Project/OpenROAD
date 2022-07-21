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
#include <random>


#include "db_sta/dbReadVerilog.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "sta/Bfs.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/Sta.hh"
#include "utl/Logger.h"


namespace mpl {

// *********************************************************************************
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
// soft macro : the physical abstraction for a cluster. We may inflat
//              the area of the standard cells in the cluster when we
//              create soft macros from clusters. That means the bounding
//              box of a cluster is usually large than the actual size of the
//              cluster
//
//  Note in our framework, when we try to describe the postition of an object,
//  pos = (x, y) is the lower corner of the object.
//
//
// Note in our framework, except the fence constraints, all the constraints (fixed position, 
// preferred locations, and others) are on macros.  This means we do not accept pre-placed std
// cells as our inputs.
//*********************************************************************************

// Basic utility functions

// converion between dbu and micro
float Dbu2Micro(int metric, float dbu);
int Micro2Dbu(float metric, float dbu);

// Sort shapes
bool SortShape(const std::pair<float, float>& shape1, 
               const std::pair<float, float>& shape2);

bool ComparePairFirst(std::pair<float, float> p1, std::pair<float, float> p2);
bool ComparePairProduct(std::pair<float, float> p1, std::pair<float, float> p2);


// Define the position of pin access blockage
// It can be {bottom, left, top, right} boundary of the cluster
// Each pin access blockage is modeled by a movable hard macro
// along the corresponding { B, L, T, R } boundary
// The size of the hard macro blockage is determined the by the
// size of that cluster
enum PinAccess { NONE,  B, L, T, R };
std::string to_string(const PinAccess& pin_access);
PinAccess Opposite(const PinAccess& pin_access);

class Metric;
class HardMacro;
class SoftMacro;
class Cluster;


// Define the type for clusters
// StdCellCluster only has std cells. In the cluster type, it
// only has leaf_std_cells_ and dbModules_
// HardMacroCluster only has hard macros. In the cluster type,
// it only has leaf_macros_;
// MixedCluster has std cells and hard macros
enum ClusterType { StdCellCluster, HardMacroCluster, MixedCluster};

// Metric class for logical modules and clusters
class Metric {
  public:
    // default constructor
    Metric() {  }
    Metric(unsigned int num_std_cell, 
           unsigned int num_macro,
           float std_cell_area,
           float macro_area);

    void AddMetric(const Metric& metric);
    void InflatStdCellArea(float std_cell_util);
    void InflatMacroArea(float inflat_macro_area);
    const std::pair<unsigned int, unsigned int> GetCountStats() const;
    const std::pair<float, float> GetAreaStats() const;
    const std::pair<float, float> GetInflatAreaStats() const;
    unsigned int GetNumMacro() const;
    unsigned int GetNumStdCell() const;
    float GetStdCellArea() const;
    float GetMacroArea() const;
    float GetArea() const;
    float GetInflatStdCellArea() const;
    float GetInflatMacroArea() const;
    float GetInflatArea() const;

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
    // inflat_std_cell_area = std_cell_area / util
    // inflat_macro_area considers the halo width when calculate
    // each macro area
    float std_cell_area_ = 0.0;
    float macro_area_ = 0.0;
    float inflat_std_cell_area_ = 0.0;
    float inflat_macro_area_ = 0.0;
};



// In this hierarchical autoclustering part,
// we convert the original gate-level netlist into a cluster-level netlist
class Cluster {
  public:
    // constructors
    Cluster() {  }
    Cluster(int cluster_id); // cluster name can be updated
    Cluster(int cluster_id, std::string cluster_name);
    // destructor
    ~Cluster();  // we need to define this for remove pointers

    // cluster id can not be changed
    int GetId() const;
    // cluster name can be updated 
    const std::string GetName() const;
    void SetName(const std::string name);
    // cluster type (default type = MixedCluster)
    void SetClusterType(const ClusterType&  cluster_type);
    const ClusterType GetClusterType() const;
    
    // Instances (Here we store dbModule to reduce memory)
    void AddDbModule(odb::dbModule* dbModule);
    void AddLeafStdCell(odb::dbInst* leaf_std_cell);
    void AddLeafMacro(odb::dbInst* leaf_macro);
    void SpecifyHardMacros(std::vector<HardMacro*>& hard_macros);
    const std::vector<odb::dbModule*> GetDbModules() const;
    const std::vector<odb::dbInst*> GetLeafStdCells() const;
    const std::vector<odb::dbInst*> GetLeafMacros() const;
    const std::vector<HardMacro*> GetHardMacros() const;
    void ClearDbModules();
    void ClearLeafStdCells();
    void ClearLeafMacros();
    void ClearHardMacros();
    void CopyInstances(const Cluster& cluster); // only based on cluster type
    
    // IO cluster
    // When you specify the io cluster, you must specify the postion
    // of this IO cluster
    void SetIOClusterFlag(const std::pair<float, float> pos, const float width, 
                          const float height);
    bool GetIOClusterFlag() const;

    // Metric Support
    void SetMetric(const Metric& metric);
    const Metric GetMetric() const;
    int GetNumStdCell() const;
    int GetNumMacro() const;
    float GetArea()  const;
    float GetMacroArea() const;
    float GetStdCellArea() const;

    // Physical location support
    float GetWidth() const;
    float GetHeight() const;
    float GetX() const;
    float GetY() const;
    void SetX(float x);
    void SetY(float y);
    const std::pair<float, float> GetLocation() const;

    // Hierarchy Support
    void SetParent(Cluster* parent);
    void AddChild(Cluster* child);
    void RemoveChild(const Cluster* child);
    void AddChildren(const std::vector<Cluster*>& children);
    void RemoveChildren();
    Cluster* GetParent() const;
    std::set<Cluster*> GetChildren() const;
    
    bool IsLeaf() const;  // if the cluster is a leaf cluster
    bool MergeCluster(const Cluster& cluster); // return true if succeed

    // Connection signature support
    void InitConnection();
    void AddConnection(int cluster_id, float weight);
    const std::map<int, float> GetConnection() const;
    bool IsSameConnSignature(const Cluster& cluster, float net_threshold);
    // Get closely-connected cluster if such cluster exists
    // For example, if a small cluster A is closely connected to a
    // well-formed cluster B, (there are also other well-formed clusters
    // C, D), A is only connected to B and A has no connection with C, D
    int GetCloseCluster(const std::vector<int>& candidate_clusters, 
                        float net_threshold);

    // Path synthesis support
    // After path synthesis, the children cluster of current cluster will
    // not have any connections outsize the parent cluster
    // All the outside connections have been converted to the connections
    // related to pin access
    void SetPinAccess(PinAccess pin_access, float weight);
    void AddBoundaryConnection(PinAccess pin_a, PinAccess pin_b, float num_net);
    const std::map<PinAccess, float> GetPinAccessMap() const;
    const std::map<PinAccess, std::map<PinAccess, float> > GetBoundaryConnection() const;

    // Print Basic Information
    void PrintBasicInformation(utl::Logger* logger) const;

    // Macro Placement Support 
    void SetSoftMacro(SoftMacro* soft_macro);
    SoftMacro* GetSoftMacro() const;
    
    void SetMacroTilings(const std::vector<std::pair<float, float> >& tilings);
    const std::vector<std::pair<float, float> > GetMacroTilings() const;
  
  private:
    // Private Variables
    int id_ = -1;  // cluster id (a valid cluster id should be nonnegative)
    std::string name_ = "";  // cluster name
    ClusterType type_ = MixedCluster; // cluster type
   
    // Instances in the cluster
    // the logical module included in the cluster 
    // dbModule is a object representing logical module in the OpenDB
    std::vector<odb::dbModule*> dbModules_; 
    // the std cell instances in the cluster (leaf std cell instances)
    std::vector<odb::dbInst*>   leaf_std_cells_;   
    // the macros in the cluster (leaf macros)
    std::vector<odb::dbInst*>   leaf_macros_;  
    // all the macros in the cluster
    std::vector<HardMacro*>   hard_macros_;  

    // We model bundled IOS (Pads) as a cluster with no area
    // The position be the center of IOs
    bool io_cluster_flag_ = false;

    // Each cluster uses metric to store its statistics
    Metric metric_;

    // Each cluster cooresponding to a SoftMacro in the placement engine
    // which will concludes the information about real pos, width, height, area
    SoftMacro* soft_macro_ = nullptr;

    // Each cluster is a node in the physical hierarchy tree
    // Thus we need to define related to parent and children pointers
    Cluster* parent_ = nullptr;  // parent of current cluster
    std::set<Cluster*> children_;  // children of current cluster

    // macro tilings for hard macros 
    std::vector<std::pair<float, float> > macro_tilings_; // <width, height>

    // To support grouping small clusters based connection signature, 
    // we define connection_map_
    // Here we do not differentiate the input and output connections
    std::map<int, float> connection_map_;   // cluster_id, number of connections 

    // pin access for each bundled connection
    std::map<PinAccess, float> pin_access_map_; // pin_access (B, L, T, R)
    std::map<PinAccess, std::map<PinAccess, float> > boundary_connection_map_;
};


// A hard macro have fixed width and height
// User can specify a halo width for each macro
// We specify the position of macros in terms (x, y, width, height)
// Here (x, y) is the lower left corner of the macro
class HardMacro {
  public:
    HardMacro() {  }
    // Create a macro with specified size
    // In this case, we model the pin position at the center of the macro
    HardMacro(float width, float height, const std::string name);
    // create a macro from dbInst
    // dbu is needed to convert the database unit to real size
    HardMacro(odb::dbInst* inst, float dbu, float halo_width = 0.0);
 
    // overload the comparison operators
    // based on area, width, height order
    bool operator<(const HardMacro& macro) const;
    bool operator==(const HardMacro& macro) const;

    // Get Physical Information
    // Note that the default X and Y include halo_width
    void SetLocation(const std::pair<float, float>& location);
    void SetX(float x);
    void SetY(float y);
    const std::pair<float, float> GetLocation() const;
    float GetX() const;
    float GetY() const;
    // The position of pins relative to the lower left of the instance
    float GetPinX() const;
    float GetPinY() const;
    // The position of pins relative to the origin of the canvas;
    float GetAbsPinX() const;
    float GetAbsPinY() const;
    // width, height (include halo_width)
    float GetWidth() const;
    float GetHeight() const;

    // Note that the real X and Y does NOT include halo_width
    void SetRealLocation(const std::pair<float, float>&  location);
    void SetRealX(float x);
    void SetRealY(float y);
    const std::pair<float, float> GetRealLocation() const;
    float GetRealX() const;
    float GetRealY() const;
    float GetRealWidth() const;
    float GetRealHeight() const;

    // Orientation support
    std::string GetOrientation() const;
    // We do not allow rotation of macros
    // This may violate the direction of metal layers
    // axis = true, flip horizontally
    // axis = false, flip vertically
    void Flip(bool axis); 

    // Interfaces with OpenDB
    odb::dbInst* GetInst() const;
    const std::string GetName() const;
    const std::string GetMasterName() const;
    // update the location and orientation of the macro inst in OpenDB
    // The macro should be snaped to placement grids
    void UpdateDb(float pitch_x, float pitch_y);
    
  private:
    // We define x_, y_ and orientation_ here
    // to avoid keep updating OpenDB during simulated annealing
    // Also enable the multi-threading
    float x_ = 0.0;  // lower left corner
    float y_ = 0.0;  // lower left corner
    float halo_width_ = 0.0; // halo width
    float width_ = 0.0;  // width_ = macro_width + 2 * halo_width
    float height_ = 0.0; // height_ = macro_height + 2 * halo_width
    std::string name_ = ""; // macro name
    odb::dbOrientType orientation_ = odb::dbOrientType::R0;

    // we assume all the pins locate at the center of all pins
    // related to the lower left corner
    float pin_x_ = 0.0; 
    float pin_y_ = 0.0; 
    

    // Interface for OpenDB
    // Except for the fake hard macros (pin access blockage or other blockage),
    // each HardMacro cooresponds to one macro
    odb::dbInst* inst_ = nullptr;
    float dbu_  = 0.0;  // DbuPerMicro
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
// For the SoftMacro cooresponding to different types of clusters,
// we allow different shape constraints:
// For SoftMacro corresponding to MixedCluster and StdCellCluster,
// the macro must have fixed area
// For SoftMacro corresponding to HardMacroCluster,
// the macro can have different sizes. In this case, the width_list and height_list is
// not sorted.
// Generally speaking we can have following types of SoftMacro
// (1) SoftMacro : MixedCluster
// (2) SoftMacro : StdCellCluster
// (3) SoftMacro : HardMacroCluster
// (4) SoftMacro : Fixed Hard Macro (or blockage)
// (5) SoftMacro : Hard Macro (or pin access blockage)
// (6) SoftMacro : Fixed Terminals

class SoftMacro {
  public:
    SoftMacro() {  }
    // Create a SoftMacro with specified size
    // Create a SoftMacro representing the blockage
    SoftMacro(float width, float height, const std::string name);
    // Create a SoftMacro representing fixed hard macro or blockage
    SoftMacro(float width, float height, const std::string name, float lx, float ly);
    // Create a SoftMacro representing the IO cluster
    SoftMacro(const std::pair<float, float>& pos, const std::string name, 
              float width = 0.0, float height = 0.0);
    // create a SoftMacro from a cluster
    SoftMacro(Cluster* cluster);

    // name
    const std::string GetName() const;
    // Physical Information
    void SetX(float x);
    void SetY(float y);
    void SetLocation(const std::pair<float, float>& location);
    void SetWidth(float width); // only for StdCellCluster and MixedCluster
    void SetHeight(float height); // only for StdCellCluster and MixedCluster
    void SetArea(float area); // only for StdCellCluster and MixedCluster
    void ResizeRandomly(std::uniform_real_distribution<float>& distribution,
                        std::mt19937& generator);
    // This function for discrete shape curves, HardMacroCluster
    // If force_flag_ = true, it will force the update of width_list_ and height_list_
    void SetShapes(const std::vector<std::pair<float, float> >& shapes, bool force_flag = false); // < <width, height> 
    // This function for specify shape curves (piecewise function), 
    // for StdCellCluster and MixedCluster
    void SetShapes(const std::vector<std::pair<float, float> >& width_list, float area);
    float GetX() const;
    float GetY() const;
    float GetPinX() const;
    float GetPinY() const;
    const std::pair<float, float> GetLocation() const;
    float GetWidth() const;
    float GetHeight() const;
    float GetArea() const;
    // Num Macros
    bool IsMacroCluster() const;
    int GetNumMacro() const;
    // Align Flag support
    void SetAlignFlag(bool flag);
    bool GetAlignFlag() const;
    // cluster
    Cluster* GetCluster() const;
    // calculate macro utilization
    float GetMacroUtil() const;

  private:
    // We define x_, y_ and orientation_ here
    // Also enable the multi-threading
    float x_ = 0.0;  // lower left corner
    float y_ = 0.0;  // lower left corner
    float width_ = 0.0;  // width_ 
    float height_ = 0.0; // height_
    float area_ = 0.0; // area of the standard cell cluster
    std::string name_ = ""; // macro name
    // variables to describe shape curves (discrete or piecewise curves)
    std::vector<std::pair<float, float> > width_list_;  // nondecreasing order
    std::vector<std::pair<float, float> > height_list_; // nonincreasing order

    // Interfaces with hard macro
    Cluster* cluster_ = nullptr;
    bool fixed_ = false; // if the macro is fixed

    // Alignment support
    // if the cluster has been aligned related to other macro_cluster or boundaries
    bool align_flag_ = false;
    
    // utility function
    int FindPos(std::vector<std::pair<float, float> >& list,
                float& value,  bool increase_order);
};




// In our netlist model, we only have two-pin nets
struct BundledNet {
  std::pair<int, int> terminals; // id for terminals
                                 // here the id can be the id of hard macro or soft macro
  float weight;                  // Number of bundled connections (can be timing-related 
                                 // weight)
  // support for bus synthsis
  float HPWL;    // HPWL of the Net (in terms of path length)
  // shortest paths:  to minimize timing
  // store all the shortest paths between two soft macros
  std::vector<std::vector<int> > edge_paths;   
  // store all the shortest paths between two soft macros in terms of
  // boundary edges.  All the internal edges are removed
  std::vector<std::vector<int> > boundary_edge_paths; 

  BundledNet() {  }
  BundledNet(int src, int target, float weight)
  {
    terminals = std::pair<int, int>(src, target);
    weight = weight;
  }
  
  BundledNet(const std::pair<int, int>& terminals, float weight) 
  {
    this->terminals = terminals;
    this->weight = weight;
  }

  bool operator==(const BundledNet& net) {
    return (terminals.first == net.terminals.first) &&
           (terminals.second == net.terminals.second);
  }


};

// Here we redefine the Rect class 
// odb::Rect use database unit
// Rect class use float type for Micro unit
struct Rect {
  float lx = 0.0;
  float ly = 0.0;
  float ux = 0.0;
  float uy = 0.0;

  Rect() {   }
  Rect(const float lx, const float ly, const float ux, const float uy) 
    : lx(lx), ly(ly), ux(ux), uy(uy) {   }

  float xMin() const { return lx; }
  float yMin() const { return ly; }
  float xMax() const { return ux; }
  float yMax() const { return uy; }

  bool  IsValid() const {
    return (lx > 0.0) && (ly > 0.0) && (ux > 0.0) && (uy > 0.0);
  }

  void Merge(const Rect& rect) {
    if (IsValid() == false)
      return;

    lx = std::min(lx, rect.lx);
    ly = std::min(ly, rect.ly);
    ux = std::max(ux, rect.ux);
    uy = std::max(uy, rect.uy);
  }
  
  void Relocate(float outline_lx, float outline_ly,
          float outline_ux, float outline_uy) {
    if (IsValid() == false)
      return;
     
    lx = std::max(lx, outline_lx);
    ly = std::max(ly, outline_ly);
    ux = std::min(ux, outline_ux);
    uy = std::min(uy, outline_uy);
    lx -= outline_lx;
    ly -= outline_ly;
    ux -= outline_lx;
    uy -= outline_ly;
  }


};





}  // namespace mpl
