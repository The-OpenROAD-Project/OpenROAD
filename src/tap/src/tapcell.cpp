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

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/util.h"
#include "ord/OpenRoad.hh"
#include "utl/Logger.h"

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
  int min_row_width = (options.endcap_master != nullptr)
                          ? 2 * options.endcap_master->getWidth()
                          : 0;
  min_row_width = std::max(min_row_width, options.row_min_width);
  odb::cutRows(block, min_row_width, blockages, halo_x, halo_y, logger_);
}

void Tapcell::run(const Options& options)
{
  cutRows(options);

  placeEndcaps(correctEndcapOptions(options));

  placeTapcells(options);
}

int Tapcell::placeTapcells(odb::dbMaster* tapcell_master,
                           const int dist,
                           const bool disallow_one_site_gaps)
{
  std::vector<Edge> edges;

  // Collect edges
  const auto areas = getBoundaryAreas();
  auto add_edge = [&edges](const Edge& edge) {
    switch (edge.type) {
      case EdgeType::Top:
      case EdgeType::Bottom:
        edges.push_back(edge);
        break;
      case EdgeType::Left:
      case EdgeType::Right:
      case EdgeType::Unknown:
        break;
    }
  };

  for (const auto& area : areas) {
    for (const auto& edge : getBoundaryEdges(area, true)) {
      add_edge(edge);
    }

    for (auto itr = area.begin_holes(); itr != area.end_holes(); itr++) {
      for (const auto& edge : getBoundaryEdges(*itr, false)) {
        add_edge(edge);
      }
    }
  }

  std::set<odb::dbRow*> edge_rows;
  for (const auto& edge : edges) {
    const auto rows = getRows(edge, tapcell_master->getSite());
    edge_rows.insert(rows.begin(), rows.end());
  }

  std::vector<odb::dbInst*> fixed_insts;
  for (auto* inst : db_->getChip()->getBlock()->getInsts()) {
    if (inst->isFixed()) {
      fixed_insts.push_back(inst);
    }
  }
  InstTree instancetree(fixed_insts.begin(), fixed_insts.end());

  int inst = 0;
  for (auto* row : db_->getChip()->getBlock()->getRows()) {
    const bool is_edge = edge_rows.find(row) != edge_rows.end();
    inst += placeTapcells(tapcell_master,
                          dist,
                          row,
                          is_edge,
                          disallow_one_site_gaps,
                          instancetree);
  }
  logger_->info(utl::TAP, 5, "Inserted {} tapcells.", inst);
  return inst;
}

int Tapcell::placeTapcells(odb::dbMaster* tapcell_master,
                           const int dist,
                           odb::dbRow* row,
                           const bool is_edge,
                           const bool disallow_one_site_gaps,
                           const InstTree& fixed_instances)
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
  int pitch_mult = 2;
  if (is_edge) {
    pitch_mult = 1;
  }
  int pitch = tap_width
              * std::floor(pitch_mult * dist / static_cast<double>(tap_width));

  if (row->getOrient() == odb::dbOrientType::R0 || is_edge) {
    offset = pitch / pitch_mult;
  } else {
    offset = pitch;
  }

  const odb::Rect row_bb = row->getBBox();

  std::set<odb::dbInst*> row_insts(
      fixed_instances.qbegin(boost::geometry::index::covered_by(row_bb)),
      fixed_instances.qend());

  const int llx = row_bb.xMin();
  const int urx = row_bb.xMax();

  const int site_width = row->getSite()->getWidth();
  for (int x = llx + offset; x < urx; x += pitch) {
    x = odb::makeSiteLoc(x, site_width, true, llx);
    // Check if site is filled
    const odb::dbOrientType ori = row->getOrient();
    std::optional<int> x_loc = findValidLocation(x,
                                                 tap_width,
                                                 ori,
                                                 row_insts,
                                                 site_width,
                                                 tap_width,
                                                 urx,
                                                 disallow_one_site_gaps);
    if (x_loc) {
      const int lly = row_bb.yMin();
      auto* inst = makeInstance(
          db_->getChip()->getBlock(),
          tapcell_master,
          ori,
          *x_loc,
          lly,
          fmt::format("{}TAPCELL_{}_", tap_prefix_, row->getName()));
      row_insts.insert(inst);
      insts++;
      x = *x_loc;
    }
  }

  return insts;
}

inline void findStartEnd(int x,
                         int width,
                         const odb::dbOrientType& orient,
                         int& x_start,
                         int& x_end)
{
  if (orient == odb::dbOrientType::MY || orient == odb::dbOrientType::R180) {
    x_start = x - width;
    x_end = x;
  } else {
    x_start = x;
    x_end = x + width;
  }
}

std::optional<int> Tapcell::findValidLocation(
    const int x,
    const int width,
    const odb::dbOrientType& orient,
    const std::set<odb::dbInst*>& row_insts,
    const int site_width,
    const int tap_width,
    const int row_urx,
    const bool disallow_one_site_gaps)
{
  int x_start;
  int x_end;
  findStartEnd(x, width, orient, x_start, x_end);

  if (disallow_one_site_gaps) {
    // The +1 is to convert > to >= (< to <=) below
    x_start -= site_width + 1;
    x_end += site_width + 1;
  }

  PartialOverlap partially_overlap;
  bool overlap = false;
  for (const auto& inst : row_insts) {
    const odb::Rect inst_bb = inst->getBBox()->getBox();
    if (x_end > inst_bb.xMin() && x_start < inst_bb.xMax()) {
      partially_overlap.left = x_end > inst_bb.xMax();
      partially_overlap.x_start_left = inst_bb.xMax();
      partially_overlap.right = x_start < inst_bb.xMin();
      partially_overlap.x_limit_right = inst_bb.xMin();
      overlap = true;
      break;
    }
  }

  std::optional<int> x_loc;
  if (!overlap) {
    x_loc = x;
  } else if (partially_overlap.right) {
    x_loc = partially_overlap.x_limit_right - tap_width;
  } else if (partially_overlap.left) {
    x_loc = partially_overlap.x_start_left;
  }

  if (x_loc) {
    bool in_rows = (*x_loc + tap_width) <= row_urx;
    const bool location_ok
        = !overlap || !isOverlapping(*x_loc, tap_width, orient, row_insts);

    if (location_ok && in_rows) {
      return x_loc;
    }
  }

  return std::nullopt;
}

bool Tapcell::isOverlapping(const int x,
                            const int width,
                            const odb::dbOrientType& orient,
                            const std::set<odb::dbInst*>& row_insts)
{
  int x_start;
  int x_end;
  findStartEnd(x, width, orient, x_start, x_end);

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

    if (logger_->debugCheck(utl::TAP, "Endcap", 1)) {
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

void Tapcell::placeEndcaps(const EndcapCellOptions& options)
{
  const auto filled_options = correctEndcapOptions(options);

  const auto areas = getBoundaryAreas();

  int corners = 0;
  int endcaps = 0;
  for (const auto& area : areas) {
    const auto& [added_corners, added_endcaps]
        = placeEndcaps(area, true, filled_options);
    corners += added_corners;
    endcaps += added_endcaps;

    for (auto itr = area.begin_holes(); itr != area.end_holes(); itr++) {
      const auto& [added_corners, added_endcaps]
          = placeEndcaps(*itr, false, filled_options);
      corners += added_corners;
    }
  }

  if (corners > 0) {
    logger_->info(utl::TAP, 3, "Inserted {} endcap corners.", corners);
  }
  if (endcaps > 0) {
    logger_->info(utl::TAP, 4, "Inserted {} endcaps.", endcaps);
  }

  filled_edges_.clear();
}

std::vector<Tapcell::Edge> Tapcell::getBoundaryEdges(const Polygon& area,
                                                     bool outer) const
{
  Polygon90 area90;
  area90.set(area.begin(), area.end());
  return getBoundaryEdges(area90, outer);
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
               "Endcap",
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

  for (const auto& pt : area) {
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
      next = corners.data();
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
               "Endcap",
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
                 "Endcap",
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
                   "Endcap",
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

std::pair<int, int> Tapcell::placeEndcaps(const Tapcell::Polygon& area,
                                          bool outer,
                                          const EndcapCellOptions& options)
{
  Polygon90 area90;
  area90.set(area.begin(), area.end());
  return placeEndcaps(area90, outer, options);
}

std::pair<int, int> Tapcell::placeEndcaps(const Tapcell::Polygon90& area,
                                          bool outer,
                                          const EndcapCellOptions& options)
{
  int corner_count = 0;
  int endcaps = 0;

  CornerMap corners;
  // insert corners first
  for (const auto& corner : getBoundaryCorners(area, outer)) {
    for (const auto& [row, insts] : placeEndcapCorner(corner, options)) {
      corners[row].insert(insts.begin(), insts.end());
      corner_count += insts.size();
    }
  }

  for (const auto& edge : getBoundaryEdges(area, outer)) {
    if (std::find(filled_edges_.begin(), filled_edges_.end(), edge)
        == filled_edges_.end()) {
      endcaps += placeEndcapEdge(edge, corners, options);
      filled_edges_.push_back(edge);
    }
  }

  return {corner_count, endcaps};
}

Tapcell::CornerMap Tapcell::placeEndcapCorner(const Tapcell::Corner& corner,
                                              const EndcapCellOptions& options)
{
  odb::dbSite* site = nullptr;
  if (options.left_bottom_corner != nullptr) {
    site = options.left_bottom_corner->getSite();
  } else if (options.right_bottom_corner != nullptr) {
    site = options.right_bottom_corner->getSite();
  } else if (options.left_top_corner != nullptr) {
    site = options.left_top_corner->getSite();
  } else if (options.right_top_corner != nullptr) {
    site = options.right_top_corner->getSite();
  } else if (options.left_bottom_edge != nullptr) {
    site = options.left_bottom_edge->getSite();
  } else if (options.right_bottom_edge != nullptr) {
    site = options.right_bottom_edge->getSite();
  } else if (options.left_top_edge != nullptr) {
    site = options.left_top_edge->getSite();
  } else if (options.right_top_edge != nullptr) {
    site = options.right_top_edge->getSite();
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
        master = options.left_bottom_corner;
      } else {
        master = options.left_top_corner;
      }
      break;
    case CornerType::OuterBottomRight:
      if (row_orient == odb::dbOrientType::R0) {
        master = options.right_bottom_corner;
      } else {
        master = options.right_top_corner;
      }
      break;
    case CornerType::OuterTopLeft:
      if (row_orient == odb::dbOrientType::R0) {
        master = options.left_top_corner;
      } else {
        master = options.left_bottom_corner;
      }
      break;
    case CornerType::OuterTopRight:
      if (row_orient == odb::dbOrientType::R0) {
        master = options.right_top_corner;
      } else {
        master = options.right_bottom_corner;
      }
      break;
    case CornerType::InnerBottomLeft:
      if (row_orient == odb::dbOrientType::R0) {
        master = options.left_bottom_edge;
      } else {
        master = options.left_top_edge;
      }
      break;
    case CornerType::InnerBottomRight:
      if (row_orient == odb::dbOrientType::R0) {
        master = options.right_bottom_edge;
      } else {
        master = options.right_top_edge;
      }
      break;
    case CornerType::InnerTopLeft:
      if (row_orient == odb::dbOrientType::R0) {
        master = options.left_top_edge;
      } else {
        master = options.left_bottom_edge;
      }
      break;
    case CornerType::InnerTopRight:
      if (row_orient == odb::dbOrientType::R0) {
        master = options.right_top_edge;
      } else {
        master = options.right_bottom_edge;
      }
      break;
    case CornerType::Unknown:
      break;
  }

  debugPrint(
      logger_,
      utl::TAP,
      "Endcap",
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

  // Adjust placement position
  odb::Point ll = corner.pt;
  switch (corner.type) {
    case CornerType::OuterBottomLeft:
    case CornerType::InnerTopRight:
      break;
    case CornerType::OuterBottomRight:
    case CornerType::InnerTopLeft:
      ll.addX(-width);
      break;
    case CornerType::OuterTopRight:
    case CornerType::InnerBottomLeft:
      ll.addX(-width);
      ll.addY(-height);
      break;
    case CornerType::OuterTopLeft:
    case CornerType::InnerBottomRight:
      ll.addY(-height);
      break;
    case CornerType::Unknown:
      break;
  }

  // Adjust orientation
  odb::dbOrientType orient = row_orient;
  switch (corner.type) {
    case CornerType::OuterBottomLeft:
    case CornerType::OuterTopLeft:
    case CornerType::InnerBottomRight:
    case CornerType::InnerTopRight:
      break;
    case CornerType::OuterBottomRight:
    case CornerType::OuterTopRight:
    case CornerType::InnerBottomLeft:
    case CornerType::InnerTopLeft:
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

int Tapcell::placeEndcapEdge(const Tapcell::Edge& edge,
                             const Tapcell::CornerMap& corners,
                             const EndcapCellOptions& options)
{
  switch (edge.type) {
    case EdgeType::Bottom:
    case EdgeType::Top:
      return placeEndcapEdgeHorizontal(edge, corners, options);
      break;
    case EdgeType::Left:
    case EdgeType::Right:
      return placeEndcapEdgeVertical(edge, corners, options);
      break;
    case EdgeType::Unknown:
      break;
  }

  return 0;
}

int Tapcell::placeEndcapEdgeHorizontal(const Tapcell::Edge& edge,
                                       const Tapcell::CornerMap& corners,
                                       const EndcapCellOptions& options)
{
  int insts = 0;

  odb::dbSite* site = nullptr;
  if (site == nullptr) {
    for (auto* master : options.top_edge) {
      site = master->getSite();
    }
  }
  if (site == nullptr) {
    for (auto* master : options.bottom_edge) {
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
            masters.end(), options.top_edge.begin(), options.top_edge.end());
      } else {
        masters.insert(masters.end(),
                       options.bottom_edge.begin(),
                       options.bottom_edge.end());
      }
      break;
    case EdgeType::Bottom:
      if (row->getOrient() == odb::dbOrientType::R0) {
        masters.insert(masters.end(),
                       options.bottom_edge.begin(),
                       options.bottom_edge.end());
      } else {
        masters.insert(
            masters.end(), options.top_edge.begin(), options.top_edge.end());
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
               "Endcap",
               3,
               "From {} -> {}: picked {}",
               ll.getX(),
               e1.getX(),
               master->getName());

    if (!checkSymmetry(master, row->getOrient())) {
      continue;
    }

    if (ll.getX() + master->getWidth() > e1.getX()) {
      const double dbus = row->getBlock()->getDbUnitsPerMicron();
      logger_->error(
          utl::TAP,
          20,
          "Unable to fill {} boundary in {} from {:.4f}um to {:.4f}um",
          toString(edge.type),
          row->getName(),
          ll.getX() / dbus,
          e1.getX() / dbus);
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

int Tapcell::placeEndcapEdgeVertical(const Tapcell::Edge& edge,
                                     const Tapcell::CornerMap& corners,
                                     const EndcapCellOptions& options)
{
  int insts = 0;

  odb::dbMaster* edge_master = nullptr;

  switch (edge.type) {
    case EdgeType::Left:
      edge_master = options.left_edge;
      break;
    case EdgeType::Right:
      edge_master = options.right_edge;
      break;
    case EdgeType::Top:
    case EdgeType::Bottom:
    case EdgeType::Unknown:
      break;
  }

  if (edge_master == nullptr) {
    return 0;
  }

  auto rows = getRows(edge, edge_master->getSite());
  if (rows.empty()) {
    return 0;
  }

  for (auto* row : rows) {
    auto check_row = corners.find(row);
    if (check_row != corners.end()) {
      // corner is already placed in this row
      continue;
    }

    const int width = edge_master->getWidth();

    odb::dbOrientType orient = row->getOrient();
    odb::Point ll;
    switch (edge.type) {
      case EdgeType::Right:
        ll = row->getBBox().lr();
        ll.addX(-width);
        if (edge_master->getSymmetryY()) {
          orient = orient.flipY();
        }
        break;
      case EdgeType::Left:
        ll = row->getBBox().ll();
        break;
      case EdgeType::Top:
      case EdgeType::Bottom:
      case EdgeType::Unknown:
        break;
    }

    if (!checkSymmetry(edge_master, orient)) {
      continue;
    }

    makeInstance(db_->getChip()->getBlock(),
                 edge_master,
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

EndcapCellOptions Tapcell::correctEndcapOptions(
    const EndcapCellOptions& options) const
{
  EndcapCellOptions bopts = options;

  auto set_single_master
      = [this](odb::dbMaster*& master, const odb::dbMasterType& type) {
          if (master == nullptr) {
            master = getMasterByType(type);
          }
        };

  auto set_multiple_master = [this](std::vector<odb::dbMaster*>& masters,
                                    const odb::dbMasterType& type) {
    if (masters.empty()) {
      const auto found = findMasterByType(type);
      masters.insert(masters.begin(), found.begin(), found.end());
    }
  };
  auto set_corner_master = [](odb::dbMaster*& master_ur,
                              odb::dbMaster*& master_ul,
                              odb::dbMaster*& master_lr,
                              odb::dbMaster*& master_ll) {
    odb::dbMaster* pref_l = master_ul == nullptr ? master_ll : master_ul;
    odb::dbMaster* pref_r = master_ur == nullptr ? master_lr : master_ur;
    if (pref_r == nullptr) {
      pref_r = pref_l;
    }
    if (pref_l == nullptr) {
      pref_l = pref_r;
    }
    if (master_ur == nullptr) {
      master_ur = pref_r;
    }
    if (master_ul == nullptr) {
      master_ul = pref_l;
    }
    if (master_lr == nullptr) {
      master_lr = pref_r;
    }
    if (master_ll == nullptr) {
      master_ll = pref_l;
    }
  };

  set_single_master(bopts.right_top_corner,
                    odb::dbMasterType::ENDCAP_LEF58_RIGHTTOPCORNER);
  set_single_master(bopts.left_top_corner,
                    odb::dbMasterType::ENDCAP_LEF58_LEFTTOPCORNER);
  set_single_master(bopts.right_bottom_corner,
                    odb::dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMCORNER);
  set_single_master(bopts.left_bottom_corner,
                    odb::dbMasterType::ENDCAP_LEF58_LEFTBOTTOMCORNER);
  set_single_master(bopts.right_top_edge,
                    odb::dbMasterType::ENDCAP_LEF58_RIGHTTOPEDGE);
  set_single_master(bopts.left_top_edge,
                    odb::dbMasterType::ENDCAP_LEF58_LEFTTOPEDGE);
  set_single_master(bopts.right_bottom_edge,
                    odb::dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMEDGE);
  set_single_master(bopts.left_bottom_edge,
                    odb::dbMasterType::ENDCAP_LEF58_LEFTBOTTOMEDGE);
  set_single_master(bopts.left_edge, odb::dbMasterType::ENDCAP_LEF58_LEFTEDGE);
  set_single_master(bopts.right_edge,
                    odb::dbMasterType::ENDCAP_LEF58_RIGHTEDGE);
  set_multiple_master(bopts.top_edge, odb::dbMasterType::ENDCAP_LEF58_TOPEDGE);
  set_multiple_master(bopts.bottom_edge,
                      odb::dbMasterType::ENDCAP_LEF58_BOTTOMEDGE);

  set_corner_master(bopts.right_top_corner,
                    bopts.left_top_corner,
                    bopts.right_bottom_corner,
                    bopts.left_bottom_corner);
  set_corner_master(bopts.right_top_edge,
                    bopts.left_top_edge,
                    bopts.right_bottom_edge,
                    bopts.left_bottom_edge);

  if (bopts.right_edge == nullptr) {
    bopts.right_edge = bopts.left_edge;
  }
  if (bopts.left_edge == nullptr) {
    bopts.left_edge = bopts.right_edge;
  }
  if (bopts.top_edge.empty()) {
    bopts.top_edge = bopts.bottom_edge;
  }
  if (bopts.bottom_edge.empty()) {
    bopts.bottom_edge = bopts.top_edge;
  }

  return bopts;
}

odb::dbMaster* Tapcell::getMasterByType(const odb::dbMasterType& type) const
{
  const std::set<odb::dbMaster*> masters = findMasterByType(type);

  if (masters.size() > 1) {
    logger_->error(utl::TAP,
                   104,
                   "Unable to find a single master for {}",
                   type.getString());
  }
  if (masters.empty()) {
    return nullptr;
  }

  return *masters.begin();
}

std::set<odb::dbMaster*> Tapcell::findMasterByType(
    const odb::dbMasterType& type) const
{
  std::set<odb::dbMaster*> masters;
  for (auto* lib : db_->getLibs()) {
    for (auto* master : lib->getMasters()) {
      if (master->getType() == type) {
        masters.insert(master);
      }
    }
  }

  return masters;
}

EndcapCellOptions Tapcell::correctEndcapOptions(const Options& options) const
{
  EndcapCellOptions bopts;

  bopts.prefix = endcap_prefix_;

  // Endcaps
  bopts.left_edge = options.endcap_master;
  bopts.right_edge = options.endcap_master;

  for (auto* master : {options.tap_nwin3_master,
                       options.tap_nwin2_master,
                       options.tap_nwintie_master}) {
    if (master != nullptr) {
      bopts.bottom_edge.push_back(master);
    }
  }
  for (auto* master : {options.tap_nwout3_master,
                       options.tap_nwout2_master,
                       options.tap_nwouttie_master}) {
    if (master != nullptr) {
      bopts.top_edge.push_back(master);
    }
  }

  bopts.left_top_corner = options.cnrcap_nwout_master;
  bopts.left_bottom_corner = options.cnrcap_nwin_master;
  bopts.right_top_corner = options.cnrcap_nwout_master;
  bopts.right_bottom_corner = options.cnrcap_nwin_master;

  bopts.left_top_edge = options.incnrcap_nwin_master;
  bopts.left_bottom_edge = options.incnrcap_nwout_master;
  bopts.right_top_edge = options.incnrcap_nwin_master;
  bopts.right_bottom_edge = options.incnrcap_nwout_master;

  return bopts;
}

void Tapcell::placeTapcells(const Options& options)
{
  if (options.tapcell_master == nullptr) {
    return;
  }

  const int dist = options.dist >= 0 ? options.dist : defaultDistance();

  placeTapcells(options.tapcell_master, dist, options.disallow_one_site_gaps);
}

odb::dbBlock* Tapcell::getBlock() const
{
  return db_->getChip()->getBlock();
}

bool Tapcell::Edge::operator==(const Edge& edge) const
{
  return type == edge.type
         && ((pt0 == edge.pt0 && pt1 == edge.pt1)
             || (pt0 == edge.pt1 && pt1 == edge.pt0));
}

}  // namespace tap
