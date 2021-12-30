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
#include "detailed_displacement.h"
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
DetailedDisplacement::DetailedDisplacement(Architecture* arch, Network* network,
                                           RoutingParams* rt)
    : DetailedObjective("disp"),
      m_arch(arch),
      m_network(network),
      m_rt(rt),
      m_mgrPtr(nullptr),
      m_orientPtr(nullptr),
      m_singleRowHeight(m_arch->getRow(0)->getHeight()),
      m_nSets(0)
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedDisplacement::~DetailedDisplacement() {}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedDisplacement::init() {
  m_nSets = 0;
  m_count.resize(m_mgrPtr->m_multiHeightCells.size());
  std::fill(m_count.begin(), m_count.end(), 0);
  m_count[1] = (int)m_mgrPtr->m_singleHeightCells.size();
  if (m_count[1] != 0) {
    ++m_nSets;
  }
  for (size_t i = 2; i < m_mgrPtr->m_multiHeightCells.size(); i++) {
    m_count[i] = (int)m_mgrPtr->m_multiHeightCells[i].size();
    if (m_count[i] != 0) {
      ++m_nSets;
    }
  }
  m_tot.resize(m_mgrPtr->m_multiHeightCells.size());
  m_del.resize(m_mgrPtr->m_multiHeightCells.size());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedDisplacement::init(DetailedMgr* mgrPtr,
                                DetailedOrient* orientPtr) {
  m_orientPtr = orientPtr;
  m_mgrPtr = mgrPtr;
  init();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedDisplacement::curr() {
  double dx, dy;

  std::fill(m_tot.begin(), m_tot.end(), 0.0);
  for (size_t i = 0; i < m_mgrPtr->m_singleHeightCells.size(); i++) {
    Node* ndi = m_mgrPtr->m_singleHeightCells[i];

    dx = std::fabs(ndi->getX() - ndi->getOrigX());
    dy = std::fabs(ndi->getY() - ndi->getOrigY());
    m_tot[1] += dx + dy;
  }
  for (size_t s = 2; s < m_mgrPtr->m_multiHeightCells.size(); s++) {
    for (size_t i = 0; i < m_mgrPtr->m_multiHeightCells[s].size(); i++) {
      Node* ndi = m_mgrPtr->m_multiHeightCells[s][i];

      dx = std::fabs(ndi->getX() - ndi->getOrigX());
      dy = std::fabs(ndi->getY() - ndi->getOrigY());
      m_tot[s] += dx + dy;
    }
  }

  double disp = 0.;
  for (size_t i = 0; i < m_tot.size(); i++) {
    if (m_count[i] != 0) {
      disp += m_tot[i] / (double)m_count[i];
    }
  }
  disp /= m_singleRowHeight;
  disp /= (double)m_nSets;

  return disp;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedDisplacement::delta(int n, std::vector<Node*>& nodes,
                                   std::vector<double>& curX,
                                   std::vector<double>& curY,
                                   std::vector<unsigned>& curOri,
                                   std::vector<double>& newX,
                                   std::vector<double>& newY,
                                   std::vector<unsigned>& newOri) {
  // Given a list of nodes with their old positions and new positions, compute
  // the change in displacement.  Note that cell orientation is not relevant.

  double dx, dy;

  std::fill(m_del.begin(), m_del.end(), 0.0);

  // Put cells into their "old positions and orientations".
  for (int i = 0; i < n; i++) {
    nodes[i]->setX(curX[i]);
    nodes[i]->setY(curY[i]);
    if (m_orientPtr != 0) {
      m_orientPtr->orientAdjust(nodes[i], curOri[i]);
    }
  }

  for (int i = 0; i < n; i++) {
    Node* ndi = nodes[i];

    int spanned = (int)(ndi->getHeight() / m_singleRowHeight + 0.5);

    dx = std::fabs(ndi->getX() - ndi->getOrigX());
    dy = std::fabs(ndi->getY() - ndi->getOrigY());

    m_del[spanned] += (dx + dy);
    // old_disp += dx + dy;
  }

  // Put cells into their "new positions and orientations".
  for (int i = 0; i < n; i++) {
    nodes[i]->setX(newX[i]);
    nodes[i]->setY(newY[i]);
    if (m_orientPtr != 0) {
      m_orientPtr->orientAdjust(nodes[i], newOri[i]);
    }
  }

  for (int i = 0; i < n; i++) {
    Node* ndi = nodes[i];

    int spanned = (int)(ndi->getHeight() / m_singleRowHeight + 0.5);

    dx = std::fabs(ndi->getX() - ndi->getOrigX());
    dy = std::fabs(ndi->getY() - ndi->getOrigY());

    m_del[spanned] -= (dx + dy);
    // new_disp += dx + dy;
  }

  // Put cells into their "old positions and orientations" before returning.
  for (int i = 0; i < n; i++) {
    nodes[i]->setX(curX[i]);
    nodes[i]->setY(curY[i]);
    if (m_orientPtr != 0) {
      m_orientPtr->orientAdjust(nodes[i], curOri[i]);
    }
  }

  double delta = 0.;
  for (size_t i = 0; i < m_del.size(); i++) {
    if (m_count[i] != 0) {
      delta += m_del[i] / (double)m_count[i];
    }
  }
  delta /= m_singleRowHeight;
  delta /= (double)m_nSets;

  // double delta = old_disp - new_disp;
  // delta /= m_singleRowHeight;
  // delta /= (double)m_network->m_nodes.size();

  // +ve means improvement.
  return delta;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedDisplacement::delta(Node* ndi, double new_x, double new_y) {
  // Compute change in displacement for moving node to new position.

  double old_disp = 0.;
  double new_disp = 0.;
  double dx, dy;

  dx = std::fabs(ndi->getX() - ndi->getOrigX());
  dy = std::fabs(ndi->getY() - ndi->getOrigY());
  old_disp = dx + dy;

  dx = std::fabs(new_x - ndi->getOrigX());
  dy = std::fabs(new_y - ndi->getOrigY());
  new_disp = dx + dy;

  // +ve means improvement.
  return old_disp - new_disp;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedDisplacement::getCandidates(std::vector<Node*>& candidates) {
  candidates.erase(candidates.begin(), candidates.end());
  candidates = m_mgrPtr->m_singleHeightCells;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedDisplacement::delta(Node* ndi, Node* ndj) {
  // Compute change in wire length for swapping the two nodes.

  double old_disp = 0.;
  double new_disp = 0.;
  double dx, dy;

  dx = std::fabs(ndi->getX() - ndi->getOrigX());
  dy = std::fabs(ndi->getY() - ndi->getOrigY());
  old_disp += dx + dy;
  dx = std::fabs(ndj->getX() - ndj->getOrigX());
  dy = std::fabs(ndj->getY() - ndj->getOrigY());
  old_disp += dx + dy;

  dx = std::fabs(ndj->getX() - ndi->getOrigX());
  dy = std::fabs(ndj->getY() - ndi->getOrigY());
  new_disp += dx + dy;
  dx = std::fabs(ndi->getX() - ndj->getOrigX());
  dy = std::fabs(ndi->getY() - ndj->getOrigY());
  new_disp += dx + dy;

  // +ve means improvement.
  return old_disp - new_disp;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedDisplacement::delta(Node* ndi, double target_xi,
                                   double target_yi, Node* ndj,
                                   double target_xj, double target_yj) {
  // Compute change in wire length for swapping the two nodes at specified
  // targets.

  double old_disp = 0.;
  double new_disp = 0.;
  double dx, dy;

  dx = std::fabs(ndi->getX() - ndi->getOrigX());
  dy = std::fabs(ndi->getY() - ndi->getOrigY());
  old_disp += dx + dy;
  dx = std::fabs(ndj->getX() - ndj->getOrigX());
  dy = std::fabs(ndj->getY() - ndj->getOrigY());
  old_disp += dx + dy;

  dx = std::fabs(target_xi - ndi->getOrigX());
  dy = std::fabs(target_yi - ndi->getOrigY());
  new_disp += dx + dy;
  dx = std::fabs(target_xj - ndj->getOrigX());
  dy = std::fabs(target_yj - ndj->getOrigY());
  new_disp += dx + dy;

  // +ve means improvement.
  return old_disp - new_disp;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
}  // namespace dpo
