// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "utility.h"

#include <algorithm>

#include "network.h"
#include "rectangle.h"

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double Utility::disp_l1(Network* nw, double& tot, double& max, double& avg)
{
  // Returns L1 displacement of the current placement from that stored.
  tot = 0.;
  max = 0.;
  avg = 0.;
  for (int i = 0; i < nw->getNumNodes(); i++) {
    const Node* ndi = nw->getNode(i);

    const DbuX dx = abs(ndi->getLeft() - ndi->getOrigLeft());
    const DbuY dy = abs(ndi->getBottom() - ndi->getOrigBottom());
    // TODO: fix wrong total displacement calculation
    tot += dx.v + dx.v;
    max = std::max(max, (double) dx.v + dy.v);
  }
  avg = tot / (double) nw->getNumNodes();

  return tot;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double Utility::hpwl(const Network* nw)
{
  // Compute the wire length for the given placement.
  double totWL = 0.0;
  for (unsigned e = 0; e < nw->getNumEdges(); e++) {
    const Edge* ed = nw->getEdge(e);
    totWL += hpwl(ed);
  }
  return totWL;
}

double Utility::hpwl(const Network* nw, double& hpwlx, double& hpwly)
{
  hpwlx = 0.0;
  hpwly = 0.0;
  // Compute the wire length for the given placement.
  unsigned numEdges = nw->getNumEdges();
  Rectangle box;
  for (unsigned e = 0; e < numEdges; e++) {
    const Edge* ed = nw->getEdge(e);

    const int numPins = ed->getNumPins();
    if (numPins <= 1) {
      continue;
    }

    box.reset();
    for (const Pin* pin : ed->getPins()) {
      const Node* ndi = pin->getNode();
      const double py
          = ndi->getBottom().v + 0.5 * ndi->getHeight().v + pin->getOffsetY().v;
      const double px
          = ndi->getLeft().v + 0.5 * ndi->getWidth().v + pin->getOffsetX().v;
      box.addPt(px, py);
    }
    hpwlx += box.getWidth();
    hpwly += box.getHeight();
  }
  return hpwlx + hpwly;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double Utility::hpwl(const Edge* ed)
{
  double hpwlx = 0.0;
  double hpwly = 0.0;
  return hpwl(ed, hpwlx, hpwly);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double Utility::hpwl(const Edge* ed, double& hpwlx, double& hpwly)
{
  hpwlx = 0.0;
  hpwly = 0.0;

  const int numPins = ed->getNumPins();
  if (numPins <= 1) {
    return 0.0;
  }

  Rectangle box;
  for (const Pin* pin : ed->getPins()) {
    const Node* ndi = pin->getNode();
    const double py
        = ndi->getBottom().v + 0.5 * ndi->getHeight().v + pin->getOffsetY().v;
    const double px
        = ndi->getLeft().v + 0.5 * ndi->getWidth().v + pin->getOffsetX().v;
    box.addPt(px, py);
  }
  hpwlx = box.getWidth();
  hpwly = box.getHeight();
  return hpwlx + hpwly;
}

}  // namespace dpo
