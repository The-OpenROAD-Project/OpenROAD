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
#include "utility.h"
#include <stdio.h>
#include <cmath>
#include <deque>
#include <iostream>
#include <stack>
#include <vector>
#include "architecture.h"
#include "network.h"
#include "rectangle.h"

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double Utility::disp_l1(Network* nw, double& tot, double& max, double& avg) {
  // Returns L1 displacement of the current placement from that stored.
  tot = 0.;
  max = 0.;
  avg = 0.;
  for (int i = 0; i < nw->getNumNodes(); i++) {
    Node* ndi = nw->getNode(i);

    double dx = std::fabs(ndi->getLeft() - ndi->getOrigLeft());
    double dy = std::fabs(ndi->getBottom() - ndi->getOrigBottom());

    tot += dx + dx;
    max = std::max(max, dx + dy);
  }
  avg = tot / (double)nw->getNumNodes(); 

  return tot;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double Utility::hpwl(Network* nw) {
  // Compute the wire length for the given placement.
  double totWL = 0.0;
  for (unsigned e = 0; e < nw->getNumEdges(); e++) {
    Edge* ed = nw->getEdge(e);
    totWL += hpwl(nw, ed);
  }
  return totWL;
}

double Utility::hpwl(Network* nw, double& hpwlx, double& hpwly) {
  hpwlx = 0.0;
  hpwly = 0.0;
  // Compute the wire length for the given placement.
  unsigned numEdges = nw->getNumEdges();
  Rectangle box;
  for (unsigned e = 0; e < numEdges; e++) {
    Edge* ed = nw->getEdge(e);

    int numPins = ed->getNumPins();
    if (numPins <= 1) {
      continue;
    }

    box.reset();
    for (unsigned p = 0; p < ed->getPins().size(); p++) {
      Pin* pin = ed->getPins()[p];

      Node* ndi = pin->getNode();
      double py = ndi->getBottom()+0.5*ndi->getHeight() + pin->getOffsetY();
      double px = ndi->getLeft()+0.5*ndi->getWidth() + pin->getOffsetX();
      box.addPt(px, py);
    }
    hpwlx += box.getWidth();
    hpwly += box.getHeight();
  }
  return hpwlx+hpwly;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double Utility::hpwl(Network* nw, Edge* ed) {
  double hpwlx = 0.0;
  double hpwly = 0.0;
  return hpwl(nw, ed, hpwlx, hpwly);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double Utility::hpwl(Network* nw, Edge* ed, double& hpwlx, double& hpwly) {
  hpwlx = 0.0;
  hpwly = 0.0;

  int numPins = ed->getNumPins();
  if (numPins <= 1) {
    return 0.0;
  }

  Rectangle box;
  for (int p = 0; p < ed->getPins().size(); p++) {
    Pin* pin = ed->getPins()[p];

    Node* ndi = pin->getNode();
    double py = ndi->getBottom()+0.5*ndi->getHeight() + pin->getOffsetY();
    double px = ndi->getLeft()+0.5*ndi->getWidth() + pin->getOffsetX();
    box.addPt(px, py);
  }
  hpwlx = box.getWidth();
  hpwly = box.getHeight();
  return hpwlx+hpwly;
}

void Utility::get_row_blockages(
    Network* network, Architecture* arch, std::vector<Node*>& fixed,
    std::vector<std::vector<std::pair<double, double> > >& blockages) {
  // Given a list of fixed objects (determined by some other routine), this
  // routine intersects the fixed objects to create a list of intervals on a per
  // row basis which act as blockages within each row.
  //
  // This information can be used, for example, to segment a row and determine
  // which cells can be placed into a row.

  blockages.erase(blockages.begin(), blockages.end());

  int numRows = arch->getNumRows();

  // Allocate proper space for the blockages.
  blockages.resize(numRows);
  for (int r = 0; r < numRows; r++) {
    blockages[r] = std::vector<std::pair<double, double> >();
  }

  // Fixed items create the blockages.  Intersect the blockages with each of the
  // rows to find the blockages per row.
  for (int i = 0; i < fixed.size(); i++) {
    Node* nd = fixed[i];

    double xmin = std::max(arch->getMinX(), nd->getLeft());
    double xmax = std::min(arch->getMaxX(), nd->getRight());
    double ymin = std::max(arch->getMinY(), nd->getBottom());
    double ymax = std::min(arch->getMaxY(), nd->getTop());

    for (int r = 0; r < numRows; r++) {
      double lb = arch->getMinY() + r * arch->getRow(r)->getHeight();
      double ub = lb + arch->getRow(r)->getHeight();

      // Note that a blockage only needs to overlap with a bit of the row before
      // it is considered a blockage!
      if (!(ymax - 1.0e-3 <= lb || ymin + 1.0e-3 >= ub)) {
        blockages[r].push_back(std::pair<double, double>(xmin, xmax));
      }
    }
  }

  // Actually, fixed objects might overlap with each other... We need to check
  // for this overlap and turn the blockages into non-overlapping blockages by
  // removing some of the crap; i.e., we want continuous intervals...
  for (int r = 0; r < numRows; r++) {
    if (blockages[r].size() == 0) {
      continue;
    }

    std::sort(blockages[r].begin(), blockages[r].end(),
              Utility::compare_blockages());

    std::stack<std::pair<double, double> > s;
    s.push(blockages[r][0]);
    for (int i = 1; i < blockages[r].size(); i++) {
      std::pair<double, double> top = s.top();  // copy.
      if (top.second < blockages[r][i].first) {
        s.push(blockages[r][i]);  // new interval.
      } else {
        if (top.second < blockages[r][i].second) {
          top.second = blockages[r][i].second;  // extend interval.
        }
        s.pop();      // remove old.
        s.push(top);  // expanded interval.
      }
    }

    // Create a "cleaned-up" list of blockages.
    blockages[r].erase(blockages[r].begin(), blockages[r].end());
    while (!s.empty()) {
      std::pair<double, double> temp = s.top();  // copy.
      blockages[r].push_back(temp);
      s.pop();
    }

    // Sort the blockages from left-to-right... Several of the calling routines
    // expect this so do it here...
    std::sort(blockages[r].begin(), blockages[r].end(),
              Utility::compare_blockages());
  }
}

double Utility::compute_overlap(double xmin1, double xmax1, double ymin1,
                                double ymax1, double xmin2, double xmax2,
                                double ymin2, double ymax2) {
  if (xmin1 >= xmax2) return 0.0;
  if (xmax1 <= xmin2) return 0.0;
  if (ymin1 >= ymax2) return 0.0;
  if (ymax1 <= ymin2) return 0.0;
  double ww = std::min(xmax1, xmax2) - std::max(xmin1, xmin2);
  double hh = std::min(ymax1, ymax2) - std::max(ymin1, ymin2);
  return ww * hh;
}

bool Utility::setOrientation(Network* network, Node* ndi, unsigned newOri) {
  // Set the orientation of the node to the specified orientation.  We don't
  // really check if the orientation is valid or not...  To change a cell
  // orientation, we need to figure out how the cell edges and pins need to be
  // flipped.  We need to actually make the flip to ensure future computation
  // will reflect the orientation change.
  unsigned curOri = ndi->getCurrOrient();
  if (curOri == newOri) {
    return false;
  }

  // Determine how pins need to be flipped.  I guess the easiest thing to do it
  // to first return the node to the N orientation and then figure out how to
  // get it into the new orientation!
  int mY = 1;  // Multiplier to adjust pin offsets for a flip around the X-axis.
  int mX = 1;  // Multiplier to adjust pin offsets for a flip around the Y-axis.

  switch (curOri) {
    case Orientation_N:
      break;
    case Orientation_S:
      mX *= -1;
      mY *= -1;
      break;
    case Orientation_FS:
      mY *= -1;
      break;
    case Orientation_FN:
      mX *= -1;
      break;
    default:
      break;
  }

  // Here, assume the cell is in the North Orientation...
  switch (newOri) {
    case Orientation_N:
      break;
    case Orientation_S:
      mX *= -1;
      mY *= -1;
      break;
    case Orientation_FS:
      mY *= -1;
      break;
    case Orientation_FN:
      mX *= -1;
      break;
    default:
      break;
  }

  for (int pi = 0; pi < ndi->getPins().size(); pi++) {
    Pin* pin = ndi->getPins()[pi];

    if (mX == -1) {
     pin->setOffsetX( pin->getOffsetX() * (double)mX );
    }
    if (mY == -1) {
     pin->setOffsetY( pin->getOffsetY() * (double)mY );
    }
  }
  ndi->setCurrOrient( newOri );

  if (mX == -1) {
    ndi->swapEdgeTypes();
  }
  return false;
}

}  // namespace dpo
