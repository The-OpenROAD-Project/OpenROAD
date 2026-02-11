// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "mpl-util.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "shapes.h"

namespace odb {
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

namespace mpl {
struct BundledNet;
class HardMacro;
class SoftMacro;
class Cluster;

using ConnectionsMap = std::map<int, float>;
using IntervalList = std::vector<Interval>;
using TilingList = std::vector<Tiling>;
using TilingSet = std::set<Tiling>;
using UniqueClusterVector = std::vector<std::unique_ptr<Cluster>>;
using BundledNetList = std::vector<BundledNet>;

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
          int64_t std_cell_area,
          int64_t macro_area);

  void addMetrics(const Metrics& metrics);
  std::pair<unsigned int, unsigned int> getCountStats() const;
  std::pair<int64_t, int64_t> getAreaStats() const;
  unsigned int getNumMacro() const;
  unsigned int getNumStdCell() const;
  int64_t getStdCellArea() const;
  int64_t getMacroArea() const;
  int64_t getArea() const;
  bool empty() const;

 private:
  unsigned int num_std_cell_ = 0;
  unsigned int num_macro_ = 0;
  int64_t std_cell_area_ = 0;
  int64_t macro_area_ = 0;
};

class Cluster
{
 public:
  Cluster(int cluster_id, utl::Logger* logger);
  Cluster(int cluster_id, const std::string& cluster_name, utl::Logger* logger);

  int getId() const;
  const std::string& getName() const;
  void setName(const std::string& name);
  void setClusterType(const ClusterType& cluster_type);
  ClusterType getClusterType() const;
  std::string getClusterTypeString() const;

  void addDbModule(odb::dbModule* db_module);
  void addLeafStdCell(odb::dbInst* leaf_std_cell);
  void addLeafMacro(odb::dbInst* leaf_macro);
  void addLeafInst(odb::dbInst* inst);
  void specifyHardMacros(std::vector<HardMacro*>& hard_macros);
  // TODO: the following return internal arrays by value. This should be
  // good as const reference iff the callsites don't then call Add*() methods
  // while iterating over it. Verify and then possibly optimize.
  std::vector<odb::dbModule*> getDbModules() const;
  std::vector<odb::dbInst*> getLeafStdCells() const;
  std::vector<odb::dbInst*> getLeafMacros() const;
  std::vector<HardMacro*> getHardMacros() const;
  void clearDbModules();
  void clearLeafStdCells();
  void clearLeafMacros();
  void clearHardMacros();
  void copyInstances(const Cluster& cluster);

  bool isIOCluster() const;
  bool isClusterOfUnconstrainedIOPins() const;
  bool isClusterOfUnplacedIOPins() const;
  void setAsClusterOfUnplacedIOPins(const odb::Point& pos,
                                    int width,
                                    int height,
                                    bool is_cluster_of_unconstrained_io_pins);
  bool isIOPadCluster() const { return is_io_pad_cluster_; }
  void setAsIOPadCluster(const odb::Point& pos, int width, int height);
  bool isIOBundle() const { return is_io_bundle_; }
  void setAsIOBundle(const odb::Point& pos, int width, int height);

  bool isFixedMacro() const { return is_fixed_macro_; }
  void setAsFixedMacro(const HardMacro* hard_macro);

  void setAsArrayOfInterconnectedMacros();
  bool isArrayOfInterconnectedMacros() const;
  void setAsMacroArray() { is_macro_array_ = true; }
  bool isMacroArray() const { return is_macro_array_; }
  bool isEmpty() const;
  bool correspondsToLogicalModule() const;

  // Metrics support:
  void setMetrics(const Metrics& metrics);
  const Metrics& getMetrics() const;
  int getNumStdCell() const;
  int getNumMacro() const;
  int64_t getArea() const;
  int64_t getMacroArea() const;
  int64_t getStdCellArea() const;

  // Physical support:
  int getWidth() const;
  int getHeight() const;
  int getX() const;
  int getY() const;
  void setX(int x);
  void setY(int y);
  odb::Point getLocation() const;
  odb::Rect getBBox() const;
  odb::Point getCenter() const;
  void setSoftMacro(std::unique_ptr<SoftMacro> soft_macro);
  SoftMacro* getSoftMacro() const;
  void setTilings(const TilingList& tilings);
  const TilingList& getTilings() const;

  // Hierarchy support:
  void setParent(Cluster* parent);
  void addChild(std::unique_ptr<Cluster> child);
  std::unique_ptr<Cluster> releaseChild(const Cluster* candidate);
  void addChildren(UniqueClusterVector children);
  UniqueClusterVector releaseChildren();
  Cluster* getParent() const;
  const UniqueClusterVector& getChildren() const;
  std::vector<Cluster*> getRawChildren() const;
  bool isLeaf() const;
  std::string getIsLeafString() const;
  bool attemptMerge(Cluster* incomer, bool& incomer_deleted);
  void initConnection();
  void addConnection(Cluster* cluster, float connection_weight);
  void removeConnection(int cluster_id);
  const ConnectionsMap& getConnectionsMap() const;
  float allConnectionsWeight() const;

  // TODO: return const reference iff precondition ok (see comment in Cluster)
  std::vector<std::pair<int, int>> getVirtualConnections() const;
  void addVirtualConnection(int src, int target);

  // For Debug
  void reportConnections() const;

 private:
  int id_{-1};
  std::string name_;

  ClusterType type_{MixedCluster};
  Metrics metrics_;

  std::vector<odb::dbModule*> db_modules_;
  std::vector<odb::dbInst*> leaf_std_cells_;
  std::vector<odb::dbInst*> leaf_macros_;
  std::vector<HardMacro*> hard_macros_;

  bool is_cluster_of_unplaced_io_pins_{false};
  bool is_cluster_of_unconstrained_io_pins_{false};
  bool is_io_pad_cluster_{false};
  bool is_io_bundle_{false};
  bool is_array_of_interconnected_macros_{false};
  bool is_macro_array_{false};
  bool is_fixed_macro_{false};

  std::unique_ptr<SoftMacro> soft_macro_;
  TilingList tilings_;

  Cluster* parent_{nullptr};
  UniqueClusterVector children_;

  ConnectionsMap connections_map_;  // cluster id -> connection weight
  std::vector<std::pair<int, int>> virtual_connections_;  // id -> id

  utl::Logger* logger_;
};

class HardMacro
{
 public:
  struct Halo
  {
    int width{0};
    int height{0};
  };

  HardMacro(const odb::Point& location,
            const std::string& name,
            int width,
            int height,
            Cluster* cluster);
  HardMacro(int width, int height, const std::string& name);
  HardMacro(odb::dbInst* inst, Halo halo);

  bool operator<(const HardMacro& macro) const;
  bool operator==(const HardMacro& macro) const;

  void setCluster(Cluster* cluster) { cluster_ = cluster; }
  Cluster* getCluster() const { return cluster_; }
  bool isClusterOfUnplacedIOPins() const;
  bool isClusterOfUnconstrainedIOPins() const;

  // Note that the default X and Y include halo
  void setLocation(const odb::Point& location);
  void setX(int x);
  void setY(int y);
  odb::Point getLocation() const;
  int getX() const { return x_; }
  int getY() const { return y_; }

  // The position of pins relative to the lower left of the instance
  int getPinX() const { return x_ + pin_x_; }
  int getPinY() const { return y_ + pin_y_; }

  // The position of pins relative to the origin of the canvas;
  int getAbsPinX() const { return pin_x_; }
  int getAbsPinY() const { return pin_y_; }

  int getWidth() const { return width_; }
  int getHeight() const { return height_; }
  int64_t getArea() const { return width_ * static_cast<int64_t>(height_); }
  odb::Rect getBBox() const;
  bool isFixed() const { return fixed_; }

  // Note that the real X and Y does NOT include halo
  void setRealLocation(const odb::Point& location);
  void setRealX(int x);
  void setRealY(int y);
  odb::Point getRealLocation() const;
  int getRealX() const;
  int getRealY() const;
  int getRealWidth() const;
  int getRealHeight() const;
  int64_t getRealArea() const;

  odb::dbOrientType getOrientation() const;

  // Interfaces with OpenDB
  odb::dbInst* getInst() const;
  const std::string& getName() const;
  std::string getMasterName() const;

 private:
  std::string name_;

  // Lower-left corner:
  int x_{0};
  int y_{0};

  Halo halo_;

  // Dimensions considering halo:
  int width_{0};
  int height_{0};

  odb::dbOrientType orientation_{odb::dbOrientType::R0};

  // We abstract the macro pins into a single point based on their location:
  int pin_x_{0};
  int pin_y_{0};

  bool fixed_{false};

  odb::dbInst* inst_{nullptr};
  odb::dbBlock* block_{nullptr};
  Cluster* cluster_{nullptr};
};

// This is the abstraction of the moveable objects inside the simulated
// annealing for:
//  1. Coarse shaping of Mixed clusters (tilings generation);
//  2. Cluster placement.
//
// A SoftMacro can represent:
//  - a "regular" Cluster (Mixed, StdCell or Macro);
//  - an IO Cluster (PAD or group of unplaced pins) which has its position
//    fixed;
//  - a fixed terminal;
//  - a blockage;
//  - a fixed macro.
//
// Obs: The bundled pin of a SoftMacro is always its center.
class SoftMacro
{
 public:
  SoftMacro(Cluster* cluster);
  SoftMacro(const odb::Rect& blockage, const std::string& name);
  SoftMacro(const odb::Point& location,
            const std::string& name,
            int width,
            int height,
            Cluster* cluster);
  SoftMacro(utl::Logger* logger,
            const HardMacro* hard_macro,
            const odb::Rect* outline = nullptr);

  const std::string& getName() const;

  void setX(int x);
  void setY(int y);
  void setLocation(const odb::Point& location);

  // Only for Mixed and Std Cell clusters:
  void setWidth(int width);
  void setHeight(int height);
  void setArea(int64_t area);

  void resizeRandomly(std::uniform_real_distribution<float>& distribution,
                      std::mt19937& generator);
  void setShapes(const TilingList& tilings, bool force = false);
  void setShapes(const IntervalList& width_intervals, int64_t area);

  int getX() const { return x_; }
  int getY() const { return y_; }

  // The position of pins relative to the lower left of the instance
  int getPinX() const { return x_ + (0.5 * width_); }
  int getPinY() const { return y_ + (0.5 * height_); }

  odb::Point getLocation() const { return {x_, y_}; }

  bool isFixed() const { return fixed_; }

  int getWidth() const { return width_; }
  int getHeight() const { return height_; }
  int64_t getArea() const;
  odb::Rect getBBox() const;
  bool isMacroCluster() const;
  bool isStdCellCluster() const;
  bool isMixedCluster() const;
  bool isClusterOfUnplacedIOPins() const;
  bool isClusterOfUnconstrainedIOPins() const;
  void setLocationF(int x, int y);
  void setShapeF(int width, int height);
  int getNumMacro() const;
  bool isBlockage() const;
  Cluster* getCluster() const;
  float getMacroUtil() const;

  // Debug
  void reportShapeCurve(utl::Logger* logger) const;

 private:
  int findIntervalIndex(const IntervalList& interval_list,
                        int& value,
                        bool increasing_list);

  std::string name_;

  // Lower-left corner:
  int x_{0};
  int y_{0};

  int width_{0};
  int height_{0};
  int64_t area_{0};

  // The shape curve (discrete or piecewise) of a cluster is the
  // combination of its width/height intervals.
  IntervalList width_intervals_;   // nondecreasing order
  IntervalList height_intervals_;  // nonincreasing order

  Cluster* cluster_ = nullptr;
  bool fixed_ = false;  // if the macro is fixed
  bool is_blockage_ = false;
};

struct BundledNet
{
  BundledNet(int src, int target, float weight)
  {
    this->terminals = std::pair<int, int>(src, target);
    this->weight = weight;
  }

  bool operator==(const BundledNet& net) const
  {
    return (terminals.first == net.terminals.first)
           && (terminals.second == net.terminals.second);
  }

  std::pair<int, int> terminals;  // source_id <--> target_id (undirected)
  float weight;
};

struct SequencePair
{
  std::vector<int> pos_sequence;
  std::vector<int> neg_sequence;
};

}  // namespace mpl
