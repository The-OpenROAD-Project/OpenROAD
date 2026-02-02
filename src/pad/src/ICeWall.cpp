// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "pad/ICeWall.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include "PadPlacer.h"
#include "RDLGui.h"
#include "RDLRouter.h"
#include "Utilities.h"
#include "boost/icl/interval_set.hpp"
#include "gui/gui.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/isotropy.h"
#include "utl/Logger.h"

namespace pad {

ICeWall::ICeWall(odb::dbDatabase* db, utl::Logger* logger)
    : db_(db), logger_(logger)
{
}

ICeWall::~ICeWall() = default;

odb::dbBlock* ICeWall::getBlock() const
{
  auto* chip = db_->getChip();
  if (chip == nullptr) {
    return nullptr;
  }

  return chip->getBlock();
}

void ICeWall::assertMasterType(odb::dbMaster* master,
                               const odb::dbMasterType& type) const
{
  if (master == nullptr) {
    logger_->error(utl::PAD, 23, "Master must be specified.");
  }
  if (master->getType() != type) {
    logger_->error(utl::PAD,
                   11,
                   "{} is not of type {}, but is instead {}",
                   master->getName(),
                   type.getString(),
                   master->getType().getString());
  }
}

void ICeWall::assertMasterType(odb::dbInst* inst,
                               const odb::dbMasterType& type) const
{
  auto* master = inst->getMaster();
  if (master->getType() != type) {
    logger_->error(utl::PAD,
                   12,
                   "{} is not of type {}, but is instead {}",
                   inst->getName(),
                   type.getString(),
                   master->getType().getString());
  }
}

void ICeWall::makeBumpArray(odb::dbMaster* master,
                            const odb::Point& start,
                            int rows,
                            int columns,
                            int xpitch,
                            int ypitch,
                            const std::string& prefix)
{
  auto* block = getBlock();
  if (block == nullptr) {
    return;
  }

  assertMasterType(master, odb::dbMasterType::COVER_BUMP);

  for (int xn = 0; xn < columns; xn++) {
    for (int yn = 0; yn < rows; yn++) {
      const odb::Point pos(start.x() + xn * xpitch, start.y() + yn * ypitch);
      const std::string name = fmt::format("{}{}_{}", prefix, xn, yn);
      auto* inst = odb::dbInst::create(block, master, name.c_str());

      inst->setOrigin(pos.x(), pos.y());
      inst->setPlacementStatus(odb::dbPlacementStatus::FIRM);
    }
  }
}

void ICeWall::removeBump(odb::dbInst* inst)
{
  if (inst == nullptr) {
    return;
  }

  assertMasterType(inst, odb::dbMasterType::COVER_BUMP);

  odb::dbInst::destroy(inst);
}

void ICeWall::removeBumpArray(odb::dbMaster* master)
{
  auto* block = getBlock();
  if (block == nullptr) {
    return;
  }

  assertMasterType(master, odb::dbMasterType::COVER_BUMP);

  for (auto* inst : block->getInsts()) {
    if (inst->getMaster() == master) {
      removeBump(inst);
    }
  }
}

void ICeWall::makeBTerm(odb::dbNet* net,
                        odb::dbTechLayer* layer,
                        const odb::Rect& shape) const
{
  odb::dbBTerm* bterm = net->get1stBTerm();
  if (bterm == nullptr) {
    logger_->warn(utl::PAD,
                  33,
                  "Could not find a block terminal associated with net: "
                  "\"{}\", creating now.",
                  net->getName());
    bterm = odb::dbBTerm::create(net, net->getConstName());
    if (bterm == nullptr) {
      logger_->error(
          utl::PAD, 34, "Unable to create block terminal: {}", net->getName());
    }
  }
  bterm->setSigType(net->getSigType());
  odb::dbBPin* pin = odb::dbBPin::create(bterm);
  odb::dbBox::create(
      pin, layer, shape.xMin(), shape.yMin(), shape.xMax(), shape.yMax());
  pin->setPlacementStatus(odb::dbPlacementStatus::FIRM);

  const double dbus = getBlock()->getDbUnitsPerMicron();
  logger_->info(utl::PAD,
                116,
                "Creating terminal for {} on {} at ({:.3f}um, {:.3f}um) - "
                "({:.3f}um, {:.3f}um)",
                net->getName(),
                layer->getName(),
                shape.xMin() / dbus,
                shape.yMin() / dbus,
                shape.xMax() / dbus,
                shape.yMax() / dbus);
  Utilities::makeSpecial(net);
}

void ICeWall::assignBump(odb::dbInst* inst,
                         odb::dbNet* net,
                         odb::dbITerm* terminal,
                         bool dont_route)
{
  if (inst == nullptr) {
    logger_->error(
        utl::PAD, 24, "Instance must be specified to assign it to a bump.");
  }

  if (net == nullptr) {
    logger_->error(
        utl::PAD, 25, "Net must be specified to assign it to a bump.");
  }

  assertMasterType(inst, odb::dbMasterType::COVER_BUMP);

  const odb::dbTransform xform = inst->getTransform();

  odb::dbTechLayer* top_layer = nullptr;
  std::set<odb::Rect> top_shapes;
  for (auto* iterm : inst->getITerms()) {
    if (iterm->getNet() != net) {
      iterm->connect(net);
    }
    if (terminal) {
      auto already_assigned = std::ranges::find_if(
          routing_map_,
          [terminal](const auto& other) { return other.second == terminal; });
      if (already_assigned != routing_map_.end()) {
        logger_->error(
            utl::PAD, 35, "{} has already been assigned.", terminal->getName());
      }
      if (terminal->getNet() == nullptr) {
        terminal->connect(net);
      } else if (terminal->getNet() != net) {
        logger_->error(utl::PAD,
                       36,
                       "{} is not connected to {}, but connected to {}.",
                       terminal->getName(),
                       net->getName(),
                       terminal->getNet()->getName());
      }
      routing_map_[iterm] = terminal;
      terminal = nullptr;
    }
    if (dont_route) {
      routing_map_[iterm] = nullptr;
    }

    for (auto* mpin : iterm->getMTerm()->getMPins()) {
      for (auto* geom : mpin->getGeometry()) {
        auto* layer = geom->getTechLayer();
        if (layer == nullptr) {
          continue;
        }

        if (top_layer == nullptr
            || top_layer->getRoutingLevel() <= layer->getRoutingLevel()) {
          top_layer = layer;
          if (top_layer->getRoutingLevel() < layer->getRoutingLevel()) {
            top_shapes.clear();
          }
          top_shapes.insert(geom->getBox());
        }
      }
    }
  }

  if (top_layer != nullptr) {
    odb::Rect master_box;
    inst->getMaster()->getPlacementBoundary(master_box);
    const odb::Point center = master_box.center();

    const odb::Rect* top_shape_ptr = nullptr;
    for (const odb::Rect& shape : top_shapes) {
      if (shape.intersects(center)) {
        top_shape_ptr = &shape;
      }
    }

    if (top_shape_ptr == nullptr) {
      top_shape_ptr = &(*top_shapes.begin());
    }

    odb::Rect top_shape = *top_shape_ptr;

    xform.apply(top_shape);
    makeBTerm(net, top_layer, top_shape);
  }
}

void ICeWall::makeFakeSite(const std::string& name, int width, int height)
{
  auto* lib = db_->findLib(kFakeLibraryName);
  if (lib == nullptr) {
    lib = odb::dbLib::create(db_, kFakeLibraryName, db_->getTech());
  }

  auto* site = odb::dbSite::create(lib, name.c_str());
  site->setWidth(width);
  site->setHeight(height);
  site->setClass(odb::dbSiteClass::PAD);
}

std::string ICeWall::getRowName(const std::string& name, int ring_index) const
{
  if (ring_index < 0) {
    return name;
  }
  return fmt::format("{}_{}", name, ring_index);
}

void ICeWall::makeIORow(odb::dbSite* horizontal_site,
                        odb::dbSite* vertical_site,
                        odb::dbSite* corner_site,
                        int west_offset,
                        int north_offset,
                        int east_offset,
                        int south_offset,
                        const odb::dbOrientType& rotation_hor,
                        const odb::dbOrientType& rotation_ver,
                        const odb::dbOrientType& rotation_cor,
                        int ring_index)
{
  auto* block = getBlock();
  if (block == nullptr) {
    return;
  }

  if (horizontal_site == nullptr) {
    logger_->error(utl::PAD, 14, "Horizontal site must be speficied.");
  }
  if (vertical_site == nullptr) {
    logger_->error(utl::PAD, 15, "Vertical site must be speficied.");
  }
  if (corner_site == nullptr) {
    logger_->error(utl::PAD, 16, "Corner site must be speficied.");
  }

  const odb::Rect die = block->getDieArea();

  odb::Rect outer_io(die.xMin() + west_offset,
                     die.yMin() + south_offset,
                     die.xMax() - east_offset,
                     die.yMax() - north_offset);

  const double dbus = block->getDbUnitsPerMicron();
  debugPrint(logger_,
             utl::PAD,
             "Rows",
             1,
             "Outer bounds ({:.4f}, {:.4f}) - ({:.4f}, {:.4f})",
             outer_io.xMin() / dbus,
             outer_io.yMin() / dbus,
             outer_io.xMax() / dbus,
             outer_io.yMax() / dbus);

  const odb::Rect corner_box(
      0, 0, corner_site->getWidth(), corner_site->getHeight());
  const odb::Rect horizontal_box(
      0, 0, horizontal_site->getWidth(), horizontal_site->getHeight());
  const odb::Rect vertical_box(
      0, 0, vertical_site->getWidth(), vertical_site->getHeight());

  const int cheight = corner_site->getHeight();
  const int cwidth
      = std::max(horizontal_site->getHeight(), corner_site->getWidth());

  const int x_sites = std::floor(static_cast<double>(outer_io.dx() - 2 * cwidth)
                                 / vertical_box.minDXDY());
  outer_io.set_xhi(outer_io.xMin() + 2 * cwidth
                   + x_sites * vertical_box.minDXDY());
  const int y_sites
      = std::floor(static_cast<double>(outer_io.dy() - 2 * cheight)
                   / horizontal_box.minDXDY());
  outer_io.set_yhi(outer_io.yMin() + 2 * cheight
                   + y_sites * horizontal_box.minDXDY());

  const odb::Rect corner_origins(outer_io.xMin(),
                                 outer_io.yMin(),
                                 outer_io.xMax() - cwidth,
                                 outer_io.yMax() - cheight);

  debugPrint(logger_,
             utl::PAD,
             "Rows",
             2,
             "x-sites {} at {:.4f}",
             x_sites,
             vertical_box.minDXDY() / dbus);

  debugPrint(logger_,
             utl::PAD,
             "Rows",
             2,
             "y-sites {} at {:.4f}",
             y_sites,
             horizontal_box.minDXDY() / dbus);

  debugPrint(logger_,
             utl::PAD,
             "Rows",
             1,
             "Corner origins ({:.4f}, {:.4f}) - ({:.4f}, {:.4f})",
             corner_origins.xMin() / dbus,
             corner_origins.yMin() / dbus,
             corner_origins.xMax() / dbus,
             corner_origins.yMax() / dbus);

  // Create corners
  const int corner_sites
      = std::max(horizontal_box.maxDXDY(), corner_site->getWidth())
        / corner_site->getWidth();
  debugPrint(logger_,
             utl::PAD,
             "Rows",
             1,
             "corner-sites {} at {:.4f}",
             corner_sites,
             corner_site->getWidth() / dbus);

  auto create_corner
      = [this, block, corner_site, corner_sites, ring_index, &rotation_cor](
            const std::string& name,
            const odb::Point& origin,
            const odb::dbOrientType& orient) -> odb::dbRow* {
    const std::string row_name = getRowName(name, ring_index);
    odb::dbTransform rotation(rotation_cor);
    rotation.concat(orient);
    return odb::dbRow::create(block,
                              row_name.c_str(),
                              corner_site,
                              origin.x(),
                              origin.y(),
                              rotation.getOrient(),
                              odb::dbRowDir::HORIZONTAL,
                              corner_sites,
                              corner_site->getWidth());
  };
  auto* nw
      = create_corner(kCornerNw, corner_origins.ul(), odb::dbOrientType::MX);
  create_corner(kCornerNe, corner_origins.ur(), odb::dbOrientType::R180);
  auto* se
      = create_corner(kCornerSe, corner_origins.lr(), odb::dbOrientType::MY);
  auto* sw
      = create_corner(kCornerSw, corner_origins.ll(), odb::dbOrientType::R0);

  // Create rows
  auto create_row
      = [this, block, ring_index](const std::string& name,
                                  odb::dbSite* site,
                                  int sites,
                                  const odb::Point& origin,
                                  const odb::dbOrientType& orient,
                                  const odb::dbOrientType& row_rotation,
                                  const odb::dbRowDir& direction,
                                  int site_width) {
          const std::string row_name = getRowName(name, ring_index);
          odb::dbTransform rotation(row_rotation);
          rotation.concat(orient);
          odb::dbRow::create(block,
                             row_name.c_str(),
                             site,
                             origin.x(),
                             origin.y(),
                             rotation.getOrient(),
                             direction,
                             sites,
                             site_width);
        };
  const odb::dbOrientType south_rotation_ver = odb::dbOrientType::R0;
  const odb::dbOrientType north_rotation_ver = south_rotation_ver.flipX();
  odb::dbOrientType west_rotation_hor = odb::dbOrientType::MXR90;
  if (vertical_site != horizontal_site) {
    west_rotation_hor = odb::dbOrientType::R0;
  }
  const odb::dbOrientType east_rotation_hor = west_rotation_hor.flipY();
  create_row(kRowNorth,
             vertical_site,
             x_sites,
             {nw->getBBox().xMax(), outer_io.yMax() - vertical_box.maxDXDY()},
             north_rotation_ver,
             rotation_ver,
             odb::dbRowDir::HORIZONTAL,
             vertical_box.minDXDY());
  create_row(kRowEast,
             horizontal_site,
             y_sites,
             {outer_io.xMax() - horizontal_box.maxDXDY(), se->getBBox().yMax()},
             east_rotation_hor,
             rotation_hor,
             odb::dbRowDir::VERTICAL,
             horizontal_box.minDXDY());
  create_row(kRowSouth,
             vertical_site,
             x_sites,
             {sw->getBBox().xMax(), outer_io.yMin()},
             south_rotation_ver,
             rotation_ver,
             odb::dbRowDir::HORIZONTAL,
             vertical_box.minDXDY());
  create_row(kRowWest,
             horizontal_site,
             y_sites,
             {outer_io.xMin(), sw->getBBox().yMax()},
             west_rotation_hor,
             rotation_hor,
             odb::dbRowDir::VERTICAL,
             horizontal_box.minDXDY());
}

void ICeWall::removeIORows()
{
  auto* block = getBlock();
  if (block == nullptr) {
    return;
  }

  for (auto* row : getRows()) {
    odb::dbRow::destroy(row);
  }
}

void ICeWall::placeCorner(odb::dbMaster* master, int ring_index)
{
  auto* block = getBlock();
  if (block == nullptr) {
    return;
  }

  if (master == nullptr) {
    logger_->error(utl::PAD, 28, "Corner master must be specified.");
  }

  for (const char* row_name : {kCornerNw, kCornerNe, kCornerSw, kCornerSe}) {
    odb::dbRow* row = findRow(getRowName(row_name, ring_index));
    if (row == nullptr) {
      logger_->error(utl::PAD,
                     13,
                     "Unable to find {} row to place a corner cell in",
                     row_name);
    }

    const std::string corner_name = fmt::format("{}_INST", row->getName());
    odb::dbInst* inst = block->findInst(corner_name.c_str());

    const odb::Rect row_bbox = row->getBBox();

    // Check for instances overlapping the corner site
    bool place_inst = true;
    for (auto* check_inst : block->getInsts()) {
      if (check_inst == inst) {
        continue;
      }
      if (!check_inst->isFixed()) {
        continue;
      }
      if (check_inst->getMaster()->isCover()) {
        continue;
      }
      const odb::Rect check_rect = check_inst->getBBox()->getBox();
      if (row_bbox.overlaps(check_rect)) {
        place_inst = false;
        break;
      }
    }
    if (!place_inst) {
      logger_->warn(
          utl::PAD,
          44,
          "Skipping corner cell placement in {} due to overlapping instances",
          row->getName());
      continue;
    }

    const bool create_inst = inst == nullptr;
    if (create_inst) {
      inst = odb::dbInst::create(block, master, corner_name.c_str());
    }

    inst->setOrient(row->getOrient());
    inst->setLocation(row_bbox.xMin(), row_bbox.yMin());
    inst->setPlacementStatus(odb::dbPlacementStatus::FIRM);

    const CheckerOnlyPadPlacer checker(logger_, block, row);
    if (!checker.check(inst)) {
      if (create_inst) {
        logger_->warn(utl::PAD,
                      45,
                      "Skipping corner cell generation for {} due to "
                      "overlapping bump cell",
                      inst->getName());
        odb::dbInst::destroy(inst);
      } else {
        logger_->warn(utl::PAD,
                      46,
                      "Skipping corner cell placement for {} due to "
                      "overlapping bump cell",
                      inst->getName());
        inst->setPlacementStatus(odb::dbPlacementStatus::UNPLACED);
      }
    }
  }
}

void ICeWall::placePad(odb::dbMaster* master,
                       const std::string& name,
                       odb::dbRow* row,
                       int location,
                       bool mirror)
{
  auto* block = getBlock();
  if (block == nullptr) {
    return;
  }

  odb::dbInst* inst = block->findInst(name.c_str());
  if (inst == nullptr) {
    if (master == nullptr) {
      logger_->error(
          utl::PAD, 18, "Unable to create instance {} without master", name);
    }
    inst = odb::dbInst::create(block, master, name.c_str());
  }

  if (master != nullptr && inst->getMaster() != master) {
    logger_->error(utl::PAD,
                   118,
                   "Master cell mismatch for {} ({}) is not {}",
                   inst->getName(),
                   inst->getMaster()->getName(),
                   master->getName());
  }

  if (row == nullptr) {
    logger_->error(utl::PAD, 19, "Row must be specified to place a pad");
  }

  odb::dbTransform orient(odb::dbOrientType::R0);
  if (mirror) {
    const auto row_edge = getRowEdge(row);
    switch (row_edge) {
      case odb::Direction2D::North:
      case odb::Direction2D::South: {
        orient.concat({odb::dbOrientType::MY});
        break;
      }
      case odb::Direction2D::West:
      case odb::Direction2D::East: {
        if (row->getSite()->getHeight() < row->getSite()->getWidth()) {
          orient.concat({odb::dbOrientType::MX});
        } else {
          orient.concat({odb::dbOrientType::MY});
        }
        break;
      }
    }
  }

  SingleInstPadPlacer placer(logger_, getBlock(), getRowEdge(row), row);
  placer.place(inst, location, orient.getOrient());
}

void ICeWall::placePads(const std::vector<odb::dbInst*>& insts,
                        odb::dbRow* row,
                        const PlacementStrategy& mode)
{
  auto* block = getBlock();
  if (block == nullptr) {
    return;
  }

  const odb::Rect row_bbox = row->getBBox();
  const odb::dbTransform xform(row->getOrient());
  const odb::Direction2D::Value row_dir = getRowEdge(row);

  // Check total width
  int total_width = 0;
  for (auto* inst : insts) {
    auto* master = inst->getMaster();
    odb::Rect inst_bbox;
    master->getPlacementBoundary(inst_bbox);
    xform.apply(inst_bbox);

    switch (row_dir) {
      case odb::Direction2D::North:
      case odb::Direction2D::South:
        total_width += inst_bbox.dx();
        break;
      case odb::Direction2D::West:
      case odb::Direction2D::East:
        total_width += inst_bbox.dy();
        break;
    }
  }

  int row_width = 0;
  switch (row_dir) {
    case odb::Direction2D::North:
    case odb::Direction2D::South:
      row_width = row_bbox.dx();
      break;
    case odb::Direction2D::West:
    case odb::Direction2D::East:
      row_width = row_bbox.dy();
      break;
  }

  const double dbus = block->getDbUnitsPerMicron();
  debugPrint(logger_,
             utl::PAD,
             "Place",
             1,
             "{}: Row width ({:.4f} um), total instance ({}) width ({:.4f} um)",
             row->getName(),
             row_width / dbus,
             insts.size(),
             total_width / dbus);

  if (total_width > row_width) {
    logger_->error(
        utl::PAD,
        40,
        "Total pad width ({:.4f}um) cannot fit into {} with width {:.4f}um.",
        total_width / dbus,
        row->getName(),
        row_width / dbus);
  }

  // Check if bumps are present and have assignments
  std::map<odb::dbInst*, std::set<odb::dbITerm*>> iterm_connections;
  for (auto* inst : insts) {
    for (auto* iterm : inst->getITerms()) {
      odb::dbNet* net = iterm->getNet();
      if (net == nullptr) {
        continue;
      }
      if (net->getSigType().isSupply()) {
        // ignore supply nets as these can likely be connected via the power
        // grid
        continue;
      }
      for (auto* net_iterm : net->getITerms()) {
        auto* master = net_iterm->getInst()->getMaster();
        if (master->isCover()) {
          iterm_connections[inst].insert(net_iterm);
        }
      }
    }
  }

  PlacementStrategy use_mode = mode;
  if (use_mode == PlacementStrategy::DEFAULT) {
    if (!iterm_connections.empty()) {
      use_mode = PlacementStrategy::BUMP_ALIGNED;
    }
  }
  if (use_mode == PlacementStrategy::BUMP_ALIGNED
      && iterm_connections.empty()) {
    logger_->warn(
        utl::PAD, 9, "Unable to use bump_aligned mode, switching to uniform");
    use_mode = PlacementStrategy::UNIFORM;
  }

  std::unique_ptr<PadPlacer> placer;
  switch (use_mode) {
    case PlacementStrategy::BUMP_ALIGNED: {
      auto bump_placer = std::make_unique<BumpAlignedPadPlacer>(
          logger_, block, insts, row_dir, row);
      bump_placer->setConnections(iterm_connections);
      placer = std::move(bump_placer);
      break;
    }
    case PlacementStrategy::PLACER: {
      auto bump_placer = std::make_unique<PlacerPadPlacer>(
          logger_, block, insts, row_dir, row);
      bump_placer->setConnections(iterm_connections);
      placer = std::move(bump_placer);
      break;
    }
    case PlacementStrategy::LINEAR:
      placer = std::make_unique<UniformPadPlacer>(
          logger_, block, insts, row_dir, row, 0);
      break;
    case PlacementStrategy::UNIFORM:
    case PlacementStrategy::DEFAULT:
      placer = std::make_unique<UniformPadPlacer>(
          logger_, block, insts, row_dir, row);
      break;
  }

  if (logger_->debugCheck(utl::PAD, "Place", 2)) {
    int idx = 0;
    logger_->debug(
        utl::PAD, "Place", "Pad placement order ({}):", insts.size());
    for (auto* inst : insts) {
      logger_->debug(utl::PAD, "Place", "  {:>5}: {}", ++idx, inst->getName());
    }
  }

  placer->place();

  logger_->info(
      utl::PAD, 41, "Placed {} pads in {}.", insts.size(), row->getName());
}

odb::dbRow* ICeWall::findRow(const std::string& name) const
{
  for (auto* row : getBlock()->getRows()) {
    if (row->getName() == name) {
      return row;
    }
  }
  return nullptr;
}

odb::Direction2D::Value ICeWall::getRowEdge(odb::dbRow* row) const
{
  const std::string row_name = row->getName();
  auto check_name = [&row_name](const std::string& check) -> bool {
    return row_name.substr(0, check.length()) == check;
  };
  if (check_name(kRowNorth)) {
    return odb::Direction2D::North;
  }
  if (check_name(kRowSouth)) {
    return odb::Direction2D::South;
  }
  if (check_name(kRowEast)) {
    return odb::Direction2D::East;
  }
  if (check_name(kRowWest)) {
    return odb::Direction2D::West;
  }
  logger_->error(utl::PAD, 29, "{} is not a recognized IO row.", row_name);
}

void ICeWall::placeFiller(
    const std::vector<odb::dbMaster*>& masters,
    odb::dbRow* row,
    const std::vector<odb::dbMaster*>& overlapping_masters)
{
  auto* block = getBlock();
  if (block == nullptr) {
    return;
  }

  if (row == nullptr) {
    logger_->error(utl::PAD, 20, "Row must be specified to place IO filler");
  }

  bool use_height = false;
  if (getRowEdge(row) == odb::Direction2D::West
      || getRowEdge(row) == odb::Direction2D::East) {
    use_height = true;
  }

  SingleInstPadPlacer placer(logger_, block, getRowEdge(row), row);

  const odb::dbTransform row_xform(row->getOrient());

  const double dbus = block->getDbUnitsPerMicron();

  std::vector<odb::dbMaster*> fillers = masters;
  // remove nullptrs
  std::erase(fillers, nullptr);
  // sort by width
  std::ranges::stable_sort(
      fillers,
      [use_height, row_xform](odb::dbMaster* r, odb::dbMaster* l) -> bool {
        odb::Rect r_bbox;
        r->getPlacementBoundary(r_bbox);
        odb::Rect l_bbox;
        l->getPlacementBoundary(l_bbox);
        row_xform.apply(r_bbox);
        row_xform.apply(l_bbox);
        // sort biggest to smallest
        if (use_height) {
          return r_bbox.dy() > l_bbox.dy();
        }
        return r_bbox.dx() > l_bbox.dx();
      });

  const odb::Rect rowbbox = row->getBBox();

  using Interval = boost::icl::interval_set<int>;
  Interval placed_io;
  for (auto* inst : getPadInstsInRow(row)) {
    const odb::Rect bbox = inst->getBBox()->getBox();
    if (row->getDirection() == odb::dbRowDir::HORIZONTAL) {
      placed_io.insert(
          Interval::interval_type::closed(bbox.xMin(), bbox.xMax()));
    } else {
      placed_io.insert(
          Interval::interval_type::closed(bbox.yMin(), bbox.yMax()));
    }

    debugPrint(
        logger_,
        utl::PAD,
        "Fill",
        2,
        "Instance in {} -> {} ({:.3f}um, {:.3f}um) -> ({:.3f}um, {:.3f}um)",
        row->getName(),
        inst->getName(),
        bbox.ll().x() / dbus,
        bbox.ll().y() / dbus,
        bbox.ur().x() / dbus,
        bbox.ur().y() / dbus);
  }

  Interval row_interval;
  if (row->getDirection() == odb::dbRowDir::HORIZONTAL) {
    row_interval.insert(
        Interval::interval_type::closed(rowbbox.xMin(), rowbbox.xMax()));
  } else {
    row_interval.insert(
        Interval::interval_type::closed(rowbbox.yMin(), rowbbox.yMax()));
  }

  row_interval -= placed_io;

  const int row_start = placer.getRowStart();
  const int site_width
      = std::min(row->getSite()->getWidth(), row->getSite()->getHeight());
  int fill_group = 0;
  for (Interval::iterator it = row_interval.begin(); it != row_interval.end();
       it++) {
    const int width = it->upper() - it->lower();
    const int start = it->lower();
    if (width % site_width != 0) {
      logger_->error(utl::PAD,
                     26,
                     "Filling {} ({:.3f}um -> {:.3f}um) will result in a gap.",
                     row->getName(),
                     it->lower() / dbus,
                     it->upper() / dbus);
    }
    int sites = width / site_width;
    const int start_site_index = placer.snapToRowSite(start);

    debugPrint(logger_,
               utl::PAD,
               "Fill",
               1,
               "Filling {} : {:.3f}um -> {:.3f}um",
               row->getName(),
               it->lower() / dbus,
               it->upper() / dbus);
    debugPrint(logger_,
               utl::PAD,
               "Fill",
               2,
               "  start index {} width {}",
               start_site_index,
               sites);

    int site_offset = 0;
    for (auto* filler : fillers) {
      const bool allow_overlap = std::ranges::find(overlapping_masters, filler)
                                 != overlapping_masters.end();
      odb::Rect filler_bbox;
      filler->getPlacementBoundary(filler_bbox);
      row_xform.apply(filler_bbox);
      int filler_width;
      if (use_height) {
        filler_width = filler_bbox.dy();
      } else {
        filler_width = filler_bbox.dx();
      }
      const int fill_width = filler_width / site_width;
      while (fill_width <= sites || allow_overlap) {
        debugPrint(logger_,
                   utl::PAD,
                   "Fill",
                   2,
                   "    fill cell {} width {} remaining sites {} / overlap {}",
                   filler->getName(),
                   fill_width,
                   sites,
                   allow_overlap);

        const std::string name = fmt::format(
            "{}{}_{}_{}", kFillPrefix, row->getName(), fill_group, site_offset);
        auto* fill_inst = odb::dbInst::create(block, filler, name.c_str());

        placer.place(
            fill_inst,
            row_start + (site_width * (start_site_index + site_offset)),
            odb::dbOrientType::R0,
            allow_overlap);

        site_offset += fill_width;
        sites -= fill_width;

        if (sites <= 0) {
          break;
        }
      }
    }

    if (sites > 0) {
      logger_->error(
          utl::PAD,
          30,
          "Unable to fill gap completely {:.3f}um -> {:.3f}um in row {}",
          it->lower() / dbus,
          it->upper() / dbus,
          row->getName());
    }
    fill_group++;
  }
}

void ICeWall::removeFiller(odb::dbRow* row)
{
  auto* block = getBlock();
  if (block == nullptr) {
    return;
  }

  if (row == nullptr) {
    logger_->error(utl::PAD, 21, "Row must be specified to remove IO filler");
  }

  const std::string prefix = fmt::format("{}{}_", kFillPrefix, row->getName());

  for (auto* inst : block->getInsts()) {
    const std::string name = inst->getName();
    if (name.substr(0, prefix.length()) == prefix) {
      odb::dbInst::destroy(inst);
    }
  }
}

void ICeWall::placeBondPads(odb::dbMaster* bond,
                            const std::vector<odb::dbInst*>& pads,
                            const odb::dbOrientType& rotation,
                            const odb::Point& offset,
                            const std::string& prefix)
{
  auto* block = getBlock();
  if (block == nullptr) {
    return;
  }

  if (bond == nullptr) {
    logger_->error(
        utl::PAD, 27, "Bond master must be specified to place bond pads");
  }

  assertMasterType(bond, odb::dbMasterType::COVER);

  odb::Rect bond_rect;
  odb::dbTechLayer* bond_layer = nullptr;
  std::map<odb::dbTechLayer*, int> shape_count;
  for (auto* mterm : bond->getMTerms()) {
    for (auto* mpin : mterm->getMPins()) {
      for (auto* geom : mpin->getGeometry()) {
        auto* pin_layer = geom->getTechLayer();
        if (pin_layer == nullptr) {
          continue;
        }
        if (pin_layer->getRoutingLevel() == 0) {
          continue;
        }

        if (bond_layer == nullptr
            || bond_layer->getRoutingLevel() < pin_layer->getRoutingLevel()) {
          bond_layer = pin_layer;
          bond_rect = geom->getBox();
          shape_count[bond_layer]++;
        }
      }
    }
  }
  if (bond_layer == nullptr) {
    logger_->error(utl::PAD,
                   32,
                   "Unable to determine the top layer of {}",
                   bond->getName());
  }
  if (shape_count[bond_layer] > 1) {
    logger_->warn(utl::PAD,
                  31,
                  "{} contains more than 1 pin shape on {}",
                  bond->getName(),
                  bond_layer->getName());
  }

  const odb::dbTransform pad_xform(rotation);

  for (auto* inst : pads) {
    if (inst == nullptr) {
      continue;
    }
    if (!inst->isFixed()) {
      continue;
    }

    odb::dbTransform pad_transform(inst->getOrient());
    odb::Point pad_offset = offset;
    pad_transform.apply(pad_offset);
    const odb::Point origin = inst->getOrigin();
    const odb::Point pad_loc(origin.x() + pad_offset.x(),
                             origin.y() + pad_offset.y());

    pad_transform.concat(pad_xform);
    const odb::dbOrientType pad_orient = pad_transform.getOrient();

    const std::string name = fmt::format("{}{}", prefix, inst->getName());

    odb::dbInst* bond_inst = odb::dbInst::create(block, bond, name.c_str());
    bond_inst->setOrient(pad_orient);
    bond_inst->setOrigin(pad_loc.x(), pad_loc.y());
    bond_inst->setPlacementStatus(odb::dbPlacementStatus::FIRM);

    const odb::dbTransform xform = bond_inst->getTransform();
    odb::Rect bpin_shape = bond_rect;
    xform.apply(bpin_shape);

    // connect bond and pad
    for (const auto& [iterm0, iterm1] : getTouchingIterms(inst, bond_inst)) {
      odb::dbITerm* inst_term = iterm0;
      odb::dbITerm* pad_term = iterm1;
      if (inst_term->getInst() != inst) {
        std::swap(inst_term, pad_term);
      }

      auto* net = inst_term->getNet();
      if (net == nullptr) {
        continue;
      }
      pad_term->connect(net);
      makeBTerm(net, bond_layer, bpin_shape);
    }
  }
}

void ICeWall::placeTerminals(const std::vector<odb::dbITerm*>& iterms,
                             const bool allow_non_top_layer)
{
  auto* block = getBlock();
  if (block == nullptr) {
    return;
  }

  auto* tech = block->getDataBase()->getTech();
  auto* top_layer = tech->findRoutingLayer(tech->getRoutingLayerCount());

  for (auto* iterm : iterms) {
    if (iterm == nullptr) {
      continue;
    }
    auto* net = iterm->getNet();
    if (net == nullptr) {
      continue;
    }
    auto* inst = iterm->getInst();
    if (!inst->isFixed()) {
      continue;
    }
    if (!inst->isPad()) {
      continue;
    }

    const odb::dbTransform pad_transform = inst->getTransform();

    auto* mterm = iterm->getMTerm();
    odb::dbBox* pin_shape = nullptr;
    int highest_level = 0;
    for (auto* mpin : mterm->getMPins()) {
      for (auto* geom : mpin->getGeometry()) {
        auto* layer = geom->getTechLayer();
        if (layer == nullptr) {
          continue;
        }
        if (layer->getRoutingLevel() <= highest_level) {
          continue;
        }

        pin_shape = geom;
        highest_level = layer->getRoutingLevel();
      }
    }

    if (pin_shape == nullptr) {
      logger_->error(utl::PAD,
                     115,
                     "Unable to place a terminal on {} for {}/{}",
                     top_layer->getName(),
                     inst->getName(),
                     mterm->getName());
    }

    auto layer = tech->findRoutingLayer(highest_level);
    if (!allow_non_top_layer && layer != top_layer) {
      logger_->error(utl::PAD,
                     120,
                     "No shape in terminal {}/{} found on layer {}",
                     inst->getName(),
                     mterm->getName(),
                     top_layer->getName());
    }

    odb::Rect shape = pin_shape->getBox();
    pad_transform.apply(shape);

    makeBTerm(net, layer, shape);
  }
}

void ICeWall::connectByAbutment()
{
  const std::vector<odb::dbInst*> io_insts = getPadInsts();

  debugPrint(logger_,
             utl::PAD,
             "Connect",
             1,
             "Connecting {} instances by abutment",
             io_insts.size());

  // Collect all touching iterms
  std::vector<std::pair<odb::dbITerm*, odb::dbITerm*>> connections;
  for (size_t i = 0; i < io_insts.size(); i++) {
    auto* inst0 = io_insts[i];
    for (size_t j = i + 1; j < io_insts.size(); j++) {
      auto* inst1 = io_insts[j];
      const auto inst_connections = getTouchingIterms(inst0, inst1);
      connections.insert(
          connections.end(), inst_connections.begin(), inst_connections.end());
    }
  }
  debugPrint(logger_,
             utl::PAD,
             "Connect",
             1,
             "{} touching iterms found",
             connections.size());

  // begin connections for current signals
  std::set<odb::dbNet*> special_nets = connectByAbutment(connections);

  // make nets for newly formed nets
  for (const auto& [iterm0, iterm1] : connections) {
    auto* net = iterm0->getNet();
    if (net == nullptr) {
      const std::string netname = fmt::format("{}_RING", iterm0->getName('.'));
      odb::dbNet* new_net = odb::dbNet::create(getBlock(), netname.c_str());
      iterm0->connect(new_net);
      iterm1->connect(new_net);

      const auto new_nets = connectByAbutment(connections);
      special_nets.insert(new_nets.begin(), new_nets.end());
    }
  }

  for (auto* net : special_nets) {
    Utilities::makeSpecial(net);
  }
}

std::set<odb::dbNet*> ICeWall::connectByAbutment(
    const std::vector<std::pair<odb::dbITerm*, odb::dbITerm*>>& connections)
    const
{
  std::set<odb::dbNet*> special_nets;
  bool changed = false;
  int iter = 0;

  // remove nets with a single iterm/bterm connection
  for (const auto& [iterm0, iterm1] : connections) {
    auto* net0 = iterm0->getNet();
    if (net0 != nullptr) {
      const int connections = net0->getITermCount() + net0->getBTermCount();
      if (connections == 1) {
        odb::dbNet::destroy(net0);
      }
    }

    auto* net1 = iterm1->getNet();
    if (net1 != nullptr) {
      const int connections = net1->getITermCount() + net1->getBTermCount();
      if (connections == 1) {
        odb::dbNet::destroy(net1);
      }
    }
  }

  do {
    changed = false;
    debugPrint(logger_,
               utl::PAD,
               "Connect",
               1,
               "Start of connecting iteration {}",
               iter);

    for (const auto& [iterm0, iterm1] : connections) {
      auto* net0 = iterm0->getNet();
      auto* net1 = iterm1->getNet();

      if (net0 == net1) {
        continue;
      }

      if (net0 != nullptr && net1 != nullptr) {
        // ERROR, touching, but different nets
        logger_->error(utl::PAD,
                       2,
                       "{} ({}) and {} ({}) are touching, but are "
                       "connected to different nets",
                       iterm0->getName(),
                       net0->getName(),
                       iterm1->getName(),
                       net1->getName());
      }

      auto* connect_net = net0;
      if (connect_net == nullptr) {
        connect_net = net1;
      }

      debugPrint(logger_,
                 utl::PAD,
                 "Connect",
                 1,
                 "Connecting net {} to {} ({}) and {} ({})",
                 connect_net->getName(),
                 iterm0->getName(),
                 net0 != nullptr ? net0->getName() : "NULL",
                 iterm1->getName(),
                 net1 != nullptr ? net1->getName() : "NULL");

      if (net0 != connect_net) {
        iterm0->connect(connect_net);
        special_nets.insert(connect_net);
        changed = true;
      }
      if (net1 != connect_net) {
        iterm1->connect(connect_net);
        special_nets.insert(connect_net);
        changed = true;
      }
    }
    iter++;
  } while (changed);

  return special_nets;
}

std::vector<std::pair<odb::dbITerm*, odb::dbITerm*>> ICeWall::getTouchingIterms(
    odb::dbInst* inst0,
    odb::dbInst* inst1)
{
  if (!inst0->getBBox()->getBox().intersects(inst1->getBBox()->getBox())) {
    return {};
  }

  using ShapeMap = std::map<odb::dbTechLayer*, std::set<odb::Rect>>;
  auto populate_map = [](odb::dbITerm* iterm) -> ShapeMap {
    ShapeMap map;
    const odb::dbTransform xform = iterm->getInst()->getTransform();

    for (auto* mpin : iterm->getMTerm()->getMPins()) {
      for (auto* geom : mpin->getGeometry()) {
        auto* layer = geom->getTechLayer();
        if (layer == nullptr) {
          continue;
        }
        odb::Rect shape = geom->getBox();
        xform.apply(shape);
        map[layer].insert(shape);
      }
    }
    return map;
  };

  std::map<odb::dbITerm*, ShapeMap> iterm_map;
  for (auto* iterm : inst0->getITerms()) {
    iterm_map[iterm] = populate_map(iterm);
  }
  for (auto* iterm : inst1->getITerms()) {
    iterm_map[iterm] = populate_map(iterm);
  }

  std::set<std::pair<odb::dbITerm*, odb::dbITerm*>> connections;
  for (auto* iterm0 : inst0->getITerms()) {
    const ShapeMap& shapes0 = iterm_map[iterm0];
    for (auto* iterm1 : inst1->getITerms()) {
      const ShapeMap& shapes1 = iterm_map[iterm1];

      for (const auto& [layer, shapes] : shapes0) {
        auto find_layer = shapes1.find(layer);
        if (find_layer == shapes1.end()) {
          continue;
        }
        const auto& other_shapes = find_layer->second;

        for (const auto& rect0 : shapes) {
          for (const auto& rect1 : other_shapes) {
            if (rect0.intersects(rect1) || rect1.intersects(rect0)) {
              connections.insert({iterm0, iterm1});
            }
          }
        }
      }
    }
  }
  std::vector<std::pair<odb::dbITerm*, odb::dbITerm*>> conns(
      connections.begin(), connections.end());
  return conns;
}

std::vector<odb::dbRow*> ICeWall::getRows() const
{
  auto* block = getBlock();
  if (block == nullptr) {
    return {};
  }

  std::vector<odb::dbRow*> rows;

  for (auto* row : block->getRows()) {
    const std::string name = row->getName();

    if (name.substr(0, 3) == "IO_") {
      rows.push_back(row);
    }
  }

  return rows;
}

std::vector<odb::dbInst*> ICeWall::getPadInstsInRow(odb::dbRow* row) const
{
  auto* block = getBlock();
  if (block == nullptr) {
    return {};
  }

  if (row == nullptr) {
    return {};
  }

  std::vector<odb::dbInst*> insts;
  const odb::Rect row_bbox = row->getBBox();

  for (auto* inst : block->getInsts()) {
    if (!inst->isFixed()) {
      continue;
    }

    if (inst->getMaster()->isCover()) {
      continue;
    }

    const odb::Rect instbbox = inst->getBBox()->getBox();

    if (row_bbox.overlaps(instbbox)) {
      insts.push_back(inst);
    }
  }

  return insts;
}

std::vector<odb::dbInst*> ICeWall::getPadInsts() const
{
  std::vector<odb::dbInst*> insts;

  for (auto* row : getRows()) {
    const auto row_insts = getPadInstsInRow(row);

    debugPrint(logger_,
               utl::PAD,
               "Insts",
               1,
               "Found {} instances in {}",
               row_insts.size(),
               row->getName());

    insts.insert(insts.end(), row_insts.begin(), row_insts.end());
  }

  return insts;
}

void ICeWall::routeRDL(odb::dbTechLayer* layer,
                       odb::dbTechVia* bump_via,
                       odb::dbTechVia* pad_via,
                       const std::vector<odb::dbNet*>& nets,
                       int width,
                       int spacing,
                       bool allow45,
                       float turn_penalty,
                       int max_iterations)
{
  if (layer == nullptr) {
    logger_->error(utl::PAD, 22, "Layer must be specified to perform routing.");
  }

  router_ = std::make_unique<RDLRouter>(logger_,
                                        getBlock(),
                                        layer,
                                        bump_via,
                                        pad_via,
                                        routing_map_,
                                        width,
                                        spacing,
                                        allow45,
                                        turn_penalty,
                                        max_iterations);
  router_->setRDLDebugNet(rdl_net_debug_);
  router_->setRDLDebugPin(rdl_pin_debug_);
  if (router_gui_ != nullptr) {
    router_gui_->setRouter(router_.get());
  }
  router_->route(nets);
  if (router_gui_ != nullptr) {
    router_gui_->redraw();
  }
}

void ICeWall::routeRDLDebugGUI(bool enable)
{
  if (enable) {
    if (router_gui_ == nullptr) {
      router_gui_ = std::make_unique<RDLGui>();
      if (router_ != nullptr) {
        router_gui_->setRouter(router_.get());
      }
    }
    gui::Gui::get()->registerRenderer(router_gui_.get());
  } else {
    if (router_gui_ != nullptr) {
      gui::Gui::get()->unregisterRenderer(router_gui_.get());
      router_gui_ = nullptr;
    }
  }
}

void ICeWall::routeRDLDebugNet(const char* net)
{
  auto* block = getBlock();
  if (block == nullptr) {
    return;
  }

  rdl_net_debug_ = block->findNet(net);

  if (router_ != nullptr) {
    router_->setRDLDebugNet(rdl_net_debug_);
  }
}

void ICeWall::routeRDLDebugPin(const char* pin)
{
  auto* block = getBlock();
  if (block == nullptr) {
    return;
  }

  rdl_pin_debug_ = block->findITerm(pin);

  if (router_ != nullptr) {
    router_->setRDLDebugPin(rdl_pin_debug_);
  }
}

}  // namespace pad
