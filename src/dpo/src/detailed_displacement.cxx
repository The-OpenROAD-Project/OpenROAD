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
DetailedDisplacement::DetailedDisplacement(Architecture* arch,
                                           Network* network,
                                           RoutingParams* rt)
    : DetailedObjective("disp"),
      arch_(arch),
      network_(network),
      rt_(rt),
      mgrPtr_(nullptr),
      orientPtr_(nullptr),
      singleRowHeight_(arch_->getRow(0)->getHeight()),
      nSets_(0)
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedDisplacement::~DetailedDisplacement()
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedDisplacement::init()
{
  nSets_ = 0;
  count_.resize(mgrPtr_->multiHeightCells_.size());
  std::fill(count_.begin(), count_.end(), 0);
  count_[1] = (int) mgrPtr_->singleHeightCells_.size();
  if (count_[1] != 0) {
    ++nSets_;
  }
  for (size_t i = 2; i < mgrPtr_->multiHeightCells_.size(); i++) {
    count_[i] = (int) mgrPtr_->multiHeightCells_[i].size();
    if (count_[i] != 0) {
      ++nSets_;
    }
  }
  tot_.resize(mgrPtr_->multiHeightCells_.size());
  del_.resize(mgrPtr_->multiHeightCells_.size());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedDisplacement::init(DetailedMgr* mgrPtr, DetailedOrient* orientPtr)
{
  orientPtr_ = orientPtr;
  mgrPtr_ = mgrPtr;
  init();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedDisplacement::curr()
{
  double dx, dy;

  std::fill(tot_.begin(), tot_.end(), 0.0);
  for (size_t i = 0; i < mgrPtr_->singleHeightCells_.size(); i++) {
    Node* ndi = mgrPtr_->singleHeightCells_[i];

    dx = std::fabs(ndi->getLeft() - ndi->getOrigLeft());
    dy = std::fabs(ndi->getBottom() - ndi->getOrigBottom());
    tot_[1] += dx + dy;
  }
  for (size_t s = 2; s < mgrPtr_->multiHeightCells_.size(); s++) {
    for (size_t i = 0; i < mgrPtr_->multiHeightCells_[s].size(); i++) {
      Node* ndi = mgrPtr_->multiHeightCells_[s][i];

      dx = std::fabs(ndi->getLeft() - ndi->getOrigLeft());
      dy = std::fabs(ndi->getBottom() - ndi->getOrigBottom());
      tot_[s] += dx + dy;
    }
  }

  double disp = 0.;
  for (size_t i = 0; i < tot_.size(); i++) {
    if (count_[i] != 0) {
      disp += tot_[i] / (double) count_[i];
    }
  }
  disp /= singleRowHeight_;
  disp /= (double) nSets_;

  return disp;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedDisplacement::delta(int n,
                                   std::vector<Node*>& nodes,
                                   std::vector<int>& curLeft,
                                   std::vector<int>& curBottom,
                                   std::vector<unsigned>& curOri,
                                   std::vector<int>& newLeft,
                                   std::vector<int>& newBottom,
                                   std::vector<unsigned>& newOri)
{
  // Given a list of nodes with their old positions and new positions, compute
  // the change in displacement.  Note that cell orientation is not relevant.

  double dx, dy;

  std::fill(del_.begin(), del_.end(), 0.0);

  // Put cells into their "old positions and orientations".
  for (int i = 0; i < n; i++) {
    nodes[i]->setLeft(curLeft[i]);
    nodes[i]->setBottom(curBottom[i]);
    if (orientPtr_ != 0) {
      orientPtr_->orientAdjust(nodes[i], curOri[i]);
    }
  }

  for (int i = 0; i < n; i++) {
    Node* ndi = nodes[i];

    int spanned = (int) (ndi->getHeight() / singleRowHeight_ + 0.5);

    dx = std::fabs(ndi->getLeft() - ndi->getOrigLeft());
    dy = std::fabs(ndi->getBottom() - ndi->getOrigBottom());

    del_[spanned] += (dx + dy);
    // old_disp += dx + dy;
  }

  // Put cells into their "new positions and orientations".
  for (int i = 0; i < n; i++) {
    nodes[i]->setLeft(newLeft[i]);
    nodes[i]->setBottom(newBottom[i]);
    if (orientPtr_ != 0) {
      orientPtr_->orientAdjust(nodes[i], newOri[i]);
    }
  }

  for (int i = 0; i < n; i++) {
    Node* ndi = nodes[i];

    dx = std::fabs(ndi->getLeft() - ndi->getOrigLeft());
    dy = std::fabs(ndi->getBottom() - ndi->getOrigBottom());

    int spanned = arch_->getCellHeightInRows(ndi);
    del_[spanned] -= (dx + dy);
  }

  // Put cells into their "old positions and orientations" before returning.
  for (int i = 0; i < n; i++) {
    nodes[i]->setLeft(curLeft[i]);
    nodes[i]->setBottom(curBottom[i]);
    if (orientPtr_ != 0) {
      orientPtr_->orientAdjust(nodes[i], curOri[i]);
    }
  }

  double delta = 0.;
  for (size_t i = 0; i < del_.size(); i++) {
    if (count_[i] != 0) {
      delta += del_[i] / (double) count_[i];
    }
  }
  delta /= singleRowHeight_;
  delta /= (double) nSets_;

  // +ve means improvement.
  return delta;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedDisplacement::delta(Node* ndi, double new_x, double new_y)
{
  // Compute change in displacement for moving node to new position.

  double old_disp = 0.;
  double new_disp = 0.;
  double dx, dy;

  // Targets are centers, but computation is with left and bottom...
  new_x -= 0.5 * ndi->getWidth();
  new_y -= 0.5 * ndi->getHeight();

  dx = std::fabs(ndi->getLeft() - ndi->getOrigLeft());
  dy = std::fabs(ndi->getBottom() - ndi->getOrigBottom());
  old_disp = dx + dy;

  dx = std::fabs(new_x - ndi->getOrigLeft());
  dy = std::fabs(new_y - ndi->getOrigBottom());
  new_disp = dx + dy;

  // +ve means improvement.
  return old_disp - new_disp;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedDisplacement::getCandidates(std::vector<Node*>& candidates)
{
  candidates.erase(candidates.begin(), candidates.end());
  candidates = mgrPtr_->singleHeightCells_;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedDisplacement::delta(Node* ndi, Node* ndj)
{
  // Compute change in wire length for swapping the two nodes.

  double old_disp = 0.;
  double new_disp = 0.;
  double dx, dy;

  dx = std::fabs(ndi->getLeft() - ndi->getOrigLeft());
  dy = std::fabs(ndi->getBottom() - ndi->getOrigBottom());
  old_disp += dx + dy;
  dx = std::fabs(ndj->getLeft() - ndj->getOrigLeft());
  dy = std::fabs(ndj->getBottom() - ndj->getOrigBottom());
  old_disp += dx + dy;

  dx = std::fabs(ndj->getLeft() - ndi->getOrigLeft());
  dy = std::fabs(ndj->getBottom() - ndi->getOrigBottom());
  new_disp += dx + dy;
  dx = std::fabs(ndi->getLeft() - ndj->getOrigLeft());
  dy = std::fabs(ndi->getBottom() - ndj->getOrigBottom());
  new_disp += dx + dy;

  // +ve means improvement.
  return old_disp - new_disp;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedDisplacement::delta(Node* ndi,
                                   double target_xi,
                                   double target_yi,
                                   Node* ndj,
                                   double target_xj,
                                   double target_yj)
{
  // Compute change in wire length for swapping the two nodes at specified
  // targets.

  double old_disp = 0.;
  double new_disp = 0.;
  double dx, dy;

  // Targets are centers, but computation is with left and bottom...
  target_xi -= 0.5 * ndi->getWidth();
  target_yi -= 0.5 * ndi->getHeight();

  target_xj -= 0.5 * ndj->getWidth();
  target_yj -= 0.5 * ndj->getHeight();

  dx = std::fabs(ndi->getLeft() - ndi->getOrigLeft());
  dy = std::fabs(ndi->getBottom() - ndi->getOrigBottom());
  old_disp += dx + dy;
  dx = std::fabs(ndj->getLeft() - ndj->getOrigLeft());
  dy = std::fabs(ndj->getBottom() - ndj->getOrigBottom());
  old_disp += dx + dy;

  dx = std::fabs(target_xi - ndi->getOrigLeft());
  dy = std::fabs(target_yi - ndi->getOrigBottom());
  new_disp += dx + dy;
  dx = std::fabs(target_xj - ndj->getOrigLeft());
  dy = std::fabs(target_yj - ndj->getOrigBottom());
  new_disp += dx + dy;

  // +ve means improvement.
  return old_disp - new_disp;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
}  // namespace dpo
