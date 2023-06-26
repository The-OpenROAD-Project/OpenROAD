/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2023, The Regents of the University of California
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

#include "pad/ICeWall.h"

#include <boost/icl/interval_set.hpp>

#include "RDLRouter.h"
#include "Utilities.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace pad {

ICeWall::ICeWall() = default;

ICeWall::~ICeWall() = default;

void ICeWall::init(odb::dbDatabase* db, utl::Logger* logger)
{
  db_ = db;
  logger_ = logger;
}

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
  Utilities::makeSpecial(net);
}

void ICeWall::assignBump(odb::dbInst* inst, odb::dbNet* net)
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

  odb::dbTransform xform;
  inst->getTransform(xform);

  odb::dbTechLayer* top_layer = nullptr;
  odb::Rect top_shape;
  for (auto* iterm : inst->getITerms()) {
    if (iterm->getNet() != net) {
      iterm->connect(net);
    }

    for (auto* mpin : iterm->getMTerm()->getMPins()) {
      for (auto* geom : mpin->getGeometry()) {
        auto* layer = geom->getTechLayer();
        if (layer == nullptr) {
          continue;
        }

        if (top_layer == nullptr
            || top_layer->getRoutingLevel() < layer->getRoutingLevel()) {
          top_layer = layer;
          top_shape = geom->getBox();
        }
      }
    }
  }

  if (top_layer != nullptr) {
    xform.apply(top_shape);
    makeBTerm(net, top_layer, top_shape);
  }
}

void ICeWall::makeFakeSite(const std::string& name, int width, int height)
{
  auto* lib = db_->findLib(fake_library_name_);
  if (lib == nullptr) {
    lib = odb::dbLib::create(db_, fake_library_name_);
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

  const int cheight = corner_site->getHeight();
  const int cwidth
      = std::max(horizontal_site->getHeight(), corner_site->getWidth());

  const int x_sites = std::floor(static_cast<double>(outer_io.dx() - 2 * cwidth)
                                 / vertical_site->getWidth());
  outer_io.set_xhi(outer_io.xMin() + 2 * cwidth
                   + x_sites * vertical_site->getWidth());
  const int y_sites
      = std::floor(static_cast<double>(outer_io.dy() - 2 * cheight)
                   / horizontal_site->getWidth());
  outer_io.set_yhi(outer_io.yMin() + 2 * cheight
                   + y_sites * horizontal_site->getWidth());

  const odb::Rect corner_origins(outer_io.xMin(),
                                 outer_io.yMin(),
                                 outer_io.xMax() - cwidth,
                                 outer_io.yMax() - cheight);

  // Create corners
  const int corner_sites
      = std::max(horizontal_site->getHeight(), corner_site->getWidth())
        / corner_site->getWidth();
  auto create_corner
      = [this, block, corner_site, corner_sites, ring_index, &rotation_cor](
            const std::string& name,
            const odb::Point& origin,
            const odb::dbOrientType& orient) -> odb::dbRow* {
    const std::string row_name = getRowName(name, ring_index);
    odb::dbTransform rotation(orient);
    rotation.concat(rotation_cor);
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
      = create_corner(corner_nw_, corner_origins.ul(), odb::dbOrientType::MX);
  create_corner(corner_ne_, corner_origins.ur(), odb::dbOrientType::R180);
  auto* se
      = create_corner(corner_se_, corner_origins.lr(), odb::dbOrientType::MY);
  auto* sw
      = create_corner(corner_sw_, corner_origins.ll(), odb::dbOrientType::R0);

  // Create rows
  auto create_row
      = [this, block, ring_index](const std::string& name,
                                  odb::dbSite* site,
                                  int sites,
                                  const odb::Point& origin,
                                  const odb::dbOrientType& orient,
                                  const odb::dbOrientType& row_rotation,
                                  const odb::dbRowDir& direction) {
          const std::string row_name = getRowName(name, ring_index);
          odb::dbTransform rotation(orient);
          rotation.concat(row_rotation);
          odb::dbRow::create(block,
                             row_name.c_str(),
                             site,
                             origin.x(),
                             origin.y(),
                             rotation.getOrient(),
                             direction,
                             sites,
                             site->getWidth());
        };
  create_row(row_north_,
             vertical_site,
             x_sites,
             {nw->getBBox().xMax(),
              outer_io.yMax() - static_cast<int>(vertical_site->getHeight())},
             odb::dbOrientType::MX,
             rotation_ver,
             odb::dbRowDir::HORIZONTAL);
  odb::dbOrientType east_rotation_hor = odb::dbOrientType::R90;
  if (vertical_site != horizontal_site) {
    east_rotation_hor = odb::dbOrientType::R0;
  }
  create_row(row_east_,
             horizontal_site,
             y_sites,
             {outer_io.xMax() - static_cast<int>(horizontal_site->getHeight()),
              se->getBBox().yMax()},
             east_rotation_hor,
             rotation_hor,
             odb::dbRowDir::VERTICAL);
  create_row(row_south_,
             vertical_site,
             x_sites,
             {sw->getBBox().xMax(), outer_io.yMin()},
             odb::dbOrientType::R0,
             rotation_ver,
             odb::dbRowDir::HORIZONTAL);
  const odb::dbOrientType west_rotation_hor = east_rotation_hor.flipY();
  create_row(row_west_,
             horizontal_site,
             y_sites,
             {outer_io.xMin(), sw->getBBox().yMax()},
             west_rotation_hor,
             rotation_hor,
             odb::dbRowDir::VERTICAL);
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

  for (const char* row_name :
       {corner_nw_, corner_ne_, corner_sw_, corner_se_}) {
    odb::dbRow* row = findRow(getRowName(row_name, ring_index));
    if (row == nullptr) {
      logger_->error(utl::PAD,
                     13,
                     "Unable to find {} row to place a corner cell in",
                     row_name);
    }

    const std::string corner_name = fmt::format("{}_INST", row->getName());
    odb::dbInst* inst = block->findInst(corner_name.c_str());
    if (inst == nullptr) {
      inst = odb::dbInst::create(block, master, corner_name.c_str());
    }

    const odb::Rect row_bbox = row->getBBox();

    inst->setOrient(row->getOrient());
    inst->setLocation(row_bbox.xMin(), row_bbox.yMin());
    inst->setPlacementStatus(odb::dbPlacementStatus::FIRM);
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

  if (row == nullptr) {
    logger_->error(utl::PAD, 19, "Row must be specified to place a pad");
  }

  odb::dbTransform orient(odb::dbOrientType::R0);
  if (mirror) {
    const odb::dbTransform mirror_transform(odb::dbOrientType::MY);
    orient.concat(mirror_transform);
  }

  placeInstance(row, snapToRowSite(row, location), inst, orient.getOrient());
}

int ICeWall::snapToRowSite(odb::dbRow* row, int location) const
{
  int x, y;
  row->getOrigin(x, y);
  const odb::Point origin(x, y);

  const double spacing = row->getSpacing();
  int relative_location;
  if (row->getDirection() == odb::dbRowDir::HORIZONTAL) {
    relative_location = location - origin.x();
  } else {
    relative_location = location - origin.y();
  }

  int site_count = std::round(relative_location / spacing);
  site_count = std::max(0, site_count);
  site_count = std::min(site_count, row->getSiteCount());

  return site_count;
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
  if (check_name(row_north_)) {
    return odb::Direction2D::North;
  }
  if (check_name(row_south_)) {
    return odb::Direction2D::South;
  }
  if (check_name(row_east_)) {
    return odb::Direction2D::East;
  }
  if (check_name(row_west_)) {
    return odb::Direction2D::West;
  }
  logger_->error(utl::PAD, 29, "{} is not a recognized IO row.", row_name);
}

void ICeWall::placeInstance(odb::dbRow* row,
                            int index,
                            odb::dbInst* inst,
                            const odb::dbOrientType& base_orient,
                            bool allow_overlap) const
{
  const int origin_offset = index * row->getSpacing();

  const odb::Rect row_bbox = row->getBBox();

  odb::dbTransform xform(base_orient);
  xform.concat(row->getOrient());
  inst->setOrient(xform.getOrient());
  const odb::Rect inst_bbox = inst->getBBox()->getBox();

  odb::Point index_pt;
  switch (getRowEdge(row)) {
    case odb::Direction2D::North:
      index_pt = odb::Point(row_bbox.xMin() + origin_offset,
                            row_bbox.yMax() - inst_bbox.dy());
      break;
    case odb::Direction2D::South:
      index_pt = odb::Point(row_bbox.xMin() + origin_offset, row_bbox.yMin());
      break;
    case odb::Direction2D::West:
      index_pt = odb::Point(row_bbox.xMin(), row_bbox.yMin() + origin_offset);
      break;
    case odb::Direction2D::East:
      index_pt = odb::Point(row_bbox.xMax() - inst_bbox.dx(),
                            row_bbox.yMin() + origin_offset);
      break;
  }

  inst->setLocation(index_pt.x(), index_pt.y());

  // check for overlaps
  const odb::Rect inst_rect = inst->getBBox()->getBox();
  auto* block = getBlock();
  for (auto* check_inst : block->getInsts()) {
    if (check_inst == inst) {
      continue;
    }
    if (!check_inst->isFixed()) {
      continue;
    }
    const odb::Rect check_rect = check_inst->getBBox()->getBox();
    if (!allow_overlap && inst_rect.overlaps(check_rect)) {
      const double dbus = block->getDbUnitsPerMicron();
      logger_->error(utl::PAD,
                     1,
                     "Unable to place {} ({}) at ({:.3f}um, {:.3f}um) - "
                     "({:.3f}um, {:.3f}um) as it "
                     "overlaps with {} ({})",
                     inst->getName(),
                     inst->getMaster()->getName(),
                     inst_rect.xMin() / dbus,
                     inst_rect.yMin() / dbus,
                     inst_rect.xMax() / dbus,
                     inst_rect.yMax() / dbus,
                     check_inst->getName(),
                     check_inst->getMaster()->getName());
    }
  }
  inst->setPlacementStatus(odb::dbPlacementStatus::FIRM);
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

  const odb::dbTransform row_xform(row->getOrient());

  const double dbus = block->getDbUnitsPerMicron();

  std::vector<odb::dbMaster*> fillers = masters;
  // remove nullptrs
  fillers.erase(std::remove(fillers.begin(), fillers.end(), nullptr),
                fillers.end());
  // sort by width
  std::stable_sort(
      fillers.begin(),
      fillers.end(),
      [use_height, row_xform](odb::dbMaster* r, odb::dbMaster* l) -> bool {
        odb::Rect r_bbox;
        r->getPlacementBoundary(r_bbox);
        odb::Rect l_bbox;
        r->getPlacementBoundary(l_bbox);
        row_xform.apply(r_bbox);
        row_xform.apply(l_bbox);
        // sort biggest to smallest
        if (use_height) {
          return r_bbox.dy() > l_bbox.dy();
        } else {
          return r_bbox.dx() > l_bbox.dx();
        }
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

  const int site_width = row->getSite()->getWidth();
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
    const int start_site_index = snapToRowSite(row, start);

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
      const bool allow_overlap
          = std::find(
                overlapping_masters.begin(), overlapping_masters.end(), filler)
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

        const std::string name = fmt::format("{}{}_{}_{}",
                                             fill_prefix_,
                                             row->getName(),
                                             fill_group,
                                             site_offset);
        auto* fill_inst = odb::dbInst::create(block, filler, name.c_str());

        placeInstance(row,
                      start_site_index + site_offset,
                      fill_inst,
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

  const std::string prefix = fmt::format("{}{}_", fill_prefix_, row->getName());

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
    int x, y;
    inst->getOrigin(x, y);
    const odb::Point pad_loc(x + pad_offset.x(), y + pad_offset.y());

    pad_transform.concat(pad_xform);
    const odb::dbOrientType pad_orient = pad_transform.getOrient();

    const std::string name = fmt::format("{}{}", prefix, inst->getName());

    odb::dbInst* bond_inst = odb::dbInst::create(block, bond, name.c_str());
    bond_inst->setOrient(pad_orient);
    bond_inst->setOrigin(pad_loc.x(), pad_loc.y());
    bond_inst->setPlacementStatus(odb::dbPlacementStatus::FIRM);

    odb::dbTransform xform;
    bond_inst->getTransform(xform);
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
      const std::string netname = fmt::format("{}.{}_RING",
                                              iterm0->getInst()->getName(),
                                              iterm0->getMTerm()->getName());
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
                       "{}/{} ({}) and {}/{} ({}) are touching, but are "
                       "connected to different nets",
                       iterm0->getInst()->getName(),
                       iterm0->getMTerm()->getName(),
                       net0->getName(),
                       iterm1->getInst()->getName(),
                       iterm1->getMTerm()->getName(),
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
                 "Connecting net {} to {}/{} ({}) and {}/{} ({})",
                 connect_net->getName(),
                 iterm0->getInst()->getName(),
                 iterm0->getMTerm()->getName(),
                 net0 != nullptr ? net0->getName() : "NULL",
                 iterm1->getInst()->getName(),
                 iterm1->getMTerm()->getName(),
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
    odb::dbInst* inst1) const
{
  if (!inst0->getBBox()->getBox().intersects(inst1->getBBox()->getBox())) {
    return {};
  }

  using ShapeMap = std::map<odb::dbTechLayer*, std::set<odb::Rect>>;
  auto populate_map = [](odb::dbITerm* iterm) -> ShapeMap {
    ShapeMap map;
    odb::dbTransform xform;
    iterm->getInst()->getTransform(xform);

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
    if (!inst->isPlaced()) {
      continue;
    }

    if (inst->getMaster()->isCover()) {
      continue;
    }

    const odb::Rect instbbox = inst->getBBox()->getBox();

    if (row_bbox.intersects(instbbox)) {
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
                       bool allow45)
{
  if (layer == nullptr) {
    logger_->error(utl::PAD, 22, "Layer must be specified to perform routing.");
  }

  router_ = std::make_unique<RDLRouter>(
      logger_, getBlock(), layer, bump_via, pad_via, width, spacing, allow45);
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
  if (router_ == nullptr) {
    return;
  }

  if (enable) {
    if (router_gui_ == nullptr) {
      router_gui_ = std::make_unique<RDLGui>();
      if (router_ != nullptr) {
        router_gui_->setRouter(router_.get());
      }
    }
    gui::Gui::get()->registerRenderer(router_gui_.get());
  } else {
    gui::Gui::get()->unregisterRenderer(router_gui_.get());
  }
}

}  // namespace pad
