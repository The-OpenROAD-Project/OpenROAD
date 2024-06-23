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
////////////////////////////////////////////////////////////////////////////////
#include "detailed_orient.h"

#include <boost/tokenizer.hpp>

#include "architecture.h"
#include "detailed_manager.h"
#include "detailed_segment.h"
#include "orientation.h"
#include "symmetry.h"
#include "utility.h"
#include "utl/Logger.h"

using utl::DPO;

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
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
  std::fill(edgeMask_.begin(), edgeMask_.end(), traversal_);

  bool doFlip = false;
  for (size_t i = 1; i < args.size(); i++) {
    if (args[i] == "-f") {
      doFlip = true;
    }
  }

  mgrPtr_->getLogger()->info(DPO, 380, "Cell flipping.");
  double hpwl_x, hpwl_y;
  double init_hpwl = Utility::hpwl(network_, hpwl_x, hpwl_y);

  // Orient cells correctly for each row.
  int changed = 0;
  int errors = orientCells(changed);
  if (errors != 0) {
    mgrPtr_->getLogger()->warn(
        DPO,
        381,
        "Encountered {:d} issues when orienting cells for rows.",
        errors);
  }
  mgrPtr_->getLogger()->info(
      DPO,
      382,
      "Changed {:d} cell orientations for row compatibility.",
      changed);

  // Optionally perform cell flipping.
  if (doFlip) {
    int nflips = flipCells();
    mgrPtr_->getLogger()->info(DPO, 383, "Performed {:d} cell flips.", nflips);
  }

  double curr_hpwl = Utility::hpwl(network_, hpwl_x, hpwl_y);
  double curr_imp = (((init_hpwl - curr_hpwl) / init_hpwl) * 100.);
  mgrPtr_->getLogger()->info(
      DPO,
      384,
      "End of flipping; objective is {:.6e}, improvement is {:.2f} percent.",
      curr_hpwl,
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
        unsigned origOrient = ndi->getCurrOrient();
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
        if (origOrient != ndi->getCurrOrient()) {
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
  // Takes a multi height cell and fixes its orientation so
  // that it is correct/agrees with the power stripes.
  // Return true is orientation is okay, otherwise false to
  // indicate some sort of problem.
  //
  // Is this correct?
  bool flip = false;
  if (arch_->powerCompatible(ndi, arch_->getRow(row), flip)) {
    if (flip) {
      // Flip the pins.
      for (Pin* pin : ndi->getPins()) {
        pin->setOffsetY(pin->getOffsetY() * (-1));
      }
      // I'm not sure the following is correct, but I am going
      // to change the orientation the same way I would for a
      // single height cell when flipping about X.
      switch (ndi->getCurrOrient()) {
        case Orientation_N:
          ndi->setCurrOrient(Orientation_FS);
          break;
        case Orientation_FN:
          ndi->setCurrOrient(Orientation_S);
          break;
        case Orientation_FS:
          ndi->setCurrOrient(Orientation_N);
          break;
        case Orientation_S:
          ndi->setCurrOrient(Orientation_FN);
          break;
        default:
          return false;
          break;
      }
      return true;
    }

    // No need to flip.
    return true;
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

  unsigned rowOri = arch_->getRow(row)->getOrient();
  unsigned cellOri = ndi->getCurrOrient();

  if (rowOri == Orientation_N || rowOri == Orientation_FN) {
    if (cellOri == Orientation_N || cellOri == Orientation_FN) {
      return true;
    }

    if (cellOri == Orientation_FS) {
      for (Pin* pin : ndi->getPins()) {
        pin->setOffsetY(pin->getOffsetY() * (-1));
      }
      ndi->setCurrOrient(Orientation_N);
      return true;
    }
    if (cellOri == Orientation_S) {
      for (Pin* pin : ndi->getPins()) {
        pin->setOffsetY(pin->getOffsetY() * (-1));
      }
      ndi->setCurrOrient(Orientation_FN);
      return true;
    }
    return false;
  }
  if (rowOri == Orientation_FS || Orientation_S) {
    if (cellOri == Orientation_FS || cellOri == Orientation_S) {
      return true;
    }

    if (cellOri == Orientation_N) {
      for (Pin* pin : ndi->getPins()) {
        pin->setOffsetY(pin->getOffsetY() * (-1));
      }
      ndi->setCurrOrient(Orientation_FS);
      return true;
    }
    if (cellOri == Orientation_FN) {
      for (Pin* pin : ndi->getPins()) {
        pin->setOffsetY(pin->getOffsetY() * (-1));
      }
      ndi->setCurrOrient(Orientation_S);
      return true;
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
  double lx, rx;

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

          double x
              = ndj->getLeft() + 0.5 * ndj->getWidth() + pinj->getOffsetX();
          oldMinX = std::min(oldMinX, x);
          oldMaxX = std::max(oldMaxX, x);

          if (ndj == ndi) {
            x = ndj->getLeft() + 0.5 * ndj->getWidth()
                - pinj->getOffsetX();  // flipped.
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
      lx = (ndl == nullptr) ? segment->getMinX() : (ndl->getRight());
      if (ndl) {
        arch_->getCellPadding(ndl, leftPadding, rightPadding);
        lx += rightPadding;
      }
      rx = (ndr == nullptr) ? segment->getMaxX() : (ndr->getLeft());
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

      // Check potential violation due to edge spacing.
      lx = (ndl == nullptr) ? segment->getMinX() : (ndl->getRight());
      if (ndl) {
        lx += arch_->getCellSpacingUsingTable(ndl->getRightEdgeType(),
                                              ndi->getRightEdgeType());
      }
      rx = (ndr == nullptr) ? segment->getMaxX() : (ndr->getLeft());
      if (ndr) {
        rx -= arch_->getCellSpacingUsingTable(ndi->getLeftEdgeType(),
                                              ndr->getLeftEdgeType());
      }
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

      // Update pin offsets.
      for (Pin* pin : ndi->getPins()) {
        pin->setOffsetX(pin->getOffsetX() * (-1));
      }
      // Update/swap edge types.
      ndi->swapEdgeTypes();
      // Update/swap paddings.
      arch_->getCellPadding(ndi, leftPadding, rightPadding);
      arch_->addCellPadding(ndi, rightPadding, leftPadding);
      // Update the orientation.
      switch (ndi->getCurrOrient()) {
        case Orientation_N:
          ndi->setCurrOrient(Orientation_FN);
          break;
        case Orientation_S:
          ndi->setCurrOrient(Orientation_FS);
          break;
        case Orientation_FN:
          ndi->setCurrOrient(Orientation_N);
          break;
        case Orientation_FS:
          ndi->setCurrOrient(Orientation_S);
          break;
        default:
          // ?
          break;
      }

      ++nflips;
    }
  }
  return nflips;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedOrient::orientAdjust(Node* ndi, unsigned newOri)
{
  unsigned curOri = ndi->getCurrOrient();
  if (curOri == newOri) {
    return false;
  }

  // Determine how pins need to be flipped.  I guess the easiest thing to do it
  // to first return the node to the N orientation and then figure out how to
  // get it into the new orientation!
  int mY = 1;  // Multiplier to adjust pin offsets for a flip around the X-axis.
  int mX = 1;  // Multiplier to adjust pin offsets for a flip around the Y-axis.

  switch (curOri) {
    case Orientation_N:
      break;
    case Orientation_S:
      mX *= -1;
      mY *= -1;
      break;
    case Orientation_FS:
      mY *= -1;
      break;
    case Orientation_FN:
      mX *= -1;
      break;
    default:
      break;
  }

  // Here, assume the cell is in the North Orientation...
  switch (newOri) {
    case Orientation_N:
      break;
    case Orientation_S:
      mX *= -1;
      mY *= -1;
      break;
    case Orientation_FS:
      mY *= -1;
      break;
    case Orientation_FN:
      mX *= -1;
      break;
    default:
      break;
  }

  for (Pin* pin : ndi->getPins()) {
    if (mX == -1) {
      pin->setOffsetX(pin->getOffsetX() * (double) mX);
    }
    if (mY == -1) {
      pin->setOffsetY(pin->getOffsetY() * (double) mY);
    }
  }
  ndi->setCurrOrient(newOri);

  if (mX == -1) {
    ndi->swapEdgeTypes();
  }
  return false;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
unsigned DetailedOrient::orientFind(Node* ndi, int row)
{
  // Given a node, determine a valid orientation for the node if the node is
  // placed into the specified row.  Actually, we could just return the row's
  // orientation, but this might be a little smarter if cells have been flipped
  // around the Y-axis previously to improve WL...

  unsigned rowOri = arch_->getRow(row)->getOrient();
  unsigned cellOri = ndi->getCurrOrient();

  if (rowOri == Orientation_N || rowOri == Orientation_FN) {
    if (cellOri == Orientation_N || cellOri == Orientation_FN) {
      return cellOri;
    }
    if (cellOri == Orientation_FS) {
      return Orientation_N;
    }
    if (cellOri == Orientation_S) {
      return Orientation_FN;
    }
  } else if (rowOri == Orientation_FS || rowOri == Orientation_S) {
    if (cellOri == Orientation_FS || cellOri == Orientation_S) {
      return cellOri;
    }
    if (cellOri == Orientation_N) {
      return Orientation_FS;
    }
    if (cellOri == Orientation_FN) {
      return Orientation_S;
    }
  }
  return rowOri;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
bool DetailedOrient::isLegalSym(unsigned rowOri,
                                unsigned siteSym,
                                unsigned cellOri)
{
  // Messy...
  if (siteSym == Symmetry_Y) {
    if (rowOri == Orientation_N) {
      if (cellOri == Orientation_N || cellOri == Orientation_FN) {
        return true;
      }
      return false;
    }
    if (rowOri == Orientation_FS) {
      if (cellOri == Orientation_S || cellOri == Orientation_FS) {
        return true;
      }
      return false;
    }
    // XXX: Odd...
    return false;
  }
  if (siteSym == Symmetry_X) {
    if (rowOri == Orientation_N) {
      if (cellOri == Orientation_N || cellOri == Orientation_FS) {
        return true;
      }
      return false;
    }
    if (rowOri == Orientation_FS) {
      if (cellOri == Orientation_N || cellOri == Orientation_FS) {
        return true;
      }
      return false;
    }
    // XXX: Odd...
    return false;
  }
  if (siteSym == (Symmetry_X | Symmetry_Y)) {
    if (rowOri == Orientation_N) {
      if (cellOri == Orientation_N || cellOri == Orientation_FN
          || cellOri == Orientation_S || cellOri == Orientation_FS) {
        return true;
      }
      return false;
    }
    if (rowOri == Orientation_FS) {
      if (cellOri == Orientation_N || cellOri == Orientation_FN
          || cellOri == Orientation_S || cellOri == Orientation_FS) {
        return true;
      }
      return false;
    }
    // XXX: Odd...
    return false;
  }
  if (siteSym == Symmetry_UNKNOWN) {
    if (rowOri == Orientation_N) {
      if (cellOri == Orientation_N) {
        return true;
      }
      return false;
    }
    if (rowOri == Orientation_FS) {
      if (cellOri == Orientation_FS) {
        return true;
      }
      return false;
    }
    // XXX: Odd...
    return false;
  }
  if (siteSym == Symmetry_ROT90) {
    // XXX: Not handled.
    return true;
  }
  // siteSym = X Y ROT90... Anything is okay.
  return true;
}

}  // namespace dpo
