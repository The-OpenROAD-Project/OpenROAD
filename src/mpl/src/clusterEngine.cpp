// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "clusterEngine.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <memory>
#include <ranges>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "MplObserver.h"
#include "boost/polygon/polygon.hpp"
#include "boost/polygon/polygon_90_set_data.hpp"
#include "db_sta/dbNetwork.hh"
#include "mpl-util.h"
#include "object.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/geom_boost.h"
#include "par/PartitionMgr.h"
#include "sta/Liberty.hh"
#include "utl/Logger.h"

namespace mpl {
using utl::MPL;

ClusteringEngine::ClusteringEngine(odb::dbBlock* block,
                                   sta::dbNetwork* network,
                                   utl::Logger* logger,
                                   par::PartitionMgr* triton_part,
                                   MplObserver* graphics)
    : block_(block),
      network_(network),
      logger_(logger),
      triton_part_(triton_part),
      graphics_(graphics)
{
}

void ClusteringEngine::run()
{
  init();

  if (!tree_->has_unfixed_macros) {
    return;
  }

  createRoot();
  setBaseThresholds();
  createIOClusters();

  if (design_metrics_->getNumStdCell() == 0) {
    logger_->warn(MPL, 25, "Design has no standard cells!");
    tree_->has_std_cells = false;
    treatEachMacroAsSingleCluster();
  } else {
    multilevelAutocluster(tree_->root.get());

    std::vector<std::vector<Cluster*>> mixed_leaves;
    fetchMixedLeaves(tree_->root.get(), mixed_leaves);
    breakMixedLeaves(mixed_leaves);
  }

  if (logger_->debugCheck(MPL, "multilevel_autoclustering", 1)) {
    logger_->report("\nPrint Physical Hierarchy\n");
    printPhysicalHierarchyTree(tree_->root.get(), 0);
  }

  // Map the macros in each cluster to their HardMacro objects
  for (auto& [cluster_id, cluster] : tree_->maps.id_to_cluster) {
    mapMacroInCluster2HardMacro(cluster);
  }
}

void ClusteringEngine::setTree(PhysicalHierarchy* tree)
{
  tree_ = tree;
}

void ClusteringEngine::setHalos(
    std::map<odb::dbInst*, HardMacro::Halo>& macro_to_halo)
{
  macro_to_halo_ = macro_to_halo;
}

// Check if macro placement is both needed and feasible.
// Also report some design data relevant for the user and
// initialize the tree with data from the design.
void ClusteringEngine::init()
{
  setDieArea();
  setFloorplanShape();
  createHardMacros();

  if (!movableCellsFitInMacroPlacementArea()) {
    logger_->error(
        MPL, 65, "The movable cells do not fit in the macro placement area.");
  }

  design_metrics_ = computeModuleMetrics(block_->getTopModule());

  const std::vector<odb::dbInst*> unfixed_macros = getUnfixedMacros();
  if (unfixed_macros.empty()) {
    tree_->has_unfixed_macros = false;
    logger_->info(MPL, 17, "No unfixed macros.");
    return;
  }

  tree_->macro_with_halo_area = computeMacroWithHaloArea(unfixed_macros);
  const float inst_area_with_halos
      = tree_->macro_with_halo_area + design_metrics_->getStdCellArea();

  if (inst_area_with_halos > tree_->floorplan_shape.area()) {
    logger_->error(MPL,
                   16,
                   "The instance area considering the macros' halos {} exceeds "
                   "the floorplan area {}",
                   inst_area_with_halos,
                   tree_->floorplan_shape.area());
  }

  tree_->io_pads = getIOPads();

  reportDesignData();
}

bool ClusteringEngine::movableCellsFitInMacroPlacementArea() const
{
  const odb::Rect& macro_placement_area = tree_->floorplan_shape;

  int64_t occupied_area = 0;
  for (odb::dbInst* inst : block_->getInsts()) {
    const odb::Rect bbox = inst->getBBox()->getBox();

    if (inst->isFixed()) {
      // Note that we can handle fixed cells outside the macro placement area.
      // Also, it may exist macros such as physical markers that can be
      // partially inside the core so we need to compute the intersection.
      odb::Rect intersection;
      bbox.intersection(macro_placement_area, intersection);
      occupied_area += intersection.area();
      continue;
    }

    if (inst->isBlock()) {
      occupied_area += tree_->maps.inst_to_hard.at(inst)->getArea();
    } else {
      occupied_area += bbox.area();
    }
  }

  namespace gtl = boost::polygon;
  using gtl::operators::operator+=;
  using PolygonSet = gtl::polygon_90_set_data<int>;

  PolygonSet polygons;
  for (odb::dbBlockage* blockage : block_->getBlockages()) {
    const odb::Rect bbox = blockage->getBBox()->getBox();
    odb::Rect intersection;
    bbox.intersection(macro_placement_area, intersection);
    polygons += intersection;
  }

  std::vector<odb::Rect> blockages_without_overlap;
  polygons.get_rectangles(blockages_without_overlap);

  for (const odb::Rect& blockage : blockages_without_overlap) {
    occupied_area += blockage.area();
  }

  return occupied_area <= macro_placement_area.area();
}

// Note: The die area's dimensions will be used inside
// SA Core when computing the wirelength in a situation in which
// the target cluster is a cluster of unplaced IOs.
void ClusteringEngine::setDieArea()
{
  tree_->die_area = block_->getDieArea();
}

int64_t ClusteringEngine::computeMacroWithHaloArea(
    const std::vector<odb::dbInst*>& unfixed_macros)
{
  int64_t macro_with_halo_area = 0;
  for (odb::dbInst* unfixed_macro : unfixed_macros) {
    macro_with_halo_area
        += tree_->maps.inst_to_hard.at(unfixed_macro)->getArea();
  }
  return macro_with_halo_area;
}

std::vector<odb::dbInst*> ClusteringEngine::getUnfixedMacros()
{
  std::vector<odb::dbInst*> unfixed_macros;
  for (odb::dbInst* inst : block_->getInsts()) {
    if (inst->isBlock() && !inst->getPlacementStatus().isFixed()) {
      unfixed_macros.push_back(inst);
    }
  }
  return unfixed_macros;
}

void ClusteringEngine::setFloorplanShape()
{
  tree_->floorplan_shape = block_->getCoreArea().intersect(tree_->global_fence);

  if (tree_->floorplan_shape.area() == 0) {
    logger_->error(
        MPL, 68, "The global fence set is completely outside the core area.");
  }
}

Metrics* ClusteringEngine::computeModuleMetrics(odb::dbModule* module)
{
  unsigned int num_std_cell = 0;
  int64_t std_cell_area = 0;
  unsigned int num_macro = 0;
  int64_t macro_area = 0;

  for (odb::dbInst* inst : module->getInsts()) {
    if (inst->isBlock()) {
      num_macro += 1;
      macro_area += computeArea(inst);
    } else if (inst->isFixed() && !inst->getMaster()->isCover()
               && inst->getBBox()->getBox().overlaps(tree_->floorplan_shape)) {
      logger_->error(MPL,
                     50,
                     "Found fixed non-macro instance {} inside the macro "
                     "placement area.",
                     inst->getName());
    } else if (!isIgnoredInst(inst)) {
      num_std_cell += 1;
      std_cell_area += computeArea(inst);
    }
  }

  for (odb::dbModInst* child_module_inst : module->getChildren()) {
    Metrics* metrics = computeModuleMetrics(child_module_inst->getMaster());
    num_std_cell += metrics->getNumStdCell();
    std_cell_area += metrics->getStdCellArea();
    num_macro += metrics->getNumMacro();
    macro_area += metrics->getMacroArea();
  }

  auto metrics = std::make_unique<Metrics>(
      num_std_cell, num_macro, std_cell_area, macro_area);
  tree_->maps.module_to_metrics[module] = std::move(metrics);

  return tree_->maps.module_to_metrics[module].get();
}

std::vector<odb::dbInst*> ClusteringEngine::getIOPads() const
{
  std::vector<odb::dbInst*> io_pads;

  for (odb::dbInst* inst : block_->getInsts()) {
    const odb::dbMasterType& master_type = inst->getMaster()->getType();

    // Skip PADs without core signal connections.
    if (master_type == odb::dbMasterType::PAD_POWER
        || master_type == odb::dbMasterType::PAD_SPACER) {
      continue;
    }

    if (inst->isPad()) {
      io_pads.push_back(inst);
    }
  }

  return io_pads;
}

void ClusteringEngine::reportDesignData()
{
  const odb::Rect& die = block_->getDieArea();
  logger_->report(
      "Die Area: ({:.2f}, {:.2f}) ({:.2f}, {:.2f}),  Floorplan Area: ({:.2f}, "
      "{:.2f}) ({:.2f}, {:.2f})",
      block_->dbuToMicrons(die.xMin()),
      block_->dbuToMicrons(die.yMin()),
      block_->dbuToMicrons(die.xMax()),
      block_->dbuToMicrons(die.yMax()),
      block_->dbuToMicrons(tree_->floorplan_shape.xMin()),
      block_->dbuToMicrons(tree_->floorplan_shape.yMin()),
      block_->dbuToMicrons(tree_->floorplan_shape.xMax()),
      block_->dbuToMicrons(tree_->floorplan_shape.yMax()));

  double util
      = (design_metrics_->getStdCellArea() + design_metrics_->getMacroArea())
        / static_cast<double>(tree_->floorplan_shape.area());
  double floorplan_util
      = design_metrics_->getStdCellArea()
        / static_cast<double>(
            (tree_->floorplan_shape.area() - design_metrics_->getMacroArea()));
  logger_->report(
      "\tNumber of std cell instances: {}\n"
      "\tArea of std cell instances: {:.2f}\n"
      "\tNumber of macros: {}\n"
      "\tArea of macros: {:.2f}\n"
      "\tHalo width: {:.2f}\n"
      "\tHalo height: {:.2f}\n"
      "\tArea of macros with halos: {:.2f}\n"
      "\tArea of std cell instances + Area of macros: {:.2f}\n"
      "\tFloorplan area: {:.2f}\n"
      "\tDesign Utilization: {:.2f}\n"
      "\tFloorplan Utilization: {:.2f}\n"
      "\tManufacturing Grid: {}\n",
      design_metrics_->getNumStdCell(),
      block_->dbuAreaToMicrons(design_metrics_->getStdCellArea()),
      design_metrics_->getNumMacro(),
      block_->dbuAreaToMicrons(design_metrics_->getMacroArea()),
      block_->dbuToMicrons(tree_->default_halo.width),
      block_->dbuToMicrons(tree_->default_halo.height),
      block_->dbuAreaToMicrons(tree_->macro_with_halo_area),
      block_->dbuAreaToMicrons(design_metrics_->getStdCellArea()
                               + design_metrics_->getMacroArea()),
      block_->dbuAreaToMicrons(tree_->floorplan_shape.area()),
      util,
      floorplan_util,
      block_->getTech()->getManufacturingGrid());
}

void ClusteringEngine::createRoot()
{
  tree_->root = std::make_unique<Cluster>(id_, std::string("root"), logger_);
  tree_->root->addDbModule(block_->getTopModule());
  tree_->root->setMetrics(*design_metrics_);

  tree_->maps.id_to_cluster[id_++] = tree_->root.get();

  // Associate all instances to root
  for (odb::dbInst* inst : block_->getInsts()) {
    tree_->maps.inst_to_cluster_id[inst] = id_;
  }
}

void ClusteringEngine::setBaseThresholds()
{
  // TODO: Allow hierarchical clustering (level > 1) with fixed macros.
  if (tree_->has_fixed_macros) {
    tree_->max_level = 1;
  }

  if (tree_->base_max_macro <= 0 || tree_->base_min_macro <= 0
      || tree_->base_max_std_cell <= 0 || tree_->base_min_std_cell <= 0) {
    // From original implementation: Reset maximum level based on number
    // of macros.
    const int min_num_macros_for_multilevel = 150;
    if (design_metrics_->getNumMacro() <= min_num_macros_for_multilevel) {
      tree_->max_level = 1;
      debugPrint(
          logger_,
          MPL,
          "multilevel_autoclustering",
          1,
          "Number of macros is below {}. Resetting number of levels to {}",
          min_num_macros_for_multilevel,
          tree_->max_level);
    }

    // Set base values for std cell lower/upper thresholds
    const int min_num_std_cells_allowed = 1000;
    tree_->base_min_std_cell
        = std::floor(design_metrics_->getNumStdCell()
                     / std::pow(tree_->cluster_size_ratio, tree_->max_level));
    tree_->base_min_std_cell
        = std::max(tree_->base_min_std_cell, min_num_std_cells_allowed);
    tree_->base_max_std_cell
        = tree_->base_min_std_cell * tree_->cluster_size_ratio / 2.0;

    // Set base values for macros lower/upper thresholds
    tree_->base_min_macro
        = std::floor(design_metrics_->getNumMacro()
                     / std::pow(tree_->cluster_size_ratio, tree_->max_level));
    if (tree_->base_min_macro <= 0) {
      tree_->base_min_macro = 1;
    }
    tree_->base_max_macro
        = tree_->base_min_macro * tree_->cluster_size_ratio / 2.0;
  }

  // Set sizes for root
  const unsigned coarsening_factor
      = std::pow(tree_->cluster_size_ratio, tree_->max_level - 1);
  tree_->base_max_macro *= coarsening_factor;
  tree_->base_min_macro *= coarsening_factor;
  tree_->base_max_std_cell *= coarsening_factor;
  tree_->base_min_std_cell *= coarsening_factor;

  debugPrint(
      logger_,
      MPL,
      "multilevel_autoclustering",
      1,
      "num level: {}, max_macro: {}, min_macro: {}, max_inst:{}, min_inst:{}",
      tree_->max_level,
      tree_->base_max_macro,
      tree_->base_min_macro,
      tree_->base_max_std_cell,
      tree_->base_min_std_cell);
}

// An IO Cluster may represent:
// 1. A Group of Unplaced Pins;
// 2. An IO Pad.
//
// For 1:
// We group IO pins that are constrained to the same region - the cluster's
// shape is the constraint region. If a pin has no constraints, we consider
// it constrained to all edges of the die area - for this case, the cluster's
// shape is the die area.
void ClusteringEngine::createIOClusters()
{
  if (!tree_->io_pads.empty()) {
    createIOPadClusters();
    return;
  }

  if (block_->getBTerms().empty()) {
    logger_->warn(MPL, 26, "Design has no IO pins!");
    tree_->has_io_clusters = false;
  }

  std::map<int, bool> is_empty_io_bundle;
  if (designHasFixedIOPins()) {
    first_io_bundle_id_ = id_;
    io_bundle_spans_ = computeIOBundleSpans();
    createIOBundles();
    for (const auto& child : tree_->root->getChildren()) {
      if (child->isIOBundle()) {
        is_empty_io_bundle[child->getId()] = true;
      }
    }
  }

  for (odb::dbBTerm* bterm : block_->getBTerms()) {
    if (bterm->getFirstPinPlacementStatus().isFixed()) {
      const int io_bundle_id = findAssociatedBundledIOId(bterm);
      tree_->maps.bterm_to_cluster_id[bterm] = io_bundle_id;
      is_empty_io_bundle.at(io_bundle_id) = false;
    } else {
      Cluster* same_constraint_cluster = findIOClusterWithSameConstraint(bterm);
      if (same_constraint_cluster) {
        tree_->maps.bterm_to_cluster_id[bterm]
            = same_constraint_cluster->getId();
      } else {
        createClusterOfUnplacedIOs(bterm);
      }
    }
  }

  if (graphics_) {
    graphics_->setIOConstraintsMap(tree_->io_cluster_to_constraint);
  }

  // Delete IO bundles without bterms associated to them.
  for (const auto& [id, is_empty] : is_empty_io_bundle) {
    if (is_empty) {
      auto itr = tree_->maps.id_to_cluster.find(id);
      Cluster* empty_io_bundle = itr->second;
      tree_->root->releaseChild(empty_io_bundle);
      tree_->maps.id_to_cluster.erase(itr);
    }
  }
}

bool ClusteringEngine::designHasFixedIOPins() const
{
  for (odb::dbBTerm* bterm : block_->getBTerms()) {
    if (bterm->getFirstPinPlacementStatus().isFixed()) {
      return true;
    }
  }

  return false;
}

IOBundleSpans ClusteringEngine::computeIOBundleSpans() const
{
  IOBundleSpans spans;
  const odb::Rect& die = block_->getDieArea();
  spans.x = die.dx() / tree_->io_bundles_per_edge;
  spans.y = die.dy() / tree_->io_bundles_per_edge;
  return spans;
}

// The order in which we create the io bundles in each edge is
// relevant for when we'll associate each to its io bundle bterm.
void ClusteringEngine::createIOBundles()
{
  createIOBundles(Boundary::L);
  createIOBundles(Boundary::T);
  createIOBundles(Boundary::R);
  createIOBundles(Boundary::B);
}

// Create IO bundles for a certain edge of the die area.
void ClusteringEngine::createIOBundles(Boundary boundary)
{
  for (int i = 0; i < tree_->io_bundles_per_edge; ++i) {
    createIOBundle(boundary, i);
  }
}

void ClusteringEngine::createIOBundle(Boundary boundary, const int bundle_index)
{
  const std::string name
      = fmt::format("{}_{}", toString(boundary), std::to_string(bundle_index));
  auto cluster = std::make_unique<Cluster>(id_, name, logger_);
  cluster->setParent(tree_->root.get());
  tree_->maps.id_to_cluster[id_++] = cluster.get();

  const odb::Rect& die = block_->getDieArea();
  const int bundle_extension
      = isVertical(boundary) ? io_bundle_spans_.y : io_bundle_spans_.x;

  int width = 0;
  int height = 0;
  int x = 0;
  int y = 0;

  switch (boundary) {
    case (Boundary::L): {
      x = die.xMin();
      y = die.yMin() + bundle_extension * bundle_index;
      height = bundle_extension;
      break;
    }
    case (Boundary::T): {
      x = die.xMin() + bundle_extension * bundle_index;
      y = die.yMax();
      width = bundle_extension;
      break;
    }
    case (Boundary::R): {
      x = die.xMax();
      y = die.yMax() - bundle_extension * (bundle_index + 1);
      height = bundle_extension;
      break;
    }
    case (Boundary::B): {
      x = die.xMax() - bundle_extension * (bundle_index + 1);
      y = die.yMin();
      width = bundle_extension;
      break;
    }
  }

  cluster->setAsIOBundle({x, y}, width, height);
  tree_->root->addChild(std::move(cluster));
}

int ClusteringEngine::findAssociatedBundledIOId(odb::dbBTerm* bterm) const
{
  const odb::Rect& bbox = bterm->getBBox();
  const odb::Rect& die = block_->getDieArea();
  int id = first_io_bundle_id_;

  if (bbox.xMin() <= die.xMin()) {  // Left
    const int dy = bbox.yCenter() - die.yMin();
    id += std::floor(dy / io_bundle_spans_.y);
  } else if (bbox.yMax() >= die.yMax()) {  // Top
    const int dx = bbox.xCenter() - die.xMin();
    id += tree_->io_bundles_per_edge + std::floor(dx / io_bundle_spans_.x);
  } else if (bbox.xMax() >= die.xMax()) {  // Right
    const int dy = die.yMax() - bbox.yCenter();
    id += (tree_->io_bundles_per_edge * 2)
          + std::floor(dy / io_bundle_spans_.y);
  } else if (bbox.yMin() <= die.yMin()) {  // Bottom
    const int dx = die.xMax() - bbox.xCenter();
    id += (tree_->io_bundles_per_edge * 3)
          + std::floor(dx / io_bundle_spans_.x);
  }

  return id;
}

Cluster* ClusteringEngine::findIOClusterWithSameConstraint(
    odb::dbBTerm* bterm) const
{
  const auto& bterm_constraint = bterm->getConstraintRegion();
  if (!bterm_constraint) {
    return cluster_of_unconstrained_io_pins_;
  }

  for (const auto& [cluster, constraint_region] :
       tree_->io_cluster_to_constraint) {
    if (bterm_constraint == lineToRect(constraint_region.line)) {
      return cluster;
    }
  }

  return nullptr;
}

void ClusteringEngine::createClusterOfUnplacedIOs(odb::dbBTerm* bterm)
{
  auto cluster
      = std::make_unique<Cluster>(id_, "ios_" + std::to_string(id_), logger_);
  cluster->setParent(tree_->root.get());

  bool is_cluster_of_unconstrained_io_pins = false;
  odb::Rect constraint_shape;
  const auto& bterm_constraint = bterm->getConstraintRegion();
  if (bterm_constraint) {
    constraint_shape = *bterm_constraint;
    BoundaryRegion constraint_region(
        rectToLine(block_, constraint_shape, logger_),
        getBoundary(block_, constraint_shape));
    tree_->io_cluster_to_constraint[cluster.get()] = constraint_region;
  } else {
    constraint_shape = block_->getDieArea();
    is_cluster_of_unconstrained_io_pins = true;
    cluster_of_unconstrained_io_pins_ = cluster.get();
  }

  cluster->setAsClusterOfUnplacedIOPins(
      {constraint_shape.xMin(), constraint_shape.yMin()},
      constraint_shape.dx(),
      constraint_shape.dy(),
      is_cluster_of_unconstrained_io_pins);

  tree_->maps.bterm_to_cluster_id[bterm] = id_;
  tree_->maps.id_to_cluster[id_++] = cluster.get();
  tree_->root->addChild(std::move(cluster));
}

void ClusteringEngine::createIOPadClusters()
{
  for (odb::dbInst* pad : tree_->io_pads) {
    createIOPadCluster(pad);
  }
}

void ClusteringEngine::createIOPadCluster(odb::dbInst* pad)
{
  auto cluster = std::make_unique<Cluster>(id_, pad->getName(), logger_);
  tree_->maps.inst_to_cluster_id[pad] = id_;
  tree_->maps.id_to_cluster[id_++] = cluster.get();

  const odb::Rect& pad_bbox = pad->getBBox()->getBox();

  cluster->setAsIOPadCluster(
      {pad_bbox.xMin(), pad_bbox.yMin()}, pad_bbox.dx(), pad_bbox.dy());
  tree_->root->addChild(std::move(cluster));
}

void ClusteringEngine::addIgnorableMacro(odb::dbInst* inst)
{
  if (!inst->isBlock()) {
    logger_->error(MPL,
                   7,
                   "Trying to add non-macro instance {} to ignorable macros.",
                   inst->getName());
  }

  ignorable_macros_.insert(inst);
}

bool ClusteringEngine::isIgnoredInst(odb::dbInst* inst)
{
  if (inst->isBlock()
      && (ignorable_macros_.find(inst) != ignorable_macros_.end())) {
    return true;
  }

  odb::dbMaster* master = inst->getMaster();
  return master->isPad() || master->isCover() || master->isEndCap();
}

void ClusteringEngine::connect(Cluster* a,
                               Cluster* b,
                               const float connection_weight) const
{
  a->addConnection(b, connection_weight);
  b->addConnection(a, connection_weight);
}

void ClusteringEngine::treatEachMacroAsSingleCluster()
{
  odb::dbModule* module = block_->getTopModule();
  for (odb::dbInst* inst : module->getInsts()) {
    if (isIgnoredInst(inst)) {
      continue;
    }

    if (inst->isBlock()) {
      const std::string cluster_name = inst->getName();
      auto cluster = std::make_unique<Cluster>(id_, cluster_name, logger_);
      cluster->addLeafMacro(inst);
      cluster->setClusterType(HardMacroCluster);
      incorporateNewCluster(std::move(cluster), tree_->root.get());

      debugPrint(logger_,
                 MPL,
                 "multilevel_autoclustering",
                 1,
                 "model {} as a cluster.",
                 cluster_name);
    }

    if (!tree_->has_io_clusters) {
      tree_->has_only_macros = true;
    }
  }
}

void ClusteringEngine::incorporateNewCluster(std::unique_ptr<Cluster> cluster,
                                             Cluster* parent)
{
  updateInstancesAssociation(cluster.get());
  setClusterMetrics(cluster.get());
  tree_->maps.id_to_cluster[id_++] = cluster.get();

  // modify physical hierarchy
  cluster->setParent(parent);
  parent->addChild(std::move(cluster));
}

// We don't change the association of ignored instances throughout
// the multilevel autoclustering process. I.e., ignored instances
// should remain at the root.
//
// Attention: A pad is a special ignored instance in the sense that
// it will be associated with a pad cluster. By not changing the
// association of ignored instances during autoclustering, we prevent
// the autoclustering process from messing up the association of
// PAD inst -> PAD Cluster as these last ones are created beforehand.
void ClusteringEngine::updateInstancesAssociation(Cluster* cluster)
{
  const int cluster_id = cluster->getId();
  const ClusterType cluster_type = cluster->getClusterType();
  if (cluster_type == HardMacroCluster || cluster_type == MixedCluster) {
    for (odb::dbInst* inst : cluster->getLeafMacros()) {
      if (isIgnoredInst(inst)) {
        continue;
      }

      tree_->maps.inst_to_cluster_id[inst] = cluster_id;
    }
  }

  if (cluster_type == StdCellCluster || cluster_type == MixedCluster) {
    for (odb::dbInst* inst : cluster->getLeafStdCells()) {
      if (isIgnoredInst(inst)) {
        continue;
      }

      tree_->maps.inst_to_cluster_id[inst] = cluster_id;
    }
  }

  // Note: macro clusters have no module.
  if (cluster_type == StdCellCluster) {
    for (odb::dbModule* module : cluster->getDbModules()) {
      updateInstancesAssociation(module, cluster_id, false);
    }
  } else if (cluster_type == MixedCluster) {
    for (odb::dbModule* module : cluster->getDbModules()) {
      updateInstancesAssociation(module, cluster_id, true);
    }
  }
}

// Unlike macros, std cells are always considered when when updating
// the inst -> cluster map with the data from a module.
void ClusteringEngine::updateInstancesAssociation(odb::dbModule* module,
                                                  int cluster_id,
                                                  bool include_macro)
{
  for (odb::dbInst* inst : module->getInsts()) {
    if (isIgnoredInst(inst)) {
      continue;
    }

    if (!include_macro && inst->isBlock()) {
      continue;
    }

    tree_->maps.inst_to_cluster_id[inst] = cluster_id;
  }

  for (odb::dbModInst* child_module_inst : module->getChildren()) {
    updateInstancesAssociation(
        child_module_inst->getMaster(), cluster_id, include_macro);
  }
}

void ClusteringEngine::setClusterMetrics(Cluster* cluster)
{
  int64_t std_cell_area = 0;
  for (odb::dbInst* std_cell : cluster->getLeafStdCells()) {
    std_cell_area += computeArea(std_cell);
  }

  int64_t macro_area = 0;
  for (odb::dbInst* macro : cluster->getLeafMacros()) {
    macro_area += computeArea(macro);
  }

  const unsigned int num_std_cell = cluster->getLeafStdCells().size();
  const unsigned int num_macro = cluster->getLeafMacros().size();

  Metrics metrics(num_std_cell, num_macro, std_cell_area, macro_area);
  for (auto& module : cluster->getDbModules()) {
    metrics.addMetrics(*tree_->maps.module_to_metrics[module]);
  }
  cluster->setMetrics(metrics);

  debugPrint(logger_,
             MPL,
             "multilevel_autoclustering",
             1,
             "Setting Cluster Metrics for {}: Num Macros: {} Num Std Cells: {}",
             cluster->getName(),
             metrics.getNumMacro(),
             metrics.getNumStdCell());
}

int64_t ClusteringEngine::computeArea(odb::dbInst* inst)
{
  return inst->getBBox()->getBox().area();
}

// Post-order DFS for clustering
void ClusteringEngine::multilevelAutocluster(Cluster* parent)
{
  bool force_split_root = false;
  if (level_ == 0) {
    const int leaf_max_std_cell
        = tree_->base_max_std_cell
          / std::pow(tree_->cluster_size_ratio, tree_->max_level - 1);
    if (parent->getNumStdCell() < leaf_max_std_cell) {
      force_split_root = true;
      debugPrint(logger_,
                 MPL,
                 "multilevel_autoclustering",
                 1,
                 "Root number of std cells ({}) is below leaf cluster max "
                 "({}). Root will be force split.",
                 parent->getNumStdCell(),
                 leaf_max_std_cell);
    }
  }

  if (level_ >= tree_->max_level) {
    return;
  }

  if (logger_->debugCheck(MPL, "multilevel_autoclustering", 1)) {
    logger_->report("\nCurrent cluster: {}\n  Macros: {}\n  Std Cells: {}",
                    parent->getName(),
                    parent->getNumMacro(),
                    parent->getNumStdCell());
  }

  level_++;
  updateSizeThresholds();

  if (force_split_root || (parent->getNumStdCell() > max_std_cell_)
      || (parent->getNumMacro() > max_macro_)) {
    breakCluster(parent);
    updateSubTree(parent);

    for (auto& child : parent->getChildren()) {
      updateInstancesAssociation(child.get());
    }

    for (auto& child : parent->getChildren()) {
      debugPrint(logger_,
                 MPL,
                 "multilevel_autoclustering",
                 1,
                 "\tChild Cluster: {}",
                 child->getName());
      multilevelAutocluster(child.get());
    }
  } else {
    multilevelAutocluster(parent);
  }

  updateInstancesAssociation(parent);
  level_--;
}

void ClusteringEngine::updateSizeThresholds()
{
  const double coarse_factor = std::pow(tree_->cluster_size_ratio, level_ - 1);

  // A high cluster size ratio per level helps the
  // clustering process converge fast
  max_macro_ = tree_->base_max_macro / coarse_factor;
  min_macro_ = tree_->base_min_macro / coarse_factor;
  max_std_cell_ = tree_->base_max_std_cell / coarse_factor;
  min_std_cell_ = tree_->base_min_std_cell / coarse_factor;

  if (min_macro_ <= 0) {
    min_macro_ = 1;
    max_macro_ = min_macro_ * tree_->cluster_size_ratio / 2.0;
  }

  if (min_std_cell_ <= 0) {
    min_std_cell_ = 100;
    max_std_cell_ = min_std_cell_ * tree_->cluster_size_ratio / 2.0;
  }

  if (logger_->debugCheck(MPL, "multilevel_autoclustering", 1)) {
    reportThresholds();
  }
}

// We expand the parent cluster into a subtree based on logical
// hierarchy in a DFS manner.  During the expansion process,
// we merge small clusters in the same logical hierarchy
void ClusteringEngine::breakCluster(Cluster* parent)
{
  debugPrint(logger_,
             MPL,
             "multilevel_autoclustering",
             1,
             "Breaking Cluster: {}",
             parent->getName());

  if (parent->isEmpty()) {
    return;
  }

  if (parent->correspondsToLogicalModule()) {
    odb::dbModule* module = parent->getDbModules().front();
    // Flat module that will be partitioned with TritonPart when updating
    // the subtree later on.
    if (module->getChildren().empty()) {
      if (parent == tree_->root.get()) {
        createFlatCluster(module, parent);
      } else {
        addModuleLeafInstsToCluster(parent, module);
        parent->clearDbModules();
        updateInstancesAssociation(parent);
      }
      return;
    }

    for (odb::dbModInst* child_module_inst : module->getChildren()) {
      createCluster(child_module_inst->getMaster(), parent);
    }
    createFlatCluster(module, parent);
  } else {
    // Parent is a cluster generated by merging small clusters:
    // It may have a few logical modules or many glue insts.
    for (odb::dbModule* module : parent->getDbModules()) {
      createCluster(module, parent);
    }

    if (!parent->getLeafStdCells().empty()
        || !parent->getLeafMacros().empty()) {
      createCluster(parent);
    }
  }

  // Recursively break down non-flat large clusters with logical modules
  for (auto& child : parent->getChildren()) {
    if (!child->getDbModules().empty()) {
      if (child->getNumStdCell() > max_std_cell_
          || child->getNumMacro() > max_macro_) {
        breakCluster(child.get());
      }
    }
  }

  std::vector<Cluster*> small_children;
  for (auto& child : parent->getChildren()) {
    if (!child->isIOCluster() && child->getNumStdCell() < min_std_cell_
        && child->getNumMacro() < min_macro_) {
      small_children.push_back(child.get());
    }
  }

  mergeChildrenBelowThresholds(small_children);

  // Update the cluster_id
  // This is important to maintain the clustering results
  updateInstancesAssociation(parent);
}

// A flat cluster is a cluster created from the leaf instances of a module.
void ClusteringEngine::createFlatCluster(odb::dbModule* module, Cluster* parent)
{
  const std::string cluster_name
      = std::string("(") + parent->getName() + ")_glue_logic";
  auto cluster = std::make_unique<Cluster>(id_, cluster_name, logger_);
  addModuleLeafInstsToCluster(cluster.get(), module);

  bool empty_leaf_instances
      = cluster->getLeafStdCells().empty() && cluster->getLeafMacros().empty();

  if (!empty_leaf_instances) {
    incorporateNewCluster(std::move(cluster), parent);
  }  // The cluster will be deleted otherwise
}

void ClusteringEngine::addModuleLeafInstsToCluster(Cluster* cluster,
                                                   odb::dbModule* module)
{
  for (odb::dbInst* inst : module->getInsts()) {
    if (isIgnoredInst(inst)) {
      continue;
    }
    cluster->addLeafInst(inst);
  }
}

// Map a module to a cluster.
void ClusteringEngine::createCluster(odb::dbModule* module, Cluster* parent)
{
  Metrics* module_metrics = tree_->maps.module_to_metrics.at(module).get();
  if (module_metrics->empty()) {
    return;
  }

  const std::string cluster_name = module->getHierarchicalName();
  auto cluster = std::make_unique<Cluster>(id_, cluster_name, logger_);
  cluster->addDbModule(module);
  incorporateNewCluster(std::move(cluster), parent);
}

void ClusteringEngine::createCluster(Cluster* parent)
{
  const std::string cluster_name
      = std::string("(") + parent->getName() + ")_glue_logic";
  auto cluster = std::make_unique<Cluster>(id_, cluster_name, logger_);
  for (odb::dbInst* std_cell : parent->getLeafStdCells()) {
    cluster->addLeafStdCell(std_cell);
  }
  for (odb::dbInst* macro : parent->getLeafMacros()) {
    cluster->addLeafMacro(macro);
  }

  incorporateNewCluster(std::move(cluster), parent);
}

// This function has two purposes:
// 1) Remove internal clusters between parent and leaf clusters in its subtree.
// 2) Call TritonPart to partition large flat clusters.
void ClusteringEngine::updateSubTree(Cluster* parent)
{
  UniqueClusterVector children_clusters;
  UniqueClusterVector internal_clusters;
  UniqueClusterQueue wavefront;
  for (auto& child : parent->releaseChildren()) {
    wavefront.push(std::move(child));
  }

  while (!wavefront.empty()) {
    std::unique_ptr<Cluster> cluster = std::move(wavefront.front());
    wavefront.pop();
    if (cluster->getChildren().empty()) {
      children_clusters.push_back(std::move(cluster));
    } else {
      for (auto& child : cluster->releaseChildren()) {
        wavefront.push(std::move(child));
      }
      internal_clusters.push_back(std::move(cluster));
    }
  }

  for (auto& cluster : internal_clusters) {
    // Internal clusters will be deleted automatically.
    tree_->maps.id_to_cluster.erase(cluster->getId());
  }

  parent->addChildren(std::move(children_clusters));

  // Note that use a copy of the current children's list in order to be
  // able to use a range-based loop. That is needed, because the parent's
  // children will change if a child of the list is broken.
  std::vector<Cluster*> new_children = parent->getRawChildren();
  for (Cluster* new_child : new_children) {
    new_child->setParent(parent);
    if (isLargeFlatCluster(new_child)) {
      breakLargeFlatCluster(new_child);
    }
  }
}

bool ClusteringEngine::isLargeFlatCluster(const Cluster* cluster) const
{
  return (cluster->getDbModules().empty()
          && (cluster->getLeafStdCells().size() > max_std_cell_
              || cluster->getLeafMacros().size() > max_macro_));
}

// Break large flat clusters with TritonPart
// Binary coding method to differentiate partitions:
// cluster -> cluster_0, cluster_1
// cluster_0 -> cluster_0_0, cluster_0_1
// cluster_1 -> cluster_1_0, cluster_1_1 [...]
void ClusteringEngine::breakLargeFlatCluster(Cluster* parent)
{
  updateInstancesAssociation(parent);

  std::map<int, int> cluster_vertex_id_map;
  std::vector<float> vertex_weight;
  int vertex_id = 0;
  for (auto& [cluster_id, cluster] : tree_->maps.id_to_cluster) {
    cluster_vertex_id_map[cluster_id] = vertex_id++;
    vertex_weight.push_back(0.0f);
  }
  const int num_other_cluster_vertices = vertex_id;

  std::vector<odb::dbInst*> insts;
  std::map<odb::dbInst*, int> inst_vertex_id_map;
  for (auto& macro : parent->getLeafMacros()) {
    inst_vertex_id_map[macro] = vertex_id++;
    vertex_weight.push_back(block_->dbuAreaToMicrons(computeArea(macro)));
    insts.push_back(macro);
  }
  for (auto& std_cell : parent->getLeafStdCells()) {
    inst_vertex_id_map[std_cell] = vertex_id++;
    vertex_weight.push_back(block_->dbuAreaToMicrons(computeArea(std_cell)));
    insts.push_back(std_cell);
  }

  std::vector<std::vector<int>> hyperedges;
  for (odb::dbNet* net : block_->getNets()) {
    if (net->getSigType().isSupply()) {
      continue;
    }

    int driver_id = -1;
    std::set<int> loads_id;
    bool ignore = false;
    for (odb::dbITerm* iterm : net->getITerms()) {
      odb::dbInst* inst = iterm->getInst();
      if (isIgnoredInst(inst)) {
        ignore = true;
        break;
      }

      const int cluster_id = tree_->maps.inst_to_cluster_id.at(inst);
      int vertex_id = (cluster_id != parent->getId())
                          ? cluster_vertex_id_map[cluster_id]
                          : inst_vertex_id_map[inst];
      if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
        driver_id = vertex_id;
      } else {
        loads_id.insert(vertex_id);
      }
    }

    if (ignore) {
      continue;
    }

    for (odb::dbBTerm* bterm : net->getBTerms()) {
      const int cluster_id = tree_->maps.bterm_to_cluster_id.at(bterm);
      if (bterm->getIoType() == odb::dbIoType::INPUT) {
        driver_id = cluster_vertex_id_map[cluster_id];
      } else {
        loads_id.insert(cluster_vertex_id_map[cluster_id]);
      }
    }
    loads_id.insert(driver_id);
    if (driver_id != -1 && loads_id.size() > 1
        && loads_id.size() < tree_->large_net_threshold) {
      std::vector<int> hyperedge;
      hyperedge.insert(hyperedge.end(), loads_id.begin(), loads_id.end());
      hyperedges.push_back(std::move(hyperedge));
    }
  }

  const int seed = 0;
  constexpr float default_balance_constraint = 1.0f;
  float balance_constraint = default_balance_constraint;
  const int num_parts = 2;  // We use two-way partitioning here
  const int num_vertices = static_cast<int>(vertex_weight.size());
  std::vector<float> hyperedge_weights(hyperedges.size(), 1.0f);

  // Due to the discrepancy that may exist between the weight of vertices
  // that represent macros/std cells, the partitioner may fail to meet the
  // balance constraint. This may cause the output to be completely unbalanced
  // and lead to infinite partitioning recursion. To handle that, we relax
  // the constraint until we find a reasonable split.
  constexpr float balance_constraint_relaxation_factor = 10.0f;
  std::vector<int> solution;
  do {
    debugPrint(
        logger_,
        MPL,
        "multilevel_autoclustering",
        1,
        "Attempting flat cluster {} partitioning with balance constraint = {}",
        parent->getName(),
        balance_constraint);

    if (balance_constraint >= 90) {
      logger_->error(
          MPL, 45, "Cannot find a balanced partitioning for the clusters.");
    }

    solution = triton_part_->PartitionKWaySimpleMode(num_parts,
                                                     balance_constraint,
                                                     seed,
                                                     hyperedges,
                                                     vertex_weight,
                                                     hyperedge_weights);

    balance_constraint += balance_constraint_relaxation_factor;
  } while (partitionerSolutionIsFullyUnbalanced(solution,
                                                num_other_cluster_vertices));

  parent->clearLeafStdCells();
  parent->clearLeafMacros();

  const std::string cluster_name = parent->getName();
  parent->setName(cluster_name + std::string("_0"));
  auto cluster_part_1 = std::make_unique<Cluster>(
      id_, cluster_name + std::string("_1"), logger_);

  for (int i = num_other_cluster_vertices; i < num_vertices; i++) {
    odb::dbInst* inst = insts[i - num_other_cluster_vertices];
    if (solution[i] == 0) {
      parent->addLeafInst(inst);
    } else {
      cluster_part_1->addLeafInst(inst);
    }
  }

  Cluster* raw_part_1 = cluster_part_1.get();

  updateInstancesAssociation(parent);
  setClusterMetrics(parent);
  incorporateNewCluster(std::move(cluster_part_1), parent->getParent());

  // Recursive break the cluster
  // until the size of the cluster is less than max_num_inst_
  if (isLargeFlatCluster(parent)) {
    breakLargeFlatCluster(parent);
  }

  if (isLargeFlatCluster(raw_part_1)) {
    breakLargeFlatCluster(raw_part_1);
  }
}

bool ClusteringEngine::partitionerSolutionIsFullyUnbalanced(
    const std::vector<int>& solution,
    const int num_other_cluster_vertices)
{
  // The partition of the first vertex which represents
  // an actual macro or std cell.
  const int first_vertex_partition = solution[num_other_cluster_vertices];
  const int number_of_vertices = static_cast<int>(solution.size());

  // Skip all the vertices that represent other clusters.
  for (int vertex_id = num_other_cluster_vertices;
       vertex_id < number_of_vertices;
       ++vertex_id) {
    if (solution[vertex_id] != first_vertex_partition) {
      return false;
    }
  }

  return true;
}

// Recursively merge children whose number of std cells and macro
// is below the current level thresholds. There are three cases:
// 1) Children are closely connected.
// 2) Children have the same connection signature.
// 3) "Dust" children that were not merged in the previous cases
//    which are made of very few std cells (< 10)
void ClusteringEngine::mergeChildrenBelowThresholds(
    std::vector<Cluster*>& small_children)
{
  if (small_children.empty()) {
    return;
  }

  int merge_iter = 0;
  debugPrint(logger_,
             MPL,
             "multilevel_autoclustering",
             1,
             "Merge Cluster Iter: {}",
             merge_iter++);
  for (auto& small_child : small_children) {
    debugPrint(logger_,
               MPL,
               "multilevel_autoclustering",
               1,
               "Cluster: {}, num std cell: {}, num macros: {}",
               small_child->getName(),
               small_child->getNumStdCell(),
               small_child->getNumMacro());
  }

  int num_small_children = static_cast<int>(small_children.size());
  while (true) {
    clearConnections();
    buildNetListConnections();

    std::vector<int> cluster_class(num_small_children, -1);  // merge flag
    std::vector<int> small_children_ids;                     // store cluster id
    small_children_ids.reserve(num_small_children);
    for (auto& small_child : small_children) {
      small_children_ids.push_back(small_child->getId());
    }

    // Firstly we perform Type 1 merge
    for (int i = 0; i < num_small_children; i++) {
      Cluster* close_cluster = findSingleWellFormedConnectedCluster(
          small_children[i], small_children_ids);
      if (close_cluster != nullptr
          && mergeHonorsMaxThresholds(close_cluster, small_children[i])
          && attemptMerge(close_cluster, small_children[i])) {
        cluster_class[i] = close_cluster->getId();
      }
    }

    // Then we perform Type 2 merge
    for (int i = 0; i < num_small_children; i++) {
      if (cluster_class[i] == -1) {  // the cluster has not been merged
        for (int j = i + 1; j < num_small_children; j++) {
          if (cluster_class[j] != -1) {
            continue;
          }

          if (mergeHonorsMaxThresholds(small_children[i], small_children[j])
              && sameConnectionSignature(small_children[i],
                                         small_children[j])) {
            if (attemptMerge(small_children[i], small_children[j])) {
              cluster_class[j] = i;
            } else {
              logger_->critical(
                  MPL,
                  23,
                  "Merge attempt between siblings {} and {} with same "
                  "connection signature failed!",
                  small_children[i]->getName(),
                  small_children[j]->getName());
            }
          }
        }
      }
    }

    // Then we perform Type 3 merge:  merge all dust cluster
    std::vector<Cluster*> new_small_children;
    const int dust_cluster_std_cell = 10;
    for (int i = 0; i < num_small_children; i++) {
      if (cluster_class[i] == -1) {  // the cluster has not been merged
        new_small_children.push_back(small_children[i]);
        if (small_children[i]->getNumStdCell() <= dust_cluster_std_cell
            && small_children[i]->getNumMacro() == 0) {
          for (int j = i + 1; j < num_small_children; j++) {
            if (cluster_class[j] != -1 || small_children[j]->getNumMacro() > 0
                || small_children[j]->getNumStdCell() > dust_cluster_std_cell) {
              continue;
            }

            if (attemptMerge(small_children[i], small_children[j])) {
              cluster_class[j] = i;
            } else {
              logger_->critical(
                  MPL,
                  24,
                  "Merge attempt between dust siblings {} and {} failed!",
                  small_children[i]->getName(),
                  small_children[j]->getName());
            }
          }
        }
      }
    }

    // Some small children have become well-formed clusters
    small_children.clear();
    for (Cluster* new_small_child : new_small_children) {
      if (new_small_child->getNumStdCell() < min_std_cell_
          && new_small_child->getNumMacro() < min_macro_) {
        small_children.push_back(new_small_child);
      }
    }

    // If no more clusters have been merged, exit the merging loop
    if (num_small_children == static_cast<int>(new_small_children.size())) {
      break;
    }

    num_small_children = small_children.size();

    debugPrint(logger_,
               MPL,
               "multilevel_autoclustering",
               1,
               "Merge Cluster Iter: {}",
               merge_iter++);
    for (auto& small_child : small_children) {
      debugPrint(logger_,
                 MPL,
                 "multilevel_autoclustering",
                 1,
                 "Cluster: {}",
                 small_child->getName());
    }
    // merge small clusters
    if (small_children.empty()) {
      break;
    }
  }
  debugPrint(logger_,
             MPL,
             "multilevel_autoclustering",
             1,
             "Finished merging clusters");
}

bool ClusteringEngine::mergeHonorsMaxThresholds(const Cluster* a,
                                                const Cluster* b) const
{
  return ((a->getNumMacro() + b->getNumMacro()) <= max_macro_)
         && ((a->getNumStdCell() + b->getNumStdCell()) <= max_std_cell_);
}

bool ClusteringEngine::sameConnectionSignature(Cluster* a, Cluster* b) const
{
  std::vector<int> a_neighbors = findNeighbors(a, /* ignore */ b);
  if (a_neighbors.empty()) {
    return false;
  }

  std::vector<int> b_neighbors = findNeighbors(b, /* ignore */ a);
  if (b_neighbors.size() != a_neighbors.size()) {
    return false;
  }

  std::ranges::sort(a_neighbors);
  std::ranges::sort(b_neighbors);

  for (int i = 0; i < a_neighbors.size(); i++) {
    if (a_neighbors[i] != b_neighbors[i]) {
      return false;
    }
  }

  return true;
}

std::vector<int> ClusteringEngine::findNeighbors(Cluster* target_cluster,
                                                 Cluster* ignored_cluster) const
{
  std::vector<int> neighbors;
  const ConnectionsMap& target_connections
      = target_cluster->getConnectionsMap();

  for (const auto& [cluster_id, connection_weight] : target_connections) {
    if (cluster_id == target_cluster->getId()) {
      logger_->error(MPL,
                     53,
                     "Cluster {} is connected to itself.",
                     target_cluster->getName());
    }

    if (cluster_id == ignored_cluster->getId()) {
      continue;
    }

    const float connection_ratio
        = connection_weight / target_cluster->allConnectionsWeight();

    if (connection_ratio >= minimum_connection_ratio_) {
      neighbors.push_back(cluster_id);
    }
  }

  return neighbors;
}

bool ClusteringEngine::strongConnection(Cluster* a,
                                        Cluster* b,
                                        const float* connection_weight) const
{
  if (a == b) {
    logger_->error(
        MPL,
        61,
        "Attempt to evaluate if cluster {} has strong connection with itself.",
        a->getName());
  }

  // Attention that we need to subtract the weight of the connection that
  // we're evaluating otherwise we'll be taking it into account twice.
  float total_weight = a->allConnectionsWeight() + b->allConnectionsWeight();
  float connection_ratio = 0.0;
  if (connection_weight) {
    total_weight -= *connection_weight;
    connection_ratio = *connection_weight / total_weight;
  } else {
    const ConnectionsMap& a_connections = a->getConnectionsMap();
    auto itr = a_connections.find(b->getId());

    if (itr != a_connections.end()) {
      const float conn_weight = itr->second;
      total_weight -= conn_weight;
      connection_ratio = conn_weight / total_weight;
    }
  }

  return connection_ratio >= minimum_connection_ratio_;
}

Cluster* ClusteringEngine::findSingleWellFormedConnectedCluster(
    Cluster* target_cluster,
    const std::vector<int>& small_clusters_id_list) const
{
  int number_of_close_clusters = 0;
  Cluster* close_cluster = nullptr;
  const ConnectionsMap& target_connections
      = target_cluster->getConnectionsMap();

  for (auto& [cluster_id, connection_weight] : target_connections) {
    Cluster* candidate = tree_->maps.id_to_cluster.at(cluster_id);

    if (candidate->isIOCluster()) {
      continue;
    }

    if (strongConnection(target_cluster, candidate, &connection_weight)) {
      auto small_child_found
          = std::ranges::find(small_clusters_id_list, cluster_id);

      // A small child is not well-formed, so we avoid them.
      if (small_child_found == small_clusters_id_list.end()) {
        number_of_close_clusters++;
        close_cluster = candidate;
      }
    }
  }

  if (number_of_close_clusters == 1) {
    return close_cluster;
  }

  return nullptr;
}

bool ClusteringEngine::attemptMerge(Cluster* receiver, Cluster* incomer)
{
  // Cache incomer data in case it is deleted.
  const int incomer_id = incomer->getId();
  const ConnectionsMap incomer_connections = incomer->getConnectionsMap();

  bool incomer_deleted = false;
  if (receiver->attemptMerge(incomer, incomer_deleted)) {
    if (incomer_deleted) {
      tree_->maps.id_to_cluster.erase(incomer_id);

      // Update connections of clusters connected to the deleted cluster.
      for (const auto& [cluster_id, connection_weight] : incomer_connections) {
        Cluster* cluster = tree_->maps.id_to_cluster.at(cluster_id);
        cluster->removeConnection(incomer_id);

        // If the incomer and the receiver were connected, we forget that
        // connection, otherwise we'll end up with the receiver connected
        // to itself.
        if (cluster_id != receiver->getId()) {
          cluster->addConnection(receiver, connection_weight);
        }
      }
    }

    updateInstancesAssociation(receiver);
    setClusterMetrics(receiver);

    return true;
  }

  return false;
}

void ClusteringEngine::rebuildConnections()
{
  clearConnections();
  buildNetListConnections();
}

void ClusteringEngine::clearConnections()
{
  for (auto& [cluster_id, cluster] : tree_->maps.id_to_cluster) {
    cluster->initConnection();
  }
}

void ClusteringEngine::buildNetListConnections()
{
  for (odb::dbNet* db_net : block_->getNets()) {
    if (!isValidNet(db_net)) {
      continue;
    }

    Net net = buildNet(db_net);
    connectClusters(net);
  }
}

ClusteringEngine::Net ClusteringEngine::buildNet(odb::dbNet* db_net) const
{
  Net net;

  for (odb::dbITerm* iterm : db_net->getITerms()) {
    odb::dbInst* inst = iterm->getInst();
    const int cluster_id = tree_->maps.inst_to_cluster_id.at(inst);

    if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
      net.driver_id = cluster_id;
    } else {
      net.loads_ids.push_back(cluster_id);
    }
  }

  if (tree_->io_pads.empty()) {
    for (odb::dbBTerm* bterm : db_net->getBTerms()) {
      const int cluster_id = tree_->maps.bterm_to_cluster_id.at(bterm);

      if (bterm->getIoType() == odb::dbIoType::INPUT) {
        net.driver_id = cluster_id;
      } else {
        net.loads_ids.push_back(cluster_id);
      }
    }
  }

  return net;
}

void ClusteringEngine::connectClusters(const Net& net)
{
  if (net.driver_id == -1 || net.loads_ids.empty()
      || net.loads_ids.size() >= tree_->large_net_threshold) {
    return;
  }

  const float connection_weight = 1.0;
  Cluster* driver = tree_->maps.id_to_cluster.at(net.driver_id);

  for (const int load_cluster_id : net.loads_ids) {
    if (load_cluster_id != net.driver_id) {
      Cluster* load = tree_->maps.id_to_cluster.at(load_cluster_id);
      connect(driver, load, connection_weight);
    }
  }
}

bool ClusteringEngine::isValidNet(odb::dbNet* net)
{
  if (net->getSigType().isSupply()) {
    return false;
  }

  for (odb::dbITerm* iterm : net->getITerms()) {
    if (!isIgnoredInst(iterm->getInst())) {
      return true;
    }
  }

  return false;
}

void ClusteringEngine::fetchMixedLeaves(
    Cluster* parent,
    std::vector<std::vector<Cluster*>>& mixed_leaves)
{
  std::vector<Cluster*> sister_mixed_leaves;

  for (auto& child : parent->getChildren()) {
    updateInstancesAssociation(child.get());

    if (child->getNumMacro() == 0) {
      child->setClusterType(StdCellCluster);
    }

    if (child->getChildren().empty()) {
      if (child->getClusterType() != StdCellCluster) {
        sister_mixed_leaves.push_back(child.get());
      }
    } else {
      fetchMixedLeaves(child.get(), mixed_leaves);
    }
  }

  // We push the leaves after finishing searching the children so
  // that each vector of clusters represents the children of one
  // parent.
  mixed_leaves.push_back(std::move(sister_mixed_leaves));
}

void ClusteringEngine::breakMixedLeaves(
    const std::vector<std::vector<Cluster*>>& mixed_leaves)
{
  for (const std::vector<Cluster*>& sister_mixed_leaves : mixed_leaves) {
    if (!sister_mixed_leaves.empty()) {
      Cluster* parent = sister_mixed_leaves.front()->getParent();
      for (Cluster* mixed_leaf : sister_mixed_leaves) {
        breakMixedLeaf(mixed_leaf);
      }

      updateInstancesAssociation(parent);
    }
  }
}

// "Split" mixed leaf by replacing it for standard-cell and macro
// clusters. Macro clusters are formed by grouping the macros in
// the mixed leaf according to footprint, connection signature and
// interconnectivity.
void ClusteringEngine::breakMixedLeaf(Cluster* mixed_leaf)
{
  Cluster* parent = mixed_leaf->getParent();

  mapMacroInCluster2HardMacro(mixed_leaf);

  std::vector<HardMacro*> hard_macros = mixed_leaf->getHardMacros();
  std::vector<Cluster*> macro_clusters;
  createOneClusterForEachMacro(parent, hard_macros, macro_clusters);

  clearConnections();
  buildNetListConnections();

  std::vector<HardMacro*> movable_hard_macros;
  std::vector<Cluster*> movable_macro_clusters;
  std::vector<Cluster*> fixed_macro_clusters;
  for (Cluster* macro_cluster : macro_clusters) {
    odb::dbInst* macro = macro_cluster->getLeafMacros().front();
    HardMacro* hard_macro = tree_->maps.inst_to_hard.at(macro).get();
    if (!hard_macro->isFixed()) {
      movable_hard_macros.push_back(hard_macro);
      movable_macro_clusters.push_back(macro_cluster);
    } else {
      fixed_macro_clusters.push_back(macro_cluster);
    }
  }

  const int number_of_movable_macros
      = static_cast<int>(movable_hard_macros.size());
  std::vector<int> size_class(number_of_movable_macros, -1);
  std::vector<int> signature_class(number_of_movable_macros, -1);
  std::vector<int> interconn_class(number_of_movable_macros, -1);
  std::vector<int> macro_class(number_of_movable_macros, -1);

  if (number_of_movable_macros == 1) {
    // We don't want the single-macro macro cluster to be treated
    // as an array of interconnected macros with one macro.
    interconn_class.front() = -1;
    macro_class.front() = 0;
  } else {
    classifyMacrosBySize(movable_hard_macros, size_class);
    classifyMacrosByConnSignature(movable_macro_clusters, signature_class);
    classifyMacrosByInterconn(movable_macro_clusters, interconn_class);
    groupSingleMacroClusters(movable_macro_clusters,
                             size_class,
                             signature_class,
                             interconn_class,
                             macro_class);
  }

  mixed_leaf->clearHardMacros();

  // IMPORTANT: Restore the structure of physical hierarchical tree. Thus the
  // order of leaf clusters will not change the final macro grouping results.
  updateInstancesAssociation(mixed_leaf);

  // Never use SetInstProperty in the following lines for the reason above!
  std::vector<int> virtual_conn_clusters;

  replaceByStdCellCluster(mixed_leaf, virtual_conn_clusters);

  // Deal with the movable macros.
  for (int i = 0; i < macro_class.size(); i++) {
    if (macro_class[i] != i) {
      continue;  // this macro cluster has been merged
    }

    Cluster* movable_macro_cluster = movable_macro_clusters[i];
    movable_macro_cluster->setAsMacroArray();

    if (interconn_class[i] != -1) {
      movable_macro_cluster->setAsArrayOfInterconnectedMacros();
    }

    movable_macro_cluster->setClusterType(HardMacroCluster);
    setClusterMetrics(movable_macro_cluster);
    virtual_conn_clusters.push_back(movable_macro_cluster->getId());
  }

  // Deal with the fixed macros.
  for (Cluster* fixed_macro_cluster : fixed_macro_clusters) {
    fixed_macro_cluster->setClusterType(HardMacroCluster);
    setClusterMetrics(fixed_macro_cluster);
    virtual_conn_clusters.push_back(fixed_macro_cluster->getId());
  }

  // add virtual connections
  for (int i = 0; i < virtual_conn_clusters.size(); i++) {
    for (int j = i + 1; j < virtual_conn_clusters.size(); j++) {
      parent->addVirtualConnection(virtual_conn_clusters[i],
                                   virtual_conn_clusters[j]);
    }
  }
}

// Map all the macros into their HardMacro objects for all the clusters
void ClusteringEngine::mapMacroInCluster2HardMacro(Cluster* cluster)
{
  if (cluster->getClusterType() == StdCellCluster) {
    return;
  }

  std::vector<HardMacro*> hard_macros;
  for (const auto& inst : cluster->getLeafMacros()) {
    hard_macros.push_back(tree_->maps.inst_to_hard.at(inst).get());
  }
  for (const auto& module : cluster->getDbModules()) {
    getHardMacros(module, hard_macros);
  }
  cluster->specifyHardMacros(hard_macros);

  for (HardMacro* hard_macro : hard_macros) {
    hard_macro->setCluster(cluster);
  }
}

// Get all the hard macros in a logical module
void ClusteringEngine::getHardMacros(odb::dbModule* module,
                                     std::vector<HardMacro*>& hard_macros)
{
  for (odb::dbInst* inst : module->getInsts()) {
    if (isIgnoredInst(inst)) {
      continue;
    }

    if (inst->isBlock()) {
      hard_macros.push_back(tree_->maps.inst_to_hard.at(inst).get());
    }
  }

  for (odb::dbModInst* child_module_inst : module->getChildren()) {
    getHardMacros(child_module_inst->getMaster(), hard_macros);
  }
}

void ClusteringEngine::createOneClusterForEachMacro(
    Cluster* mixed_leaf_parent,
    const std::vector<HardMacro*>& hard_macros,
    std::vector<Cluster*>& macro_clusters)
{
  for (auto& hard_macro : hard_macros) {
    const std::string& cluster_name = hard_macro->getName();
    auto single_macro_cluster
        = std::make_unique<Cluster>(id_, cluster_name, logger_);
    single_macro_cluster->addLeafMacro(hard_macro->getInst());
    macro_clusters.push_back(single_macro_cluster.get());

    Cluster* new_cluster_parent;
    if (hard_macro->isFixed()) {
      single_macro_cluster->setAsFixedMacro(hard_macro);
      new_cluster_parent = tree_->root.get();
    } else {
      new_cluster_parent = mixed_leaf_parent;
    }

    incorporateNewCluster(std::move(single_macro_cluster), new_cluster_parent);
  }
}

void ClusteringEngine::classifyMacrosBySize(
    const std::vector<HardMacro*>& hard_macros,
    std::vector<int>& size_class)
{
  for (int i = 0; i < hard_macros.size(); i++) {
    if (size_class[i] == -1) {
      for (int j = i + 1; j < hard_macros.size(); j++) {
        if ((size_class[j] == -1) && ((*hard_macros[i]) == (*hard_macros[j]))) {
          size_class[j] = i;
        }
      }
    }
  }

  for (int i = 0; i < hard_macros.size(); i++) {
    size_class[i] = (size_class[i] == -1) ? i : size_class[i];
  }
}

void ClusteringEngine::classifyMacrosByConnSignature(
    const std::vector<Cluster*>& macro_clusters,
    std::vector<int>& signature_class)
{
  for (int i = 0; i < macro_clusters.size(); i++) {
    if (signature_class[i] == -1) {
      signature_class[i] = i;
      for (int j = i + 1; j < macro_clusters.size(); j++) {
        if (signature_class[j] != -1) {
          continue;
        }

        if (sameConnectionSignature(macro_clusters[i], macro_clusters[j])) {
          signature_class[j] = i;
        }
      }
    }
  }

  if (logger_->debugCheck(MPL, "multilevel_autoclustering", 2)) {
    logger_->report("\nPrint Connection Signature\n");
    for (Cluster* cluster : macro_clusters) {
      logger_->report("Macro Signature: {}", cluster->getName());
      for (auto& [cluster_id, weight] : cluster->getConnectionsMap()) {
        logger_->report(" {} {} ",
                        tree_->maps.id_to_cluster[cluster_id]->getName(),
                        weight);
      }
    }
  }
}

void ClusteringEngine::classifyMacrosByInterconn(
    const std::vector<Cluster*>& macro_clusters,
    std::vector<int>& interconn_class)
{
  for (int i = 0; i < macro_clusters.size(); i++) {
    if (interconn_class[i] == -1) {
      interconn_class[i] = i;
      for (int j = 0; j < macro_clusters.size(); j++) {
        if (macro_clusters[i] == macro_clusters[j]) {
          continue;
        }

        if (strongConnection(macro_clusters[i], macro_clusters[j])) {
          if (interconn_class[j] != -1) {
            interconn_class[i] = interconn_class[j];
            break;
          }

          interconn_class[j] = i;
        }
      }
    }
  }
}

// We determine if the macros belong to the same class based on:
// 1. Size && and Interconnection (Directly connected macro clusters
//    should be grouped)
// 2. Size && Connection Signature (Macros with same connection
//    signature should be grouped)
void ClusteringEngine::groupSingleMacroClusters(
    const std::vector<Cluster*>& macro_clusters,
    const std::vector<int>& size_class,
    const std::vector<int>& signature_class,
    std::vector<int>& interconn_class,
    std::vector<int>& macro_class)
{
  for (int i = 0; i < macro_clusters.size(); i++) {
    if (macro_class[i] != -1) {
      continue;
    }
    macro_class[i] = i;

    for (int j = i + 1; j < macro_clusters.size(); j++) {
      if (macro_class[j] != -1) {
        continue;
      }

      if (size_class[i] == size_class[j]) {
        if (interconn_class[i] == interconn_class[j]) {
          debugPrint(logger_,
                     MPL,
                     "multilevel_autoclustering",
                     1,
                     "Merging interconnected macro clusters {} and {}",
                     macro_clusters[j]->getId(),
                     macro_clusters[i]->getId());
          if (attemptMerge(macro_clusters[i], macro_clusters[j])) {
            macro_class[j] = i;
          } else {
            logger_->critical(MPL,
                              28,
                              "Merge attempt between interconnected macro "
                              "siblings failed!");
          }
        } else {
          // We need this so we can distinguish arrays of interconnected macros
          // from grouped macro clusters with same signature.
          interconn_class[i] = -1;
          if (signature_class[i] == signature_class[j]) {
            debugPrint(logger_,
                       MPL,
                       "multilevel_autoclustering",
                       1,
                       "Merging same signature clusters {} and {}.",
                       macro_clusters[j]->getId(),
                       macro_clusters[i]->getId());
            if (attemptMerge(macro_clusters[i], macro_clusters[j])) {
              macro_class[j] = i;
            } else {
              logger_->critical(MPL,
                                29,
                                "Merge attempt between macro siblings with "
                                "same connection signature failed!");
            }
          }
        }
      }
    }
  }
}

void ClusteringEngine::replaceByStdCellCluster(
    Cluster* mixed_leaf,
    std::vector<int>& virtual_conn_clusters)
{
  mixed_leaf->clearLeafMacros();
  mixed_leaf->setClusterType(StdCellCluster);

  setClusterMetrics(mixed_leaf);

  virtual_conn_clusters.push_back(mixed_leaf->getId());
}

std::string ClusteringEngine::generateMacroAndCoreDimensionsTable(
    const HardMacro* hard_macro,
    const odb::Rect& core) const
{
  std::string table;

  table += fmt::format("\n          |   Macro + Halos   |   Core   ");
  table += fmt::format("\n-----------------------------------------");
  table += fmt::format("\n   Width  | {:>17.2f} | {:>8.2f}",
                       block_->dbuToMicrons(hard_macro->getWidth()),
                       block_->dbuToMicrons(core.dx()));
  table += fmt::format("\n  Height  | {:>17.2f} | {:>8.2f}\n",
                       block_->dbuToMicrons(hard_macro->getHeight()),
                       block_->dbuToMicrons(core.dy()));

  return table;
}

// Creates the hard macros objects and inserts them in the tree map
void ClusteringEngine::createHardMacros()
{
  const odb::Rect& core = block_->getCoreArea();

  for (odb::dbInst* inst : block_->getInsts()) {
    if (inst->isBlock()) {
      if (inst->isFixed()) {
        logger_->info(MPL, 62, "Found fixed macro {}.", inst->getName());

        if (!inst->getBBox()->getBox().overlaps(tree_->floorplan_shape)) {
          addIgnorableMacro(inst);
          logger_->info(MPL,
                        63,
                        "{} is outside the macro placement area and will be "
                        "ignored.",
                        inst->getName());
          continue;
        }

        tree_->has_fixed_macros = true;
      }

      HardMacro::Halo halo = macro_to_halo_.contains(inst)
                                 ? macro_to_halo_.at(inst)
                                 : tree_->default_halo;

      auto macro = std::make_unique<HardMacro>(inst, halo);

      if (macro->getWidth() > core.dx() || macro->getHeight() > core.dy()) {
        logger_->error(
            MPL,
            6,
            "Found macro that does not fit in the core.\nName: {}\n{}",
            inst->getName(),
            generateMacroAndCoreDimensionsTable(macro.get(), core));
      }

      tree_->maps.inst_to_hard[inst] = std::move(macro);
    }
  }
}

// When placing the HardMacros of a macro cluster, we create temporary
// internal macro clusters - one representing each HardMacro - so we
// can use them to compute the connections with the fixed terminals.
void ClusteringEngine::createTempMacroClusters(
    const std::vector<HardMacro*>& hard_macros,
    std::vector<HardMacro>& sa_macros,
    UniqueClusterVector& macro_clusters,
    std::map<int, int>& cluster_to_macro,
    std::set<odb::dbMaster*>& masters)
{
  int macro_id = 0;
  std::string cluster_name;

  for (auto& hard_macro : hard_macros) {
    macro_id = sa_macros.size();
    cluster_name = hard_macro->getName();

    auto macro_cluster = std::make_unique<Cluster>(id_, cluster_name, logger_);
    macro_cluster->addLeafMacro(hard_macro->getInst());
    updateInstancesAssociation(macro_cluster.get());

    sa_macros.push_back(*hard_macro);
    cluster_to_macro[id_] = macro_id;
    masters.insert(hard_macro->getInst()->getMaster());

    tree_->maps.id_to_cluster[id_++] = macro_cluster.get();
    macro_clusters.push_back(std::move(macro_cluster));
  }
}

// As the temporary internal macro clusters are destroyed once the
// macro placement is done. We need to remove their raw pointer
// from the id --> cluster map to avoid deleting them twice.
void ClusteringEngine::clearTempMacroClusterMapping(
    const UniqueClusterVector& macro_clusters)
{
  for (auto& macro_cluster : macro_clusters) {
    tree_->maps.id_to_cluster.erase(macro_cluster->getId());
  }
}

int ClusteringEngine::getNumberOfIOs(Cluster* target) const
{
  int number_of_ios = 0;
  for (const auto& [pin, io_cluster_id] : tree_->maps.bterm_to_cluster_id) {
    if (io_cluster_id == target->getId()) {
      ++number_of_ios;
    }
  }
  return number_of_ios;
}

///////////////////////////////////////////////

void ClusteringEngine::reportThresholds() const
{
  logger_->report("\n    Level {}  |  Min  |  Max", level_);
  logger_->report("-----------------------------");
  logger_->report(
      "  Std Cells  | {:>5d} | {:>6d}", min_std_cell_, max_std_cell_);
  logger_->report("     Macros  | {:>5d} | {:>6d}\n", min_macro_, max_macro_);
}

void ClusteringEngine::printPhysicalHierarchyTree(Cluster* parent, int level)
{
  std::string line;
  for (int i = 0; i < level; i++) {
    line += "+---";
  }

  line += fmt::format("{}  ({}) Type: {}",
                      parent->getName(),
                      parent->getId(),
                      parent->getClusterTypeString());

  if (parent->isClusterOfUnplacedIOPins() || parent->isIOBundle()) {
    line += fmt::format(" Pins: {}", getNumberOfIOs(parent));
  } else if (!parent->isIOPadCluster()) {
    line += fmt::format(" {}", parent->getIsLeafString());

    // Using 'or' on purpose to certify that there is no discrepancy going on.
    if (parent->getNumStdCell() != 0 || parent->getStdCellArea() != 0) {
      line += fmt::format(", StdCells: {} ({} μ²)",
                          parent->getNumStdCell(),
                          parent->getStdCellArea());
    }

    if (parent->getNumMacro() != 0 || parent->getMacroArea() != 0) {
      line += fmt::format(", Macros: {} ({} μ²),",
                          parent->getNumMacro(),
                          parent->getMacroArea());
    }
  }

  logger_->report("{}", line);

  for (auto& cluster : parent->getChildren()) {
    printPhysicalHierarchyTree(cluster.get(), level + 1);
  }
}

}  // namespace mpl
