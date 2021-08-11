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

#include "stt/SteinerTreeBuilder.h"
#include "stt/flute.h"
#include "stt/pdrev.h"

#include <map>
#include <vector>

#include "ord/OpenRoad.hh"
#include "odb/db.h"

namespace stt{

SteinerTreeBuilder::SteinerTreeBuilder() :
  alpha_(0.3),
  min_fanout_alpha_({0, -1}),
  min_hpwl_alpha_({0, -1}),
  logger_(nullptr),
  db_(nullptr)
{
}

void SteinerTreeBuilder::init(odb::dbDatabase* db, Logger* logger)
{
  db_ = db;
  logger_ = logger;
}

Tree SteinerTreeBuilder::makeSteinerTree(std::vector<int>& x,
                                         std::vector<int>& y,
                                         int drvr_index)
{
  Tree tree = makeTree(x, y, drvr_index, alpha_);

  return tree;
}

Tree SteinerTreeBuilder::makeSteinerTree(odb::dbNet* net,
                                         std::vector<int>& x,
                                         std::vector<int>& y,
                                         int drvr_index)
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
    if (net->getTermCount()-1 >= min_fanout) {
      net_alpha = min_fanout_alpha_.second;
    }
  }

  Tree tree = makeTree(x, y, drvr_index, net_alpha);

  return tree;
}

Tree SteinerTreeBuilder::makeSteinerTree(const std::vector<int>& x,
                                         const std::vector<int>& y,
                                         const std::vector<int>& s,
                                         int accuracy)
{
  Tree tree = flt::flutes(x, y, s, accuracy);
  return tree;
}

void SteinerTreeBuilder::setAlpha(float alpha)
{
  alpha_ = alpha;
}

float SteinerTreeBuilder::getAlpha(const odb::dbNet* net) const
{
  float net_alpha = net_alpha_map_.find(net) != net_alpha_map_.end() ?
                    net_alpha_map_.at(net) : alpha_;
  return net_alpha;
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
  for (int i = 0; i < tree.branchCount(); ++i) {
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
      if (r1.intersects(r2) && b1.n != j && b2.n != i && b1.n != b2.n) {
        debugPrint(logger_, utl::STT, "check", 1,
                   "check failed ({}, {}) ({}, {}) [{}, {}] vs ({}, {}) ({}, {}) [{}, {}] degree={}",
                      r1.xMin(), r1.yMin(), r1.xMax(), r1.yMax(),
                      i, b1.n,
                      r2.xMin(), r2.yMin(), r2.xMax(), r2.yMax(),
                      j, b2.n, tree.deg);
        return false;
      }
    }
  }

  return true;
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

Tree SteinerTreeBuilder::makeTree(std::vector<int>& x,
                                  std::vector<int>& y,
                                  int drvr_index,
                                  float alpha)
{
  Tree tree;

  if (alpha > 0.0) {
    tree = pdr::primDijkstra(x, y, drvr_index, alpha, logger_);

    if (checkTree(tree)) {
      return tree;
    }

    // Try a smaller alpha if possible
    if (alpha > 0.1) {
      tree = pdr::primDijkstra(x, y, drvr_index, alpha - 0.1, logger_);
      if (checkTree(tree)) {
        return tree;
      }
    }

    // Give up and use flute
  } 

  return flt::flute(x, y, flute_accuracy);
}

int SteinerTreeBuilder::computeHPWL(odb::dbNet* net)
{
  int min_x = std::numeric_limits<int>::max();
  int min_y = std::numeric_limits<int>::max();
  int max_x = std::numeric_limits<int>::min();
  int max_y = std::numeric_limits<int>::min();

  for (odb::dbITerm* iterm : net->getITerms()) {
    odb::dbPlacementStatus status = iterm->getInst()->getPlacementStatus();
    if (status != odb::dbPlacementStatus::NONE &&
        status != odb::dbPlacementStatus::UNPLACED) {
      int x, y;
      iterm->getAvgXY(&x, &y);
      min_x = std::min(min_x, x);
      max_x = std::max(max_x, x);
      min_y = std::min(min_y, y);
      max_y = std::max(max_y, y);
    } else {
      logger_->error(utl::STT, 4, "Net {} is connected to unplaced instance {}.",
                     net->getName(),
                     iterm->getInst()->getName());
    }
  }

  for (odb::dbBTerm* bterm : net->getBTerms()) {
    if (bterm->getFirstPinPlacementStatus() != odb::dbPlacementStatus::NONE ||
        bterm->getFirstPinPlacementStatus() != odb::dbPlacementStatus::UNPLACED) {
      int x, y;
      bterm->getFirstPinLocation(x, y);
      min_x = std::min(min_x, x);
      max_x = std::max(max_x, x);
      min_y = std::min(min_y, y);
      max_y = std::max(max_y, y);
    } else {
      logger_->error(utl::STT, 5, "Net {} is connected to unplaced pin {}.",
                     net->getName(),
                     bterm->getName());
    }
  }

  int hpwl = (max_x - min_x) + (max_y - min_y);

  return hpwl;
}

void Tree::printTree(utl::Logger* logger)
{
  if (deg > 1) {
    for (int i = 0; i < deg; i++)
      logger->report(" {:2d}:  x={:4g}  y={:4g}  e={}",
                     i, (float)branch[i].x, (float)branch[i].y, branch[i].n);
    for (int i = deg; i < 2 * deg - 2; i++)
      logger->report("s{:2d}:  x={:4g}  y={:4g}  e={}",
                     i, (float)branch[i].x, (float)branch[i].y, branch[i].n);
    logger->report("");
  }
}

}
