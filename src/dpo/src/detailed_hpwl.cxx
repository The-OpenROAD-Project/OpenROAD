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
#include "detailed_hpwl.h"
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <boost/tokenizer.hpp>
#include <cmath>
#include <iostream>
#include <stack>
#include <utility>
#include "detailed_manager.h"
#include "detailed_orient.h"

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
// Defines.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedHPWL::DetailedHPWL(Architecture* arch, Network* network,
                           RoutingParams* rt)
    : DetailedObjective("hpwl"),
      m_arch(arch),
      m_network(network),
      m_rt(rt),
      m_mgrPtr(nullptr),
      m_orientPtr(nullptr),
      m_skipNetsLargerThanThis(100),
      m_traversal(0),
      m_edgeMask(m_network->getNumEdges(), m_traversal)
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedHPWL::~DetailedHPWL() {}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedHPWL::init() {
  m_traversal = 0;
  m_edgeMask.resize(m_network->getNumEdges());
  std::fill(m_edgeMask.begin(), m_edgeMask.end(), m_traversal);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedHPWL::init(DetailedMgr* mgrPtr, DetailedOrient* orientPtr) {
  m_orientPtr = orientPtr;
  m_mgrPtr = mgrPtr;
  init();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedHPWL::curr() {
  double xmin, xmax, ymin, ymax;
  double x, y;
  double hpwl = 0.;
  for (int i = 0; i < m_network->getNumEdges(); i++) {
    Edge* edi = m_network->getEdge(i);

    int npins = edi->getPins().size();
    if (npins <= 1 || npins >= m_skipNetsLargerThanThis) {
      continue;
    }

    xmin = std::numeric_limits<double>::max();
    xmax = -std::numeric_limits<double>::max();
    ymin = std::numeric_limits<double>::max();
    ymax = -std::numeric_limits<double>::max();

    for (int pj = 0; pj < edi->getPins().size(); pj++) {
      Pin* pinj = edi->getPins()[pj];

      Node* ndj = pinj->getNode();

      x = ndj->getX() + pinj->getOffsetX();
      y = ndj->getY() + pinj->getOffsetY();

      xmin = std::min(xmin, x);
      xmax = std::max(xmax, x);
      ymin = std::min(ymin, y);
      ymax = std::max(ymax, y);
    }

    hpwl += ((xmax - xmin) + (ymax - ymin));
  }
  return hpwl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedHPWL::delta(int n, std::vector<Node*>& nodes,
                           std::vector<double>& curX, std::vector<double>& curY,
                           std::vector<unsigned>& curOri,
                           std::vector<double>& newX, std::vector<double>& newY,
                           std::vector<unsigned>& newOri) {
  // Given a list of nodes with their old positions and new positions, compute
  // the change in WL. Note that we need to know the orientation information and
  // might need to adjust pin information...

  double x, y;
  double old_wl = 0.;
  double old_xmin, old_xmax, old_ymin, old_ymax;
  double new_wl = 0.;
  double new_xmin, new_xmax, new_ymin, new_ymax;

  // Put cells into their "old positions and orientations".
  for (int i = 0; i < n; i++) {
    nodes[i]->setX(curX[i]);
    nodes[i]->setY(curY[i]);
    if (m_orientPtr != 0) {
      m_orientPtr->orientAdjust(nodes[i], curOri[i]);
    }
  }

  ++m_traversal;
  for (int i = 0; i < n; i++) {
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

      old_xmin = std::numeric_limits<double>::max();
      old_xmax = -std::numeric_limits<double>::max();
      old_ymin = std::numeric_limits<double>::max();
      old_ymax = -std::numeric_limits<double>::max();
      for (int pj = 0; pj < edi->getPins().size(); pj++) {
        Pin* pinj = edi->getPins()[pj];

        Node* curr = pinj->getNode();

        x = curr->getX() + pinj->getOffsetX();
        y = curr->getY() + pinj->getOffsetY();

        old_xmin = std::min(old_xmin, x);
        old_xmax = std::max(old_xmax, x);
        old_ymin = std::min(old_ymin, y);
        old_ymax = std::max(old_ymax, y);
      }

      old_wl += ((old_xmax - old_xmin) + (old_ymax - old_ymin));
    }
  }

  // Put cells into their "new positions and orientations".
  for (int i = 0; i < n; i++) {
    nodes[i]->setX(newX[i]);
    nodes[i]->setY(newY[i]);
    if (m_orientPtr != 0) {
      m_orientPtr->orientAdjust(nodes[i], newOri[i]);
    }
  }

  ++m_traversal;
  for (int i = 0; i < n; i++) {
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

      new_xmin = std::numeric_limits<double>::max();
      new_xmax = -std::numeric_limits<double>::max();
      new_ymin = std::numeric_limits<double>::max();
      new_ymax = -std::numeric_limits<double>::max();

      for (int pj = 0; pj < edi->getPins().size(); pj++) {
        Pin* pinj = edi->getPins()[pj];

        Node* curr = pinj->getNode();

        x = curr->getX() + pinj->getOffsetX();
        y = curr->getY() + pinj->getOffsetY();

        new_xmin = std::min(new_xmin, x);
        new_xmax = std::max(new_xmax, x);
        new_ymin = std::min(new_ymin, y);
        new_ymax = std::max(new_ymax, y);
      }

      new_wl += ((new_xmax - new_xmin) + (new_ymax - new_ymin));
    }
  }

  // Put cells into their "old positions and orientations" before returning
  // (leave things as they were provided to us...).
  for (int i = 0; i < n; i++) {
    nodes[i]->setX(curX[i]);
    nodes[i]->setY(curY[i]);
    if (m_orientPtr != 0) {
      m_orientPtr->orientAdjust(nodes[i], curOri[i]);
    }
  }

  // +ve means improvement.
  return old_wl - new_wl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedHPWL::delta(Node* ndi, double new_x, double new_y) {
  // Compute change in wire length for moving node to new position.

  double old_wl = 0.;
  double new_wl = 0.;
  double x, y;
  double old_xmin, old_xmax, old_ymin, old_ymax;
  double new_xmin, new_xmax, new_ymin, new_ymax;

  ++m_traversal;
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

    old_xmin = std::numeric_limits<double>::max();
    old_xmax = -std::numeric_limits<double>::max();
    old_ymin = std::numeric_limits<double>::max();
    old_ymax = -std::numeric_limits<double>::max();

    new_xmin = std::numeric_limits<double>::max();
    new_xmax = -std::numeric_limits<double>::max();
    new_ymin = std::numeric_limits<double>::max();
    new_ymax = -std::numeric_limits<double>::max();

    for (int pj = 0; pj < edi->getPins().size(); pj++) {
      Pin* pinj = edi->getPins()[pj];

      Node* ndj = pinj->getNode();

      x = ndj->getX() + pinj->getOffsetX();
      y = ndj->getY() + pinj->getOffsetY();

      old_xmin = std::min(old_xmin, x);
      old_xmax = std::max(old_xmax, x);
      old_ymin = std::min(old_ymin, y);
      old_ymax = std::max(old_ymax, y);

      if (ndj == ndi) {
        x = new_x + pinj->getOffsetX();
        y = new_y + pinj->getOffsetY();
      }
      new_xmin = std::min(new_xmin, x);
      new_xmax = std::max(new_xmax, x);
      new_ymin = std::min(new_ymin, y);
      new_ymax = std::max(new_ymax, y);
    }
    old_wl += old_xmax - old_xmin + old_ymax - old_ymin;
    new_wl += new_xmax - new_xmin + new_ymax - new_ymin;
  }
  return old_wl - new_wl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedHPWL::getCandidates(std::vector<Node*>& candidates) {
  candidates.erase(candidates.begin(), candidates.end());
  candidates = m_mgrPtr->m_singleHeightCells;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedHPWL::delta(Node* ndi, Node* ndj) {
  // Compute change in wire length for swapping the two nodes.

  double old_wl = 0.;
  double new_wl = 0.;
  double x, y;
  double old_xmin, old_xmax, old_ymin, old_ymax;
  double new_xmin, new_xmax, new_ymin, new_ymax;
  Node* nodes[2];
  nodes[0] = ndi;
  nodes[1] = ndj;

  ++m_traversal;
  for (int c = 0; c <= 1; c++) {
    Node* ndi = nodes[c];
    for (int pi = 0; pi < ndi->getPins().size(); pi++) {
      Pin* pini = ndi->getPins()[pi];

      Edge* edi = pini->getEdge();

      //int npins = edi->getNumPins();
      int npins = edi->getPins().size();
      if (npins <= 1 || npins >= m_skipNetsLargerThanThis) {
        continue;
      }
      if (m_edgeMask[edi->getId()] == m_traversal) {
        continue;
      }
      m_edgeMask[edi->getId()] = m_traversal;

      old_xmin = std::numeric_limits<double>::max();
      old_xmax = -std::numeric_limits<double>::max();
      old_ymin = std::numeric_limits<double>::max();
      old_ymax = -std::numeric_limits<double>::max();

      new_xmin = std::numeric_limits<double>::max();
      new_xmax = -std::numeric_limits<double>::max();
      new_ymin = std::numeric_limits<double>::max();
      new_ymax = -std::numeric_limits<double>::max();

      for (int pj = 0; pj < edi->getPins().size(); pj++) {
        Pin* pinj = edi->getPins()[pj];

        Node* ndj = pinj->getNode();

        x = ndj->getX() + pinj->getOffsetX();
        y = ndj->getY() + pinj->getOffsetY();

        old_xmin = std::min(old_xmin, x);
        old_xmax = std::max(old_xmax, x);
        old_ymin = std::min(old_ymin, y);
        old_ymax = std::max(old_ymax, y);

        if (ndj == nodes[0]) {
          ndj = nodes[1];
        } else if (ndj == nodes[1]) {
          ndj = nodes[0];
        }

        x = ndj->getX() + pinj->getOffsetX();
        y = ndj->getY() + pinj->getOffsetY();

        new_xmin = std::min(new_xmin, x);
        new_xmax = std::max(new_xmax, x);
        new_ymin = std::min(new_ymin, y);
        new_ymax = std::max(new_ymax, y);
      }

      old_wl += old_xmax - old_xmin + old_ymax - old_ymin;
      new_wl += new_xmax - new_xmin + new_ymax - new_ymin;
    }
  }
  return old_wl - new_wl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedHPWL::delta(Node* ndi, double target_xi, double target_yi,
                           Node* ndj, double target_xj, double target_yj) {
  // Compute change in wire length for swapping the two nodes.

  double old_wl = 0.;
  double new_wl = 0.;
  double x, y;
  double old_xmin, old_xmax, old_ymin, old_ymax;
  double new_xmin, new_xmax, new_ymin, new_ymax;
  Node* nodes[2];
  nodes[0] = ndi;
  nodes[1] = ndj;

  ++m_traversal;
  for (int c = 0; c <= 1; c++) {
    Node* ndi = nodes[c];
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

      old_xmin = std::numeric_limits<double>::max();
      old_xmax = -std::numeric_limits<double>::max();
      old_ymin = std::numeric_limits<double>::max();
      old_ymax = -std::numeric_limits<double>::max();

      new_xmin = std::numeric_limits<double>::max();
      new_xmax = -std::numeric_limits<double>::max();
      new_ymin = std::numeric_limits<double>::max();
      new_ymax = -std::numeric_limits<double>::max();

      for (int pj = 0; pj < edi->getPins().size(); pj++) {
        Pin* pinj = edi->getPins()[pj];

        Node* curr = pinj->getNode();

        x = curr->getX() + pinj->getOffsetX();
        y = curr->getY() + pinj->getOffsetY();

        old_xmin = std::min(old_xmin, x);
        old_xmax = std::max(old_xmax, x);
        old_ymin = std::min(old_ymin, y);
        old_ymax = std::max(old_ymax, y);

        if (curr == nodes[0]) {
          x = target_xi + pinj->getOffsetX();
          y = target_yi + pinj->getOffsetY();
        } else if (curr == nodes[1]) {
          x = target_xj + pinj->getOffsetX();
          y = target_yj + pinj->getOffsetY();
        }

        new_xmin = std::min(new_xmin, x);
        new_xmax = std::max(new_xmax, x);
        new_ymin = std::min(new_ymin, y);
        new_ymax = std::max(new_ymax, y);
      }

      old_wl += ((old_xmax - old_xmin) + (old_ymax - old_ymin));
      new_wl += ((new_xmax - new_xmin) + (new_ymax - new_ymin));
    }
  }
  return old_wl - new_wl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
}  // namespace dpo
