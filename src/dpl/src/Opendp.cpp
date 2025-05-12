// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#include "dpl/Opendp.h"

#include <cfloat>
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "PlacementDRC.h"
#include "dpl/OptMirror.h"
#include "graphics/DplObserver.h"
#include "infrastructure/Grid.h"
#include "infrastructure/Objects.h"
#include "infrastructure/Padding.h"
#include "infrastructure/network.h"
#include "odb/util.h"
#include "util/journal.h"
#include "utl/Logger.h"

namespace dpl {

using std::round;
using std::string;

using utl::DPL;

using odb::Rect;

////////////////////////////////////////////////////////////////

bool Opendp::isMultiRow(const Node* cell) const
{
  return network_->getMaster(cell->getDbInst()->getMaster())->isMultiRow();
}

////////////////////////////////////////////////////////////////

Opendp::Opendp()
{
  dummy_cell_ = std::make_unique<Node>();
  dummy_cell_->setPlaced(true);
}

Opendp::~Opendp() = default;

void Opendp::init(dbDatabase* db, Logger* logger)
{
  db_ = db;
  logger_ = logger;
  padding_ = std::make_shared<Padding>();
  grid_ = std::make_unique<Grid>();
  grid_->init(logger);
  network_ = std::make_unique<Network>();
  arch_ = std::make_unique<Architecture>();
}

void Opendp::setPaddingGlobal(const int left, const int right)
{
  padding_->setPaddingGlobal(GridX{left}, GridX{right});
}

void Opendp::setPadding(dbInst* inst, const int left, const int right)
{
  padding_->setPadding(inst, GridX{left}, GridX{right});
}

void Opendp::setPadding(dbMaster* master, const int left, const int right)
{
  padding_->setPadding(master, GridX{left}, GridX{right});
}

void Opendp::setDebug(std::unique_ptr<DplObserver>& observer)
{
  debug_observer_ = std::move(observer);
}

void Opendp::setJournal(Journal* journal)
{
  journal_ = journal;
}

Journal* Opendp::getJournal() const
{
  return journal_;
}

void Opendp::detailedPlacement(const int max_displacement_x,
                               const int max_displacement_y,
                               const std::string& report_file_name)
{
  importDb();
  adjustNodesOrient();
  for (const auto& node : network_->getNodes()) {
    if (node->getType() == Node::CELL && !node->isFixed()) {
      node->setPlaced(false);
    }
  }

  if (have_fillers_) {
    logger_->warn(DPL, 37, "Use remove_fillers before detailed placement.");
  }

  if (max_displacement_x == 0 || max_displacement_y == 0) {
    // defaults
    max_displacement_x_ = 500;
    max_displacement_y_ = 100;
  } else {
    max_displacement_x_ = max_displacement_x;
    max_displacement_y_ = max_displacement_y;
  }

  odb::WireLengthEvaluator eval(block_);
  hpwl_before_ = eval.hpwl();
  detailedPlacement();
  // Save displacement stats before updating instance DB locations.
  findDisplacementStats();
  updateDbInstLocations();
  if (!placement_failures_.empty()) {
    logger_->info(DPL,
                  34,
                  "Detailed placement failed on the following {} instances:",
                  placement_failures_.size());
    for (auto cell : placement_failures_) {
      logger_->info(DPL, 35, " {}", cell->name());
    }

    saveFailures({}, {}, {}, {}, {}, {}, placement_failures_, {});
    if (!report_file_name.empty()) {
      writeJsonReport(report_file_name);
    }
    logger_->error(DPL, 36, "Detailed placement failed.");
  }
}

void Opendp::updateDbInstLocations()
{
  for (auto& cell : network_->getNodes()) {
    if (!cell->isFixed() && cell->isStdCell()) {
      dbInst* db_inst_ = cell->getDbInst();
      // Only move the instance if necessary to avoid triggering callbacks.
      if (db_inst_->getOrient() != cell->getOrient()) {
        db_inst_->setOrient(cell->getOrient());
      }
      const DbuX x = core_.xMin() + cell->getLeft();
      const DbuY y = core_.yMin() + cell->getBottom();
      int inst_x, inst_y;
      db_inst_->getLocation(inst_x, inst_y);
      if (x != inst_x || y != inst_y) {
        db_inst_->setLocation(x.v, y.v);
      }
    }
  }
}

void Opendp::reportLegalizationStats() const
{
  logger_->report("Placement Analysis");
  logger_->report("---------------------------------");
  logger_->report("total displacement   {:10.1f} u",
                  block_->dbuToMicrons(displacement_sum_));
  logger_->metric("design__instance__displacement__total",
                  block_->dbuToMicrons(displacement_sum_));
  logger_->report("average displacement {:10.1f} u",
                  block_->dbuToMicrons(displacement_avg_));
  logger_->metric("design__instance__displacement__mean",
                  block_->dbuToMicrons(displacement_avg_));
  logger_->report("max displacement     {:10.1f} u",
                  block_->dbuToMicrons(displacement_max_));
  logger_->metric("design__instance__displacement__max",
                  block_->dbuToMicrons(displacement_max_));
  logger_->report("original HPWL        {:10.1f} u",
                  block_->dbuToMicrons(hpwl_before_));
  odb::WireLengthEvaluator eval(block_);
  const double hpwl_legal = eval.hpwl();
  logger_->report("legalized HPWL       {:10.1f} u",
                  block_->dbuToMicrons(hpwl_legal));
  logger_->metric("route__wirelength__estimated",
                  block_->dbuToMicrons(hpwl_legal));
  const int hpwl_delta
      = (hpwl_before_ == 0.0)
            ? 0.0
            : round((hpwl_legal - hpwl_before_) / hpwl_before_ * 100);
  logger_->report("delta HPWL           {:10} %", hpwl_delta);
  logger_->report("");
}

////////////////////////////////////////////////////////////////

void Opendp::findDisplacementStats()
{
  displacement_avg_ = 0;
  displacement_sum_ = 0;
  displacement_max_ = 0;
  for (auto& cell : network_->getNodes()) {
    if (cell->getType() != Node::CELL) {
      continue;
    }
    const int displacement = disp(cell.get());
    displacement_sum_ += displacement;
    if (displacement > displacement_max_) {
      displacement_max_ = displacement;
    }
  }
  if (network_->getNumCells() != 0) {
    displacement_avg_ = displacement_sum_ / network_->getNumCells();
  } else {
    displacement_avg_ = 0.0;
  }
}

////////////////////////////////////////////////////////////////

void Opendp::optimizeMirroring()
{
  OptimizeMirroring opt(logger_, db_);
  opt.run();
}

int Opendp::disp(const Node* cell) const
{
  const DbuPt init = initialLocation(cell, false);
  return sumXY(abs(init.x - cell->getLeft()), abs(init.y - cell->getBottom()));
}

int Opendp::padGlobalLeft() const
{
  return padding_->padGlobalLeft().v;
}

int Opendp::padGlobalRight() const
{
  return padding_->padGlobalRight().v;
}

int Opendp::padLeft(dbInst* inst) const
{
  return padding_->padLeft(inst).v;
}

int Opendp::padRight(dbInst* inst) const
{
  return padding_->padRight(inst).v;
}

void Opendp::initGrid()
{
  grid_->initGrid(
      db_, block_, padding_, max_displacement_x_, max_displacement_y_);
}

void Opendp::deleteGrid()
{
  grid_->clear();
}

void Opendp::findOverlapInRtree(const bgBox& queryBox,
                                std::vector<bgBox>& overlaps) const
{
  overlaps.clear();
  regions_rtree_.query(boost::geometry::index::intersects(queryBox),
                       std::back_inserter(overlaps));
}

void Opendp::setFixedGridCells()
{
  for (auto& cell : network_->getNodes()) {
    if (cell->getType() == Node::CELL && cell->isFixed()) {
      grid_->visitCellPixels(
          *cell, true, [&](Pixel* pixel) { setGridCell(*cell, pixel); });
    }
  }
}

void Opendp::setGridCell(Node& cell, Pixel* pixel)
{
  pixel->cell = &cell;
  pixel->util = 1.0;
  if (cell.isBlock()) {
    // Try the is_hopeless strategy to get off of a block
    pixel->is_hopeless = true;
  }
}

void Opendp::groupAssignCellRegions()
{
  const int64_t site_width = grid_->getSiteWidth().v;
  const GridX row_site_count = grid_->getRowSiteCount();
  const GridY row_count = grid_->getRowCount();

  for (auto& group : arch_->getRegions()) {
    int64_t total_site_area = 0;
    if (!group->getCells().empty()) {
      for (GridX x{0}; x < row_site_count; x++) {
        for (GridY y{0}; y < row_count; y++) {
          const Pixel* pixel = grid_->gridPixel(x, y);
          if (pixel->is_valid && pixel->group == group) {
            total_site_area += grid_->rowHeight(y).v * site_width;
          }
        }
      }
    }

    double cell_area = 0;
    for (Node* cell : group->getCells()) {
      cell_area += cell->area();

      for (const auto& rect : group->getRects()) {
        if (isInside(cell, rect)) {
          cell->setRegion(&rect);
        }
      }
      if (cell->getRegion() == nullptr) {
        cell->setRegion(group->getRects().data());
      }
    }
    group->setUtil(total_site_area ? cell_area / total_site_area : 0.0);
  }
}

void Opendp::groupInitPixels2()
{
  for (GridX x{0}; x < grid_->getRowSiteCount(); x++) {
    for (GridY y{0}; y < grid_->getRowCount(); y++) {
      const Rect sub(x.v * grid_->getSiteWidth().v,
                     grid_->gridYToDbu(y).v,
                     (x + 1).v * grid_->getSiteWidth().v,
                     grid_->gridYToDbu(y + 1).v);
      Pixel* pixel = grid_->gridPixel(x, y);
      for (auto& group : arch_->getRegions()) {
        for (const Rect& rect : group->getRects()) {
          if (!isInside(sub, rect) && checkOverlap(sub, rect)) {
            pixel->util = 0.0;
            pixel->cell = dummy_cell_.get();
            pixel->is_valid = false;
            debugPrint(logger_,
                       DPL,
                       "group",
                       1,
                       "Block pixel [({}, {}) on region boundary",
                       x.v,
                       y.v);
          }
        }
      }
    }
  }
}

dbInst* Opendp::getAdjacentInstance(dbInst* inst, bool left) const
{
  const Rect inst_rect = inst->getBBox()->getBox();
  DbuX x_dbu = left ? DbuX{inst_rect.xMin() - 1} : DbuX{inst_rect.xMax() + 1};
  x_dbu -= core_.xMin();
  GridX x = grid_->gridX(x_dbu);

  GridY y = grid_->gridSnapDownY(DbuY{inst_rect.yMin() - core_.yMin()});

  Pixel* pixel = grid_->gridPixel(x, y);

  dbInst* adjacent_inst = nullptr;

  // do not return macros, endcaps and tapcells
  if (pixel != nullptr && pixel->cell && pixel->cell->getDbInst()->isCore()) {
    adjacent_inst = pixel->cell->getDbInst();
  }

  return adjacent_inst;
}

std::vector<dbInst*> Opendp::getAdjacentInstancesCluster(dbInst* inst) const
{
  const bool left = true;
  const bool right = false;
  std::vector<dbInst*> adj_inst_cluster;

  dbInst* left_inst = getAdjacentInstance(inst, left);
  while (left_inst != nullptr) {
    adj_inst_cluster.push_back(left_inst);
    // the right instance can be ignored, since it was added in the line above
    left_inst = getAdjacentInstance(left_inst, left);
  }

  std::reverse(adj_inst_cluster.begin(), adj_inst_cluster.end());
  adj_inst_cluster.push_back(inst);

  dbInst* right_inst = getAdjacentInstance(inst, right);
  while (right_inst != nullptr) {
    adj_inst_cluster.push_back(right_inst);
    // the left instance can be ignored, since it was added in the line above
    right_inst = getAdjacentInstance(right_inst, right);
  }

  return adj_inst_cluster;
}

/* static */
bool Opendp::isInside(const Rect& cell, const Rect& box)
{
  return cell.xMin() >= box.xMin() && cell.xMax() <= box.xMax()
         && cell.yMin() >= box.yMin() && cell.yMax() <= box.yMax();
}

bool Opendp::checkOverlap(const Rect& cell, const Rect& box)
{
  return box.xMin() < cell.xMax() && box.xMax() > cell.xMin()
         && box.yMin() < cell.yMax() && box.yMax() > cell.yMin();
}

void Opendp::groupInitPixels()
{
  for (GridX x{0}; x < grid_->getRowSiteCount(); x++) {
    for (GridY y{0}; y < grid_->getRowCount(); y++) {
      Pixel* pixel = grid_->gridPixel(x, y);
      pixel->util = 0.0;
    }
  }
  for (auto& group : arch_->getRegions()) {
    if (group->getCells().empty()) {
      if (group->getId() != 0) {
        logger_->warn(
            DPL, 42, "No cells found in group {}. ", group->getName());
      }
      continue;
    }
    const DbuX site_width = grid_->getSiteWidth();
    for (const DbuRect rect : group->getRects()) {
      debugPrint(logger_,
                 DPL,
                 "detailed",
                 1,
                 "Group {} region [x{} y{}] [x{} y{}]",
                 group->getName(),
                 rect.xl.v,
                 rect.yl.v,
                 rect.xh.v,
                 rect.yh.v);
      const GridRect grid_rect{grid_->gridWithin(rect)};

      for (GridY k{grid_rect.ylo}; k < grid_rect.yhi; k++) {
        for (GridX l{grid_rect.xlo}; l < grid_rect.xhi; l++) {
          Pixel* pixel = grid_->gridPixel(l, k);
          pixel->util += 1.0;
        }
        if (rect.xl % site_width != 0) {
          Pixel* pixel = grid_->gridPixel(grid_rect.xlo, k);
          pixel->util
              -= (rect.xl % site_width).v / static_cast<double>(site_width.v);
        }
        if (rect.xh % site_width != 0) {
          Pixel* pixel = grid_->gridPixel(grid_rect.xhi - 1, k);
          pixel->util -= ((site_width - rect.xh) % site_width).v
                         / static_cast<double>(site_width.v);
        }
      }
    }
    for (const DbuRect rect : group->getRects()) {
      const GridRect grid_rect{grid_->gridWithin(rect)};

      for (GridY k{grid_rect.ylo}; k < grid_rect.yhi; k++) {
        for (GridX l{grid_rect.xlo}; l < grid_rect.xhi; l++) {
          // Assign group to each pixel.
          Pixel* pixel = grid_->gridPixel(l, k);
          if (pixel->util == 1.0) {
            pixel->group = group;
            pixel->is_valid = true;
            pixel->util = 1.0;
          } else if (pixel->util > 0.0 && pixel->util < 1.0) {
            pixel->cell = dummy_cell_.get();
            pixel->util = 0.0;
            pixel->is_valid = false;
          }
        }
      }
    }
  }
}

int divRound(const int dividend, const int divisor)
{
  return round(static_cast<double>(dividend) / divisor);
}

int divCeil(const int dividend, const int divisor)
{
  return ceil(static_cast<double>(dividend) / divisor);
}

int divFloor(const int dividend, const int divisor)
{
  return dividend / divisor;
}

}  // namespace dpl
