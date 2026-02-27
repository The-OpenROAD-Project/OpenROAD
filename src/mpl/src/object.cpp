// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "object.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "boost/random/uniform_int_distribution.hpp"
#include "mpl-util.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace mpl {
using utl::MPL;

///////////////////////////////////////////////////////////////////////

Metrics::Metrics(unsigned int num_std_cell,
                 unsigned int num_macro,
                 int64_t std_cell_area,
                 int64_t macro_area)
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

std::pair<unsigned int, unsigned int> Metrics::getCountStats() const
{
  return std::pair<unsigned int, unsigned int>(num_std_cell_, num_macro_);
}

std::pair<int64_t, int64_t> Metrics::getAreaStats() const
{
  return std::pair<int64_t, int64_t>(std_cell_area_, macro_area_);
}

unsigned int Metrics::getNumMacro() const
{
  return num_macro_;
}

unsigned int Metrics::getNumStdCell() const
{
  return num_std_cell_;
}

int64_t Metrics::getStdCellArea() const
{
  return std_cell_area_;
}

int64_t Metrics::getMacroArea() const
{
  return macro_area_;
}

int64_t Metrics::getArea() const
{
  return std_cell_area_ + macro_area_;
}

bool Metrics::empty() const
{
  return num_macro_ == 0 && num_std_cell_ == 0;
}

///////////////////////////////////////////////////////////////////////

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

int Cluster::getId() const
{
  return id_;
}

const std::string& Cluster::getName() const
{
  return name_;
}

void Cluster::setName(const std::string& name)
{
  name_ = name;
}

void Cluster::setClusterType(const ClusterType& cluster_type)
{
  type_ = cluster_type;
}

ClusterType Cluster::getClusterType() const
{
  return type_;
}

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

void Cluster::addLeafInst(odb::dbInst* inst)
{
  if (inst->isBlock()) {
    addLeafMacro(inst);
  } else {
    addLeafStdCell(inst);
  }
}

void Cluster::specifyHardMacros(std::vector<HardMacro*>& hard_macros)
{
  hard_macros_ = hard_macros;
}

std::vector<odb::dbModule*> Cluster::getDbModules() const
{
  return db_modules_;
}

std::vector<odb::dbInst*> Cluster::getLeafStdCells() const
{
  return leaf_std_cells_;
}

std::vector<odb::dbInst*> Cluster::getLeafMacros() const
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

  if (is_io_bundle_) {
    return "IO Bundle";
  }

  if (is_cluster_of_unconstrained_io_pins_) {
    return "Unconstrained IOs";
  }

  if (is_cluster_of_unplaced_io_pins_) {
    return "Unplaced IOs";
  }

  if (is_io_pad_cluster_) {
    return "IO Pad";
  }

  if (is_fixed_macro_) {
    return "Fixed Macro";
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

  if (!isIOCluster() && children_.empty()) {
    is_leaf_string = "Leaf";
  }

  return is_leaf_string;
}

void Cluster::copyInstances(const Cluster& cluster)
{
  db_modules_.clear();
  leaf_std_cells_.clear();
  leaf_macros_.clear();
  hard_macros_.clear();

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

void Cluster::setAsClusterOfUnplacedIOPins(
    const odb::Point& pos,
    const int width,
    const int height,
    const bool is_cluster_of_unconstrained_io_pins)
{
  is_cluster_of_unplaced_io_pins_ = true;
  is_cluster_of_unconstrained_io_pins_ = is_cluster_of_unconstrained_io_pins;
  soft_macro_ = std::make_unique<SoftMacro>(pos, name_, width, height, this);
}

void Cluster::setAsIOPadCluster(const odb::Point& pos, int width, int height)
{
  is_io_pad_cluster_ = true;
  soft_macro_ = std::make_unique<SoftMacro>(pos, name_, width, height, this);
}

void Cluster::setAsIOBundle(const odb::Point& pos, int width, int height)
{
  is_io_bundle_ = true;
  soft_macro_ = std::make_unique<SoftMacro>(pos, name_, width, height, this);
}

void Cluster::setAsFixedMacro(const HardMacro* hard_macro)
{
  is_fixed_macro_ = true;
  soft_macro_ = std::make_unique<SoftMacro>(logger_, hard_macro);
}

bool Cluster::isIOCluster() const
{
  return is_cluster_of_unplaced_io_pins_ || is_io_pad_cluster_ || is_io_bundle_;
}

bool Cluster::isClusterOfUnconstrainedIOPins() const
{
  return is_cluster_of_unconstrained_io_pins_;
}

bool Cluster::isClusterOfUnplacedIOPins() const
{
  return is_cluster_of_unplaced_io_pins_;
}

void Cluster::setAsArrayOfInterconnectedMacros()
{
  is_array_of_interconnected_macros_ = true;
}

bool Cluster::isArrayOfInterconnectedMacros() const
{
  return is_array_of_interconnected_macros_;
}

bool Cluster::isEmpty() const
{
  return getLeafStdCells().empty() && getLeafMacros().empty()
         && getDbModules().empty();
}

bool Cluster::correspondsToLogicalModule() const
{
  return getLeafStdCells().empty() && getLeafMacros().empty()
         && (getDbModules().size() == 1);
}

void Cluster::setMetrics(const Metrics& metrics)
{
  metrics_ = metrics;
}

const Metrics& Cluster::getMetrics() const
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

int64_t Cluster::getArea() const
{
  if (isFixedMacro()) {
    return soft_macro_->getArea();
  }

  return getStdCellArea() + getMacroArea();
}

int64_t Cluster::getStdCellArea() const
{
  if (type_ == HardMacroCluster) {
    return 0;
  }

  return metrics_.getStdCellArea();
}

int64_t Cluster::getMacroArea() const
{
  if (type_ == StdCellCluster) {
    return 0;
  }

  return metrics_.getMacroArea();
}

int Cluster::getWidth() const
{
  if (!soft_macro_) {
    return 0;
  }

  return soft_macro_->getWidth();
}

int Cluster::getHeight() const
{
  if (!soft_macro_) {
    return 0;
  }

  return soft_macro_->getHeight();
}

int Cluster::getX() const
{
  if (!soft_macro_) {
    return 0;
  }

  return soft_macro_->getX();
}

int Cluster::getY() const
{
  if (!soft_macro_) {
    return 0;
  }

  return soft_macro_->getY();
}

void Cluster::setX(int x)
{
  if (soft_macro_) {
    soft_macro_->setX(x);
  }
}

void Cluster::setY(int y)
{
  if (soft_macro_) {
    soft_macro_->setY(y);
  }
}

odb::Point Cluster::getLocation() const
{
  if (!soft_macro_) {
    return {0, 0};
  }

  return soft_macro_->getLocation();
}

odb::Rect Cluster::getBBox() const
{
  return soft_macro_->getBBox();
}

odb::Point Cluster::getCenter() const
{
  return {getX() + (getWidth() / 2), getY() + (getHeight() / 2)};
}

void Cluster::setParent(Cluster* parent)
{
  parent_ = parent;
}

void Cluster::addChild(std::unique_ptr<Cluster> child)
{
  children_.push_back(std::move(child));
}

std::unique_ptr<Cluster> Cluster::releaseChild(const Cluster* candidate)
{
  auto it = std::ranges::find_if(children_, [candidate](const auto& child) {
    return child.get() == candidate;
  });

  if (it != children_.end()) {
    std::unique_ptr<Cluster> released_child = std::move(*it);
    children_.erase(it);
    return released_child;
  }

  return nullptr;
}

void Cluster::addChildren(UniqueClusterVector children)
{
  std::ranges::move(children, std::back_inserter(children_));
}

UniqueClusterVector Cluster::releaseChildren()
{
  UniqueClusterVector released_children = std::move(children_);
  children_.clear();

  return released_children;
}

Cluster* Cluster::getParent() const
{
  return parent_;
}

const UniqueClusterVector& Cluster::getChildren() const
{
  return children_;
}

std::vector<Cluster*> Cluster::getRawChildren() const
{
  std::vector<Cluster*> raw_children(children_.size());
  std::ranges::transform(children_,
                         raw_children.begin(),
                         [](const auto& child) { return child.get(); });
  return raw_children;
}

bool Cluster::isLeaf() const
{
  return children_.empty();
}

bool Cluster::attemptMerge(Cluster* incomer, bool& incomer_deleted)
{
  if (parent_ != incomer->parent_) {
    return false;
  }

  // If the ownership is not passed to the receiver. The incomer
  // is destroyed.
  std::unique_ptr<Cluster> released_incomer = parent_->releaseChild(incomer);

  metrics_.addMetrics(incomer->metrics_);
  name_ += "||" + incomer->name_;

  leaf_macros_.insert(leaf_macros_.end(),
                      incomer->leaf_macros_.begin(),
                      incomer->leaf_macros_.end());
  leaf_std_cells_.insert(leaf_std_cells_.end(),
                         incomer->leaf_std_cells_.begin(),
                         incomer->leaf_std_cells_.end());
  db_modules_.insert(db_modules_.end(),
                     incomer->db_modules_.begin(),
                     incomer->db_modules_.end());
  incomer_deleted = true;

  // If the receiver is not a leaf, the incomer becomes another
  // one of its children.
  if (!children_.empty()) {
    incomer->setParent(this);
    children_.push_back(std::move(released_incomer));
    incomer_deleted = false;
  }
  return true;
}

void Cluster::initConnection()
{
  connections_map_.clear();
}

void Cluster::addConnection(Cluster* cluster, const float connection_weight)
{
  if (connection_weight == 0.0) {
    logger_->error(MPL,
                   66,
                   "Attempting to create connection with zero weight.\nCluster "
                   "A: {}\nCluster B: {}",
                   name_,
                   cluster->getName());
  }

  connections_map_[cluster->getId()] += connection_weight;
}

void Cluster::removeConnection(int cluster_id)
{
  connections_map_.erase(cluster_id);
}

const ConnectionsMap& Cluster::getConnectionsMap() const
{
  return connections_map_;
}

void Cluster::setSoftMacro(std::unique_ptr<SoftMacro> soft_macro)
{
  soft_macro_.reset();
  soft_macro_ = std::move(soft_macro);
}

SoftMacro* Cluster::getSoftMacro() const
{
  return soft_macro_.get();
}

void Cluster::setTilings(const TilingList& tilings)
{
  tilings_ = tilings;
}

const TilingList& Cluster::getTilings() const
{
  return tilings_;
}

std::vector<std::pair<int, int>> Cluster::getVirtualConnections() const
{
  return virtual_connections_;
}

void Cluster::addVirtualConnection(int src, int target)
{
  virtual_connections_.emplace_back(src, target);
}

///////////////////////////////////////////////////////////////////////

HardMacro::HardMacro(const odb::Point& location,
                     const std::string& name,
                     int width,
                     int height,
                     Cluster* cluster)
{
  width_ = width;
  height_ = height;
  name_ = name;
  pin_x_ = 0;
  pin_y_ = 0;
  x_ = location.x();
  y_ = location.y();
  cluster_ = cluster;
}

HardMacro::HardMacro(int width, int height, const std::string& name)
{
  width_ = width;
  height_ = height;
  name_ = name;
  pin_x_ = width / 2;
  pin_y_ = height / 2;
}

HardMacro::HardMacro(odb::dbInst* inst, Halo halo)
{
  inst_ = inst;
  block_ = inst->getBlock();
  name_ = inst->getName();

  halo_ = halo;

  odb::dbMaster* master = inst->getMaster();
  width_ = master->getWidth() + 2 * halo_.width;
  height_ = master->getHeight() + 2 * halo.height;

  if (inst_->isFixed()) {
    const odb::Rect& box = inst->getBBox()->getBox();
    x_ = box.xMin() - halo_.width;
    y_ = box.yMin() - halo_.height;
    fixed_ = true;
  }

  // Set the position of virtual pins
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
  pin_x_ = ((bbox.xMin() + bbox.xMax()) / 2) + halo_.width;
  pin_y_ = ((bbox.yMin() + bbox.yMax()) / 2) + halo_.height;
}

bool HardMacro::operator<(const HardMacro& macro) const
{
  if (getArea() != macro.getArea()) {
    return getArea() < macro.getArea();
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

// Cluster support to identify if a fixed terminal correponds
// to a cluster of unplaced IO pins when running HardMacro SA.
bool HardMacro::isClusterOfUnplacedIOPins() const
{
  if (!cluster_) {
    return false;
  }

  return cluster_->isClusterOfUnplacedIOPins();
}

// Cluster support to identify if a fixed terminal correponds
// to the cluster of unconstrained IO pins when running HardMacro SA.
bool HardMacro::isClusterOfUnconstrainedIOPins() const
{
  if (!cluster_) {
    return false;
  }

  return cluster_->isClusterOfUnconstrainedIOPins();
}

odb::Rect HardMacro::getBBox() const
{
  return odb::Rect(x_, y_, x_ + width_, y_ + height_);
}

void HardMacro::setLocation(const odb::Point& location)
{
  if (getArea() == 0) {
    return;
  }
  x_ = location.x();
  y_ = location.y();
}

void HardMacro::setX(int x)
{
  if (getArea() == 0) {
    return;
  }
  x_ = x;
}

void HardMacro::setY(int y)
{
  if (getArea() == 0) {
    return;
  }
  y_ = y;
}

odb::Point HardMacro::getLocation() const
{
  return {x_, y_};
}

void HardMacro::setRealLocation(const odb::Point& location)
{
  if (getArea() == 0) {
    return;
  }

  x_ = location.x() - halo_.width;
  y_ = location.y() - halo_.height;
}

void HardMacro::setRealX(int x)
{
  if (getArea() == 0) {
    return;
  }

  x_ = x - halo_.width;
}

void HardMacro::setRealY(int y)
{
  if (getArea() == 0) {
    return;
  }

  y_ = y - halo_.height;
}

odb::Point HardMacro::getRealLocation() const
{
  return {x_ + halo_.width, y_ + halo_.height};
}

int HardMacro::getRealX() const
{
  return x_ + halo_.width;
}

int HardMacro::getRealY() const
{
  return y_ + halo_.height;
}

int HardMacro::getRealWidth() const
{
  return width_ - 2 * halo_.width;
}

int HardMacro::getRealHeight() const
{
  return height_ - 2 * halo_.height;
}

int64_t HardMacro::getRealArea() const
{
  return getRealWidth() * static_cast<int64_t>(getRealHeight());
}

odb::dbOrientType HardMacro::getOrientation() const
{
  return orientation_;
}

odb::dbInst* HardMacro::getInst() const
{
  return inst_;
}

const std::string& HardMacro::getName() const
{
  return name_;
}

std::string HardMacro::getMasterName() const
{
  if (inst_ == nullptr) {
    return name_;
  }
  return inst_->getMaster()->getName();
}

///////////////////////////////////////////////////////////////////////

// Represent a "regular" cluster (Mixed, StdCell or Macro).
SoftMacro::SoftMacro(Cluster* cluster)
{
  name_ = cluster->getName();
  cluster_ = cluster;
}

// Represent a blockage.
SoftMacro::SoftMacro(const odb::Rect& blockage, const std::string& name)
{
  name_ = name;
  x_ = blockage.xMin();
  y_ = blockage.yMin();
  width_ = blockage.dx();
  height_ = blockage.dy();
  area_ = blockage.area();
  cluster_ = nullptr;
  fixed_ = true;
  is_blockage_ = true;
}

// Represent an IO cluster or fixed terminal.
SoftMacro::SoftMacro(const odb::Point& location,
                     const std::string& name,
                     int width,
                     int height,
                     Cluster* cluster)
{
  name_ = name;
  x_ = location.x();
  y_ = location.y();
  width_ = width;
  height_ = height;

  // Even though clusters of unplaced IOs have shapes, i.e., are not
  // just points, their area should be zero, because we use the area
  // to check whether or not a SoftMacro if a fixed terminal or cluster
  // of unplaced IOs inside SA. Ideally we should check the fixed flag.
  area_ = 0;

  cluster_ = cluster;
  fixed_ = true;
}

// Represent a fixed macro.
SoftMacro::SoftMacro(utl::Logger* logger,
                     const HardMacro* hard_macro,
                     const odb::Rect* outline)
{
  if (!hard_macro->isFixed()) {
    logger->error(
        MPL,
        37,
        "Attempting to create fixed soft macro for unfixed hard macro {}.",
        hard_macro->getName());
  }

  name_ = hard_macro->getName();

  odb::Rect shape;
  odb::Rect hard_macro_bbox = hard_macro->getBBox();

  if (outline) {
    hard_macro_bbox.intersection(*outline, shape);
    shape.moveDelta(-outline->xMin(), -outline->yMin());
  } else {
    shape = hard_macro_bbox;
  }

  x_ = shape.xMin();
  y_ = shape.yMin();
  width_ = shape.dx();
  height_ = shape.dy();
  area_ = shape.area();

  cluster_ = hard_macro->getCluster();
  fixed_ = true;
}

const std::string& SoftMacro::getName() const
{
  return name_;
}

void SoftMacro::setX(int x)
{
  if (!fixed_) {
    x_ = x;
  }
}

void SoftMacro::setY(int y)
{
  if (!fixed_) {
    y_ = y;
  }
}

void SoftMacro::setLocation(const odb::Point& location)
{
  if (fixed_) {
    return;
  }
  x_ = location.x();
  y_ = location.y();
}

// Find the index of the interval to which 'value' belongs.
int SoftMacro::findIntervalIndex(const IntervalList& interval_list,
                                 int& value,
                                 bool increasing_list)
{
  // We assume the value is within the range of list
  int idx = 0;
  if (increasing_list) { /* Width Intervals */
    while ((idx < interval_list.size()) && (interval_list[idx].max < value)) {
      idx++;
    }
    value = std::max(interval_list[idx].min, value);
  } else { /* Height Intervals */
    while ((idx < interval_list.size()) && (interval_list[idx].min > value)) {
      idx++;
    }
    value = std::min(interval_list[idx].max, value);
  }
  return idx;
}

void SoftMacro::setWidth(int width)
{
  if (width <= 0 || area_ == 0
      || width_intervals_.size() != height_intervals_.size()
      || width_intervals_.empty() || cluster_ == nullptr
      || cluster_->getClusterType() == HardMacroCluster
      || cluster_->isIOCluster()) {
    return;
  }

  // The width intervals are sorted in nondecreasing order.
  if (width <= width_intervals_.front().min) {
    width_ = width_intervals_.front().min;
    height_ = height_intervals_.front().max;
    area_ = width_ * static_cast<int64_t>(height_);
  } else if (width >= width_intervals_.back().max) {
    width_ = width_intervals_.back().max;
    height_ = height_intervals_.back().min;
    area_ = width_ * static_cast<int64_t>(height_);
  } else {
    width_ = width;
    int idx = findIntervalIndex(width_intervals_, width_, true);
    area_ = width_intervals_[idx].max
            * static_cast<int64_t>(height_intervals_[idx].min);
    height_ = area_ / width_;
  }
}

void SoftMacro::setHeight(int height)
{
  if (height <= 0 || area_ == 0
      || width_intervals_.size() != height_intervals_.size()
      || width_intervals_.empty() || cluster_ == nullptr
      || cluster_->getClusterType() == HardMacroCluster
      || cluster_->isIOCluster()) {
    return;
  }

  // The height intervals are sorted in nonincreasing order.
  if (height >= height_intervals_.front().max) {
    height_ = height_intervals_.front().max;
    width_ = width_intervals_.front().min;
    area_ = width_ * static_cast<int64_t>(height_);
  } else if (height <= height_intervals_.back().min) {
    height_ = height_intervals_.back().min;
    width_ = width_intervals_.back().max;
    area_ = width_ * static_cast<int64_t>(height_);
  } else {
    height_ = height;
    int idx = findIntervalIndex(height_intervals_, height_, false);
    area_ = width_intervals_[idx].max
            * static_cast<int64_t>(height_intervals_[idx].min);
    width_ = area_ / height_;
  }
}

void SoftMacro::setArea(int64_t area)
{
  if (area_ == 0 || width_intervals_.size() != height_intervals_.size()
      || width_intervals_.empty() || cluster_ == nullptr
      || cluster_->getClusterType() == HardMacroCluster
      || cluster_->isIOCluster()
      || area <= (width_intervals_.front().min
                  * static_cast<int64_t>(height_intervals_.front().max))) {
    return;
  }

  // area must be larger than area_
  IntervalList width_intervals;
  IntervalList height_intervals;
  for (int i = 0; i < width_intervals_.size(); i++) {
    const float min_width = width_intervals_[i].min;
    const float min_height = height_intervals_[i].min;
    const float max_width = area / min_height;
    const float max_height = area / min_width;
    if (width_intervals.empty() || min_width > width_intervals.back().max) {
      width_intervals.emplace_back(min_width, max_width);
      height_intervals.emplace_back(min_height, max_height);
    } else {
      width_intervals.back().max = max_width;
      height_intervals.back().min = min_height;
    }
  }

  width_intervals_ = std::move(width_intervals);
  height_intervals_ = std::move(height_intervals);
  area_ = area;
  width_ = width_intervals_.front().min;
  height_ = height_intervals_.front().max;
}

// Method to set the shape curve for Macro clusters.
// The shape curve is discrete.
void SoftMacro::setShapes(const TilingList& tilings, bool force)
{
  if (!force
      && (tilings.empty() || cluster_ == nullptr
          || cluster_->getClusterType() != HardMacroCluster)) {
    return;
  }

  // Here we do not need to sort the intervals.
  for (auto& tiling : tilings) {
    width_intervals_.emplace_back(tiling.width(), tiling.width());
    height_intervals_.emplace_back(tiling.height(), tiling.height());
  }

  width_ = tilings.front().width();
  height_ = tilings.front().height();
  area_ = width_ * static_cast<int64_t>(height_);
}

// Method to set the shape curve for the following cluster types:
// - Mixed
// - Std Cell
//
// The shape curve is piecewise.
void SoftMacro::setShapes(const IntervalList& width_intervals, int64_t area)
{
  if (width_intervals.empty() || area <= 0 || cluster_ == nullptr
      || cluster_->isIOCluster()
      || cluster_->getClusterType() == HardMacroCluster) {
    return;
  }

  width_intervals_.clear();
  height_intervals_.clear();

  // Copy & sort the width intervals list.
  IntervalList old_width_intervals = width_intervals;
  std::ranges::sort(old_width_intervals,

                    isMinWidthSmaller);

  // Merge the overlapping intervals.
  for (auto& old_width_interval : old_width_intervals) {
    if (width_intervals_.empty()
        || old_width_interval.min > width_intervals_.back().max) {
      width_intervals_.push_back(old_width_interval);
    } else if (old_width_interval.max > width_intervals_.back().max) {
      width_intervals_.back().max = old_width_interval.max;
    }
  }

  // Set height intervals based on the new width intervals.
  for (auto& width_interval : width_intervals_) {
    height_intervals_.emplace_back(area / width_interval.max /* min */,
                                   area / width_interval.min /* max */);
  }

  width_ = width_intervals_.front().min;
  height_ = height_intervals_.front().max;
  area_ = area;
}

int64_t SoftMacro::getArea() const
{
  return area_ > 1 ? area_ : 0;
}

odb::Rect SoftMacro::getBBox() const
{
  return odb::Rect(x_, y_, x_ + width_, y_ + height_);
}

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
  if (width_intervals_.empty()) {
    return;
  }

  boost::random::uniform_int_distribution<> index_distribution(
      0, width_intervals_.size() - 1);
  const int idx = index_distribution(generator);

  const int min_width = width_intervals_[idx].min;
  const int max_width = width_intervals_[idx].max;
  width_ = min_width + distribution(generator) * (max_width - min_width);
  area_ = width_intervals_[idx].min
          * static_cast<int64_t>(height_intervals_[idx].max);
  height_ = area_ / width_;
}

bool SoftMacro::isBlockage() const
{
  return is_blockage_;
}

Cluster* SoftMacro::getCluster() const
{
  return cluster_;
}

float SoftMacro::getMacroUtil() const
{
  if (cluster_ == nullptr || area_ == 0) {
    return 0.0;
  }

  if (cluster_->getClusterType() == HardMacroCluster) {
    return 1.0;
  }

  if (cluster_->getClusterType() == MixedCluster) {
    return cluster_->getMacroArea() / static_cast<float>(area_);
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

// Cluster support to identify if a fixed terminal correponds
// to a cluster of unplaced IO pins when running SoftMacro SA.
bool SoftMacro::isClusterOfUnplacedIOPins() const
{
  if (!cluster_) {
    return false;
  }

  return cluster_->isClusterOfUnplacedIOPins();
}

// Cluster support to identify if a fixed terminal correponds
// to the cluster of unconstrained IO pins when running SoftMacro SA.
bool SoftMacro::isClusterOfUnconstrainedIOPins() const
{
  if (!cluster_) {
    return false;
  }

  return cluster_->isClusterOfUnconstrainedIOPins();
}

void SoftMacro::setLocationF(int x, int y)
{
  x_ = x;
  y_ = y;
}

void SoftMacro::setShapeF(int width, int height)
{
  if (fixed_) {
    return;
  }

  width_ = width;
  height_ = height;
  area_ = width * static_cast<int64_t>(height);
}

void SoftMacro::reportShapeCurve(utl::Logger* logger) const
{
  logger->report("Name: {}", name_);
  logger->report("Has Cluster: {}", cluster_ != nullptr);
  if (cluster_) {
    logger->report("Type: {}", cluster_->getClusterTypeString());
  }

  std::string widths_text, heights_text;

  for (const Interval& width_interval : width_intervals_) {
    widths_text
        += fmt::format("\t{} -> {}", width_interval.min, width_interval.max);
  }

  for (const Interval& height_interval : height_intervals_) {
    heights_text
        += fmt::format("\t{} -> {}", height_interval.max, height_interval.min);
  }

  logger->report("Widths: {}", widths_text);
  logger->report("Heights: {}\n", heights_text);
}

void Cluster::reportConnections() const
{
  logger_->report("{} ({}) Connections:", name_, id_);
  logger_->report("\n  Cluster Id  |  Connection Weight  ");
  logger_->report("------------------------------------");
  for (const auto& [cluster_id, connections_weight] : connections_map_) {
    logger_->report(" {:>12d} | {:>19.2f}", cluster_id, connections_weight);
  }
  logger_->report("");
}

float Cluster::allConnectionsWeight() const
{
  float all_weight = 0;
  for (const auto& [cluster_id, connection_weight] : connections_map_) {
    all_weight += connection_weight;
  }
  return all_weight;
}

}  // namespace mpl
