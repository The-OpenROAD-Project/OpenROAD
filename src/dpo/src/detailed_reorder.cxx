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
// Includes.
////////////////////////////////////////////////////////////////////////////////
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

////////////////////////////////////////////////////////////////////////////////
// Defines.
////////////////////////////////////////////////////////////////////////////////

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedReorderer::DetailedReorderer(Architecture* arch, Network* network,
                                     RoutingParams* rt)
    : m_arch(arch),
      m_network(network),
      m_rt(rt),
      m_mgrPtr(nullptr),
      m_skipNetsLargerThanThis(100),
      m_traversal(0),
      m_windowSize(3) {}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedReorderer::~DetailedReorderer() {}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
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

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
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

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedReorderer::reorder() {
  m_traversal = 0;
  m_edgeMask.resize(m_network->getNumEdges());
  std::fill(m_edgeMask.begin(), m_edgeMask.end(), m_traversal);

  double rightLimit = 0.;
  double leftLimit = 0.;
  double rightPadding = 0.;
  double leftPadding = 0.;

  // Loop over each segment; find single height cells and reorder.
  for (size_t s = 0; s < m_mgrPtr->getNumSegments(); s++) {
    DetailedSeg* segPtr = m_mgrPtr->getSegment(s);
    int segId = segPtr->getSegId();
    int rowId = segPtr->getRowId();

    std::vector<Node*>& nodes = m_mgrPtr->m_cellsInSeg[segId];
    if (nodes.size() < 2) {
      continue;
    }
    std::sort(nodes.begin(), nodes.end(), DetailedMgr::compareNodesX());

    int j = 0;
    int n = nodes.size();
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
              nextPtr->getX() - 0.5 * nextPtr->getWidth() - leftPadding,
              rightLimit);
        }
        Node* prevPtr = (istrt != 0) ? nodes[istrt - 1] : 0;
        leftLimit = segPtr->getMinX();
        if (prevPtr != 0) {
          m_arch->getCellPadding(prevPtr, leftPadding, rightPadding);
          leftLimit = std::max(
              prevPtr->getX() + 0.5 * prevPtr->getWidth() + rightPadding,
              leftLimit);
        }

        reorder(nodes, istrt, istop, leftLimit, rightLimit, segId, rowId);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedReorderer::reorder(std::vector<Node*>& nodes, int jstrt, int jstop,
                                double leftLimit, double rightLimit, int segId,
                                int rowId) {
  int size = jstop - jstrt + 1;

  double siteWidth = m_arch->getRow(0)->getSiteWidth();

  std::vector<double> widths;
  widths.resize(size);
  std::fill(widths.begin(), widths.end(), 0.0);
  std::map<Node*, double> origX;
  for (int i = 0; i < size; i++) {
    Node* ndi = nodes[jstrt + i];
    origX[ndi] = ndi->getX();
  }

  // We want to space cells out evenly while also satisfying gaps.
  // See if we need to adjust boundary limits.
  double rightEdge = 0;
  double leftEdge = 0;
  {
    double leftPadding, rightPadding, dummyPadding;

    Node* ndr = nodes[jstop];
    Node* ndl = nodes[jstrt];

    // Determine current left and right edge for the cells
    // involved.
    m_arch->getCellPadding(ndr, dummyPadding, rightPadding);
    rightEdge = std::min(ndr->getX() + 0.5 * ndr->getWidth() + rightPadding,
                         rightLimit);
    m_arch->getCellPadding(ndl, leftPadding, dummyPadding);
    leftEdge =
        std::max(ndl->getX() - 0.5 * ndl->getWidth() - leftPadding, leftLimit);

    // Determine width and padding requirements.
    double totalPadding = 0.;
    double totalWidth = 0.;
    for (size_t i = jstrt; i <= jstop; i++) {
      Node* ndi = nodes[i];
      m_arch->getCellPadding(ndi, leftPadding, rightPadding);
      totalPadding += (leftPadding + rightPadding);
      totalWidth += ndi->getWidth();
    }

    // Enlarge if we do not have enough space.
    bool changed = true;
    while (changed &&
           (rightEdge - leftEdge < totalWidth + totalPadding - 1.0e-3)) {
      changed = false;
      if (rightEdge + siteWidth <= rightLimit) {
        rightEdge += siteWidth;
        changed = true;
      }
      if (leftEdge - siteWidth >= leftLimit) {
        leftEdge -= siteWidth;
        changed = true;
      }
    }

    if (rightEdge - leftEdge >= totalWidth + totalPadding) {
      // Proceed with padding, and maybe a bit more space.
      double amtPerCell =
          ((rightEdge - leftEdge) - (totalWidth + totalPadding)) / (double)size;
      int sitesPerCell = std::max(1, (int)std::floor(amtPerCell / siteWidth));

      totalWidth = 0.;
      for (size_t i = 0; i < size; i++) {
        Node* ndi = nodes[jstrt + i];
        m_arch->getCellPadding(ndi, leftPadding, rightPadding);
        widths[i] = ndi->getWidth() + leftPadding + rightPadding;

        totalWidth += widths[i];
      }
      for (size_t i = 0; i < size; i++) {
        if (totalWidth + sitesPerCell * siteWidth < rightEdge - leftEdge) {
          totalWidth -= widths[i];
          widths[i] += sitesPerCell * siteWidth;
          totalWidth += widths[i];
        }
      }
    } else if (rightEdge - leftEdge >= totalWidth) {
      // Can proceed without padding, but maybe a bit of space.
      double amtPerCell =
          ((rightEdge - leftEdge) - (totalWidth)) / (double)size;
      int sitesPerCell = std::max(1, (int)std::floor(amtPerCell / siteWidth));

      totalWidth = 0.;
      for (size_t i = 0; i < size; i++) {
        Node* ndi = nodes[jstrt + i];
        widths[i] = ndi->getWidth();
        totalWidth += widths[i];
      }
      for (size_t i = 0; i < size; i++) {
        if (totalWidth + sitesPerCell * siteWidth < rightEdge - leftEdge) {
          totalWidth -= widths[i];
          widths[i] += sitesPerCell * siteWidth;
          totalWidth += widths[i];
        }
      }
    } else {
      // Not enough space so abort.
      return;
    }
  }
  // std::cout << "left edge " << leftEdge << ", right edge " << rightEdge <<
  // std::endl;
  //
  // std::cout << "Solving => [" << leftEdge << "," << rightEdge << "]" <<
  // std::endl; std::cout << "Widths "; for( int i = 0; i < size; i++ )
  //{
  //   std::cout << " " << widths[i];
  //}
  // std::cout << std::endl;

  // Generate the different permutations.  Evaluate each one and keep
  // the best one.  Note that the first permutation, which is the
  // original placement, might not generate the original placement
  // since the spacing might be different.  Consequently, figure out
  // the WL of the current placement and set it as the best.

  std::vector<double> position(size, 0.0);
  for (int i = 0; i < size; i++) {
    Node* ndi = nodes[jstrt + i];
    position[i] = ndi->getX();
  }
  double best = cost(nodes, jstrt, jstop);
  double orig = best;
  // std::cout << "orig:";
  // for( int i = 0; i < size; i++ )
  //{
  //    Node* ndi = nodes[jstrt+i];
  //    std::cout << " " << ndi->getId() << " @ " << ndi->getX();
  //}
  // std::cout << ", cost is " << best << std::endl;

  std::vector<int> order(size, 0);
  for (int i = 0; i < size; i++) {
    order[i] = i;
  }
  bool found = false;
  int count = 0;
  do {
    ++count;

    // std::cout << boost::format( "perm %3d, order: " ) % count;
    // for( int i = 0; i < size; i++ ) { std::cout << " " << order[i]; }

    double x = leftEdge;
    for (int i = 0; i < size; i++) {
      int ix = order[i];

      Node* ndi = nodes[jstrt + ix];

      x += 0.5 * widths[ix];
      ndi->setX(x);
      x += 0.5 * widths[ix];

      // std::cout << " " << ndi->getId() << " @ " << ndi->getX() << ", "
      //    << ndi->getX()-0.5*ndi->getWidth() << "-" <<
      //    ndi->getX()+0.5*ndi->getWidth()
      //    << std::endl;
    }
    double curr = cost(nodes, jstrt, jstop);

    // std::cout << ", cost is " << curr << std::endl;

    if (curr < best) {
      // std::cout << "New best, " << curr << std::endl;
      for (int i = 0; i < size; i++) {
        int ix = order[i];
        position[ix] = nodes[jstrt + ix]->getX();
      }
      best = curr;

      found = true;
    }
  } while (std::next_permutation(order.begin(), order.end()));

  if (!found) {
    // Might as well just put cells back at their original positions
    // and return.
    for (size_t i = 0; i < size; i++) {
      Node* ndi = nodes[jstrt + i];
      ndi->setX(origX[ndi]);
    }
    return;
  }

  // We have found an apprarent lower cost reordering.

  // Put cells at their best positions.
  for (int i = 0; i < size; i++) {
    nodes[jstrt + i]->setX(position[i]);
  }

  double last = cost(nodes, jstrt, jstop);

  // NB: Need to resort.
  std::stable_sort(nodes.begin() + jstrt, nodes.begin() + jstop + 1,
                   DetailedMgr::compareNodesX());

  // Possible things are not site aligned due to the edges or the
  // widths.  Do a simply scan and align to sites.
  {
    bool shifted = false;
    bool failed = false;
    double left = leftEdge;
    double x;
    for (int i = 0; i < size; i++) {
      Node* ndi = nodes[jstrt + i];

      x = ndi->getX() - 1.0e-3;
      if (!m_mgrPtr->alignPos(ndi, x, left, rightEdge)) {
        x = ndi->getX() + 1.0e-3;
        if (!m_mgrPtr->alignPos(ndi, x, left, rightEdge)) {
          failed = true;
          break;
        }
      }
      if (std::fabs(x - ndi->getX()) > 1.0e-3) {
        shifted = true;
      }
      ndi->setX(x);
      left = ndi->getX() + 0.5 * ndi->getWidth();
    }
    if (!failed) {
      // This implies everything got site aligned within the specified
      // interval.  However, we might have shifted something.
      if (shifted) {
        // Recost.  The shifting might have changed the cost.
        last = cost(nodes, jstrt, jstop);
        if (last >= orig) {
          failed = true;
        }
      }
    }

    if (failed) {
      // std::cout << "Restoring original placement." << std::endl;
      // Restore original placement.
      for (int i = 0; i < size; i++) {
        Node* ndi = nodes[jstrt + i];
        ndi->setX(origX[ndi]);
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

    for (int pi = 0; pi < ndi->getPins().size(); pi++) {
      Pin* pini = ndi->getPins()[pi];

      Edge* edi = pini->getEdge();

      int npins = edi->getPins().size();
      if (npins <= 1 || npins >= m_skipNetsLargerThanThis) {
        continue;
      }
      if (m_edgeMask[edi->getId()] == m_traversal) {
        continue;
      }
      m_edgeMask[edi->getId()] = m_traversal;

      double xmin = std::numeric_limits<double>::max();
      double xmax = -std::numeric_limits<double>::max();
      for (int pj = 0; pj < edi->getPins().size(); pj++) {
        Pin* pinj = edi->getPins()[pj];

        Node* ndj = pinj->getNode();

        double x = ndj->getX() + pinj->getOffsetX();

        xmin = std::min(xmin, x);
        xmax = std::max(xmax, x);
      }
      cost += xmax - xmin;
    }
  }
  return cost;
}

}  // namespace dpo
