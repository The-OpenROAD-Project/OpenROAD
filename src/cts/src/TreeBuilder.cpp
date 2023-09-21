/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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
//
///////////////////////////////////////////////////////////////////////////////

#include "TreeBuilder.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>

#include "utl/Logger.h"

namespace cts {

using utl::CTS;

// Find one blockage that contains qt
// (x1, y1) is the lower left corner
// (x2, y2) is the upper right corner
odb::dbBlockage* TreeBuilder::findBlockage(const Point<double>& qt,
                                           double scalingUnit,
                                           double& x1,
                                           double& y1,
                                           double& x2,
                                           double& y2)
{
  double qx = qt.getX();
  double qy = qt.getY();

  for (odb::dbBlockage* blockage : db_->getChip()->getBlock()->getBlockages()) {
    odb::dbBox* bbox = blockage->getBBox();
    x1 = bbox->xMin() / scalingUnit;
    y1 = bbox->yMin() / scalingUnit;
    x2 = bbox->xMax() / scalingUnit;
    y2 = bbox->yMax() / scalingUnit;

    bool inside = (qx > x1) && (qx < x2) && (qy > y1) && (qy < y2);
    if (inside) {
      return blockage;
    }
  }
  return nullptr;
}

//
// Legalize one buffer (can be L0, L1, L2, leaf or level buffer)
// bufferLoc needs to in non-dbu units: without wireSegmentUnit multiplier
// bufferName is a string that contains name of buffer master cell
//
Point<double> TreeBuilder::legalizeOneBuffer(Point<double> bufferLoc,
                                             std::string bufferName)
{
  if (options_->getObstructionAware()) {
    odb::dbMaster* libCell = db_->findMaster(bufferName.c_str());
    assert(libCell != nullptr);
    // check if current buffer sits on top of blockage
    double x1, y1, x2, y2;
    const double wireSegmentUnit = techChar_->getLengthUnit();
    odb::dbBlockage* obs
        = findBlockage(bufferLoc, wireSegmentUnit, x1, y1, x2, y2);
    if (obs != nullptr) {
      // x1, y1 are lower left corner of blockage
      // x2, y2 are upper right corner of blockage
      // move buffer to the nearest legal location by snapping it to right,
      // left, top or bottom need to consider cell height and width to avoid any
      // overlap with blockage
      Point<double> newLoc = bufferLoc;
      // first, try snapping it to the left
      double delta = bufferLoc.getX() - x1;
      double minDist = delta;
      newLoc.setX(x1 - ((double) libCell->getWidth() / wireSegmentUnit));
      // second, try snapping it to the right
      delta = x2 - bufferLoc.getX();
      if (delta < minDist) {
        minDist = delta;
        newLoc.setX(x2);
      }
      // third, try snapping it to the bottom
      delta = bufferLoc.getY() - y1;
      if (delta < minDist) {
        minDist = delta;
        newLoc.setX(bufferLoc.getX());
        newLoc.setY(y1 - ((double) libCell->getHeight() / wireSegmentUnit));
      }
      // fourth, try snapping it to the top
      delta = y2 - bufferLoc.getY();
      if (delta < minDist) {
        newLoc.setX(bufferLoc.getX());
        newLoc.setY(y2);
      }
      return newLoc;
    }
  }
  return bufferLoc;
}

}  // namespace cts
