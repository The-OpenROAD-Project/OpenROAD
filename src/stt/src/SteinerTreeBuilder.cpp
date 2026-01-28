// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "stt/SteinerTreeBuilder.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "stt/flute.h"
#include "stt/pd.h"
#include "utl/Logger.h"

namespace stt {

static void reportSteinerBranches(const stt::Tree& tree, utl::Logger* logger);

SteinerTreeBuilder::SteinerTreeBuilder(odb::dbDatabase* db, utl::Logger* logger)
    : alpha_(0.3),
      min_fanout_alpha_({0, -1}),
      min_hpwl_alpha_({0, -1}),
      logger_(logger),
      db_(db),
      flute_(new flt::Flute())
{
}

SteinerTreeBuilder::~SteinerTreeBuilder() = default;

Tree SteinerTreeBuilder::makeSteinerTree(const std::vector<int>& x,
                                         const std::vector<int>& y,
                                         const int drvr_index)
{
  Tree tree = makeSteinerTree(x, y, drvr_index, alpha_);

  return tree;
}

Tree SteinerTreeBuilder::makeSteinerTree(odb::dbNet* net,
                                         const std::vector<int>& x,
                                         const std::vector<int>& y,
                                         const int drvr_index)
{
  float net_alpha = alpha_;
  int min_fanout = min_fanout_alpha_.first;
  int min_hpwl = min_hpwl_alpha_.first;

  if (net_alpha_map_.find(net) != net_alpha_map_.end()) {
    net_alpha = net_alpha_map_[net];
  } else if (min_hpwl > 0) {
    if (computeHPWL(net) >= min_hpwl) {
      net_alpha = min_hpwl_alpha_.second;
    }
  } else if (min_fanout > 0) {
    if (net->getTermCount() - 1 >= min_fanout) {
      net_alpha = min_fanout_alpha_.second;
    }
  }

  return makeSteinerTree(x, y, drvr_index, net_alpha);
}

Tree SteinerTreeBuilder::makeSteinerTree(const std::vector<int>& x,
                                         const std::vector<int>& y,
                                         const int drvr_index,
                                         const float alpha)
{
  if (alpha > 0.0) {
    Tree tree = pdr::primDijkstra(x, y, drvr_index, alpha, logger_);
    if (checkTree(tree)) {
      return tree;
    }
    // Fall back to flute if PD fails.
  }
  return flute_->flute(x, y, flute_accuracy);
}

Tree SteinerTreeBuilder::makeSteinerTree(const std::vector<int>& x,
                                         const std::vector<int>& y,
                                         const std::vector<int>& s,
                                         int accuracy)
{
  return flute_->flutes(x, y, s, accuracy);
}

static bool rectAreaZero(const odb::Rect& rect)
{
  return rect.xMin() == rect.xMax() && rect.yMin() == rect.yMax();
}

static bool isCorner(const odb::Rect& rect, int x, int y)
{
  return (rect.xMin() == x && rect.yMin() == y)
         || (rect.xMin() == x && rect.yMax() == y)
         || (rect.xMax() == x && rect.yMin() == y)
         || (rect.xMax() == x && rect.yMax() == y);
}

static bool shareCorner(const odb::Rect& rect1, const odb::Rect& rect2)
{
  return isCorner(rect1, rect2.xMin(), rect2.yMin())
         || isCorner(rect1, rect2.xMin(), rect2.yMax())
         || isCorner(rect1, rect2.xMax(), rect2.yMin())
         || isCorner(rect1, rect2.xMax(), rect2.yMax());
}

// This checks whether the tree has the property that no two
// non-adjacent edges have intersecting bounding boxes.  If
// they do it is a failure as embedding those egdes may cause
// overlap between them.
bool SteinerTreeBuilder::checkTree(const Tree& tree) const
{
  // Such high fanout nets are going to get buffered so
  // we don't need to worry about them.
  if (tree.deg > 100) {
    return true;
  }

  std::vector<odb::Rect> rects;
  // Ignore zero length branches by picking one end as the representative.
  int branch_count = tree.branchCount();
  for (int i = 0; i < branch_count; ++i) {
    const Branch& branch = tree.branch[i];
    const int x1 = branch.x;
    const int y1 = branch.y;
    const Branch& neighbor = tree.branch[branch.n];
    const int x2 = neighbor.x;
    const int y2 = neighbor.y;
    rects.emplace_back(x1, y1, x2, y2);
  }
  for (auto i = 0; i < rects.size(); ++i) {
    for (auto j = i + 1; j < rects.size(); ++j) {
      const Branch& b1 = tree.branch[i];
      const Branch& b2 = tree.branch[j];
      const odb::Rect& r1 = rects[i];
      const odb::Rect& r2 = rects[j];
      if (!rectAreaZero(r1) && !rectAreaZero(r2) && r1.intersects(r2)
          && !shareCorner(r1, r2)) {
        debugPrint(logger_,
                   utl::STT,
                   "check",
                   1,
                   "check failed ({}, {}) ({}, {}) [{}, {}] vs ({}, {}) ({}, "
                   "{}) [{}, {}] degree={}",
                   r1.xMin(),
                   r1.yMin(),
                   r1.xMax(),
                   r1.yMax(),
                   i,
                   b1.n,
                   r2.xMin(),
                   r2.yMin(),
                   r2.xMax(),
                   r2.yMax(),
                   j,
                   b2.n,
                   tree.deg);
        return false;
      }
    }
  }

  return true;
}

////////////////////////////////////////////////////////////////

void SteinerTreeBuilder::setAlpha(float alpha)
{
  alpha_ = alpha;
}

float SteinerTreeBuilder::getAlpha(const odb::dbNet* net) const
{
  float net_alpha = net_alpha_map_.find(net) != net_alpha_map_.end()
                        ? net_alpha_map_.at(net)
                        : alpha_;
  return net_alpha;
}

void SteinerTreeBuilder::setNetAlpha(const odb::dbNet* net, float alpha)
{
  net_alpha_map_[net] = alpha;
}

void SteinerTreeBuilder::setMinFanoutAlpha(int min_fanout, float alpha)
{
  min_fanout_alpha_ = {min_fanout, alpha};
}

void SteinerTreeBuilder::setMinHPWLAlpha(int min_hpwl, float alpha)
{
  min_hpwl_alpha_ = {min_hpwl, alpha};
}

int SteinerTreeBuilder::computeHPWL(odb::dbNet* net)
{
  if (net->getBTermCount() + net->getITermCount() == 0) {
    return 0;
  }

  int min_x = std::numeric_limits<int>::max();
  int min_y = std::numeric_limits<int>::max();
  int max_x = std::numeric_limits<int>::min();
  int max_y = std::numeric_limits<int>::min();

  for (odb::dbITerm* iterm : net->getITerms()) {
    odb::dbPlacementStatus status = iterm->getInst()->getPlacementStatus();
    if (status != odb::dbPlacementStatus::NONE
        && status != odb::dbPlacementStatus::UNPLACED) {
      int x, y;
      iterm->getAvgXY(&x, &y);
      min_x = std::min(min_x, x);
      max_x = std::max(max_x, x);
      min_y = std::min(min_y, y);
      max_y = std::max(max_y, y);
    } else {
      logger_->error(utl::STT,
                     4,
                     "Net {} is connected to unplaced instance {}.",
                     net->getName(),
                     iterm->getInst()->getName());
    }
  }

  for (odb::dbBTerm* bterm : net->getBTerms()) {
    if (bterm->getFirstPinPlacementStatus() != odb::dbPlacementStatus::NONE
        || bterm->getFirstPinPlacementStatus()
               != odb::dbPlacementStatus::UNPLACED) {
      int x, y;
      bterm->getFirstPinLocation(x, y);
      min_x = std::min(min_x, x);
      max_x = std::max(max_x, x);
      min_y = std::min(min_y, y);
      max_y = std::max(max_y, y);
    } else {
      logger_->error(utl::STT,
                     5,
                     "Net {} is connected to unplaced pin {}.",
                     net->getName(),
                     bterm->getName());
    }
  }

  int hpwl = (max_x - min_x) + (max_y - min_y);

  return hpwl;
}

Tree SteinerTreeBuilder::flute(const std::vector<int>& x,
                               const std::vector<int>& y,
                               int acc)
{
  return flute_->flute(x, y, acc);
}

int SteinerTreeBuilder::wirelength(const Tree& t)
{
  return flute_->wirelength(t);
}

void SteinerTreeBuilder::plottree(const Tree& t)
{
  flute_->plottree(t);
}

Tree SteinerTreeBuilder::flutes(const std::vector<int>& xs,
                                const std::vector<int>& ys,
                                const std::vector<int>& s,
                                int acc)
{
  return flute_->flutes(xs, ys, s, acc);
};

////////////////////////////////////////////////////////////////

using PDedge = std::pair<int, int>;
using PDedges = std::vector<std::set<PDedge>>;

static int findPathDepth(const Tree& tree, int drvr_index);
static int findPathDepth(int node, int from, PDedges& edges, int length);
static int findLocationIndex(const Tree& tree, int x, int y);

// Used by regressions.
void reportSteinerTree(const stt::Tree& tree,
                       int drvr_x,
                       int drvr_y,
                       utl::Logger* logger)
{
  // flute mangles the x/y locations and pdrevII moves the driver to 0
  // so we have to find the driver location index.
  int drvr_index = findLocationIndex(tree, drvr_x, drvr_y);
  if (drvr_index >= 0) {
    logger->report("Wire length = {} Path depth = {}",
                   tree.length,
                   findPathDepth(tree, drvr_index));
    reportSteinerBranches(tree, logger);
  } else {
    logger->error(utl::STT, 7, "Invalid driver index {}.", drvr_index);
  }
}

void reportSteinerTree(const stt::Tree& tree, utl::Logger* logger)
{
  logger->report("Wire length = {}", tree.length);
  reportSteinerBranches(tree, logger);
}

static void reportSteinerBranches(const stt::Tree& tree, utl::Logger* logger)
{
  for (int i = 0; i < tree.branchCount(); i++) {
    int x1 = tree.branch[i].x;
    int y1 = tree.branch[i].y;
    int parent = tree.branch[i].n;
    int x2 = tree.branch[parent].x;
    int y2 = tree.branch[parent].y;
    int length = abs(x1 - x2) + abs(y1 - y2);
    logger->report(
        "{} ({} {}) neighbor {} length {}", i, x1, y1, parent, length);
  }
}

int findLocationIndex(const Tree& tree, int x, int y)
{
  for (int i = 0; i < tree.branchCount(); i++) {
    int x1 = tree.branch[i].x;
    int y1 = tree.branch[i].y;
    if (x1 == x && y1 == y) {
      return i;
    }
  }
  return -1;
}

static int findPathDepth(const Tree& tree, int drvr_index)
{
  int branch_count = tree.branchCount();
  PDedges edges(branch_count);
  if (branch_count > 1) {
    for (int i = 0; i < branch_count; i++) {
      const stt::Branch& branch = tree.branch[i];
      int neighbor = branch.n;
      if (neighbor != i) {
        const Branch& neighbor_branch = tree.branch[neighbor];
        int length = std::abs(branch.x - neighbor_branch.x)
                     + std::abs(branch.y - neighbor_branch.y);
        edges[neighbor].insert(PDedge(i, length));
        edges[i].insert(PDedge(neighbor, length));
      }
    }
    return findPathDepth(drvr_index, drvr_index, edges, 0);
  }
  return 0;
}

static int findPathDepth(int node, int from, PDedges& edges, int length)
{
  int max_length = length;
  for (const PDedge& edge : edges[node]) {
    int neighbor = edge.first;
    int edge_length = edge.second;
    if (neighbor != from) {
      max_length = std::max(
          max_length,
          findPathDepth(neighbor, node, edges, length + edge_length));
    }
  }
  return max_length;
}

void Tree::printTree(utl::Logger* logger) const
{
  if (deg > 1) {
    for (int i = 0; i < deg; i++) {
      logger->report(" {:2d}:  x={:4g}  y={:4g}  e={}",
                     i,
                     (float) branch[i].x,
                     (float) branch[i].y,
                     branch[i].n);
    }
    for (int i = deg; i < branch.size(); i++) {
      logger->report("s{:2d}:  x={:4g}  y={:4g}  e={}",
                     i,
                     (float) branch[i].x,
                     (float) branch[i].y,
                     branch[i].n);
    }
  }
}

}  // namespace stt
