// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "detailed_hpwl.h"

#include <vector>

#include "detailed_orient.h"

namespace dpo {

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
  double hpwl = 0.;
  Rectangle box;
  for (int i = 0; i < network_->getNumEdges(); i++) {
    const Edge* edi = network_->getEdge(i);

    const int npins = edi->getNumPins();
    if (npins <= 1 || npins >= skipNetsLargerThanThis_) {
      continue;
    }

    box.reset();
    for (const Pin* pinj : edi->getPins()) {
      const Node* ndj = pinj->getNode();

      const double x
          = ndj->getLeft().v + 0.5 * ndj->getWidth().v + pinj->getOffsetX().v;
      const double y = ndj->getBottom().v + 0.5 * ndj->getHeight().v
                       + pinj->getOffsetY().v;

      box.addPt(x, y);
    }

    hpwl += (box.getWidth() + box.getHeight());
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

  double x, y;
  double old_wl = 0.;
  double new_wl = 0.;
  Rectangle old_box, new_box;

  const auto& changes = journal.getActions();
  for (int i = changes.size() - 1; i >= 0; i--) {
    mgrPtr_->undo(changes[i], true);
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

      old_box.reset();
      for (int pj = 0; pj < edi->getNumPins(); pj++) {
        Pin* pinj = edi->getPins()[pj];

        Node* curr = pinj->getNode();

        x = curr->getLeft().v + 0.5 * curr->getWidth().v + pinj->getOffsetX().v;
        y = curr->getBottom().v + 0.5 * curr->getHeight().v
            + pinj->getOffsetY().v;

        old_box.addPt(x, y);
      }

      old_wl += (old_box.getWidth() + old_box.getHeight());
    }
  }

  // Put cells into their "new positions and orientations".
  for (const auto& change : changes) {
    mgrPtr_->redo(change, true);
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

      new_box.reset();
      for (int pj = 0; pj < edi->getNumPins(); pj++) {
        Pin* pinj = edi->getPins()[pj];

        Node* curr = pinj->getNode();

        x = curr->getLeft().v + 0.5 * curr->getWidth().v + pinj->getOffsetX().v;
        y = curr->getBottom().v + 0.5 * curr->getHeight().v
            + pinj->getOffsetY().v;

        new_box.addPt(x, y);
      }

      new_wl += (new_box.getWidth() + new_box.getHeight());
    }
  }
  // +ve means improvement.
  return old_wl - new_wl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedHPWL::delta(Node* ndi, double new_x, double new_y)
{
  // Compute change in wire length for moving node to new position.

  double old_wl = 0.;
  double new_wl = 0.;
  double x, y;
  Rectangle old_box, new_box;

  ++traversal_;
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

    old_box.reset();
    new_box.reset();
    for (int pj = 0; pj < edi->getNumPins(); pj++) {
      Pin* pinj = edi->getPins()[pj];

      Node* ndj = pinj->getNode();

      x = ndj->getLeft().v + 0.5 * ndj->getWidth().v + pinj->getOffsetX().v;
      y = ndj->getBottom().v + 0.5 * ndj->getHeight().v + pinj->getOffsetY().v;

      old_box.addPt(x, y);

      if (ndj == ndi) {
        x = new_x + pinj->getOffsetX().v;
        y = new_y + pinj->getOffsetY().v;
      }

      new_box.addPt(x, y);
    }
    old_wl += (old_box.getWidth() + old_box.getHeight());
    new_wl += (new_box.getWidth() + new_box.getHeight());
  }
  return old_wl - new_wl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedHPWL::getCandidates(std::vector<Node*>& candidates)
{
  candidates = mgrPtr_->getSingleHeightCells();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedHPWL::delta(Node* ndi, Node* ndj)
{
  // Compute change in wire length for swapping the two nodes.

  double old_wl = 0.;
  double new_wl = 0.;
  double x, y;
  Rectangle old_box, new_box;
  Node* nodes[2];
  nodes[0] = ndi;
  nodes[1] = ndj;

  ++traversal_;
  for (int c = 0; c <= 1; c++) {
    Node* ndi = nodes[c];
    for (int pi = 0; pi < ndi->getNumPins(); pi++) {
      Pin* pini = ndi->getPins()[pi];

      Edge* edi = pini->getEdge();

      // int npins = edi->getNumPins();
      int npins = edi->getNumPins();
      if (npins <= 1 || npins >= skipNetsLargerThanThis_) {
        continue;
      }
      if (edgeMask_[edi->getId()] == traversal_) {
        continue;
      }
      edgeMask_[edi->getId()] = traversal_;

      old_box.reset();
      new_box.reset();
      for (int pj = 0; pj < edi->getNumPins(); pj++) {
        Pin* pinj = edi->getPins()[pj];

        Node* ndj = pinj->getNode();

        x = ndj->getLeft().v + 0.5 * ndj->getWidth().v + pinj->getOffsetX().v;
        y = ndj->getBottom().v + 0.5 * ndj->getHeight().v
            + pinj->getOffsetY().v;

        old_box.addPt(x, y);

        if (ndj == nodes[0]) {
          ndj = nodes[1];
        } else if (ndj == nodes[1]) {
          ndj = nodes[0];
        }

        x = ndj->getLeft().v + 0.5 * ndj->getWidth().v + pinj->getOffsetX().v;
        y = ndj->getBottom().v + 0.5 * ndj->getHeight().v
            + pinj->getOffsetY().v;

        new_box.addPt(x, y);
      }

      old_wl += (old_box.getWidth() + old_box.getHeight());
      new_wl += (new_box.getWidth() + new_box.getHeight());
    }
  }
  return old_wl - new_wl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedHPWL::delta(Node* ndi,
                           double target_xi,
                           double target_yi,
                           Node* ndj,
                           double target_xj,
                           double target_yj)
{
  // Compute change in wire length for swapping the two nodes.

  double old_wl = 0.;
  double new_wl = 0.;
  double x, y;
  Rectangle old_box, new_box;
  Node* nodes[2];
  nodes[0] = ndi;
  nodes[1] = ndj;

  ++traversal_;
  for (int c = 0; c <= 1; c++) {
    Node* ndi = nodes[c];
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

      old_box.reset();
      new_box.reset();
      for (Pin* pinj : edi->getPins()) {
        Node* curr = pinj->getNode();

        x = curr->getLeft().v + 0.5 * curr->getWidth().v + pinj->getOffsetX().v;
        y = curr->getBottom().v + 0.5 * curr->getHeight().v
            + pinj->getOffsetY().v;

        old_box.addPt(x, y);

        if (curr == nodes[0]) {
          x = target_xi + pinj->getOffsetX().v;
          y = target_yi + pinj->getOffsetY().v;
        } else if (curr == nodes[1]) {
          x = target_xj + pinj->getOffsetX().v;
          y = target_yj + pinj->getOffsetY().v;
        }

        new_box.addPt(x, y);
      }

      old_wl += (old_box.getWidth() + old_box.getHeight());
      new_wl += (new_box.getWidth() + new_box.getHeight());
    }
  }
  return old_wl - new_wl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
}  // namespace dpo
