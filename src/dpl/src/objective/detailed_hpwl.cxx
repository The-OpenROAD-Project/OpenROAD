// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "detailed_hpwl.h"

#include "optimization/detailed_orient.h"

namespace dpl {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedHPWL::DetailedHPWL(Network* network)
    : DetailedObjective("hpwl"),
      network_(network),
      edgeMask_(network_->getNumEdges(), traversal_)
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedHPWL::init()
{
  traversal_ = 0;
  edgeMask_.resize(network_->getNumEdges());
  std::fill(edgeMask_.begin(), edgeMask_.end(), traversal_);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedHPWL::init(DetailedMgr* mgrPtr, DetailedOrient* orientPtr)
{
  orientPtr_ = orientPtr;
  mgrPtr_ = mgrPtr;
  init();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedHPWL::curr()
{
  uint64_t hpwl = 0.;
  for (int i = 0; i < network_->getNumEdges(); i++) {
    const Edge* edi = network_->getEdge(i);
    const int npins = edi->getNumPins();
    if (npins <= 1 || npins >= skipNetsLargerThanThis_) {
      continue;
    }
    hpwl += edi->hpwl();
  }
  return hpwl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedHPWL::delta(const Journal& journal)
{
  // Given a list of nodes with their old positions and new positions, compute
  // the change in WL. Note that we need to know the orientation information and
  // might need to adjust pin information...
  uint64_t old_wl = 0.;
  uint64_t new_wl = 0.;

  const auto& changes = journal.getActions();
  for (int i = changes.size() - 1; i >= 0; i--) {
    journal.undo(changes[i], true);
  }

  ++traversal_;
  for (const auto ndi : journal.getAffectedNodes()) {
    for (Pin* pini : ndi->getPins()) {
      Edge* edi = pini->getEdge();

      int npins = edi->getNumPins();
      if (npins <= 1 || npins >= skipNetsLargerThanThis_) {
        continue;
      }
      if (edgeMask_[edi->getId()] == traversal_) {
        continue;
      }
      edgeMask_[edi->getId()] = traversal_;
      old_wl += edi->hpwl();
    }
  }

  // Put cells into their "new positions and orientations".
  for (const auto& change : changes) {
    journal.redo(change, true);
  }

  ++traversal_;
  for (const auto ndi : journal.getAffectedNodes()) {
    for (int pi = 0; pi < ndi->getNumPins(); pi++) {
      Pin* pini = ndi->getPins()[pi];

      Edge* edi = pini->getEdge();

      int npins = edi->getNumPins();
      if (npins <= 1 || npins >= skipNetsLargerThanThis_) {
        continue;
      }
      if (edgeMask_[edi->getId()] == traversal_) {
        continue;
      }
      edgeMask_[edi->getId()] = traversal_;
      new_wl += edi->hpwl();
    }
  }
  // +ve means improvement.
  return (double) old_wl - new_wl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
}  // namespace dpl
