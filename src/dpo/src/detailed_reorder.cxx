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

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "detailed_reorder.h"
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <cmath>
#include <iostream>
#include <stack>
#include <utility>
#include "utl/Logger.h"
#include "detailed_manager.h"
#include "detailed_segment.h"
#include "plotgnu.h"
#include "utility.h"

using utl::DPO;

namespace dpo {

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
DetailedReorderer::DetailedReorderer(Architecture* arch, Network* network,
                                     RoutingParams* rt)
    : m_arch(arch),
      m_network(network),
      m_rt(rt),
      m_mgrPtr(nullptr),
      m_skipNetsLargerThanThis(100),
      m_traversal(0),
      m_windowSize(3) {}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
DetailedReorderer::~DetailedReorderer() {}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void DetailedReorderer::run(DetailedMgr* mgrPtr, std::string command) {
  // A temporary interface to allow for a string which we will decode to create
  // the arguments.
  std::string scriptString = command;
  boost::char_separator<char> separators(" \r\t\n;");
  boost::tokenizer<boost::char_separator<char> > tokens(scriptString,
                                                        separators);
  std::vector<std::string> args;
  for (boost::tokenizer<boost::char_separator<char> >::iterator it =
           tokens.begin();
       it != tokens.end(); it++) {
    args.push_back(*it);
  }
  run(mgrPtr, args);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void DetailedReorderer::run(DetailedMgr* mgrPtr,
                            std::vector<std::string>& args) {
  // Given the arguments, figure out which routine to run to do the reordering.

  m_mgrPtr = mgrPtr;

  m_windowSize = 3;

  int passes = 1;
  double tol = 0.01;
  for (size_t i = 1; i < args.size(); i++) {
    if (args[i] == "-w" && i + 1 < args.size()) {
      m_windowSize = std::atoi(args[++i].c_str());
    } else if (args[i] == "-p" && i + 1 < args.size()) {
      passes = std::atoi(args[++i].c_str());
    } else if (args[i] == "-t" && i + 1 < args.size()) {
      tol = std::atof(args[++i].c_str());
    }
  }
  m_windowSize = std::min(4, std::max(2, m_windowSize));
  tol = std::max(tol, 0.01);

  double last_hpwl, curr_hpwl, init_hpwl, hpwl_x, hpwl_y;

  m_mgrPtr->resortSegments();
  curr_hpwl = Utility::hpwl(m_network, hpwl_x, hpwl_y);
  init_hpwl = curr_hpwl;
  for (int p = 1; p <= passes; p++) {
    last_hpwl = curr_hpwl;

    reorder();

    curr_hpwl = Utility::hpwl(m_network, hpwl_x, hpwl_y);

    m_mgrPtr->getLogger()->info(
        DPO, 304, "Pass {:3d} of reordering; objective is {:.6e}.", p,
        curr_hpwl);

    if (std::fabs(curr_hpwl - last_hpwl) / last_hpwl <= tol) {
      // std::cout << "Terminating due to low improvement." << std::endl;
      break;
    }
  }
  m_mgrPtr->resortSegments();

  double curr_imp = (((init_hpwl - curr_hpwl) / init_hpwl) * 100.);
  m_mgrPtr->getLogger()->info(
      DPO, 305,
      "End of reordering; objective is {:.6e}, improvement is {:.2f} percent.",
      curr_hpwl, curr_imp);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void DetailedReorderer::reorder() {
  m_traversal = 0;
  m_edgeMask.resize(m_network->getNumEdges());
  std::fill(m_edgeMask.begin(), m_edgeMask.end(), m_traversal);

  int rightLimit = 0.;
  int leftLimit = 0.;
  int rightPadding = 0.;
  int leftPadding = 0.;

  // Loop over each segment; find single height cells and reorder.
  for (int s = 0; s < m_mgrPtr->getNumSegments(); s++) {
    DetailedSeg* segPtr = m_mgrPtr->getSegment(s);
    int segId = segPtr->getSegId();
    int rowId = segPtr->getRowId();

    std::vector<Node*>& nodes = m_mgrPtr->m_cellsInSeg[segId];
    if (nodes.size() < 2) {
      continue;
    }
    std::sort(nodes.begin(), nodes.end(), DetailedMgr::compareNodesX());

    int j = 0;
    int n = (int)nodes.size();
    while (j < n) {
      while (j < n && m_arch->isMultiHeightCell(nodes[j])) {
        ++j;
      }
      int jstrt = j;
      while (j < n && m_arch->isSingleHeightCell(nodes[j])) {
        ++j;
      }
      int jstop = j - 1;

      // Single height cells in [jstrt,jstop].
      for (int i = jstrt; i + m_windowSize <= jstop; ++i) {
        int istrt = i;
        int istop = std::min(jstop, istrt + m_windowSize - 1);
        if (istop == jstop) {
          istrt = std::max(jstrt, istop - m_windowSize + 1);
        }

        Node* nextPtr = (istop != n - 1) ? nodes[istop + 1] : 0;
        rightLimit = segPtr->getMaxX();
        if (nextPtr != 0) {
          m_arch->getCellPadding(nextPtr, leftPadding, rightPadding);
          rightLimit = std::min(
              (int)std::floor(nextPtr->getLeft() - leftPadding),
              rightLimit);
        }
        Node* prevPtr = (istrt != 0) ? nodes[istrt - 1] : 0;
        leftLimit = segPtr->getMinX();
        if (prevPtr != 0) {
          m_arch->getCellPadding(prevPtr, leftPadding, rightPadding);
          leftLimit = std::max(
              (int)std::ceil(prevPtr->getRight() + rightPadding),
              leftLimit);
        }

        reorder(nodes, istrt, istop, leftLimit, rightLimit, segId, rowId);
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void DetailedReorderer::reorder(std::vector<Node*>& nodes, int jstrt, int jstop,
                                int leftLimit, int rightLimit, int segId,
                                int rowId) {
  int size = jstop - jstrt + 1;

  // XXX: Node positions still doubles!
  std::unordered_map<Node*, int> origLeft;
  for (int i = 0; i < size; i++) {
    Node* ndi = nodes[jstrt+i];
    origLeft[ndi] = ndi->getLeft();
  }

  // Changed...  I want to work entirely with the left edge of
  // the cells.  If there is not enough space to satisfy 
  // the cell widths _and_ the padding, then don't do anything.
  int totalPadding = 0;
  int totalWidth = 0;
  std::vector<int> right(size, 0);
  std::vector<int> left(size, 0);
  std::vector<int> width(size, 0);
  for (int i = 0; i < size; i++) {
    Node* ndi = nodes[jstrt+i];
    m_arch->getCellPadding(ndi, left[i], right[i]);
    width[i] = (int)std::ceil(ndi->getWidth());
    totalPadding += (left[i]+right[i]);
    totalWidth += width[i];
  }
  if (rightLimit-leftLimit < totalWidth+totalPadding) {
    // We do not have enough space, so abort.
    return;
  }

  // We might have more space than required.  Space cells out
  // somewhat evenly by adding extra space to the padding.
  int spacePerCell = ((rightLimit-leftLimit)-(totalWidth+totalPadding)) / size;
  int siteWidth = m_arch->getRow(0)->getSiteWidth();
  int sitePerCellTotal = spacePerCell/siteWidth;
  int sitePerCellRight = (sitePerCellTotal>>1);
  int sitePerCellLeft = sitePerCellTotal-sitePerCellRight;
  for (int i = 0; i < size; i++) {
    if (totalWidth+totalPadding+sitePerCellRight*siteWidth < rightLimit-leftLimit) {
      totalPadding += sitePerCellRight*siteWidth;
      right[i] += sitePerCellRight*siteWidth;
    }
    if (totalWidth+totalPadding+sitePerCellLeft*siteWidth < rightLimit-leftLimit) {
      totalPadding += sitePerCellLeft*siteWidth;
      left[i] += sitePerCellLeft*siteWidth;
    }
  }
  if (rightLimit-leftLimit < totalWidth+totalPadding) {
    // We do not have enough space, so abort.
    return;
  }

  // Generate the different permutations.  Evaluate each one and keep
  // the best one.  
  // 
  // NOTE: The first permutation, which is the original placement, 
  // might not generate the original placement since the spacing 
  // might be different.  So, just consider the first permutation
  // like all the others.

  double bestCost = cost(nodes, jstrt, jstop);
  double origCost = bestCost;

  std::vector<int> bestPosn(size, 0); // Current positions.
  std::vector<int> currPosn(size, 0); // Current positions.
  std::vector<int> order(size, 0); // For generating permutations.
  for (int i = 0; i < size; i++) {
    order[i] = i;
  }
  bool found = false;
  do {
    // Position the cells.
    bool dispOkay = true;
    int x = leftLimit;
    for (int i = 0; i < size; i++) {
      int ix = order[i];
      Node* ndi = nodes[jstrt+ix];
      x += left[ix];
      currPosn[ix] = x;
      ndi->setLeft(currPosn[ix]);
      x += width[ix];
      x += right[ix];

      double dx = std::fabs(ndi->getLeft()-ndi->getOrigLeft());
      if ((int)std::ceil(dx) > m_mgrPtr->getMaxDisplacementX()) {
        dispOkay = false;
      }
    }
    if (dispOkay) {
      double currCost = cost(nodes, jstrt, jstop);
      if (currCost < bestCost) {
        bestPosn = currPosn;
        bestCost = currCost;

        found = true;
      }
    }
  } while (std::next_permutation(order.begin(), order.end()));

  if (!found) {
    // No improvement.  Restore positions and return.
    for (size_t i = 0; i < size; i++) {
      Node* ndi = nodes[jstrt+i];
      ndi->setLeft(origLeft[ndi]);
    }
    return;
  }

  // Put cells at their best positions.
  for (int i = 0; i < size; i++) {
    Node* ndi = nodes[jstrt+i];
    ndi->setLeft(bestPosn[i]);
  }

  // Need to resort.
  std::stable_sort(nodes.begin() + jstrt, nodes.begin() + jstop + 1,
                   DetailedMgr::compareNodesX());

  // Check that cells are site aligned and fix if needed.
  {
    bool shifted = false;
    bool failed = false;
    int left = leftLimit;
    for (int i = 0; i < size; i++) {
      Node* ndi = nodes[jstrt + i];

      int x = ndi->getLeft();
      if (!m_mgrPtr->alignPos(ndi, x, left, rightLimit)) {
        failed = true;
        break;
      }
      if (std::abs(x - ndi->getLeft()) != 0) {
        shifted = true;
      }
      ndi->setLeft(x);
      left = ndi->getRight();

      double dx = std::fabs(ndi->getLeft()-ndi->getOrigLeft());
      if ((int)std::ceil(dx) > m_mgrPtr->getMaxDisplacementX()) {
        failed = true;
        break;
      }
    }
    if (!failed) {
      // This implies everything got site aligned within the specified
      // interval.  However, we might have shifted something.
      if (shifted) {
        // Recost.  The shifting might have changed the cost.
        double lastCost = cost(nodes, jstrt, jstop);
        if (lastCost >= origCost) {
          failed = true;
        }
      }
    }

    if (failed) {
      // Restore original placement.
      for (int i = 0; i < size; i++) {
        Node* ndi = nodes[jstrt + i];
        ndi->setLeft(origLeft[ndi]);
      }
      std::stable_sort(nodes.begin() + jstrt, nodes.begin() + jstop + 1,
                       DetailedMgr::compareNodesX());
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedReorderer::cost(std::vector<Node*>& nodes, int istrt,
                               int istop) {
  // Compute hpwl for the specified sequence of cells.

  ++m_traversal;

  double cost = 0.;
  for (int i = istrt; i <= istop; i++) {
    Node* ndi = nodes[i];

    for (int pi = 0; pi < ndi->getNumPins(); pi++) {
      Pin* pini = ndi->getPins()[pi];

      Edge* edi = pini->getEdge();

      int npins = edi->getNumPins();
      if (npins <= 1 || npins >= m_skipNetsLargerThanThis) {
        continue;
      }
      if (m_edgeMask[edi->getId()] == m_traversal) {
        continue;
      }
      m_edgeMask[edi->getId()] = m_traversal;

      double xmin = std::numeric_limits<double>::max();
      double xmax = -std::numeric_limits<double>::max();
      for (int pj = 0; pj < edi->getNumPins(); pj++) {
        Pin* pinj = edi->getPins()[pj];

        Node* ndj = pinj->getNode();

        double x = ndj->getLeft() + 0.5*ndj->getWidth() + pinj->getOffsetX();

        xmin = std::min(xmin, x);
        xmax = std::max(xmax, x);
      }
      cost += xmax - xmin;
    }
  }
  return cost;
}

}  // namespace dpo
