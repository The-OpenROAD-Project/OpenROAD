/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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

#include "tap/tapcell.h"

#include <map>
#include <string>
#include <utility>

#include "odb/db.h"
#include "odb/util.h"
#include "ord/OpenRoad.hh"
#include "sta/StaMain.hh"
#include "utl/Logger.h"
#include "utl/algorithms.h"

namespace tap {

using std::max;
using std::min;
using std::string;
using std::vector;

Tapcell::Tapcell()
{
  reset();
}

void Tapcell::init(odb::dbDatabase* db, utl::Logger* logger)
{
  db_ = db;
  logger_ = logger;
}

void Tapcell::reset()
{
  phy_idx_ = 0;
  tap_prefix_ = "TAP_";
  endcap_prefix_ = "PHY_";
}

//---------------------------------------------------------------

void Tapcell::setTapPrefix(const string& tap_prefix)
{
  tap_prefix_ = tap_prefix;
}

void Tapcell::setEndcapPrefix(const string& endcap_prefix)
{
  endcap_prefix_ = endcap_prefix;
}

void Tapcell::clear()
{
  removeCells(tap_prefix_);
  removeCells(endcap_prefix_);

  // Reset global parameters
  reset();
}

int Tapcell::defaultDistance() const
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  return 2 * block->getDbUnitsPerMicron();
}

void Tapcell::cutRows(const Options& options)
{
  vector<odb::dbBox*> blockages = findBlockages();
  odb::dbBlock* block = db_->getChip()->getBlock();
  const int halo_x = options.halo_x >= 0 ? options.halo_x : defaultDistance();
  const int halo_y = options.halo_y >= 0 ? options.halo_y : defaultDistance();
  const int min_row_width = (options.endcap_master != nullptr)
                                ? 2 * options.endcap_master->getWidth()
                                : 0;
  odb::cutRows(block, min_row_width, blockages, halo_x, halo_y, logger_);
}

void Tapcell::run(const Options& options)
{
  cutRows(options);

  insertBoundaryCells(correctBoundaryOptions(options));

  insertTapcells(options);
}

int Tapcell::insertTapcells(odb::dbMaster* tapcell_master, const int dist)
{
  int inst = 0;
  for (auto* row : db_->getChip()->getBlock()->getRows()) {
    inst += insertTapcells(tapcell_master, dist, row);
  }
  logger_->info(utl::TAP, 5, "Inserted {} tapcells.", inst);
  return inst;
}

int Tapcell::insertTapcells(odb::dbMaster* tapcell_master,
                            int dist,
                            odb::dbRow* row)
{
  if (row->getSite()->getName() != tapcell_master->getSite()->getName()) {
    return 0;
  }
  if (!checkSymmetry(tapcell_master, row->getOrient())) {
    return 0;
  }

  const int tap_width = tapcell_master->getWidth();
  int insts = 0;

  int offset = 0;
  const int pitch
      = tap_width * std::floor(2 * dist / static_cast<double>(tap_width));

  if (row->getOrient() == odb::dbOrientType::R0) {
    offset = 0;
  } else {
    offset = pitch / 2;
  }

  const odb::Rect row_bb = row->getBBox();

  std::set<odb::dbInst*> row_insts;
  for (auto* inst : db_->getChip()->getBlock()->getInsts()) {
    if (!inst->isFixed()) {
      continue;
    }

    if (row_bb.contains(inst->getBBox()->getBox())) {
      row_insts.insert(inst);
    }
  }

  const int llx = row_bb.xMin();
  const int urx = row_bb.xMax();

  const int site_width = row->getSite()->getWidth();
  for (int x = llx + offset; x < urx; x += pitch) {
    x = odb::makeSiteLoc(x, site_width, true, llx);
    // Check if site is filled
    const odb::dbOrientType ori = row->getOrient();
    bool overlap = checkIfFilled(x, tap_width, ori, row_insts);
    if (!overlap) {
      const int lly = row_bb.yMin();
      auto* inst = makeInstance(
          db_->getChip()->getBlock(),
          tapcell_master,
          ori,
          x,
          lly,
          fmt::format("{}TAPCELL_{}_", tap_prefix_, row->getName()));
      row_insts.insert(inst);
      insts++;
    }
  }

  return insts;
}

bool Tapcell::checkIfFilled(const int x,
                            const int width,
                            const odb::dbOrientType& orient,
                            const std::set<odb::dbInst*>& row_insts)
{
  int x_start;
  int x_end;
  if (orient == odb::dbOrientType::MY || orient == odb::dbOrientType::R180) {
    x_start = x - width;
    x_end = x;
  } else {
    x_start = x;
    x_end = x + width;
  }

  for (const auto& inst : row_insts) {
    const odb::Rect inst_bb = inst->getBBox()->getBox();
    if (x_end > inst_bb.xMin() && x_start < inst_bb.xMax()) {
      return true;
    }
  }
  return false;
}

vector<odb::dbBox*> Tapcell::findBlockages()
{
  vector<odb::dbBox*> blockages;
  for (auto&& inst : db_->getChip()->getBlock()->getInsts()) {
    if (inst->isBlock()) {
      if (!inst->isPlaced()) {
        logger_->warn(utl::TAP, 32, "Macro {} is not placed.", inst->getName());
        continue;
      }
      blockages.push_back(inst->getBBox());
    }
  }

  return blockages;
}

odb::dbInst* Tapcell::makeInstance(odb::dbBlock* block,
                                   odb::dbMaster* master,
                                   const odb::dbOrientType& orientation,
                                   int x,
                                   int y,
                                   const string& prefix)
{
  if (x < 0 || y < 0) {
    return nullptr;
  }
  const string name = prefix + std::to_string(phy_idx_);
  odb::dbInst* inst = odb::dbInst::create(block,
                                          master,
                                          name.c_str(),
                                          /* physical_only */ true);
  debugPrint(logger_,
             utl::TAP,
             "Instance",
             1,
             "Creating instance {} at ({}, {} / {})",
             name,
             x,
             y,
             orientation.getString());
  if (inst == nullptr) {
    logger_->error(utl::TAP,
                   33,
                   "Not able to build instance {} with master {}.",
                   name,
                   master->getName());
  }
  inst->setOrient(orientation);
  inst->setLocation(x, y);
  inst->setPlacementStatus(odb::dbPlacementStatus::LOCKED);
  inst->setSourceType(odb::dbSourceType::DIST);

  phy_idx_++;

  return inst;
}

int Tapcell::removeCells(const std::string& prefix)
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  int removed = 0;

  if (prefix.length() == 0) {
    // If no prefix is given, return 0 instead of having all cells removed
    return 0;
  }

  for (odb::dbInst* inst : block->getInsts()) {
    if (inst->getName().find(prefix) == 0) {
      odb::dbInst::destroy(inst);
      removed++;
    }
  }

  return removed;
}

bool Tapcell::checkSymmetry(odb::dbMaster* master, const odb::dbOrientType& ori)
{
  const bool symmetry_x = master->getSymmetryX();
  const bool symmetry_y = master->getSymmetryY();

  switch (ori) {
    case odb::dbOrientType::R0:
      return true;
    case odb::dbOrientType::MX:
      return symmetry_x;
    case odb::dbOrientType::MY:
      return symmetry_y;
    case odb::dbOrientType::R180:
      return (symmetry_x && symmetry_y);
    default:
      return false;
  }
}

std::vector<Tapcell::Polygon90> Tapcell::getBoundaryAreas() const
{
  using namespace boost::polygon::operators;
  using Polygon90Set = boost::polygon::polygon_90_set_data<int>;

  auto rect_to_poly = [](const odb::Rect& rect) -> Polygon90 {
    using Pt = Polygon90::point_type;
    std::array<Pt, 4> pts = {Pt(rect.xMin(), rect.yMin()),
                             Pt(rect.xMax(), rect.yMin()),
                             Pt(rect.xMax(), rect.yMax()),
                             Pt(rect.xMin(), rect.yMax())};

    Polygon90 poly;
    poly.set(pts.begin(), pts.end());
    return poly;
  };

  const Polygon90 core_area
      = rect_to_poly(db_->getChip()->getBlock()->getCoreArea());

  // Generate mask of areas without rows
  Polygon90Set core_mask;
  core_mask += core_area;

  for (odb::dbRow* row : db_->getChip()->getBlock()->getRows()) {
    if (row->getSite()->getClass() == odb::dbSiteClass::PAD) {
      continue;
    }
    core_mask -= rect_to_poly(row->getBBox());
  }

  // Generate set of core areas as polygons
  Polygon90Set core_area_set;
  core_area_set += core_area;
  core_area_set -= core_mask;

  std::vector<Polygon90> core_areas;
  core_area_set.get_polygons(core_areas);

  // build list of core outlines
  std::vector<Polygon90> core_outlines;
  for (const auto& core : core_areas) {
    std::vector<Polygon90::point_type> outline;

    for (const auto& pt : core) {
      auto check = std::find(outline.begin(), outline.end(), pt);
      if (check == outline.end()) {
        outline.push_back(pt);
      } else {
        // delete everything from the found point on
        outline.erase(check, outline.end());
      }
    }

    Polygon90 core_p;
    core_p.set(outline.begin(), outline.end());
    core_outlines.push_back(core_p);
  }

  for (auto& core : core_outlines) {
    const Polygon90Set core_holes = core_mask & core;
    std::vector<Polygon90> holes;
    core_holes.get_polygons(holes);

    core.set_holes(holes.begin(), holes.end());

    if (logger_->debugCheck(utl::TAP, "Boundary", 1)) {
      logger_->report("Core");
      logger_->report(" All pts");
      for (const auto& pt : core) {
        logger_->report("  Pt {} {}", pt.x(), pt.y());
      }
      logger_->report(" Holes");
      for (auto itr = core.begin_holes(); itr != core.end_holes(); itr++) {
        logger_->report("  Hole");
        for (const auto& pt : *itr) {
          logger_->report("   Pt {} {}", pt.x(), pt.y());
        }
      }
    }
  }

  return core_outlines;
}

void Tapcell::insertBoundaryCells(const BoundaryCellOptions& options)
{
  const auto filled_options = correctBoundaryOptions(options);

  const auto areas = getBoundaryAreas();

  int corners = 0;
  int boundary = 0;
  for (const auto& area : areas) {
    const auto& [added_corners, added_boundaries]
        = insertBoundaryCells(area, true, options);
    corners += added_corners;
    boundary += added_boundaries;

    for (auto itr = area.begin_holes(); itr != area.end_holes(); itr++) {
      const auto& [added_corners, added_boundaries]
          = insertBoundaryCells(*itr, false, filled_options);
      corners += added_corners;
      boundary += added_boundaries;
    }
  }

  logger_->info(utl::TAP, 2, "Added {} corner cells", corners);
  logger_->info(utl::TAP, 3, "Added {} boundary cells", boundary);
}

std::vector<Tapcell::Edge> Tapcell::getBoundaryEdges(const Polygon90& area,
                                                     bool outer) const
{
  std::vector<Edge> edges;

  Polygon90::point_type prev_pt = *area.begin();
  auto itr = area.begin();
  itr++;
  for (; itr != area.end(); itr++) {
    const Polygon90::point_type pt = *itr;

    const odb::Point pt0(prev_pt.x(), prev_pt.y());
    const odb::Point pt1(pt.x(), pt.y());

    prev_pt = pt;

    edges.push_back(Edge{EdgeType::Unknown, pt0, pt1});
  }

  // complete the polygon
  const odb::Point pt0(prev_pt.x(), prev_pt.y());
  const odb::Point pt1((*area.begin()).x(), (*area.begin()).y());
  edges.push_back(Edge{EdgeType::Unknown, pt0, pt1});

  // Assign edge types
  for (size_t i = 0; i < edges.size(); i++) {
    Edge* prev;
    Edge* curr = &edges[i];
    if (i == 0) {
      prev = &edges[edges.size() - 1];
    } else {
      prev = &edges[i - 1];
    }

    const int in_delta_x = prev->pt1.getX() - prev->pt0.getX();
    const int in_delta_y = prev->pt1.getY() - prev->pt0.getY();
    const int out_delta_x = curr->pt1.getX() - curr->pt0.getX();
    const int out_delta_y = curr->pt1.getY() - curr->pt0.getY();

    if (in_delta_y < 0 && out_delta_x > 0) {
      curr->type = EdgeType::Bottom;
    } else if (in_delta_x > 0 && out_delta_y > 0) {
      curr->type = EdgeType::Right;
    } else if (in_delta_y > 0 && out_delta_x < 0) {
      curr->type = EdgeType::Top;
    } else if (in_delta_x < 0 && out_delta_y < 0) {
      curr->type = EdgeType::Left;
    } else if (in_delta_x < 0 && out_delta_y > 0) {
      curr->type = EdgeType::Right;
    } else if (in_delta_y < 0 && out_delta_x < 0) {
      curr->type = EdgeType::Top;
    } else if (in_delta_x > 0 && out_delta_y < 0) {
      curr->type = EdgeType::Left;
    } else {
      curr->type = EdgeType::Bottom;
    }

    if (!outer) {
      switch (curr->type) {
        case EdgeType::Bottom:
          curr->type = EdgeType::Top;
          break;
        case EdgeType::Right:
          curr->type = EdgeType::Left;
          break;
        case EdgeType::Top:
          curr->type = EdgeType::Bottom;
          break;
        case EdgeType::Left:
          curr->type = EdgeType::Right;
          break;
        case EdgeType::Unknown:
          break;
      }
    }
    debugPrint(logger_,
               utl::TAP,
               "Boundary",
               2,
               "Edge ({}, {}) - ({}, {}) : {}",
               curr->pt0.getX(),
               curr->pt0.getY(),
               curr->pt1.getX(),
               curr->pt1.getY(),
               toString(curr->type));
  }

  return edges;
}

std::vector<Tapcell::Corner> Tapcell::getBoundaryCorners(const Polygon90& area,
                                                         bool outer) const
{
  std::vector<Corner> corners;

  for (auto itr = area.begin(); itr != area.end(); itr++) {
    const Polygon90::point_type pt = *itr;
    const odb::Point corner(pt.x(), pt.y());
    corners.push_back(Corner{CornerType::Unknown, corner});
  }

  // Assign corner types
  for (size_t i = 0; i < corners.size(); i++) {
    Corner* prev;
    Corner* curr = &corners[i];
    Corner* next;
    if (i == 0) {
      prev = &corners[corners.size() - 1];
    } else {
      prev = &corners[i - 1];
    }
    if (i == corners.size() - 1) {
      next = &corners[0];
    } else {
      next = &corners[i + 1];
    }

    const int in_delta_x = curr->pt.getX() - prev->pt.getX();
    const int in_delta_y = curr->pt.getY() - prev->pt.getY();
    const int out_delta_x = next->pt.getX() - curr->pt.getX();
    const int out_delta_y = next->pt.getY() - curr->pt.getY();

    if (in_delta_y < 0 && out_delta_x > 0) {
      curr->type = CornerType::OuterBottomLeft;
    } else if (in_delta_x > 0 && out_delta_y > 0) {
      curr->type = CornerType::OuterBottomRight;
    } else if (in_delta_y > 0 && out_delta_x < 0) {
      curr->type = CornerType::OuterTopRight;
    } else if (in_delta_x < 0 && out_delta_y < 0) {
      curr->type = CornerType::OuterTopLeft;
    } else if (in_delta_x < 0 && out_delta_y > 0) {
      curr->type = CornerType::InnerBottomLeft;
    } else if (in_delta_y < 0 && out_delta_x < 0) {
      curr->type = CornerType::InnerBottomRight;
    } else if (in_delta_x > 0 && out_delta_y < 0) {
      curr->type = CornerType::InnerTopRight;
    } else {
      curr->type = CornerType::InnerTopLeft;
    }

    if (!outer) {
      switch (curr->type) {
        case CornerType::InnerBottomLeft:
          curr->type = CornerType::OuterBottomLeft;
          break;
        case CornerType::InnerBottomRight:
          curr->type = CornerType::OuterBottomRight;
          break;
        case CornerType::InnerTopLeft:
          curr->type = CornerType::OuterTopLeft;
          break;
        case CornerType::InnerTopRight:
          curr->type = CornerType::OuterTopRight;
          break;
        case CornerType::OuterBottomLeft:
          curr->type = CornerType::InnerBottomLeft;
          break;
        case CornerType::OuterBottomRight:
          curr->type = CornerType::InnerBottomRight;
          break;
        case CornerType::OuterTopLeft:
          curr->type = CornerType::InnerTopLeft;
          break;
        case CornerType::OuterTopRight:
          curr->type = CornerType::InnerTopRight;
          break;
        case CornerType::Unknown:
          break;
      }
    }

    debugPrint(logger_,
               utl::TAP,
               "Boundary",
               2,
               "Corner ({}, {}) : {}",
               curr->pt.getX(),
               curr->pt.getY(),
               toString(curr->type));
  }

  return corners;
}

std::string Tapcell::toString(Tapcell::EdgeType type) const
{
  switch (type) {
    case EdgeType::Bottom:
      return "Bottom";
    case EdgeType::Right:
      return "Right";
    case EdgeType::Top:
      return "Top";
    case EdgeType::Left:
      return "Left";
    case EdgeType::Unknown:
      return "Unknown";
  }

  return "";
}

std::string Tapcell::toString(Tapcell::CornerType type) const
{
  switch (type) {
    case CornerType::InnerBottomLeft:
      return "InnerBottomLeft";
    case CornerType::InnerBottomRight:
      return "InnerBottomRight";
    case CornerType::InnerTopLeft:
      return "InnerTopLeft";
    case CornerType::InnerTopRight:
      return "InnerTopRight";
    case CornerType::OuterBottomLeft:
      return "OuterBottomLeft";
    case CornerType::OuterBottomRight:
      return "OuterBottomRight";
    case CornerType::OuterTopLeft:
      return "OuterTopLeft";
    case CornerType::OuterTopRight:
      return "OuterTopRight";
    case CornerType::Unknown:
      return "Unknown";
  }

  return "";
}

std::vector<odb::dbRow*> Tapcell::getRows(const Tapcell::Edge& edge,
                                          odb::dbSite* site) const
{
  if (site == nullptr) {
    return {};
  }

  std::vector<odb::dbRow*> rows;

  const odb::Rect search(edge.pt0, edge.pt1);

  for (odb::dbRow* row : db_->getChip()->getBlock()->getRows()) {
    if (row->getSite()->getName() != site->getName()) {
      continue;
    }
    const auto row_bbox = row->getBBox();
    if (row_bbox.intersects(search)) {
      const odb::Rect intersect = row_bbox.intersect(search);
      if (intersect.maxDXDY() == 0) {
        // no on the edge
        continue;
      }
      rows.push_back(row);

      debugPrint(logger_,
                 utl::TAP,
                 "Boundary",
                 3,
                 "Edge row: {} {} -> {} {}: {}",
                 edge.pt0.x(),
                 edge.pt0.y(),
                 edge.pt1.x(),
                 edge.pt1.y(),
                 row->getName());
    }
  }

  return rows;
}

odb::dbRow* Tapcell::getRow(const Tapcell::Corner& corner,
                            odb::dbSite* site) const
{
  if (site == nullptr) {
    return nullptr;
  }

  const odb::Rect search(corner.pt, corner.pt);

  for (odb::dbRow* row : db_->getChip()->getBlock()->getRows()) {
    if (row->getSite()->getName() != site->getName()) {
      continue;
    }
    const auto row_bbox = row->getBBox();
    bool accept = false;
    if (row_bbox.intersects(search)) {
      switch (corner.type) {
        case CornerType::OuterBottomLeft:
        case CornerType::OuterBottomRight:
        case CornerType::OuterTopLeft:
        case CornerType::OuterTopRight:
          accept = true;
          break;
        case CornerType::InnerBottomLeft:
        case CornerType::InnerBottomRight:
          if (row_bbox.yMin() < corner.pt.y()) {
            accept = true;
          }
          break;
        case CornerType::InnerTopLeft:
        case CornerType::InnerTopRight:
          if (row_bbox.yMax() > corner.pt.y()) {
            accept = true;
          }
          break;
        case CornerType::Unknown:
          break;
      }

      if (accept) {
        debugPrint(logger_,
                   utl::TAP,
                   "Boundary",
                   3,
                   "Corner row: {} {}: {}",
                   corner.pt.x(),
                   corner.pt.y(),
                   row->getName());
        return row;
      }
    }
  }

  return nullptr;
}

std::pair<int, int> Tapcell::insertBoundaryCells(
    const Tapcell::Polygon& area,
    bool outer,
    const BoundaryCellOptions& options)
{
  Polygon90 area90;
  area90.set(area.begin(), area.end());
  return insertBoundaryCells(area90, outer, options);
}

std::pair<int, int> Tapcell::insertBoundaryCells(
    const Tapcell::Polygon90& area,
    bool outer,
    const BoundaryCellOptions& options)
{
  int corner_count = 0;
  int boundary = 0;

  CornerMap corners;
  // insert corners first
  for (const auto& corner : getBoundaryCorners(area, outer)) {
    for (const auto& [row, insts] : insertBoundaryCorner(corner, options)) {
      corners[row].insert(insts.begin(), insts.end());
      corner_count += insts.size();
    }
  }

  for (const auto& edge : getBoundaryEdges(area, outer)) {
    boundary += insertBoundaryEdge(edge, corners, options);
  }

  return {corner_count, boundary};
}

Tapcell::CornerMap Tapcell::insertBoundaryCorner(
    const Tapcell::Corner& corner,
    const BoundaryCellOptions& options)
{
  odb::dbSite* site = nullptr;
  if (options.outer_corner_bottom_left_r0 != nullptr) {
    site = options.outer_corner_bottom_left_r0->getSite();
  } else if (options.outer_corner_bottom_left_mx != nullptr) {
    site = options.outer_corner_bottom_left_mx->getSite();
  } else if (options.outer_corner_bottom_right_mx != nullptr) {
    site = options.outer_corner_bottom_right_mx->getSite();
  } else if (options.outer_corner_bottom_right_r0 != nullptr) {
    site = options.outer_corner_bottom_right_r0->getSite();
  } else if (options.outer_corner_top_left_mx != nullptr) {
    site = options.outer_corner_top_left_mx->getSite();
  } else if (options.outer_corner_top_left_r0 != nullptr) {
    site = options.outer_corner_top_left_r0->getSite();
  } else if (options.outer_corner_top_right_mx != nullptr) {
    site = options.outer_corner_top_right_mx->getSite();
  } else if (options.outer_corner_top_right_r0 != nullptr) {
    site = options.outer_corner_top_right_r0->getSite();
  } else if (options.inner_corner_bottom_left_r0 != nullptr) {
    site = options.inner_corner_bottom_left_r0->getSite();
  } else if (options.inner_corner_bottom_left_mx != nullptr) {
    site = options.inner_corner_bottom_left_mx->getSite();
  } else if (options.inner_corner_bottom_right_mx != nullptr) {
    site = options.inner_corner_bottom_right_mx->getSite();
  } else if (options.inner_corner_bottom_right_r0 != nullptr) {
    site = options.inner_corner_bottom_right_r0->getSite();
  } else if (options.inner_corner_top_left_mx != nullptr) {
    site = options.inner_corner_top_left_mx->getSite();
  } else if (options.inner_corner_top_left_r0 != nullptr) {
    site = options.inner_corner_top_left_r0->getSite();
  } else if (options.inner_corner_top_right_mx != nullptr) {
    site = options.inner_corner_top_right_mx->getSite();
  } else if (options.inner_corner_top_right_r0 != nullptr) {
    site = options.inner_corner_top_right_r0->getSite();
  }
  odb::dbRow* row = getRow(corner, site);
  if (row == nullptr) {
    return {};
  }

  odb::dbMaster* master = nullptr;
  const auto row_orient = row->getOrient();
  switch (corner.type) {
    case CornerType::OuterBottomLeft:
      if (row_orient == odb::dbOrientType::R0) {
        master = options.outer_corner_bottom_left_r0;
      } else {
        master = options.outer_corner_bottom_left_mx;
      }
      break;
    case CornerType::OuterBottomRight:
      if (row_orient == odb::dbOrientType::R0) {
        master = options.outer_corner_bottom_right_r0;
      } else {
        master = options.outer_corner_bottom_right_mx;
      }
      break;
    case CornerType::OuterTopLeft:
      if (row_orient == odb::dbOrientType::R0) {
        master = options.outer_corner_top_left_r0;
      } else {
        master = options.outer_corner_top_left_mx;
      }
      break;
    case CornerType::OuterTopRight:
      if (row_orient == odb::dbOrientType::R0) {
        master = options.outer_corner_top_right_r0;
      } else {
        master = options.outer_corner_top_right_mx;
      }
      break;
    case CornerType::InnerBottomLeft:
      if (row_orient == odb::dbOrientType::R0) {
        master = options.inner_corner_bottom_left_r0;
      } else {
        master = options.inner_corner_bottom_left_mx;
      }
      break;
    case CornerType::InnerBottomRight:
      if (row_orient == odb::dbOrientType::R0) {
        master = options.inner_corner_bottom_right_r0;
      } else {
        master = options.inner_corner_bottom_right_mx;
      }
      break;
    case CornerType::InnerTopLeft:
      if (row_orient == odb::dbOrientType::R0) {
        master = options.inner_corner_top_left_r0;
      } else {
        master = options.inner_corner_top_left_mx;
      }
      break;
    case CornerType::InnerTopRight:
      if (row_orient == odb::dbOrientType::R0) {
        master = options.inner_corner_top_right_r0;
      } else {
        master = options.inner_corner_top_right_mx;
      }
      break;
    case CornerType::Unknown:
      break;
  }

  debugPrint(
      logger_,
      utl::TAP,
      "Boundary",
      2,
      "Generating corner cell {} at ({}, {}) with master {} in row {} with {}",
      toString(corner.type),
      corner.pt.getX(),
      corner.pt.getY(),
      master != nullptr ? master->getName() : "none",
      row->getName(),
      row_orient.getString());

  if (master == nullptr) {
    return {};
  }

  const int width = master->getWidth();
  const int height = master->getHeight();

  odb::dbOrientType orient = row_orient;
  odb::Point ll = corner.pt;
  switch (corner.type) {
    case CornerType::OuterBottomLeft:
      break;
    case CornerType::OuterBottomRight:
      ll.addX(-width);
      if (master->getSymmetryY()) {
        orient = orient.flipY();
      }
      break;
    case CornerType::OuterTopRight:
      ll.addX(-width);
      ll.addY(-height);
      if (master->getSymmetryY()) {
        orient = orient.flipY();
      }
      break;
    case CornerType::OuterTopLeft:
      ll.addY(-height);
      break;
    case CornerType::InnerBottomLeft:
      ll.addX(-width);
      ll.addY(-height);
      break;
    case CornerType::InnerBottomRight:
      ll.addY(-height);
      if (master->getSymmetryY()) {
        orient = orient.flipY();
      }
      break;
    case CornerType::InnerTopLeft:
      ll.addX(-width);
      break;
    case CornerType::InnerTopRight:
      if (master->getSymmetryY()) {
        orient = orient.flipY();
      }
      break;
    case CornerType::Unknown:
      break;
  }

  if (!checkSymmetry(master, orient)) {
    return {};
  }

  auto inst = makeInstance(db_->getChip()->getBlock(),
                           master,
                           orient,
                           ll.getX(),
                           ll.getY(),
                           fmt::format("{}CORNER_{}_{}_",
                                       options.prefix,
                                       row->getName(),
                                       toString(corner.type)));

  CornerMap map;
  map[row].insert(inst);

  return map;
}

int Tapcell::insertBoundaryEdge(const Tapcell::Edge& edge,
                                const Tapcell::CornerMap& corners,
                                const BoundaryCellOptions& options)
{
  switch (edge.type) {
    case EdgeType::Bottom:
    case EdgeType::Top:
      return insertBoundaryEdgeHorizontal(edge, corners, options);
      break;
    case EdgeType::Left:
    case EdgeType::Right:
      return insertBoundaryEdgeVertical(edge, corners, options);
      break;
    case EdgeType::Unknown:
      break;
  }

  return 0;
}

int Tapcell::insertBoundaryEdgeHorizontal(const Tapcell::Edge& edge,
                                          const Tapcell::CornerMap& corners,
                                          const BoundaryCellOptions& options)
{
  int insts = 0;

  odb::dbSite* site = nullptr;
  if (site == nullptr) {
    for (auto* master : options.top_r0) {
      site = master->getSite();
    }
  }
  if (site == nullptr) {
    for (auto* master : options.bottom_r0) {
      site = master->getSite();
    }
  }
  if (site == nullptr) {
    for (auto* master : options.top_mx) {
      site = master->getSite();
    }
  }
  if (site == nullptr) {
    for (auto* master : options.bottom_mx) {
      site = master->getSite();
    }
  }
  auto rows = getRows(edge, site);
  if (rows.empty()) {
    return 0;
  }

  // there should only be one row
  odb::dbRow* row = rows[0];

  std::vector<odb::dbMaster*> masters;
  switch (edge.type) {
    case EdgeType::Top:
      if (row->getOrient() == odb::dbOrientType::R0) {
        masters.insert(
            masters.end(), options.top_r0.begin(), options.top_r0.end());
      } else {
        masters.insert(
            masters.end(), options.top_mx.begin(), options.top_mx.end());
      }
      break;
    case EdgeType::Bottom:
      if (row->getOrient() == odb::dbOrientType::R0) {
        masters.insert(
            masters.end(), options.bottom_r0.begin(), options.bottom_r0.end());
      } else {
        masters.insert(
            masters.end(), options.bottom_mx.begin(), options.bottom_mx.end());
      }
      break;
    case EdgeType::Left:
    case EdgeType::Right:
    case EdgeType::Unknown:
      break;
  }

  if (masters.empty()) {
    return 0;
  }

  std::sort(masters.begin(), masters.end(), [](auto* rhs, auto* lhs) {
    return rhs->getWidth() > lhs->getWidth();
  });

  odb::Point e0 = edge.pt0;
  odb::Point e1 = edge.pt1;
  if (e0.getX() > e1.getX()) {
    std::swap(e0, e1);
  }

  // adjust for corners
  auto check_row = corners.find(row);
  if (check_row != corners.end()) {
    for (auto* inst : check_row->second) {
      const auto bbox = inst->getBBox()->getBox();

      if (bbox.xMin() == e0.getX()) {
        e0.setX(bbox.xMax());
      }
      if (bbox.xMax() == e1.getX()) {
        e1.setX(bbox.xMin());
      }
    }
  }

  odb::Point ll = row->getBBox().ll();
  ll.setX(e0.getX());

  auto pick_next_master
      = [&e1, &masters](const odb::Point& ll) -> odb::dbMaster* {
    int remaining = e1.getX() - ll.getX();
    for (auto* master : masters) {
      if (remaining % master->getWidth() == 0) {
        return master;
      }
    }
    // pick smallest if none will divide evenly
    return masters[masters.size() - 1];
  };

  while (ll.getX() < e1.getX()) {
    auto* master = pick_next_master(ll);

    debugPrint(logger_,
               utl::TAP,
               "Boundary",
               3,
               "From {} -> {}: picked {}",
               ll.getX(),
               e1.getX(),
               master->getName());

    if (!checkSymmetry(master, row->getOrient())) {
      continue;
    }

    makeInstance(db_->getChip()->getBlock(),
                 master,
                 row->getOrient(),
                 ll.getX(),
                 ll.getY(),
                 fmt::format("{}EDGE_{}_{}_",
                             options.prefix,
                             row->getName(),
                             toString(edge.type)));
    ll.addX(master->getWidth());
    insts++;
  }

  return insts;
}

int Tapcell::insertBoundaryEdgeVertical(const Tapcell::Edge& edge,
                                        const Tapcell::CornerMap& corners,
                                        const BoundaryCellOptions& options)
{
  int insts = 0;

  odb::dbMaster* edge_master_r0 = nullptr;
  odb::dbMaster* edge_master_mx = nullptr;

  switch (edge.type) {
    case EdgeType::Left:
      edge_master_r0 = options.left_r0;
      edge_master_mx = options.left_mx;
      break;
    case EdgeType::Right:
      edge_master_r0 = options.right_r0;
      edge_master_mx = options.right_mx;
      break;
    case EdgeType::Top:
    case EdgeType::Bottom:
    case EdgeType::Unknown:
      break;
  }

  odb::dbSite* site = nullptr;
  if (edge_master_r0 != nullptr) {
    site = edge_master_r0->getSite();
  } else if (edge_master_mx != nullptr) {
    site = edge_master_mx->getSite();
  }

  auto rows = getRows(edge, site);
  if (rows.empty()) {
    return 0;
  }

  for (auto* row : rows) {
    auto check_row = corners.find(row);
    if (check_row != corners.end()) {
      // corner is already placed in this row
      continue;
    }

    odb::dbMaster* master = nullptr;
    if (row->getOrient() == odb::dbOrientType::R0) {
      master = edge_master_r0;
    } else {
      master = edge_master_mx;
    }

    if (master == nullptr) {
      continue;
    }

    const int width = master->getWidth();

    odb::dbOrientType orient = row->getOrient();
    odb::Point ll;
    switch (edge.type) {
      case EdgeType::Right:
        ll = row->getBBox().lr();
        ll.addX(-width);
        break;
      case EdgeType::Left:
        ll = row->getBBox().ll();
        break;
      case EdgeType::Top:
      case EdgeType::Bottom:
      case EdgeType::Unknown:
        break;
    }

    if (!checkSymmetry(master, orient)) {
      continue;
    }

    makeInstance(db_->getChip()->getBlock(),
                 master,
                 orient,
                 ll.getX(),
                 ll.getY(),
                 fmt::format("{}EDGE_{}_{}_",
                             options.prefix,
                             row->getName(),
                             toString(edge.type)));
    insts++;
  }

  return insts;
}

BoundaryCellOptions Tapcell::correctBoundaryOptions(
    const BoundaryCellOptions& options) const
{
  return options;
}

BoundaryCellOptions Tapcell::correctBoundaryOptions(
    const Options& options) const
{
  BoundaryCellOptions bopts;

  bopts.prefix = endcap_prefix_;

  // Boundaries
  bopts.left_r0 = options.endcap_master;
  bopts.left_mx = options.endcap_master;
  bopts.right_r0 = options.endcap_master;
  bopts.right_mx = options.endcap_master;

  if (options.tap_nwin3_master) {
    bopts.top_r0.push_back(options.tap_nwin3_master);
  }
  if (options.tap_nwin2_master) {
    bopts.top_r0.push_back(options.tap_nwin2_master);
  }
  if (options.tap_nwintie_master) {
    bopts.top_r0.push_back(options.tap_nwintie_master);
  }
  if (options.tap_nwout3_master) {
    bopts.top_mx.push_back(options.tap_nwout3_master);
  }
  if (options.tap_nwout2_master) {
    bopts.top_mx.push_back(options.tap_nwout2_master);
  }
  if (options.tap_nwouttie_master) {
    bopts.top_mx.push_back(options.tap_nwouttie_master);
  }
  bopts.bottom_r0 = bopts.top_r0;
  bopts.bottom_mx = bopts.top_mx;

  bopts.outer_corner_bottom_left_r0 = options.cnrcap_nwout_master;
  bopts.outer_corner_bottom_left_mx = options.cnrcap_nwin_master;
  bopts.outer_corner_bottom_right_r0 = options.cnrcap_nwout_master;
  bopts.outer_corner_bottom_right_mx = options.cnrcap_nwin_master;
  bopts.outer_corner_top_left_r0 = options.cnrcap_nwout_master;
  bopts.outer_corner_top_left_mx = options.cnrcap_nwin_master;
  bopts.outer_corner_top_right_r0 = options.cnrcap_nwout_master;
  bopts.outer_corner_top_right_mx = options.cnrcap_nwin_master;

  bopts.inner_corner_bottom_left_r0 = options.incnrcap_nwout_master;
  bopts.inner_corner_bottom_left_mx = options.incnrcap_nwin_master;
  bopts.inner_corner_bottom_right_r0 = options.incnrcap_nwout_master;
  bopts.inner_corner_bottom_right_mx = options.incnrcap_nwin_master;
  bopts.inner_corner_top_left_r0 = options.incnrcap_nwout_master;
  bopts.inner_corner_top_left_mx = options.incnrcap_nwin_master;
  bopts.inner_corner_top_right_r0 = options.incnrcap_nwout_master;
  bopts.inner_corner_top_right_mx = options.incnrcap_nwin_master;

  return bopts;
}

void Tapcell::insertTapcells(const Options& options)
{
  if (options.tapcell_master == nullptr) {
    return;
  }

  const int dist = options.dist >= 0 ? options.dist : defaultDistance();

  insertTapcells(options.tapcell_master, dist);
}

}  // namespace tap
