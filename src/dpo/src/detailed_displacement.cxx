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
  std::fill(tot_.begin(), tot_.end(), 0.0);
  for (auto ndi : mgrPtr_->getSingleHeightCells()) {
    const double dx = std::fabs(ndi->getLeft().v - ndi->getOrigLeft().v);
    const double dy = std::fabs(ndi->getBottom().v - ndi->getOrigBottom().v);
    tot_[1] += dx + dy;
  }
  for (size_t s = 2; s < mgrPtr_->getNumMultiHeights(); s++) {
    for (size_t i = 0; i < mgrPtr_->getMultiHeightCells(s).size(); i++) {
      const Node* ndi = mgrPtr_->getMultiHeightCells(s)[i];

      const double dx = std::fabs(ndi->getLeft().v - ndi->getOrigLeft().v);
      const double dy = std::fabs(ndi->getBottom().v - ndi->getOrigBottom().v);
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
double DetailedDisplacement::delta(const Journal& journal)
{
  // Given a list of nodes with their old positions and new positions, compute
  // the change in displacement.  Note that cell orientation is not relevant.

  std::fill(del_.begin(), del_.end(), 0.0);

  // Put cells into their "old positions and orientations".
  const auto& changes = journal.getActions();
  for (int i = changes.size() - 1; i >= 0; i--) {
    mgrPtr_->undo(changes[i], true);
  }

  for (const auto ndi : journal.getAffectedNodes()) {
    const int spanned = std::lround(ndi->getHeight().v / singleRowHeight_);

    const double dx = std::fabs(ndi->getLeft().v - ndi->getOrigLeft().v);
    const double dy = std::fabs(ndi->getBottom().v - ndi->getOrigBottom().v);

    del_[spanned] += (dx + dy);
  }

  // Put cells into their "new positions and orientations".
  for (const auto& change : changes) {
    mgrPtr_->redo(change, true);
  }

  for (const auto ndi : journal.getAffectedNodes()) {
    const double dx = std::fabs(ndi->getLeft().v - ndi->getOrigLeft().v);
    const double dy = std::fabs(ndi->getBottom().v - ndi->getOrigBottom().v);

    const int spanned = arch_->getCellHeightInRows(ndi);
    del_[spanned] -= (dx + dy);
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
  new_x -= 0.5 * ndi->getWidth().v;
  new_y -= 0.5 * ndi->getHeight().v;

  double dx = std::fabs(ndi->getLeft().v - ndi->getOrigLeft().v);
  double dy = std::fabs(ndi->getBottom().v - ndi->getOrigBottom().v);
  const double old_disp = dx + dy;

  dx = std::fabs(new_x - ndi->getOrigLeft().v);
  dy = std::fabs(new_y - ndi->getOrigBottom().v);
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
  double dx = std::fabs(ndi->getLeft().v - ndi->getOrigLeft().v);
  double dy = std::fabs(ndi->getBottom().v - ndi->getOrigBottom().v);
  double old_disp = dx + dy;
  dx = std::fabs(ndj->getLeft().v - ndj->getOrigLeft().v);
  dy = std::fabs(ndj->getBottom().v - ndj->getOrigBottom().v);
  old_disp += dx + dy;

  dx = std::fabs(ndj->getLeft().v - ndi->getOrigLeft().v);
  dy = std::fabs(ndj->getBottom().v - ndi->getOrigBottom().v);
  double new_disp = dx + dy;
  dx = std::fabs(ndi->getLeft().v - ndj->getOrigLeft().v);
  dy = std::fabs(ndi->getBottom().v - ndj->getOrigBottom().v);
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
  target_xi -= 0.5 * ndi->getWidth().v;
  target_yi -= 0.5 * ndi->getHeight().v;

  target_xj -= 0.5 * ndj->getWidth().v;
  target_yj -= 0.5 * ndj->getHeight().v;

  double dx = std::fabs(ndi->getLeft().v - ndi->getOrigLeft().v);
  double dy = std::fabs(ndi->getBottom().v - ndi->getOrigBottom().v);
  double old_disp = dx + dy;
  dx = std::fabs(ndj->getLeft().v - ndj->getOrigLeft().v);
  dy = std::fabs(ndj->getBottom().v - ndj->getOrigBottom().v);
  old_disp += dx + dy;

  dx = std::fabs(target_xi - ndi->getOrigLeft().v);
  dy = std::fabs(target_yi - ndi->getOrigBottom().v);
  double new_disp = dx + dy;
  dx = std::fabs(target_xj - ndj->getOrigLeft().v);
  dy = std::fabs(target_yj - ndj->getOrigBottom().v);
  new_disp += dx + dy;

  // +ve means improvement.
  return old_disp - new_disp;
}

}  // namespace dpo
