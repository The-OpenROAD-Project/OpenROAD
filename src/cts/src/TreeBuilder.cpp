// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "TreeBuilder.h"

#include <boost/polygon/polygon.hpp>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "odb/geom_boost.h"
#include "utl/Logger.h"

namespace cts {

using utl::CTS;

void TreeBuilder::mergeBlockages()
{
  namespace gtl = boost::polygon;
  using boost::polygon::operators::operator+=;

  odb::dbBlock* block = db_->getChip()->getBlock();
  gtl::polygon_90_set_data<int> blockage_polygons;
  // Add the macros into the polygon set
  for (odb::dbInst* inst : block->getInsts()) {
    if (inst->getMaster()->getType().isBlock()
        && inst->getPlacementStatus().isPlaced()) {
      blockage_polygons += inst->getBBox()->getBox();
    }
  }

  // Add the hard blockages into the polygon set
  for (odb::dbBlockage* blockage : block->getBlockages()) {
    if (!blockage->isSoft()) {
      blockage_polygons += blockage->getBBox()->getBox();
    }
  }

  // bloat blockages to merge if there is not enough space between them
  const int bloat_h
      = std::ceil(bufferHeight_ * techChar_->getLengthUnit() / 2.0);
  const int bloat_w
      = std::ceil(bufferWidth_ * techChar_->getLengthUnit() / 2.0);

  // Remove the gaps less than twice the bloat dimension
  gtl::bloat(blockage_polygons, bloat_w, bloat_w, bloat_h, bloat_h);
  gtl::shrink(blockage_polygons, bloat_w, bloat_w, bloat_h, bloat_h);

  std::vector<odb::Rect> blockage_rects;
  std::vector<gtl::polygon_90_with_holes_data<int>> blockage_rects_polygon;
  blockage_polygons.get_polygons(blockage_rects_polygon);

  for (const auto& poly : blockage_rects_polygon) {
    odb::Rect rect;
    gtl::extents(rect, poly);
    blockages_.emplace_back(rect);
  }
}

void TreeBuilder::initBlockages()
{
  // add tree buffer width and height for legalization
  std::string buffer;
  if (!options_->getTreeBuffer().empty()) {
    buffer = options_->getTreeBuffer();
  } else {
    buffer = options_->getRootBuffer();
  }
  odb::dbMaster* libCell = db_->findMaster(buffer.c_str());
  if (libCell != nullptr) {
    bufferWidth_ = (double) libCell->getWidth() / techChar_->getLengthUnit();
    bufferHeight_ = (double) libCell->getHeight() / techChar_->getLengthUnit();
    // clang-format off
    debugPrint(logger_, CTS, "legalizer", 3, "buf width= {:0.3f} buf ht= {:0.3f} "
               "scalingUnit={}", bufferWidth_, bufferHeight_,
               techChar_->getLengthUnit());
    // clang-format on
  } else {
    logger_->error(
        CTS, 77, "No physical master cell found for cell {}.", buffer);
  }

  mergeBlockages();

  logger_->info(CTS,
                201,
                "{} blockages from hard placement blockages and placed macros "
                "will be used.",
                blockages_.size());
}

// Check if location (x, y) is legal by checking if
// 1) it lies along edges of a known blockage (x1,y1) (x2,y2), or
// 2) it is not on any other blockages (more expensive)
bool TreeBuilder::checkLegalitySpecial(Point<double> loc,
                                       double x1,
                                       double y1,
                                       double x2,
                                       double y2,
                                       int scalingFactor)
{
  if (!isOccupiedLoc(loc)
      && isAlongBbox(loc.getX(), loc.getY(), x1, y1, x2, y2)) {
    return true;
  }

  if (checkLegalityLoc(loc, scalingFactor)) {
    return true;
  }

  return false;
}

// Find one blockage that contains bufferLoc
// (x1, y1) is the lower left corner
// (x2, y2) is the upper right corner
bool TreeBuilder::findBlockage(const Point<double>& bufferLoc,
                               double scalingUnit,
                               double& x1,
                               double& y1,
                               double& x2,
                               double& y2)
{
  double bx = bufferLoc.getX() * scalingUnit;
  double by = bufferLoc.getY() * scalingUnit;

  for (odb::Rect bbox : blockages_) {
    x1 = bbox.xMin();
    y1 = bbox.yMin();
    x2 = bbox.xMax();
    y2 = bbox.yMax();

    if (isInsideBbox(bx, by, x1, y1, x2, y2)) {
      x1 = x1 / scalingUnit;
      y1 = y1 / scalingUnit;
      x2 = x2 / scalingUnit;
      y2 = y2 / scalingUnit;
      return true;
    }
  }

  return false;
}

//
// Legalize one buffer (can be L0, L1, L2, leaf or level buffer)
// bufferLoc needs to in non-dbu units: without wireSegmentUnit multiplier
// bufferName is a string that contains name of buffer master cell
//
Point<double> TreeBuilder::legalizeOneBuffer(Point<double> bufferLoc,
                                             const std::string& bufferName)
{
  if (options_->getObstructionAware()) {
    odb::dbMaster* libCell = db_->findMaster(bufferName.c_str());
    assert(libCell != nullptr);
    // check if current buffer sits on top of blockage
    const double wireSegmentUnit = techChar_->getLengthUnit();
    double x1, y1, x2, y2;
    if (findBlockage(bufferLoc, wireSegmentUnit, x1, y1, x2, y2)) {
      // x1, y1 are lower left corner of blockage
      // x2, y2 are upper right corner of blockage
      // move buffer to the nearest legal location by snapping it to right,
      // left, top or bottom while considering cell height and width to avoid
      // any overlap with blockage
      double bx = bufferLoc.getX();
      double by = bufferLoc.getY();
      Point<double> newLoc = bufferLoc;
      std::vector<Point<double>> candidates;

      // first, try snapping around the left edge
      // need to adjust for buffer width
      double bufWidth = (double) libCell->getWidth() / wireSegmentUnit;
      double bufHeight = (double) libCell->getHeight() / wireSegmentUnit;
      double newX = x1 - bufWidth;
      addCandidatePoint(newX, by, newLoc, candidates);
      addCandidatePoint(newX, by + bufHeight, newLoc, candidates);
      addCandidatePoint(newX, by - bufHeight, newLoc, candidates);

      // second, try snapping around the right edge
      addCandidatePoint(x2, by, newLoc, candidates);
      addCandidatePoint(x2, by + bufHeight, newLoc, candidates);
      addCandidatePoint(x2, by - bufHeight, newLoc, candidates);

      // third, try snapping around the bottom edge
      // need to adjust for buffer height
      double newY = y1 - bufHeight;
      addCandidatePoint(bx, newY, newLoc, candidates);
      addCandidatePoint(bx + bufWidth, newY, newLoc, candidates);
      addCandidatePoint(bx - bufWidth, newY, newLoc, candidates);

      // fourth, try snapping around the top edge
      addCandidatePoint(bx, y2, newLoc, candidates);
      addCandidatePoint(bx + bufWidth, y2, newLoc, candidates);
      addCandidatePoint(bx - bufWidth, y2, newLoc, candidates);

      // pick the best one
      double minDist = std::numeric_limits<double>::max();
      double dist = 0.0;
      Point<double> noBest(-1e6, -1e6);
      Point<double> bestLoc = noBest;

      for (const Point<double>& candidate : candidates) {
        if (!isOccupiedLoc(candidate)) {
          dist = computeDist(candidate, bufferLoc);
          if (dist < minDist) {
            minDist = dist;
            bestLoc = candidate;
          }
        }
      }

      if (bestLoc != noBest) {
        return bestLoc;
      }
    }
  }

  return bufferLoc;
}

// Check if a particular location is legal by checking
// 1) if the location is occupied by another cell
// 2) if the location is sitting on a blockage
// 3) if the location is within core area
bool TreeBuilder::checkLegalityLoc(const Point<double>& bufferLoc,
                                   int scalingFactor)
{
  // check if location is already occupied
  if (occupiedLocations_.find(bufferLoc) != occupiedLocations_.end()) {
    // clang-format off
    debugPrint(logger_, CTS, "legalizer", 4, "loc {} is already occupied",
	       bufferLoc);
    // clang-format on
    return false;
  }

  // check if location is within core area
  odb::Rect coreArea = db_->getChip()->getBlock()->getCoreArea();
  odb::Point loc(bufferLoc.getX() * scalingFactor,
                 bufferLoc.getY() * scalingFactor);
  if (!coreArea.overlaps(loc)) {
    // clang-format off
    debugPrint(logger_, CTS, "legalizer", 4, "loc {} is outside core area",
	       bufferLoc);
    // clang-format on
    return false;
  }

  double x1, y1, x2, y2;
  if (findBlockage(bufferLoc, scalingFactor, x1, y1, x2, y2)) {
    // clang-format off
    debugPrint(logger_, CTS, "legalizer", 4, "loc {} is in blockage ({:0.3f}"
	       "{:0.3f}) ({:0.3f} {:0.3f})", bufferLoc, x1, y1, x2, y2);
    // clang-format on

    return false;
  }

  return true;
}

bool TreeBuilder::isOccupiedLoc(const Point<double>& bufferLoc)
{
  // clang-format off
  if (occupiedLocations_.find(bufferLoc) != occupiedLocations_.end()) {
    debugPrint(logger_, CTS, "legalizer", 4, "loc {} is already occupied",
	       bufferLoc);
    return true;
  }
  debugPrint(logger_, CTS, "legalizer", 4, "loc {} is not occupied", bufferLoc);
  return false;
  // clang-format on
}

void TreeBuilder::commitLoc(const Point<double>& bufferLoc)
{
  // clang-format off
  occupiedLocations_.insert(bufferLoc);
  debugPrint(logger_, CTS, "legalizer", 4, "loc {} has been committed, size={}",
             bufferLoc, occupiedLocations_.size());
  // clang-format on
}

void TreeBuilder::uncommitLoc(const Point<double>& bufferLoc)
{
  // clang-format off
  occupiedLocations_.erase(bufferLoc);
  debugPrint(logger_, CTS, "legalizer", 4, "loc {} has been uncommitted, "
	     "size={}", bufferLoc, occupiedLocations_.size());
  // clang-format on
}

void TreeBuilder::commitMoveLoc(const Point<double>& oldLoc,
                                const Point<double>& newLoc)
{
  // clang-format off
  occupiedLocations_.erase(oldLoc);
  occupiedLocations_.insert(newLoc);
  debugPrint(logger_, CTS, "legalizer", 4, "move:{} -> {} has been committed, "
	     "size={}", oldLoc, newLoc, occupiedLocations_.size());
  // clang-format on
}

}  // namespace cts
