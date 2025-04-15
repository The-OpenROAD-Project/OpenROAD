// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "clusterEngine.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "par/PartitionMgr.h"
#include "sta/Liberty.hh"

namespace mpl {
using utl::MPL;

ClusteringEngine::ClusteringEngine(odb::dbBlock* block,
                                   sta::dbNetwork* network,
                                   utl::Logger* logger,
                                   par::PartitionMgr* triton_part)
    : block_(block),
      network_(network),
      logger_(logger),
      triton_part_(triton_part)
{
}

void ClusteringEngine::run()
{
  design_metrics_ = computeModuleMetrics(block_->getTopModule());
  init();

  if (!tree_->has_unfixed_macros) {
    return;
  }

  createRoot();
  setBaseThresholds();

  mapIOPinsAndPads();
  createDataFlow();

  createIOClusters();
  classifyBoundariesStateForIOs();

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

// Check if macro placement is both needed and feasible.
// Also report some design data relevant for the user.
void ClusteringEngine::init()
{
  const std::vector<odb::dbInst*> unfixed_macros = getUnfixedMacros();
  if (unfixed_macros.empty()) {
    tree_->has_unfixed_macros = false;
    logger_->info(MPL, 17, "No unfixed macros.");
    return;
  }

  setFloorplanShape();
  searchForFixedInstsInsideFloorplanShape();

  tree_->macro_with_halo_area = computeMacroWithHaloArea(unfixed_macros);
  const float inst_area_with_halos
      = tree_->macro_with_halo_area + design_metrics_->getStdCellArea();

  if (inst_area_with_halos > tree_->floorplan_shape.getArea()) {
    logger_->error(MPL,
                   16,
                   "The instance area considering the macros' halos {} exceeds "
                   "the floorplan area {}",
                   inst_area_with_halos,
                   tree_->floorplan_shape.getArea());
  }

  reportDesignData();
}

float ClusteringEngine::computeMacroWithHaloArea(
    const std::vector<odb::dbInst*>& unfixed_macros)
{
  float macro_with_halo_area = 0.0f;
  for (odb::dbInst* unfixed_macro : unfixed_macros) {
    odb::dbMaster* master = unfixed_macro->getMaster();
    const float width
        = block_->dbuToMicrons(master->getWidth()) + 2 * tree_->halo_width;
    const float height
        = block_->dbuToMicrons(master->getHeight()) + 2 * tree_->halo_height;
    macro_with_halo_area += width * height;
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
  const odb::Rect& core_box = block_->getCoreArea();
  const float core_lx = block_->dbuToMicrons(core_box.xMin());
  const float core_ly = block_->dbuToMicrons(core_box.yMin());
  const float core_ux = block_->dbuToMicrons(core_box.xMax());
  const float core_uy = block_->dbuToMicrons(core_box.yMax());

  tree_->floorplan_shape = Rect(std::max(core_lx, tree_->global_fence.xMin()),
                                std::max(core_ly, tree_->global_fence.yMin()),
                                std::min(core_ux, tree_->global_fence.xMax()),
                                std::min(core_uy, tree_->global_fence.yMax()));
}

void ClusteringEngine::searchForFixedInstsInsideFloorplanShape()
{
  odb::Rect floorplan_shape(
      block_->micronsToDbu(tree_->floorplan_shape.xMin()),
      block_->micronsToDbu(tree_->floorplan_shape.yMin()),
      block_->micronsToDbu(tree_->floorplan_shape.xMax()),
      block_->micronsToDbu(tree_->floorplan_shape.yMax()));

  for (odb::dbInst* inst : block_->getInsts()) {
    if (inst->isFixed()
        && inst->getBBox()->getBox().overlaps(floorplan_shape)) {
      logger_->error(MPL,
                     50,
                     "Found fixed instance {} inside the floorplan area.",
                     inst->getName());
    }
  }
}

Metrics* ClusteringEngine::computeModuleMetrics(odb::dbModule* module)
{
  unsigned int num_std_cell = 0;
  float std_cell_area = 0.0;
  unsigned int num_macro = 0;
  float macro_area = 0.0;

  for (odb::dbInst* inst : module->getInsts()) {
    if (isIgnoredInst(inst)) {
      continue;
    }

    float inst_area = computeMicronArea(inst);

    if (inst->isBlock()) {  // a macro
      num_macro += 1;
      macro_area += inst_area;

      auto macro = std::make_unique<HardMacro>(
          inst, tree_->halo_width, tree_->halo_height);
      tree_->maps.inst_to_hard[inst] = std::move(macro);
    } else {
      num_std_cell += 1;
      std_cell_area += inst_area;
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
      tree_->floorplan_shape.xMin(),
      tree_->floorplan_shape.yMin(),
      tree_->floorplan_shape.xMax(),
      tree_->floorplan_shape.yMax());

  float util
      = (design_metrics_->getStdCellArea() + design_metrics_->getMacroArea())
        / tree_->floorplan_shape.getArea();
  float floorplan_util
      = design_metrics_->getStdCellArea()
        / (tree_->floorplan_shape.getArea() - design_metrics_->getMacroArea());
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
      design_metrics_->getStdCellArea(),
      design_metrics_->getNumMacro(),
      design_metrics_->getMacroArea(),
      tree_->halo_width,
      tree_->halo_height,
      tree_->macro_with_halo_area,
      design_metrics_->getStdCellArea() + design_metrics_->getMacroArea(),
      tree_->floorplan_shape.getArea(),
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
  if (tree_->base_max_macro <= 0 || tree_->base_min_macro <= 0
      || tree_->base_max_std_cell <= 0 || tree_->base_min_std_cell <= 0) {
    // Set base values for std cell lower/upper thresholds
    const int min_num_std_cells_allowed = 1000;
    tree_->base_min_std_cell
        = std::floor(design_metrics_->getNumStdCell()
                     / std::pow(tree_->cluster_size_ratio, tree_->max_level));
    if (tree_->base_min_std_cell <= min_num_std_cells_allowed) {
      tree_->base_min_std_cell = min_num_std_cells_allowed;
    }
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
// For the former, we group IO pins with the same constraints based on:
// - If an IO pin has a constraint region in a certain boundary,
//   it is constrained to that entire boundary;
// - If an IO pin has no constraints, it is constrained to all boundaries.
void ClusteringEngine::createIOClusters()
{
  if (!tree_->maps.pad_to_bterm.empty()) {
    createIOPadClusters();
    return;
  }

  // Boundary with constrained IOs -> cluster
  std::map<Boundary, Cluster*> boundary_to_cluster;
  const odb::Rect die = block_->getDieArea();

  for (odb::dbBTerm* bterm : block_->getBTerms()) {
    Boundary constraint_boundary = NONE;

    auto constraint_region = bterm->getConstraintRegion();
    if (constraint_region) {
      constraint_boundary
          = getConstraintBoundary(die, constraint_region.value());
    }

    const auto itr = boundary_to_cluster.find(constraint_boundary);
    if (itr != boundary_to_cluster.end()) {
      Cluster* io_cluster = itr->second;
      tree_->maps.bterm_to_cluster_id[bterm] = io_cluster->getId();
    } else {
      createIOCluster(die, constraint_boundary, boundary_to_cluster, bterm);
    }
  }

  if (tree_->maps.id_to_cluster.size() == 1) {
    logger_->warn(MPL, 26, "Design has no IO pins!");
    tree_->has_io_clusters = false;
  }
}

Boundary ClusteringEngine::getConstraintBoundary(
    const odb::Rect& die,
    const odb::Rect& constraint_region)
{
  Boundary constraint_boundary = NONE;
  if (constraint_region.xMin() == constraint_region.xMax()) {
    if (constraint_region.xMin() == die.xMin()) {
      constraint_boundary = L;
    } else {
      constraint_boundary = R;
    }
  } else {
    if (constraint_region.yMin() == die.yMin()) {
      constraint_boundary = B;
    } else {
      constraint_boundary = T;
    }
  }
  return constraint_boundary;
}

void ClusteringEngine::createIOCluster(
    const odb::Rect& die,
    const Boundary constraint_boundary,
    std::map<Boundary, Cluster*>& boundary_to_cluster,
    odb::dbBTerm* bterm)
{
  auto cluster
      = std::make_unique<Cluster>(id_, toString(constraint_boundary), logger_);
  tree_->maps.bterm_to_cluster_id[bterm] = id_;
  tree_->maps.id_to_cluster[id_++] = cluster.get();

  boundary_to_cluster[constraint_boundary] = cluster.get();

  int x = die.xMin(), y = die.yMin();
  int width = die.dx(), height = die.dy();

  if (constraint_boundary != NONE) {
    setIOClusterDimensions(die, constraint_boundary, x, y, width, height);
  }

  cluster->setAsClusterOfUnplacedIOPins(
      std::pair<float, float>(block_->dbuToMicrons(x), block_->dbuToMicrons(y)),
      block_->dbuToMicrons(width),
      block_->dbuToMicrons(height),
      constraint_boundary);
  tree_->root->addChild(std::move(cluster));
}

void ClusteringEngine::classifyBoundariesStateForIOs()
{
  const float blocked_boundary_threshold = 0.7;
  std::map<Boundary, float> blockage_extension_map
      = computeBlockageExtensionMap();

  for (const auto [boundary, blockage_extension] : blockage_extension_map) {
    if (blockage_extension >= blocked_boundary_threshold) {
      tree_->blocked_boundaries.insert(boundary);
    } else {
      tree_->unblocked_boundaries.insert(boundary);
    }
  }
}

// Computes how much blocked each boundary is for IOs base on PPL exclude
// contraints.
std::map<Boundary, float> ClusteringEngine::computeBlockageExtensionMap()
{
  std::map<Boundary, float> blockage_extension_map;

  blockage_extension_map[L] = 0.0;
  blockage_extension_map[R] = 0.0;
  blockage_extension_map[B] = 0.0;
  blockage_extension_map[T] = 0.0;

  const odb::Rect die = block_->getDieArea();
  for (const odb::Rect& blocked_region : block_->getBlockedRegionsForPins()) {
    Boundary blocked_region_boundary
        = getConstraintBoundary(die, blocked_region);
    float blockage_extension = 0.0;

    if (blocked_region_boundary == L || blocked_region_boundary == R) {
      blockage_extension = blocked_region.dy() / static_cast<float>(die.dy());
    } else if (blocked_region_boundary == B || blocked_region_boundary == T) {
      blockage_extension = blocked_region.dx() / static_cast<float>(die.dx());
    }

    blockage_extension_map[blocked_region_boundary] += blockage_extension;
  }

  return blockage_extension_map;
}

void ClusteringEngine::setIOClusterDimensions(const odb::Rect& die,
                                              const Boundary boundary,
                                              int& x,
                                              int& y,
                                              int& width,
                                              int& height)
{
  if (boundary == L) {
    x = die.xMin();
    y = die.yMin();
    width = 0;
  } else if (boundary == T) {
    x = die.xMin();
    y = die.yMax();
    height = 0;
  } else if (boundary == R) {
    x = die.xMax();
    y = die.yMin();
    width = 0;
  } else {  // Bottom
    x = die.xMin();
    y = die.yMin();
    height = 0;
  }
}

void ClusteringEngine::mapIOPinsAndPads()
{
  bool design_has_io_pads = false;
  for (auto inst : block_->getInsts()) {
    if (inst->getMaster()->isPad()) {
      design_has_io_pads = true;
      break;
    }
  }

  if (!design_has_io_pads) {
    return;
  }

  for (odb::dbNet* net : block_->getNets()) {
    if (net->getBTerms().size() == 0) {
      continue;
    }

    for (odb::dbBTerm* bterm : net->getBTerms()) {
      for (odb::dbITerm* iterm : net->getITerms()) {
        odb::dbInst* inst = iterm->getInst();
        tree_->maps.pad_to_bterm[inst] = bterm;
        tree_->maps.bterm_to_pad[bterm] = inst;
      }
    }
  }
}

void ClusteringEngine::createIOPadClusters()
{
  for (const auto& io_pad_path : data_connections_.io_and_regs) {
    // Registers or Macros
    const PathInsts& connected_insts = io_pad_path.second;
    bool path_is_empty = true;
    for (const std::set<odb::dbInst*>& insts_of_curr_hop_dist :
         connected_insts) {
      if (!insts_of_curr_hop_dist.empty()) {
        path_is_empty = false;
        break;
      }
    }

    if (path_is_empty) {
      continue;
    }

    odb::dbBTerm* bterm = io_pad_path.first;
    odb::dbInst* pad = tree_->maps.bterm_to_pad.at(bterm);

    createIOPadCluster(pad, bterm);
  }
}

void ClusteringEngine::createIOPadCluster(odb::dbInst* pad, odb::dbBTerm* bterm)
{
  auto cluster = std::make_unique<Cluster>(id_, pad->getName(), logger_);
  tree_->maps.bterm_to_cluster_id[bterm] = id_;
  tree_->maps.id_to_cluster[id_++] = cluster.get();

  const odb::Rect& pad_bbox = pad->getBBox()->getBox();

  cluster->setAsIOPadCluster({block_->dbuToMicrons(pad_bbox.xMin()),
                              block_->dbuToMicrons(pad_bbox.yMin())},
                             block_->dbuToMicrons(pad_bbox.dx()),
                             block_->dbuToMicrons(pad_bbox.dy()));
  tree_->root->addChild(std::move(cluster));
}

// Dataflow is used to improve quality of macro placement.
// Here we model each std cell instance, IO pin and macro pin as vertices.
void ClusteringEngine::createDataFlow()
{
  if (design_metrics_->getNumStdCell() != 0 && !stdCellsHaveLiberty()) {
    logger_->warn(
        MPL,
        14,
        "No Liberty data found for std cells. Continuing without dataflow.");
    data_connections_.is_empty = true;
    return;
  }

  // Create vertices IDs.
  const VerticesMaps vertices_maps = computeVertices();
  const int num_of_vertices = static_cast<int>(vertices_maps.stoppers.size());

  const DataFlowHypergraph hypergraph = computeHypergraph(num_of_vertices);

  // Traverse hypergraph to build dataflow.
  for (auto [src, src_pin] : vertices_maps.id_to_bterm) {
    int idx = 0;
    std::vector<bool> visited(num_of_vertices, false);
    std::vector<std::set<odb::dbInst*>> insts(max_num_of_hops_);
    dataFlowDFSIOPin(
        src, idx, vertices_maps, hypergraph, insts, visited, false);
    dataFlowDFSIOPin(src, idx, vertices_maps, hypergraph, insts, visited, true);

    data_connections_.io_and_regs.emplace_back(src_pin, insts);
  }

  for (auto [src, src_pin] : vertices_maps.id_to_macro_pin) {
    int idx = 0;
    std::vector<bool> visited(num_of_vertices, false);
    std::vector<std::set<odb::dbInst*>> std_cells(max_num_of_hops_);
    std::vector<std::set<odb::dbInst*>> macros(max_num_of_hops_);
    dataFlowDFSMacroPin(
        src, idx, vertices_maps, hypergraph, std_cells, macros, visited, false);
    dataFlowDFSMacroPin(
        src, idx, vertices_maps, hypergraph, std_cells, macros, visited, true);

    data_connections_.macro_pins_and_regs.emplace_back(src_pin, std_cells);
    data_connections_.macro_pins_and_macros.emplace_back(src_pin, macros);
  }
}

// Here we assume that there are std cells in the design!
bool ClusteringEngine::stdCellsHaveLiberty()
{
  for (odb::dbInst* inst : block_->getInsts()) {
    if (isIgnoredInst(inst) || inst->isBlock()) {
      continue;
    }

    const sta::LibertyCell* liberty_cell = network_->libertyCell(inst);
    if (liberty_cell) {
      return true;
    }
  }

  return false;
}

VerticesMaps ClusteringEngine::computeVertices()
{
  VerticesMaps vertices_maps;
  computeIOVertices(vertices_maps);
  computeStdCellVertices(vertices_maps);
  computeMacroPinVertices(vertices_maps);

  debugPrint(logger_,
             MPL,
             "multilevel_autoclustering",
             1,
             "Number of vertices: {}",
             vertices_maps.stoppers.size());

  return vertices_maps;
}

void ClusteringEngine::computeIOVertices(VerticesMaps& vertices_maps)
{
  for (odb::dbBTerm* bterm : block_->getBTerms()) {
    const int id = static_cast<int>(vertices_maps.stoppers.size());
    odb::dbIntProperty::create(bterm, "vertex_id", id);
    vertices_maps.id_to_bterm[id] = bterm;
    vertices_maps.stoppers.push_back(true);
  }
}

void ClusteringEngine::computeStdCellVertices(VerticesMaps& vertices_maps)
{
  for (odb::dbInst* inst : block_->getInsts()) {
    if (isIgnoredInst(inst) || inst->isBlock()) {
      continue;
    }

    const sta::LibertyCell* liberty_cell = network_->libertyCell(inst);
    if (!liberty_cell) {
      continue;
    }

    const int id = static_cast<int>(vertices_maps.stoppers.size());

    // Registers are stoppers.
    odb::dbIntProperty::create(inst, "vertex_id", id);
    vertices_maps.id_to_std_cell[id] = inst;

    if (liberty_cell->hasSequentials()) {
      vertices_maps.stoppers.push_back(true);
    } else {
      vertices_maps.stoppers.push_back(false);
    }
  }
}

void ClusteringEngine::computeMacroPinVertices(VerticesMaps& vertices_maps)
{
  for (auto& [macro, hard_macro] : tree_->maps.inst_to_hard) {
    for (odb::dbITerm* pin : macro->getITerms()) {
      if (pin->getSigType() != odb::dbSigType::SIGNAL
          && pin->getSigType() != odb::dbSigType::CLOCK) {
        continue;
      }

      const int id = static_cast<int>(vertices_maps.stoppers.size());
      odb::dbIntProperty::create(pin, "vertex_id", id);
      vertices_maps.id_to_macro_pin[id] = pin;
      vertices_maps.stoppers.push_back(true);
    }
  }
}

DataFlowHypergraph ClusteringEngine::computeHypergraph(
    const int num_of_vertices)
{
  DataFlowHypergraph graph;
  graph.vertices.resize(num_of_vertices);
  graph.backward_vertices.resize(num_of_vertices);

  for (odb::dbNet* net : block_->getNets()) {
    if (!isValidNet(net)) {
      continue;
    }

    int driver_id = -1;
    std::set<int> loads_id;
    bool net_has_stdcell_without_liberty = false;
    for (odb::dbITerm* iterm : net->getITerms()) {
      odb::dbInst* inst = iterm->getInst();
      int vertex_id = -1;

      if (inst->isBlock()) {
        vertex_id = odb::dbIntProperty::find(iterm, "vertex_id")->getValue();
      } else if (inst->isPad()) {
        // To properly consider the path of a signal that travels from a PAD
        // to the core we use the path's bterm as the vertex.
        odb::dbBTerm* pad_bterm = tree_->maps.pad_to_bterm.at(inst);
        vertex_id
            = odb::dbIntProperty::find(pad_bterm, "vertex_id")->getValue();
      } else {
        odb::dbIntProperty* int_prop
            = odb::dbIntProperty::find(inst, "vertex_id");

        // Std cells without liberty data are not marked as vertices
        if (int_prop) {
          vertex_id = int_prop->getValue();
        } else {
          net_has_stdcell_without_liberty = true;
          break;
        }
      }

      if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
        driver_id = vertex_id;
      } else {
        loads_id.insert(vertex_id);
      }
    }

    if (net_has_stdcell_without_liberty) {
      continue;
    }

    for (odb::dbBTerm* bterm : net->getBTerms()) {
      const int vertex_id
          = odb::dbIntProperty::find(bterm, "vertex_id")->getValue();
      if (bterm->getIoType() == odb::dbIoType::INPUT) {
        driver_id = vertex_id;
      } else {
        loads_id.insert(vertex_id);
      }
    }

    // Skip high fanout nets or nets that do not have valid driver or loads
    if (driver_id < 0 || loads_id.empty()
        || loads_id.size() > tree_->large_net_threshold) {
      continue;
    }

    std::vector<int> hyperedge{driver_id};
    for (auto& load : loads_id) {
      if (load != driver_id) {
        hyperedge.push_back(load);
      }
    }
    graph.vertices[driver_id].push_back(graph.hyperedges.size());
    for (int i = 1; i < hyperedge.size(); i++) {
      graph.backward_vertices[hyperedge[i]].push_back(graph.hyperedges.size());
    }
    graph.hyperedges.push_back(hyperedge);
  }

  return graph;
}

/* static */
// Instance that should not be touched i.e., have its location
// altered by the macro placer.
// Note: This function also takes into account the placement status
// of the instance, because, if it is placed outside the area that is
// used for the macro placement, it can be safely ignored as there's
// no risk to generate overlap.
bool ClusteringEngine::isIgnoredInst(odb::dbInst* inst)
{
  odb::dbMaster* master = inst->getMaster();

  return master->isPad() || master->isCover() || master->isEndCap()
         || inst->isFixed();
}

// Forward or Backward DFS search to find sequential paths from/to IO pins based
// on hop count to macro pins
void ClusteringEngine::dataFlowDFSIOPin(
    const int parent,
    int idx,
    const VerticesMaps& vertices_maps,
    const DataFlowHypergraph& hypergraph,
    std::vector<std::set<odb::dbInst*>>& insts,
    std::vector<bool>& visited,
    bool backward_search)
{
  visited[parent] = true;
  if (vertices_maps.stoppers[parent]) {
    if (parent < vertices_maps.id_to_bterm.size()) {
      ;  // currently we do not consider IO pin to IO pin connection
    } else if (parent < vertices_maps.id_to_bterm.size()
                            + vertices_maps.id_to_std_cell.size()) {
      insts[idx].insert(vertices_maps.id_to_std_cell.at(parent));
    } else {
      insts[idx].insert(vertices_maps.id_to_macro_pin.at(parent)->getInst());
    }
    idx++;
  }

  if (idx >= max_num_of_hops_) {
    return;
  }

  if (!backward_search) {
    for (auto& hyperedge : hypergraph.vertices[parent]) {
      for (auto& vertex : hypergraph.hyperedges[hyperedge]) {
        // we do not consider pin to pin
        if (visited[vertex] || vertex < vertices_maps.id_to_bterm.size()) {
          continue;
        }
        dataFlowDFSIOPin(vertex,
                         idx,
                         vertices_maps,
                         hypergraph,
                         insts,
                         visited,
                         backward_search);
      }
    }
  } else {
    for (auto& hyperedge : hypergraph.backward_vertices[parent]) {
      const int vertex
          = hypergraph.hyperedges[hyperedge].front();  // driver vertex
      // we do not consider pin to pin
      if (visited[vertex] || vertex < vertices_maps.id_to_bterm.size()) {
        continue;
      }
      dataFlowDFSIOPin(vertex,
                       idx,
                       vertices_maps,
                       hypergraph,
                       insts,
                       visited,
                       backward_search);
    }
  }
}

// Forward or Backward DFS search to find sequential paths between Macros based
// on hop count
void ClusteringEngine::dataFlowDFSMacroPin(
    const int parent,
    int idx,
    const VerticesMaps& vertices_maps,
    const DataFlowHypergraph& hypergraph,
    std::vector<std::set<odb::dbInst*>>& std_cells,
    std::vector<std::set<odb::dbInst*>>& macros,
    std::vector<bool>& visited,
    bool backward_search)
{
  visited[parent] = true;
  if (vertices_maps.stoppers[parent]) {
    if (parent < vertices_maps.id_to_bterm.size()) {
      ;  // the connection between IO and macro pins have been considers
    } else if (parent < vertices_maps.id_to_bterm.size()
                            + vertices_maps.id_to_std_cell.size()) {
      std_cells[idx].insert(vertices_maps.id_to_std_cell.at(parent));
    } else {
      macros[idx].insert(vertices_maps.id_to_macro_pin.at(parent)->getInst());
    }
    idx++;
  }

  if (idx >= max_num_of_hops_) {
    return;
  }

  if (!backward_search) {
    for (auto& hyperedge : hypergraph.vertices[parent]) {
      for (auto& vertex : hypergraph.hyperedges[hyperedge]) {
        // we do not consider pin to pin
        if (visited[vertex] || vertex < vertices_maps.id_to_bterm.size()) {
          continue;
        }
        dataFlowDFSMacroPin(vertex,
                            idx,
                            vertices_maps,
                            hypergraph,
                            std_cells,
                            macros,
                            visited,
                            backward_search);
      }
    }
  } else {
    for (auto& hyperedge : hypergraph.backward_vertices[parent]) {
      const int vertex = hypergraph.hyperedges[hyperedge].front();
      // we do not consider pin to pin
      if (visited[vertex] || vertex < vertices_maps.id_to_bterm.size()) {
        continue;
      }
      dataFlowDFSMacroPin(vertex,
                          idx,
                          vertices_maps,
                          hypergraph,
                          std_cells,
                          macros,
                          visited,
                          backward_search);
    }
  }
}

void ClusteringEngine::updateDataFlow()
{
  if (data_connections_.is_empty) {
    return;
  }

  // bterm, macros or ffs
  for (const auto& [bterm, insts] : data_connections_.io_and_regs) {
    const auto itr = tree_->maps.bterm_to_cluster_id.find(bterm);
    if (itr == tree_->maps.bterm_to_cluster_id.end()) {
      continue;
    }

    const int driver_id = itr->second;

    for (int hops = 0; hops < max_num_of_hops_; hops++) {
      std::set<int> sink_clusters = computeSinks(insts[hops]);
      const float conn_weight = computeConnWeight(hops);
      for (auto& sink : sink_clusters) {
        tree_->maps.id_to_cluster[driver_id]->addConnection(sink, conn_weight);
        tree_->maps.id_to_cluster[sink]->addConnection(driver_id, conn_weight);
      }
    }
  }

  // macros to ffs
  for (const auto& [iterm, insts] : data_connections_.macro_pins_and_regs) {
    const int driver_id = tree_->maps.inst_to_cluster_id.at(iterm->getInst());

    for (int hops = 0; hops < max_num_of_hops_; hops++) {
      std::set<int> sink_clusters = computeSinks(insts[hops]);
      const float conn_weight = computeConnWeight(hops);
      for (auto& sink : sink_clusters) {
        tree_->maps.id_to_cluster[driver_id]->addConnection(sink, conn_weight);
        tree_->maps.id_to_cluster[sink]->addConnection(driver_id, conn_weight);
      }
    }
  }

  // macros to macros
  for (const auto& [iterm, insts] : data_connections_.macro_pins_and_macros) {
    const int driver_id = tree_->maps.inst_to_cluster_id.at(iterm->getInst());

    for (int hops = 0; hops < max_num_of_hops_; hops++) {
      std::set<int> sink_clusters = computeSinks(insts[hops]);
      const float conn_weight = computeConnWeight(hops);
      for (auto& sink : sink_clusters) {
        tree_->maps.id_to_cluster[driver_id]->addConnection(sink, conn_weight);
      }
    }
  }
}

float ClusteringEngine::computeConnWeight(const int hops)
{
  const float base_remoteness_factor = 2.0;
  const float base_connection_weight = 1;
  const float remoteness_factor = std::pow(base_remoteness_factor, hops);

  return base_connection_weight / remoteness_factor;
}

std::set<int> ClusteringEngine::computeSinks(
    const std::set<odb::dbInst*>& insts)
{
  std::set<int> sink_clusters;
  for (auto& inst : insts) {
    const int cluster_id = tree_->maps.inst_to_cluster_id.at(inst);
    sink_clusters.insert(cluster_id);
  }
  return sink_clusters;
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

void ClusteringEngine::updateInstancesAssociation(Cluster* cluster)
{
  const int cluster_id = cluster->getId();
  const ClusterType cluster_type = cluster->getClusterType();
  if (cluster_type == HardMacroCluster || cluster_type == MixedCluster) {
    for (odb::dbInst* inst : cluster->getLeafMacros()) {
      tree_->maps.inst_to_cluster_id[inst] = cluster_id;
    }
  }

  if (cluster_type == StdCellCluster || cluster_type == MixedCluster) {
    for (odb::dbInst* inst : cluster->getLeafStdCells()) {
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
  if (include_macro) {
    for (odb::dbInst* inst : module->getInsts()) {
      tree_->maps.inst_to_cluster_id[inst] = cluster_id;
    }
  } else {  // only consider standard cells
    for (odb::dbInst* inst : module->getInsts()) {
      if (isIgnoredInst(inst) || inst->isBlock()) {
        continue;
      }

      tree_->maps.inst_to_cluster_id[inst] = cluster_id;
    }
  }
  for (odb::dbModInst* child_module_inst : module->getChildren()) {
    updateInstancesAssociation(
        child_module_inst->getMaster(), cluster_id, include_macro);
  }
}

void ClusteringEngine::setClusterMetrics(Cluster* cluster)
{
  float std_cell_area = 0.0f;
  for (odb::dbInst* std_cell : cluster->getLeafStdCells()) {
    std_cell_area += computeMicronArea(std_cell);
  }

  float macro_area = 0.0f;
  for (odb::dbInst* macro : cluster->getLeafMacros()) {
    macro_area += computeMicronArea(macro);
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

float ClusteringEngine::computeMicronArea(odb::dbInst* inst)
{
  const float width = static_cast<float>(
      block_->dbuToMicrons(inst->getBBox()->getBox().dx()));
  const float height = static_cast<float>(
      block_->dbuToMicrons(inst->getBBox()->getBox().dy()));

  return width * height;
}

// Post-order DFS for clustering
void ClusteringEngine::multilevelAutocluster(Cluster* parent)
{
  bool force_split_root = false;
  if (level_ == 0) {
    const int leaf_max_std_cell
        = tree_->base_max_std_cell
          / std::pow(tree_->cluster_size_ratio, tree_->max_level - 1)
          * (1 + size_tolerance_);
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
  debugPrint(logger_,
             MPL,
             "multilevel_autoclustering",
             1,
             "Current cluster: {} - Level: {} - Macros: {} - Std Cells: {}",
             parent->getName(),
             level_,
             parent->getNumMacro(),
             parent->getNumStdCell());

  level_++;
  updateSizeThresholds();

  if (force_split_root || (parent->getNumStdCell() > max_std_cell_)) {
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

  // We define the tolerance to improve the robustness of our hierarchical
  // clustering
  max_macro_ *= (1 + size_tolerance_);
  min_macro_ *= (1 - size_tolerance_);
  max_std_cell_ *= (1 + size_tolerance_);
  min_std_cell_ *= (1 - size_tolerance_);

  if (min_macro_ <= 0) {
    min_macro_ = 1;
    max_macro_ = min_macro_ * tree_->cluster_size_ratio / 2.0;
  }

  if (min_std_cell_ <= 0) {
    min_std_cell_ = 100;
    max_std_cell_ = min_std_cell_ * tree_->cluster_size_ratio / 2.0;
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
    if (module->getChildren().size() == 0) {
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

  // When breaking large flat clusters, the children will
  // be modified, so, we need to iterate them using indexes.
  const UniqueClusterVector& new_children = parent->getChildren();
  for (int i = 0; i < new_children.size(); ++i) {
    auto& child = new_children[i];
    child->setParent(parent);
    if (child->getNumStdCell() > max_std_cell_) {
      breakLargeFlatCluster(child.get());
    }
  }
}

// Break large flat clusters with TritonPart
// Binary coding method to differentiate partitions:
// cluster -> cluster_0, cluster_1
// cluster_0 -> cluster_0_0, cluster_0_1
// cluster_1 -> cluster_1_0, cluster_1_1 [...]
void ClusteringEngine::breakLargeFlatCluster(Cluster* parent)
{
  // Check if the cluster is a large flat cluster
  if (!parent->getDbModules().empty()
      || parent->getLeafStdCells().size() < max_std_cell_) {
    return;
  }
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
    vertex_weight.push_back(computeMicronArea(macro));
    insts.push_back(macro);
  }
  for (auto& std_cell : parent->getLeafStdCells()) {
    inst_vertex_id_map[std_cell] = vertex_id++;
    vertex_weight.push_back(computeMicronArea(std_cell));
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
      hyperedges.push_back(hyperedge);
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
  breakLargeFlatCluster(parent);
  breakLargeFlatCluster(raw_part_1);
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
    updateConnections();  // update the connections between clusters

    std::vector<int> cluster_class(num_small_children, -1);  // merge flag
    std::vector<int> small_children_ids;                     // store cluster id
    small_children_ids.reserve(num_small_children);
    for (auto& small_child : small_children) {
      small_children_ids.push_back(small_child->getId());
    }
    // Firstly we perform Type 1 merge
    for (int i = 0; i < num_small_children; i++) {
      const int cluster_id = small_children[i]->getCloseCluster(
          small_children_ids, tree_->min_net_count_for_connection);
      debugPrint(
          logger_,
          MPL,
          "multilevel_autoclustering",
          1,
          "Candidate cluster: {} - {}",
          small_children[i]->getName(),
          (cluster_id != -1 ? tree_->maps.id_to_cluster[cluster_id]->getName()
                            : "   "));
      if (cluster_id != -1
          && !tree_->maps.id_to_cluster[cluster_id]->isIOCluster()) {
        Cluster* close_cluster = tree_->maps.id_to_cluster[cluster_id];
        if (attemptMerge(close_cluster, small_children[i])) {
          cluster_class[i] = close_cluster->getId();
        }
      }
    }

    // Then we perform Type 2 merge
    std::vector<Cluster*> new_small_children;
    for (int i = 0; i < num_small_children; i++) {
      if (cluster_class[i] == -1) {  // the cluster has not been merged
        for (int j = i + 1; j < num_small_children; j++) {
          if (cluster_class[j] != -1) {
            continue;
          }

          if (small_children[i]->isSameConnSignature(
                  *small_children[j], tree_->min_net_count_for_connection)) {
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

bool ClusteringEngine::attemptMerge(Cluster* receiver, Cluster* incomer)
{
  // The incomer might be deleted so we need to cache
  // its id in order to erase it from the map if so.
  const int incomer_id = incomer->getId();

  bool incomer_deleted = false;
  if (receiver->attemptMerge(incomer, incomer_deleted)) {
    if (incomer_deleted) {
      tree_->maps.id_to_cluster.erase(incomer_id);
    }

    updateInstancesAssociation(receiver);
    setClusterMetrics(receiver);

    return true;
  }

  return false;
}

void ClusteringEngine::updateConnections()
{
  for (auto& [cluster_id, cluster] : tree_->maps.id_to_cluster) {
    cluster->initConnection();
  }

  for (odb::dbNet* net : block_->getNets()) {
    if (!isValidNet(net)) {
      continue;
    }

    int driver_cluster_id = -1;
    std::vector<int> load_clusters_ids;

    for (odb::dbITerm* iterm : net->getITerms()) {
      odb::dbInst* inst = iterm->getInst();
      const int cluster_id = tree_->maps.inst_to_cluster_id.at(inst);

      if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
        driver_cluster_id = cluster_id;
      } else {
        load_clusters_ids.push_back(cluster_id);
      }
    }

    bool net_has_io_pin = false;
    for (odb::dbBTerm* bterm : net->getBTerms()) {
      const int cluster_id = tree_->maps.bterm_to_cluster_id.at(bterm);
      net_has_io_pin = true;

      if (bterm->getIoType() == odb::dbIoType::INPUT) {
        driver_cluster_id = cluster_id;
      } else {
        load_clusters_ids.push_back(cluster_id);
      }
    }

    if (driver_cluster_id != -1 && !load_clusters_ids.empty()
        && load_clusters_ids.size() < tree_->large_net_threshold) {
      const float weight = net_has_io_pin ? tree_->virtual_weight : 1.0;

      for (const int load_cluster_id : load_clusters_ids) {
        if (load_cluster_id != driver_cluster_id) { /* undirected connection */
          tree_->maps.id_to_cluster[driver_cluster_id]->addConnection(
              load_cluster_id, weight);
          tree_->maps.id_to_cluster[load_cluster_id]->addConnection(
              driver_cluster_id, weight);
        }
      }
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
  mixed_leaves.push_back(sister_mixed_leaves);
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

// Break mixed leaf into standard-cell and hard-macro clusters.
// Merge macros based on connection signature and footprint.
// Based on types of designs, we support two types of breaking up:
//   1) Replace cluster A by A1, A2, A3
//   2) Create a subtree:
//      A  ->        A
//               |   |   |
//               A1  A2  A3
void ClusteringEngine::breakMixedLeaf(Cluster* mixed_leaf)
{
  Cluster* parent = mixed_leaf;
  const float macro_dominated_cluster_ratio = 0.01;

  // Split by replacement if macro dominated.
  if (mixed_leaf->getNumStdCell() * macro_dominated_cluster_ratio
      < mixed_leaf->getNumMacro()) {
    parent = mixed_leaf->getParent();
  }

  mapMacroInCluster2HardMacro(mixed_leaf);

  std::vector<HardMacro*> hard_macros = mixed_leaf->getHardMacros();
  std::vector<Cluster*> macro_clusters;
  createOneClusterForEachMacro(parent, hard_macros, macro_clusters);

  updateConnections();

  const int number_of_macros = static_cast<int>(hard_macros.size());
  std::vector<int> size_class(number_of_macros, -1);
  std::vector<int> signature_class(number_of_macros, -1);
  std::vector<int> interconn_class(number_of_macros, -1);
  std::vector<int> macro_class(number_of_macros, -1);

  if (number_of_macros == 1) {
    // We don't want the single-macro macro cluster to be treated
    // as an array of interconnected macros with one macro.
    interconn_class.front() = -1;
    macro_class.front() = 0;
  } else {
    classifyMacrosBySize(hard_macros, size_class);
    classifyMacrosByConnSignature(macro_clusters, signature_class);
    classifyMacrosByInterconn(macro_clusters, interconn_class);
    groupSingleMacroClusters(macro_clusters,
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

  // Deal with the std cells
  if (parent == mixed_leaf) {
    addStdCellClusterToSubTree(parent, mixed_leaf, virtual_conn_clusters);
  } else {
    replaceByStdCellCluster(mixed_leaf, virtual_conn_clusters);
  }

  // Deal with the macros
  for (int i = 0; i < macro_class.size(); i++) {
    if (macro_class[i] != i) {
      continue;  // this macro cluster has been merged
    }

    macro_clusters[i]->setClusterType(HardMacroCluster);

    if (interconn_class[i] != -1) {
      macro_clusters[i]->setAsArrayOfInterconnectedMacros();
    }

    setClusterMetrics(macro_clusters[i]);
    virtual_conn_clusters.push_back(mixed_leaf->getId());
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
    Cluster* parent,
    const std::vector<HardMacro*>& hard_macros,
    std::vector<Cluster*>& macro_clusters)
{
  for (auto& hard_macro : hard_macros) {
    const std::string cluster_name = hard_macro->getName();
    auto single_macro_cluster
        = std::make_unique<Cluster>(id_, cluster_name, logger_);
    single_macro_cluster->addLeafMacro(hard_macro->getInst());
    macro_clusters.push_back(single_macro_cluster.get());

    incorporateNewCluster(std::move(single_macro_cluster), parent);
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

        if (macro_clusters[i]->isSameConnSignature(
                *macro_clusters[j], tree_->min_net_count_for_connection)) {
          signature_class[j] = i;
        }
      }
    }
  }

  if (logger_->debugCheck(MPL, "multilevel_autoclustering", 2)) {
    logger_->report("\nPrint Connection Signature\n");
    for (Cluster* cluster : macro_clusters) {
      logger_->report("Macro Signature: {}", cluster->getName());
      for (auto& [cluster_id, weight] : cluster->getConnection()) {
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
        if (macro_clusters[i]->hasMacroConnectionWith(
                *macro_clusters[j], tree_->min_net_count_for_connection)) {
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

void ClusteringEngine::addStdCellClusterToSubTree(
    Cluster* parent,
    Cluster* mixed_leaf,
    std::vector<int>& virtual_conn_clusters)
{
  std::string std_cell_cluster_name = mixed_leaf->getName();
  auto std_cell_cluster
      = std::make_unique<Cluster>(id_, std_cell_cluster_name, logger_);

  std_cell_cluster->copyInstances(*mixed_leaf);
  std_cell_cluster->clearLeafMacros();
  std_cell_cluster->setClusterType(StdCellCluster);

  setClusterMetrics(std_cell_cluster.get());

  virtual_conn_clusters.push_back(std_cell_cluster->getId());

  tree_->maps.id_to_cluster[id_++] = std_cell_cluster.get();
  std_cell_cluster->setParent(parent);
  parent->addChild(std::move(std_cell_cluster));
}

// We don't modify the physical hierarchy when spliting by replacement
void ClusteringEngine::replaceByStdCellCluster(
    Cluster* mixed_leaf,
    std::vector<int>& virtual_conn_clusters)
{
  mixed_leaf->clearLeafMacros();
  mixed_leaf->setClusterType(StdCellCluster);

  setClusterMetrics(mixed_leaf);

  virtual_conn_clusters.push_back(mixed_leaf->getId());
}

// Print Physical Hierarchy tree in a DFS manner
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

  if (parent->isClusterOfUnplacedIOPins()) {
    int number_of_pins = 0;
    for (const auto [pin, cluster_id] : tree_->maps.bterm_to_cluster_id) {
      if (cluster_id == parent->getId()) {
        ++number_of_pins;
      }
    }

    line += fmt::format(" Pins: {}", number_of_pins);
  } else if (!parent->isIOPadCluster()) {
    line += fmt::format(" {}, StdCells: {} ({} μ²), Macros: {} ({} μ²)",
                        parent->getIsLeafString(),
                        parent->getNumStdCell(),
                        parent->getStdCellArea(),
                        parent->getNumMacro(),
                        parent->getMacroArea());
  }

  logger_->report("{}", line);

  for (auto& cluster : parent->getChildren()) {
    printPhysicalHierarchyTree(cluster.get(), level + 1);
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

}  // namespace mpl
