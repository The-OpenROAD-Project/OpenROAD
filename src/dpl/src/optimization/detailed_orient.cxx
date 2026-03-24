// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "detailed_orient.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "boost/tokenizer.hpp"
#include "detailed_manager.h"
#include "dpl/Opendp.h"
#include "infrastructure/Objects.h"
#include "infrastructure/architecture.h"
#include "infrastructure/detailed_segment.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "util/symmetry.h"
#include "util/utility.h"
#include "utl/Logger.h"

using utl::DPL;

namespace dpl {

DetailedOrient::DetailedOrient(Architecture* arch, Network* network)
    : arch_(arch),
      network_(network),
      edgeMask_(network_->getNumEdges(), traversal_)
{
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedOrient::run(DetailedMgr* mgrPtr, const std::string& command)
{
  // A temporary interface to allow for a string which we will decode to create
  // the arguments.
  boost::char_separator<char> separators(" \r\t\n;");
  boost::tokenizer<boost::char_separator<char>> tokens(command, separators);
  std::vector<std::string> args;
  for (const auto& token : tokens) {
    args.push_back(token);
  }
  run(mgrPtr, args);
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void DetailedOrient::run(DetailedMgr* mgrPtr, std::vector<std::string>& args)
{
  // Ensure each movable cell is oriented correctly.  Also consider
  // cell flipping to reduce wirelength.

  mgrPtr_ = mgrPtr;

  traversal_ = 0;
  edgeMask_.resize(network_->getNumEdges());
  std::ranges::fill(edgeMask_, traversal_);

  bool doFlip = false;
  for (size_t i = 1; i < args.size(); i++) {
    if (args[i] == "-f") {
      doFlip = true;
    }
  }

  mgrPtr_->getLogger()->info(DPL, 380, "Cell flipping.");
  uint64_t hpwl_x, hpwl_y;
  int64_t init_hpwl = Utility::hpwl(network_, hpwl_x, hpwl_y);
  if (init_hpwl == 0) {
    return;
  }

  // Orient cells correctly for each row.
  int changed = 0;
  int errors = orientCells(changed);
  if (errors != 0) {
    mgrPtr_->getLogger()->warn(
        DPL,
        381,
        "Encountered {:d} issues when orienting cells for rows.",
        errors);
  }
  mgrPtr_->getLogger()->info(
      DPL,
      382,
      "Changed {:d} cell orientations for row compatibility.",
      changed);

  // Optionally perform cell flipping.
  if (doFlip) {
    int nflips = flipCells();
    mgrPtr_->getLogger()->info(DPL, 383, "Performed {:d} cell flips.", nflips);
  }

  int64_t curr_hpwl = Utility::hpwl(network_, hpwl_x, hpwl_y);
  double curr_imp = (((init_hpwl - curr_hpwl) / (double) init_hpwl) * 100.);
  mgrPtr_->getLogger()->info(
      DPL,
      384,
      "End of flipping; objective is {:.6e}, improvement is {:.2f} percent.",
      (double) curr_hpwl,
      curr_imp);
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
int DetailedOrient::orientCells(int& changed)
{
  // Scan all cells and for those that are movable, ensure their
  // orientation is correct.  We can get the cells either from
  // the segments or from the network (and then figure out what
  // segment/row contains the cell).
  changed = 0;
  int errors = 0;
  for (int i = 0; i < network_->getNumNodes(); i++) {
    Node* ndi = network_->getNode(i);
    if (!(ndi->isTerminal() || ndi->isFixed())) {
      // Figure out the lowest row for the cell.  Not that single
      // height cells are only in one row.
      int bottom = arch_->getNumRows();
      for (int s = 0; s < mgrPtr_->getNumReverseCellToSegs(ndi->getId()); s++) {
        DetailedSeg* segPtr = mgrPtr_->getReverseCellToSegs(ndi->getId())[s];
        bottom = std::min(bottom, segPtr->getRowId());
      }
      if (bottom < arch_->getNumRows()) {
        unsigned origOrient = ndi->getOrient();
        if (arch_->isSingleHeightCell(ndi)) {
          if (!orientSingleHeightCellForRow(ndi, bottom)) {
            ++errors;
          }
        } else if (arch_->isMultiHeightCell(ndi)) {
          if (!orientMultiHeightCellForRow(ndi, bottom)) {
            ++errors;
          }
        } else {
          // ? Whoops.
          ++errors;
        }
        if (origOrient != ndi->getOrient()) {
          ++changed;
        }
      } else {
        // Cell not in a row?  Whoops.
        ++errors;
      }
    }
  }
  return errors;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedOrient::orientMultiHeightCellForRow(Node* ndi, int row)
{
  using odb::dbOrientType;

  // Takes a multi height cell and fixes its orientation so
  // that it is correct/agrees with the power stripes.
  // Return true is orientation is okay, otherwise false to
  // indicate some sort of problem.
  //

  auto* dbMaster = ndi->getDbInst()->getMaster();
  unsigned masterSym = getMasterSymmetry(dbMaster);

  bool flip = false;
  if (arch_->powerCompatible(ndi, arch_->getRow(row), flip)) {
    if (flip) {
      // I'm not sure the following is correct, but I am going
      // to change the orientation the same way I would for a
      // single height cell when flipping about X.
      odb::dbOrientType newOri;
      switch (ndi->getOrient()) {
        case dbOrientType::R0:
          newOri = dbOrientType::MX;
          break;
        case dbOrientType::MY:
          newOri = dbOrientType::R180;
          break;
        case dbOrientType::MX:
          newOri = dbOrientType::R0;
          break;
        case dbOrientType::R180:
          newOri = dbOrientType::MY;
          break;
        default:
          return false;
          break;
      }
      if (isLegalSym(masterSym, newOri)) {
        ndi->adjustCurrOrient(newOri);
        return true;
      }
      return false;
    }

    // No need to flip.
    if (isLegalSym(masterSym, ndi->getOrient())) {
      return true;
    }
    return false;
  }
  // Not power compatible!
  return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedOrient::orientSingleHeightCellForRow(Node* ndi, int row)
{
  // Takes a single height cell and fixes its orientation so
  // that it is correct/agrees with the row to which it is
  // assigned.  If the orientation is successful, then true
  // is returned, otherwise false (indicating a problem).
  //
  // The routine also sets changes the positions of the pins,
  // etc. to agree with the reorientation.  Note: There is no
  // need to swap edge types or cell paddings since this
  // routine is only going to swap using X-symmetry.

  if (!arch_->isSingleHeightCell(ndi)) {
    // Must be a single height cell.
    return false;
  }

  const unsigned rowOri = arch_->getRow(row)->getOrient();
  const unsigned cellOri = ndi->getOrient();

  auto* dbMaster = ndi->getDbInst()->getMaster();
  unsigned masterSym = getMasterSymmetry(dbMaster);

  using odb::dbOrientType;

  if (rowOri == dbOrientType::R0 || rowOri == dbOrientType::MY) {
    if (cellOri == dbOrientType::R0 || cellOri == dbOrientType::MY) {
      return isLegalSym(masterSym, cellOri);
    }

    if (cellOri == dbOrientType::MX) {
      ndi->adjustCurrOrient(dbOrientType::R0);
      return true;
    }
    if (cellOri == dbOrientType::R180) {
      if (isLegalSym(masterSym, dbOrientType::MY)) {
        ndi->adjustCurrOrient(dbOrientType::MY);
        return true;
      }
      return false;
    }
    return false;
  }
  if (rowOri == dbOrientType::MX || rowOri == dbOrientType::R180) {
    if (cellOri == dbOrientType::MX || cellOri == dbOrientType::R180) {
      return isLegalSym(masterSym, cellOri);
    }

    if (cellOri == dbOrientType::R0) {
      if (isLegalSym(masterSym, dbOrientType::MX)) {
        ndi->adjustCurrOrient(dbOrientType::MX);
        return true;
      }
      return false;
    }
    if (cellOri == dbOrientType::MY) {
      if (isLegalSym(masterSym, dbOrientType::R180)) {
        ndi->adjustCurrOrient(dbOrientType::R180);
        return true;
      }
      return false;
    }
    return false;
  }
  return false;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
int DetailedOrient::flipCells()
{
  // Scan segments and consider flipping single height cells to reduce
  // wire length.  We only flip if the row supports SYMMETRY_Y.  We
  // assume the cells are already properly oriented for the row.

  int leftPadding, rightPadding;
  DbuX lx, rx;

  int nflips = 0;
  for (int s = 0; s < mgrPtr_->getNumSegments(); s++) {
    DetailedSeg* segment = mgrPtr_->getSegment(s);

    int row = segment->getRowId();

    if ((arch_->getRow(row)->getSymmetry() & Symmetry_Y) == 0) {
      continue;
    }

    const std::vector<Node*>& nodes
        = mgrPtr_->getCellsInSeg(segment->getSegId());
    for (int i = 0; i < nodes.size(); i++) {
      Node* ndi = nodes[i];
      // Only consider single height cells.
      if (!arch_->isSingleHeightCell(ndi)) {
        continue;
      }

      // Find hpwl in current and flipped orientation.  To do this,
      // we only need to play with the pin offsets in X-direction.
      double oldHpwl = 0.;
      double newHpwl = 0.;
      ++traversal_;
      for (int pi = 0; pi < ndi->getPins().size(); pi++) {
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

        double oldMinX = std::numeric_limits<double>::max();
        double oldMaxX = std::numeric_limits<double>::lowest();

        double newMinX = std::numeric_limits<double>::max();
        double newMaxX = std::numeric_limits<double>::lowest();

        for (Pin* pinj : edi->getPins()) {
          Node* ndj = pinj->getNode();

          double x = ndj->getLeft().v + 0.5 * ndj->getWidth().v
                     + pinj->getOffsetX().v;
          oldMinX = std::min(oldMinX, x);
          oldMaxX = std::max(oldMaxX, x);

          if (ndj == ndi) {
            x = ndj->getLeft().v + 0.5 * ndj->getWidth().v
                - pinj->getOffsetX().v;  // flipped.
          }
          newMinX = std::min(newMinX, x);
          newMaxX = std::max(newMaxX, x);
        }
        oldHpwl += oldMaxX - oldMinX;
        newHpwl += newMaxX - newMinX;
      }
      // If no wire length improvement, do not flip.
      if (newHpwl >= oldHpwl) {
        continue;
      }

      // Check potential violation due to padding.
      Node* ndl = (i == 0) ? nullptr : nodes[i - 1];
      Node* ndr = (i == nodes.size() - 1) ? nullptr : nodes[i + 1];
      lx = (ndl == nullptr) ? DbuX{segment->getMinX()} : ndl->getRight();
      if (ndl) {
        arch_->getCellPadding(ndl, leftPadding, rightPadding);
        lx += rightPadding;
      }
      rx = (ndr == nullptr) ? DbuX{segment->getMaxX()} : ndr->getLeft();
      if (ndr) {
        arch_->getCellPadding(ndr, leftPadding, rightPadding);
        rx -= leftPadding;
      }
      // Based on padding, the cell plus its padding must
      // reside within [lx,rx] _after_ the flip.  So,
      // reverse the paddings.
      arch_->getCellPadding(ndi, leftPadding, rightPadding);
      if (ndi->getLeft() - rightPadding < lx
          || ndi->getRight() + leftPadding > rx) {
        continue;
      }
      using odb::dbOrientType;
      dbOrientType orig_orient = ndi->getOrient();
      dbOrientType flipped_orient;
      switch (orig_orient) {
        case dbOrientType::R0:
          flipped_orient = dbOrientType::MY;
          break;
        case dbOrientType::R180:
          flipped_orient = dbOrientType::MX;
          break;
        case dbOrientType::MY:
          flipped_orient = dbOrientType::R0;
          break;
        case dbOrientType::MX:
          flipped_orient = dbOrientType::R180;
          break;
        default:
          continue;
          break;
      }
      ndi->adjustCurrOrient(flipped_orient);
      if (mgrPtr_->hasPlacementViolation(ndi)) {
        ndi->adjustCurrOrient(orig_orient);
        continue;
      }
      // Check potential violation due to edge spacing.
      lx = (ndl == nullptr) ? DbuX{segment->getMinX()} : ndl->getRight();
      rx = (ndr == nullptr) ? DbuX{segment->getMaxX()} : ndr->getLeft();
      // Based on edge spacing, the cell must reside within
      // [lx,rx].
      if (ndi->getWidth() > (rx - lx)) {
        continue;
      }

      // Here, the WL improves if we flip and we appear to be
      // okay with regards to padding and/or edge spacing.
      // So, do the flip.  This requires flipping the pin
      // offsets, the edge types and the paddings.  Finally,
      // we need to change the orientiation.

      // Update/swap paddings.
      arch_->flipCellPadding(ndi);
      ++nflips;
    }
  }
  return nflips;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
unsigned DetailedOrient::orientFind(Node* ndi, int row)
{
  // Given a node, determine a valid orientation for the node if the node is
  // placed into the specified row.  Actually, we could just return the row's
  // orientation, but this might be a little smarter if cells have been flipped
  // around the Y-axis previously to improve WL...

  const unsigned rowOri = arch_->getRow(row)->getOrient();
  const unsigned cellOri = ndi->getOrient();

  using odb::dbOrientType;

  if (rowOri == dbOrientType::R0 || rowOri == dbOrientType::MY) {
    if (cellOri == dbOrientType::R0 || cellOri == dbOrientType::MY) {
      return cellOri;
    }
    if (cellOri == dbOrientType::MX) {
      return dbOrientType::R0;
    }
    if (cellOri == dbOrientType::R180) {
      return dbOrientType::MY;
    }
  } else if (rowOri == dbOrientType::MX || rowOri == dbOrientType::R180) {
    if (cellOri == dbOrientType::MX || cellOri == dbOrientType::R180) {
      return cellOri;
    }
    if (cellOri == dbOrientType::R0) {
      return dbOrientType::MX;
    }
    if (cellOri == dbOrientType::MY) {
      return dbOrientType::R180;
    }
  }
  return rowOri;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
bool DetailedOrient::isLegalSym(unsigned siteSym, unsigned cellOri)
{
  using odb::dbOrientType;
  //  Check for master symmetry
  bool legalSym = false;
  switch (cellOri) {
    case dbOrientType::R0:
      legalSym = true;
      break;
    case dbOrientType::MX:
      legalSym = (siteSym & Symmetry_X) != 0;
      break;
    case dbOrientType::MY:
      legalSym = (siteSym & Symmetry_Y) != 0;
      break;
    case dbOrientType::R180:
      legalSym = (siteSym & Symmetry_X) && (siteSym & Symmetry_Y);
      break;
    case dbOrientType::R90:
    case dbOrientType::R270:
      legalSym = (siteSym & Symmetry_ROT90) != 0;
      break;
    case dbOrientType::MXR90:
    case dbOrientType::MYR90:
      legalSym = (siteSym & Symmetry_ROT90) && (siteSym & Symmetry_X)
                 && (siteSym & Symmetry_Y);
      break;
    default:
      legalSym = false;
      break;
  }

  return legalSym;
}

unsigned DetailedOrient::getMasterSymmetry(odb::dbMaster* master)
{
  unsigned masterSym = 0;
  if (master->getSymmetryX()) {
    masterSym |= Symmetry_X;
  }
  if (master->getSymmetryY()) {
    masterSym |= Symmetry_Y;
  }
  if (master->getSymmetryR90()) {
    masterSym |= Symmetry_ROT90;
  }
  return masterSym;
}

}  // namespace dpl
