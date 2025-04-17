// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "detailed_displacement.h"

#include <algorithm>
#include <boost/tokenizer.hpp>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <stack>
#include <utility>
#include <vector>

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
  std::fill(tot_.begin(), tot_.end(), 0);
  for (auto ndi : mgrPtr_->getSingleHeightCells()) {
    const DbuX dx = abs(ndi->getLeft() - ndi->getOrigLeft());
    const DbuY dy = abs(ndi->getBottom() - ndi->getOrigBottom());
    tot_[1] += dx.v + dy.v;
  }
  for (size_t s = 2; s < mgrPtr_->getNumMultiHeights(); s++) {
    for (size_t i = 0; i < mgrPtr_->getMultiHeightCells(s).size(); i++) {
      const Node* ndi = mgrPtr_->getMultiHeightCells(s)[i];

      const DbuX dx = abs(ndi->getLeft() - ndi->getOrigLeft());
      const DbuY dy = abs(ndi->getBottom() - ndi->getOrigBottom());
      tot_[s] += dx.v + dy.v;
    }
  }

  double disp = 0.;
  for (size_t i = 0; i < tot_.size(); i++) {
    if (count_[i] != 0) {
      disp += tot_[i] / (double) count_[i];
    }
  }
  disp /= (double) singleRowHeight_.v;
  disp /= (double) nSets_;

  return disp;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedDisplacement::delta(const Journal& journal)
{
  // Given a list of nodes with their old positions and new positions, compute
  // the change in displacement.  Note that cell orientation is not relevant.

  std::fill(del_.begin(), del_.end(), 0);

  // Put cells into their "old positions and orientations".
  const auto& changes = journal.getActions();
  for (int i = changes.size() - 1; i >= 0; i--) {
    mgrPtr_->undo(changes[i], true);
  }

  for (const auto ndi : journal.getAffectedNodes()) {
    const int spanned = (ndi->getHeight() / singleRowHeight_).v;

    const DbuX dx = abs(ndi->getLeft() - ndi->getOrigLeft());
    const DbuY dy = abs(ndi->getBottom() - ndi->getOrigBottom());

    del_[spanned] += (dx.v + dy.v);
  }

  // Put cells into their "new positions and orientations".
  for (const auto& change : changes) {
    mgrPtr_->redo(change, true);
  }

  for (const auto ndi : journal.getAffectedNodes()) {
    const DbuX dx = abs(ndi->getLeft() - ndi->getOrigLeft());
    const DbuY dy = abs(ndi->getBottom() - ndi->getOrigBottom());

    const int spanned = arch_->getCellHeightInRows(ndi);
    del_[spanned] -= (dx.v + dy.v);
  }

  double delta = 0.;
  for (size_t i = 0; i < del_.size(); i++) {
    if (count_[i] != 0) {
      delta += del_[i] / (double) count_[i];
    }
  }
  delta /= (double) singleRowHeight_.v;
  delta /= (double) nSets_;

  // +ve means improvement.
  return delta;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
uint64_t DetailedDisplacement::delta(Node* ndi, DbuX new_x, DbuY new_y)
{
  // Compute change in displacement for moving node to new position.

  // Targets are centers, but computation is with left and bottom...
  new_x -= ndi->getWidth() / 2;
  new_y -= ndi->getHeight() / 2;

  DbuX dx = abs(ndi->getLeft() - ndi->getOrigLeft());
  DbuY dy = abs(ndi->getBottom() - ndi->getOrigBottom());
  const uint64_t old_disp = dx.v + dy.v;

  dx = abs(new_x - ndi->getOrigLeft().v);
  dy = abs(new_y - ndi->getOrigBottom().v);
  const uint64_t new_disp = dx.v + dy.v;

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
uint64_t DetailedDisplacement::delta(Node* ndi, Node* ndj)
{
  // Compute change in wire length for swapping the two nodes.
  DbuX dx = abs(ndi->getLeft() - ndi->getOrigLeft());
  DbuY dy = abs(ndi->getBottom() - ndi->getOrigBottom());
  uint64_t old_disp = dx.v + dy.v;
  dx = abs(ndj->getLeft() - ndj->getOrigLeft());
  dy = abs(ndj->getBottom() - ndj->getOrigBottom());
  old_disp += dx.v + dy.v;

  dx = abs(ndj->getLeft() - ndi->getOrigLeft());
  dy = abs(ndj->getBottom() - ndi->getOrigBottom());
  uint64_t new_disp = dx.v + dy.v;
  dx = abs(ndi->getLeft() - ndj->getOrigLeft());
  dy = abs(ndi->getBottom() - ndj->getOrigBottom());
  new_disp += dx.v + dy.v;

  // +ve means improvement.
  return old_disp - new_disp;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
uint64_t DetailedDisplacement::delta(Node* ndi,
                                     DbuX target_xi,
                                     DbuY target_yi,
                                     Node* ndj,
                                     DbuX target_xj,
                                     DbuY target_yj)
{
  // Compute change in wire length for swapping the two nodes at specified
  // targets.

  // Targets are centers, but computation is with left and bottom...
  target_xi -= ndi->getWidth() / 2;
  target_yi -= ndi->getHeight() / 2;

  target_xj -= ndj->getWidth() / 2;
  target_yj -= ndj->getHeight() / 2;

  DbuX dx = abs(ndi->getLeft() - ndi->getOrigLeft());
  DbuY dy = abs(ndi->getBottom() - ndi->getOrigBottom());
  uint64_t old_disp = dx.v + dy.v;
  dx = abs(ndj->getLeft() - ndj->getOrigLeft());
  dy = abs(ndj->getBottom() - ndj->getOrigBottom());
  old_disp += dx.v + dy.v;

  dx = abs(target_xi - ndi->getOrigLeft());
  dy = abs(target_yi - ndi->getOrigBottom());
  uint64_t new_disp = dx.v + dy.v;
  dx = abs(target_xj - ndj->getOrigLeft());
  dy = abs(target_yj - ndj->getOrigBottom());
  new_disp += dx.v + dy.v;

  // +ve means improvement.
  return old_disp - new_disp;
}

}  // namespace dpo
