// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "detailed_displacement.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "dpl/Opendp.h"
#include "infrastructure/Coordinates.h"
#include "objective/detailed_objective.h"
#include "optimization/detailed_manager.h"
#include "optimization/detailed_orient.h"

namespace dpl {

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
  std::ranges::fill(count_, 0);
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
  std::ranges::fill(tot_, 0);
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

  std::ranges::fill(del_, 0);

  // Put cells into their "old positions and orientations".
  journal.undo(true);

  for (const auto ndi : journal.getAffectedNodes()) {
    const int spanned = (ndi->getHeight() / singleRowHeight_).v;

    const DbuX dx = abs(ndi->getLeft() - ndi->getOrigLeft());
    const DbuY dy = abs(ndi->getBottom() - ndi->getOrigBottom());

    del_[spanned] += (dx.v + dy.v);
  }

  // Put cells into their "new positions and orientations".
  journal.redo(true);

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
}  // namespace dpl
