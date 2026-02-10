// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "detailed_vertical.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

#include "boost/token_functions.hpp"
#include "boost/tokenizer.hpp"
#include "detailed_manager.h"
#include "detailed_orient.h"
#include "dpl/Opendp.h"
#include "infrastructure/Objects.h"
#include "infrastructure/detailed_segment.h"
#include "objective/detailed_hpwl.h"
#include "optimization/detailed_generator.h"
#include "util/utility.h"
#include "utl/Logger.h"

using utl::DPL;

namespace dpl {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedVerticalSwap::DetailedVerticalSwap(Architecture* arch, Network* network)
    : DetailedGenerator("vertical swap"),
      mgr_(nullptr),
      arch_(arch),
      network_(network),
      skipNetsLargerThanThis_(100),
      traversal_(0),
      attempts_(0),
      moves_(0),
      swaps_(0)
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedVerticalSwap::DetailedVerticalSwap()
    : DetailedVerticalSwap(nullptr, nullptr)
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedVerticalSwap::run(DetailedMgr* mgrPtr, const std::string& command)
{
  // A temporary interface to allow for a string which we will decode to create
  // the arguments.
  boost::char_separator<char> separators(" \r\t\n;");
  boost::tokenizer<boost::char_separator<char>> tokens(command, separators);
  std::vector<std::string> args;
  for (const auto& token : tokens) {
    args.push_back(token);
  }
  run(mgrPtr, args);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedVerticalSwap::run(DetailedMgr* mgrPtr,
                               const std::vector<std::string>& args)
{
  // Given the arguments, figure out which routine to run to do the reordering.

  mgr_ = mgrPtr;
  arch_ = mgr_->getArchitecture();
  network_ = mgr_->getNetwork();

  int passes = 1;
  double tol = 0.01;
  for (size_t i = 1; i < args.size(); i++) {
    if (args[i] == "-p" && i + 1 < args.size()) {
      passes = std::atoi(args[++i].c_str());
    } else if (args[i] == "-t" && i + 1 < args.size()) {
      tol = std::atof(args[++i].c_str());
    }
  }
  passes = std::max(passes, 1);
  tol = std::max(tol, 0.01);

  uint64_t hpwl_x, hpwl_y;
  int64_t curr_hpwl = Utility::hpwl(network_, hpwl_x, hpwl_y);
  const int64_t init_hpwl = curr_hpwl;
  if (init_hpwl == 0) {
    return;
  }
  for (int p = 1; p <= passes; p++) {
    const int64_t last_hpwl = curr_hpwl;

    // XXX: Actually, vertical swapping is nothing more than random
    // greedy improvement in which the move generating is done
    // using this object to generate a target which is the optimal
    // region for each candidate cell.
    verticalSwap();

    curr_hpwl = Utility::hpwl(network_, hpwl_x, hpwl_y);

    mgr_->getLogger()->info(DPL,
                            308,
                            "Pass {:3d} of vertical swaps; hpwl is {:.6e}.",
                            p,
                            (double) curr_hpwl);

    if (last_hpwl == 0
        || std::abs(curr_hpwl - last_hpwl) / (double) last_hpwl <= tol) {
      // std::cout << "Terminating due to low improvement." << std::endl;
      break;
    }
  }
  const double curr_imp
      = (((init_hpwl - curr_hpwl) / (double) init_hpwl) * 100.);
  mgr_->getLogger()->info(DPL,
                          309,
                          "End of vertical swaps; objective is {:.6e}, "
                          "improvement is {:.2f} percent.",
                          (double) curr_hpwl,
                          curr_imp);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void DetailedVerticalSwap::verticalSwap()
{
  // Nothing for than random greedy improvement with only a hpwl objective
  // and done such that every candidate cell is considered once!!!

  traversal_ = 0;
  edgeMask_.resize(network_->getNumEdges());
  std::ranges::fill(edgeMask_, 0);

  mgr_->resortSegments();

  // Get candidate cells.
  std::vector<Node*> candidates = mgr_->getSingleHeightCells();
  mgr_->shuffle(candidates);

  // Wirelength objective.
  DetailedHPWL hpwlObj(network_);
  hpwlObj.init(mgr_, nullptr);  // Ignore orientation.

  double currHpwl = hpwlObj.curr();
  // Consider each candidate cell once.
  for (Node* ndi : candidates) {
    if (generate(ndi) == false) {
      continue;
    }

    const double delta = hpwlObj.delta(mgr_->getJournal());

    const double nextHpwl = currHpwl - delta;  // -delta is +ve is less.

    if (nextHpwl <= currHpwl) {
      hpwlObj.accept();
      mgr_->acceptMove();
      currHpwl = nextHpwl;
    } else {
      mgr_->rejectMove();
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
bool DetailedVerticalSwap::getRange(Node* nd, odb::Rect& nodeBbox)
{
  // Determines the median location for a node.

  unsigned t = 0;

  const DbuX xmin = arch_->getMinX();
  const DbuX xmax = arch_->getMaxX();
  const DbuY ymin = arch_->getMinY();
  const DbuY ymax = arch_->getMaxY();

  xpts_.clear();
  ypts_.clear();
  for (unsigned n = 0; n < nd->getPins().size(); n++) {
    const Pin* pin = nd->getPins()[n];

    const Edge* ed = pin->getEdge();

    nodeBbox.mergeInit();

    const int numPins = ed->getNumPins();
    if (numPins <= 1) {
      continue;
    }
    if (numPins > skipNetsLargerThanThis_) {
      continue;
    }
    if (!calculateEdgeBB(ed, nd, nodeBbox)) {
      continue;
    }

    // We've computed an interval for the pin.  We need to alter it to work for
    // the cell center. Also, we need to avoid going off the edge of the chip.
    nodeBbox.set_xlo(std::min(
        std::max(xmin.v, nodeBbox.xMin() - pin->getOffsetX().v), xmax.v));
    nodeBbox.set_xhi(std::max(
        std::min(xmax.v, nodeBbox.xMax() - pin->getOffsetX().v), xmin.v));
    nodeBbox.set_ylo(std::min(
        std::max(ymin.v, nodeBbox.yMin() - pin->getOffsetY().v), ymax.v));
    nodeBbox.set_yhi(std::max(
        std::min(ymax.v, nodeBbox.yMax() - pin->getOffsetY().v), ymin.v));

    // Record the location and pin offset used to generate this point.

    xpts_.push_back(nodeBbox.xMin());
    xpts_.push_back(nodeBbox.xMax());

    ypts_.push_back(nodeBbox.yMin());
    ypts_.push_back(nodeBbox.yMax());

    ++t;
    ++t;
  }

  // If, for some weird reason, we didn't find anything connected, then
  // return false to indicate that there's nowhere to move the cell.
  if (t <= 1) {
    return false;
  }

  // Get the median values.
  const unsigned mid = t / 2;

  std::ranges::sort(xpts_);
  std::ranges::sort(ypts_);

  nodeBbox.set_xlo(xpts_[mid - 1]);
  nodeBbox.set_xhi(xpts_[mid]);

  nodeBbox.set_ylo(ypts_[mid - 1]);
  nodeBbox.set_yhi(ypts_[mid]);

  return true;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
bool DetailedVerticalSwap::calculateEdgeBB(const Edge* ed,
                                           const Node* nd,
                                           odb::Rect& bbox)
{
  // Computes the bounding box of an edge.  Node 'nd' is the node to SKIP.
  bbox.mergeInit();

  int count = 0;
  for (const Pin* pin : ed->getPins()) {
    const Node* other = pin->getNode();
    if (other == nd) {
      continue;
    }
    const DbuX curX = other->getCenterX() + pin->getOffsetX();
    const DbuY curY = other->getCenterY() + pin->getOffsetY();

    bbox.set_xlo(std::min(curX.v, bbox.xMin()));
    bbox.set_xhi(std::max(curX.v, bbox.xMax()));
    bbox.set_ylo(std::min(curY.v, bbox.yMin()));
    bbox.set_yhi(std::max(curY.v, bbox.yMax()));

    ++count;
  }

  return count != 0;
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
bool DetailedVerticalSwap::generate(Node* ndi)
{
  // More or less the same as a global swap, but only attempts to look
  // up or down by a few rows from the current row in the direction
  // of the optimal box.

  // Center of cell.
  const double yi = ndi->getBottom().v + 0.5 * ndi->getHeight().v;
  const double xi = ndi->getLeft().v + 0.5 * ndi->getWidth().v;

  // Determine optimal region.
  odb::Rect bbox;
  if (!getRange(ndi, bbox)) {
    return false;
  }
  // If cell inside box, do nothing.
  if (xi >= bbox.xMin() && xi <= bbox.xMax() && yi >= bbox.yMin()
      && yi <= bbox.yMax()) {
    return false;
  }

  // Only single height cell.
  if (mgr_->getNumReverseCellToSegs(ndi->getId()) != 1) {
    return false;
  }

  // Segment and row for bottom of cell.
  const int si = mgr_->getReverseCellToSegs(ndi->getId())[0]->getSegId();
  const int ri = mgr_->getReverseCellToSegs(ndi->getId())[0]->getRowId();

  // Center of optimal rectangle.
  const DbuX xj{(int) std::floor(0.5 * (bbox.xMin() + bbox.xMax())
                                 - 0.5 * ndi->getWidth().v)};
  DbuY yj{(int) std::floor(0.5 * (bbox.yMin() + bbox.yMax())
                           - 0.5 * ndi->getHeight().v)};

  // Up or down a few rows depending on whether or not the
  // center of the optimal rectangle is above or below the
  // current position.
  int rj = -1;
  if (yj > yi) {
    const int rmin = std::min(arch_->getNumRows() - 1, ri + 1);
    const int rmax = std::min(arch_->getNumRows() - 1, ri + 2);
    rj = rmin + mgr_->getRandom(rmax - rmin + 1);
  } else {
    const int rmax = std::max(0, ri - 1);
    const int rmin = std::max(0, ri - 2);
    rj = rmin + mgr_->getRandom(rmax - rmin + 1);
  }
  if (rj == -1) {
    return false;
  }
  yj = DbuY{arch_->getRow(rj)->getBottom()};  // Row alignment.
  int sj = -1;
  for (int s = 0; s < mgr_->getNumSegsInRow(rj); s++) {
    const DetailedSeg* segPtr = mgr_->getSegsInRow(rj)[s];
    if (xj >= segPtr->getMinX() && xj <= segPtr->getMaxX()) {
      sj = segPtr->getSegId();
      break;
    }
  }
  if (sj == -1) {
    return false;
  }
  if (ndi->getGroupId() != mgr_->getSegment(sj)->getRegId()) {
    return false;
  }

  if (mgr_->tryMove(ndi, ndi->getLeft(), ndi->getBottom(), si, xj, yj, sj)) {
    ++moves_;
    return true;
  }
  if (mgr_->trySwap(ndi, ndi->getLeft(), ndi->getBottom(), si, xj, yj, sj)) {
    ++moves_;
    return true;
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedVerticalSwap::init(DetailedMgr* mgr)
{
  mgr_ = mgr;
  arch_ = mgr->getArchitecture();
  network_ = mgr->getNetwork();

  traversal_ = 0;
  edgeMask_.resize(network_->getNumEdges());
  std::ranges::fill(edgeMask_, 0);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedVerticalSwap::generate(DetailedMgr* mgr,
                                    std::vector<Node*>& candidates)
{
  ++attempts_;

  mgr_ = mgr;
  arch_ = mgr->getArchitecture();
  network_ = mgr->getNetwork();

  Node* ndi = candidates[mgr_->getRandom(candidates.size())];

  return generate(ndi);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedVerticalSwap::stats()
{
  mgr_->getLogger()->info(
      DPL,
      336,
      "Generator {:s}, "
      "Cumulative attempts {:d}, swaps {:d}, moves {:5d} since last reset.",
      getName().c_str(),
      attempts_,
      swaps_,
      moves_);
}

}  // namespace dpl
