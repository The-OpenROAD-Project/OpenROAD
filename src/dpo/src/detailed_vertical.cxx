///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, Andrew Kennings
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

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#include "detailed_vertical.h"

#include <boost/tokenizer.hpp>

#include "detailed_hpwl.h"
#include "detailed_manager.h"
#include "detailed_orient.h"
#include "detailed_segment.h"
#include "rectangle.h"
#include "utility.h"
#include "utl/Logger.h"

using utl::DPO;

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedVerticalSwap::DetailedVerticalSwap(Architecture* arch,
                                           Network* network,
                                           RoutingParams* rt)
    : DetailedGenerator("vertical swap"),
      mgr_(nullptr),
      arch_(arch),
      network_(network),
      rt_(rt),
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
    : DetailedVerticalSwap(nullptr, nullptr, nullptr)
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
  rt_ = mgr_->getRoutingParams();

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

  double hpwl_x, hpwl_y;
  double curr_hpwl = Utility::hpwl(network_, hpwl_x, hpwl_y);
  const double init_hpwl = curr_hpwl;
  for (int p = 1; p <= passes; p++) {
    const double last_hpwl = curr_hpwl;

    // XXX: Actually, vertical swapping is nothing more than random
    // greedy improvement in which the move generating is done
    // using this object to generate a target which is the optimal
    // region for each candidate cell.
    verticalSwap();

    curr_hpwl = Utility::hpwl(network_, hpwl_x, hpwl_y);

    mgr_->getLogger()->info(DPO,
                            308,
                            "Pass {:3d} of vertical swaps; hpwl is {:.6e}.",
                            p,
                            curr_hpwl);

    if (std::fabs(curr_hpwl - last_hpwl) / last_hpwl <= tol) {
      // std::cout << "Terminating due to low improvement." << std::endl;
      break;
    }
  }
  const double curr_imp = (((init_hpwl - curr_hpwl) / init_hpwl) * 100.);
  mgr_->getLogger()->info(DPO,
                          309,
                          "End of vertical swaps; objective is {:.6e}, "
                          "improvement is {:.2f} percent.",
                          curr_hpwl,
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
  std::fill(edgeMask_.begin(), edgeMask_.end(), 0);

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

    const double delta = hpwlObj.delta(mgr_->getNMoved(),
                                       mgr_->getMovedNodes(),
                                       mgr_->getCurLeft(),
                                       mgr_->getCurBottom(),
                                       mgr_->getCurOri(),
                                       mgr_->getNewLeft(),
                                       mgr_->getNewBottom(),
                                       mgr_->getNewOri());

    const double nextHpwl = currHpwl - delta;  // -delta is +ve is less.

    if (nextHpwl <= currHpwl) {
      mgr_->acceptMove();

      currHpwl = nextHpwl;
    } else {
      mgr_->rejectMove();
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
bool DetailedVerticalSwap::getRange(Node* nd, Rectangle& nodeBbox)
{
  // Determines the median location for a node.

  unsigned t = 0;

  const double xmin = arch_->getMinX();
  const double xmax = arch_->getMaxX();
  const double ymin = arch_->getMinY();
  const double ymax = arch_->getMaxY();

  xpts_.clear();
  ypts_.clear();
  for (unsigned n = 0; n < nd->getPins().size(); n++) {
    const Pin* pin = nd->getPins()[n];

    const Edge* ed = pin->getEdge();

    nodeBbox.reset();

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
    nodeBbox.set_xmin(
        std::min(std::max(xmin, nodeBbox.xmin() - pin->getOffsetX()), xmax));
    nodeBbox.set_xmax(
        std::max(std::min(xmax, nodeBbox.xmax() - pin->getOffsetX()), xmin));
    nodeBbox.set_ymin(
        std::min(std::max(ymin, nodeBbox.ymin() - pin->getOffsetY()), ymax));
    nodeBbox.set_ymax(
        std::max(std::min(ymax, nodeBbox.ymax() - pin->getOffsetY()), ymin));

    // Record the location and pin offset used to generate this point.

    xpts_.push_back(nodeBbox.xmin());
    xpts_.push_back(nodeBbox.xmax());

    ypts_.push_back(nodeBbox.ymin());
    ypts_.push_back(nodeBbox.ymax());

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

  std::sort(xpts_.begin(), xpts_.end());
  std::sort(ypts_.begin(), ypts_.end());

  nodeBbox.set_xmin(xpts_[mid - 1]);
  nodeBbox.set_xmax(xpts_[mid]);

  nodeBbox.set_ymin(ypts_[mid - 1]);
  nodeBbox.set_ymax(ypts_[mid]);

  return true;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
bool DetailedVerticalSwap::calculateEdgeBB(const Edge* ed,
                                           const Node* nd,
                                           Rectangle& bbox)
{
  // Computes the bounding box of an edge.  Node 'nd' is the node to SKIP.
  bbox.reset();

  int count = 0;
  for (const Pin* pin : ed->getPins()) {
    const Node* other = pin->getNode();
    if (other == nd) {
      continue;
    }
    const double curX
        = other->getLeft() + 0.5 * other->getWidth() + pin->getOffsetX();
    const double curY
        = other->getBottom() + 0.5 * other->getHeight() + pin->getOffsetY();

    bbox.set_xmin(std::min(curX, bbox.xmin()));
    bbox.set_xmax(std::max(curX, bbox.xmax()));
    bbox.set_ymin(std::min(curY, bbox.ymin()));
    bbox.set_ymax(std::max(curY, bbox.ymax()));

    ++count;
  }

  return count != 0;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
double DetailedVerticalSwap::delta(Node* ndi, double new_x, double new_y)
{
  // Compute change in wire length for moving node to new position.

  double old_wl = 0.;
  double new_wl = 0.;
  Rectangle old_box, new_box;

  ++traversal_;
  for (int pi = 0; pi < ndi->getPins().size(); pi++) {
    const Pin* pini = ndi->getPins()[pi];

    const Edge* edi = pini->getEdge();

    const int npins = edi->getNumPins();
    if (npins <= 1 || npins >= skipNetsLargerThanThis_) {
      continue;
    }
    if (edgeMask_[edi->getId()] == traversal_) {
      continue;
    }
    edgeMask_[edi->getId()] = traversal_;

    old_box.reset();
    new_box.reset();
    for (const Pin* pinj : edi->getPins()) {
      const Node* ndj = pinj->getNode();

      double x = ndj->getLeft() + 0.5 * ndj->getWidth() + pinj->getOffsetX();
      double y = ndj->getBottom() + 0.5 * ndj->getHeight() + pinj->getOffsetY();

      old_box.addPt(x, y);

      if (ndj == ndi) {
        x = new_x + pinj->getOffsetX();
        y = new_y + pinj->getOffsetY();
      }

      new_box.addPt(x, y);
    }
    old_wl += old_box.getWidth() + old_box.getHeight();
    new_wl += new_box.getWidth() + new_box.getHeight();
  }
  return old_wl - new_wl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedVerticalSwap::delta(Node* ndi, Node* ndj)
{
  // Compute change in wire length for swapping the two nodes.

  double old_wl = 0.;
  double new_wl = 0.;
  Rectangle old_box, new_box;
  const Node* nodes[2];
  nodes[0] = ndi;
  nodes[1] = ndj;

  ++traversal_;
  for (int c = 0; c <= 1; c++) {
    const Node* ndi = nodes[c];
    for (const Pin* pini : ndi->getPins()) {
      const Edge* edi = pini->getEdge();

      const int npins = edi->getNumPins();
      if (npins <= 1 || npins >= skipNetsLargerThanThis_) {
        continue;
      }
      if (edgeMask_[edi->getId()] == traversal_) {
        continue;
      }
      edgeMask_[edi->getId()] = traversal_;

      old_box.reset();
      new_box.reset();

      for (const Pin* pinj : edi->getPins()) {
        const Node* ndj = pinj->getNode();

        double x = ndj->getLeft() + 0.5 * ndj->getWidth() + pinj->getOffsetX();
        double y
            = ndj->getBottom() + 0.5 * ndj->getHeight() + pinj->getOffsetY();

        old_box.addPt(x, y);

        if (ndj == nodes[0]) {
          ndj = nodes[1];
        } else if (ndj == nodes[1]) {
          ndj = nodes[0];
        }

        x = ndj->getLeft() + 0.5 * ndj->getWidth() + pinj->getOffsetX();
        y = ndj->getBottom() + 0.5 * ndj->getHeight() + pinj->getOffsetY();

        new_box.addPt(x, y);
      }

      old_wl += old_box.getWidth() + old_box.getHeight();
      new_wl += new_box.getWidth() + new_box.getHeight();
    }
  }
  return old_wl - new_wl;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
bool DetailedVerticalSwap::generate(Node* ndi)
{
  // More or less the same as a global swap, but only attempts to look
  // up or down by a few rows from the current row in the direction
  // of the optimal box.

  // Center of cell.
  const double yi = ndi->getBottom() + 0.5 * ndi->getHeight();
  const double xi = ndi->getLeft() + 0.5 * ndi->getWidth();

  // Determine optimal region.
  Rectangle bbox;
  if (!getRange(ndi, bbox)) {
    return false;
  }
  // If cell inside box, do nothing.
  if (xi >= bbox.xmin() && xi <= bbox.xmax() && yi >= bbox.ymin()
      && yi <= bbox.ymax()) {
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
  const int xj = (int) std::floor(0.5 * (bbox.xmin() + bbox.xmax())
                                  - 0.5 * ndi->getWidth());
  int yj = (int) std::floor(0.5 * (bbox.ymin() + bbox.ymax())
                            - 0.5 * ndi->getHeight());

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
  yj = arch_->getRow(rj)->getBottom();  // Row alignment.
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
  if (ndi->getRegionId() != mgr_->getSegment(sj)->getRegId()) {
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
  rt_ = mgr->getRoutingParams();

  traversal_ = 0;
  edgeMask_.resize(network_->getNumEdges());
  std::fill(edgeMask_.begin(), edgeMask_.end(), 0);
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
  rt_ = mgr->getRoutingParams();

  Node* ndi = candidates[mgr_->getRandom(candidates.size())];

  return generate(ndi);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedVerticalSwap::stats()
{
  mgr_->getLogger()->info(
      DPO,
      336,
      "Generator {:s}, "
      "Cumulative attempts {:d}, swaps {:d}, moves {:5d} since last reset.",
      getName().c_str(),
      attempts_,
      swaps_,
      moves_);
}

}  // namespace dpo
