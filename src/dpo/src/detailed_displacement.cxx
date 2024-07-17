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
DetailedDisplacement::DetailedDisplacement(Architecture* arch)
    : DetailedObjective("disp"),
      arch_(arch),
      singleRowHeight_(arch_->getRow(0)->getHeight())
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedDisplacement::init()
{
  nSets_ = 0;
  count_.resize(mgrPtr_->getNumMultiHeights());
  std::fill(count_.begin(), count_.end(), 0);
  count_[1] = (int) mgrPtr_->getSingleHeightCells().size();
  if (count_[1] != 0) {
    ++nSets_;
  }
  for (size_t i = 2; i < mgrPtr_->getNumMultiHeights(); i++) {
    count_[i] = (int) mgrPtr_->getMultiHeightCells(i).size();
    if (count_[i] != 0) {
      ++nSets_;
    }
  }
  tot_.resize(mgrPtr_->getNumMultiHeights());
  del_.resize(mgrPtr_->getNumMultiHeights());
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
  std::fill(tot_.begin(), tot_.end(), 0.0);
  for (auto ndi : mgrPtr_->getSingleHeightCells()) {
    const double dx = std::fabs(ndi->getLeft() - ndi->getOrigLeft());
    const double dy = std::fabs(ndi->getBottom() - ndi->getOrigBottom());
    tot_[1] += dx + dy;
  }
  for (size_t s = 2; s < mgrPtr_->getNumMultiHeights(); s++) {
    for (size_t i = 0; i < mgrPtr_->getMultiHeightCells(s).size(); i++) {
      const Node* ndi = mgrPtr_->getMultiHeightCells(s)[i];

      const double dx = std::fabs(ndi->getLeft() - ndi->getOrigLeft());
      const double dy = std::fabs(ndi->getBottom() - ndi->getOrigBottom());
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
double DetailedDisplacement::delta(const int n,
                                   const std::vector<Node*>& nodes,
                                   const std::vector<int>& curLeft,
                                   const std::vector<int>& curBottom,
                                   const std::vector<unsigned>& curOri,
                                   const std::vector<int>& newLeft,
                                   const std::vector<int>& newBottom,
                                   const std::vector<unsigned>& newOri)
{
  // Given a list of nodes with their old positions and new positions, compute
  // the change in displacement.  Note that cell orientation is not relevant.

  std::fill(del_.begin(), del_.end(), 0.0);

  // Put cells into their "old positions and orientations".
  for (int i = 0; i < n; i++) {
    nodes[i]->setLeft(curLeft[i]);
    nodes[i]->setBottom(curBottom[i]);
    if (orientPtr_ != nullptr) {
      orientPtr_->orientAdjust(nodes[i], curOri[i]);
    }
  }

  for (int i = 0; i < n; i++) {
    const Node* ndi = nodes[i];

    const int spanned = std::lround(ndi->getHeight() / singleRowHeight_);

    const double dx = std::fabs(ndi->getLeft() - ndi->getOrigLeft());
    const double dy = std::fabs(ndi->getBottom() - ndi->getOrigBottom());

    del_[spanned] += (dx + dy);
  }

  // Put cells into their "new positions and orientations".
  for (int i = 0; i < n; i++) {
    nodes[i]->setLeft(newLeft[i]);
    nodes[i]->setBottom(newBottom[i]);
    if (orientPtr_ != nullptr) {
      orientPtr_->orientAdjust(nodes[i], newOri[i]);
    }
  }

  for (int i = 0; i < n; i++) {
    const Node* ndi = nodes[i];

    const double dx = std::fabs(ndi->getLeft() - ndi->getOrigLeft());
    const double dy = std::fabs(ndi->getBottom() - ndi->getOrigBottom());

    const int spanned = arch_->getCellHeightInRows(ndi);
    del_[spanned] -= (dx + dy);
  }

  // Put cells into their "old positions and orientations" before returning.
  for (int i = 0; i < n; i++) {
    nodes[i]->setLeft(curLeft[i]);
    nodes[i]->setBottom(curBottom[i]);
    if (orientPtr_ != nullptr) {
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

  // Targets are centers, but computation is with left and bottom...
  new_x -= 0.5 * ndi->getWidth();
  new_y -= 0.5 * ndi->getHeight();

  double dx = std::fabs(ndi->getLeft() - ndi->getOrigLeft());
  double dy = std::fabs(ndi->getBottom() - ndi->getOrigBottom());
  const double old_disp = dx + dy;

  dx = std::fabs(new_x - ndi->getOrigLeft());
  dy = std::fabs(new_y - ndi->getOrigBottom());
  const double new_disp = dx + dy;

  // +ve means improvement.
  return old_disp - new_disp;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedDisplacement::getCandidates(std::vector<Node*>& candidates)
{
  candidates.clear();
  candidates = mgrPtr_->getSingleHeightCells();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedDisplacement::delta(Node* ndi, Node* ndj)
{
  // Compute change in wire length for swapping the two nodes.
  double dx = std::fabs(ndi->getLeft() - ndi->getOrigLeft());
  double dy = std::fabs(ndi->getBottom() - ndi->getOrigBottom());
  double old_disp = dx + dy;
  dx = std::fabs(ndj->getLeft() - ndj->getOrigLeft());
  dy = std::fabs(ndj->getBottom() - ndj->getOrigBottom());
  old_disp += dx + dy;

  dx = std::fabs(ndj->getLeft() - ndi->getOrigLeft());
  dy = std::fabs(ndj->getBottom() - ndi->getOrigBottom());
  double new_disp = dx + dy;
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

  // Targets are centers, but computation is with left and bottom...
  target_xi -= 0.5 * ndi->getWidth();
  target_yi -= 0.5 * ndi->getHeight();

  target_xj -= 0.5 * ndj->getWidth();
  target_yj -= 0.5 * ndj->getHeight();

  double dx = std::fabs(ndi->getLeft() - ndi->getOrigLeft());
  double dy = std::fabs(ndi->getBottom() - ndi->getOrigBottom());
  double old_disp = dx + dy;
  dx = std::fabs(ndj->getLeft() - ndj->getOrigLeft());
  dy = std::fabs(ndj->getBottom() - ndj->getOrigBottom());
  old_disp += dx + dy;

  dx = std::fabs(target_xi - ndi->getOrigLeft());
  dy = std::fabs(target_yi - ndi->getOrigBottom());
  double new_disp = dx + dy;
  dx = std::fabs(target_xj - ndj->getOrigLeft());
  dy = std::fabs(target_yj - ndj->getOrigBottom());
  new_disp += dx + dy;

  // +ve means improvement.
  return old_disp - new_disp;
}

}  // namespace dpo
