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

}  // namespace dpo
