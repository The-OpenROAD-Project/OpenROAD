/* Authors: Lutong Wang and Bangqi Xu */
/*
 * Copyright (c) 2019, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "db/obj/frInst.h"

#include "frBlock.h"
#include "frMaster.h"
using namespace fr;

Rect frInst::getBBox() const
{
  Rect box = getMaster()->getBBox();
  dbTransform xform = getTransform();
  Point s(box.xMax(), box.yMax());
  updateXform(xform, s);
  xform.apply(box);
  return box;
}

Rect frInst::getBoundaryBBox() const
{
  Rect box = getMaster()->getDieBox();
  dbTransform xform = getTransform();
  Point s(box.xMax(), box.yMax());
  updateXform(xform, s);
  xform.apply(box);
  return box;
}

dbTransform frInst::getUpdatedXform(bool noOrient) const
{
  dbTransform xfm = getTransform();
  Rect mbox = getMaster()->getDieBox();
  Point origin(mbox.xMin(), mbox.yMin());
  dbTransform(xfm.getOrient(), Point(0, 0)).apply(origin);
  Point offset(xfm.getOffset());
  offset.addX(origin.getX());
  offset.addY(origin.getY());
  xfm.setOffset(offset);
  if (!noOrient) {
    Point s(mbox.xMax(), mbox.yMax());
    updateXform(xfm, s);
  } else {
    xfm.setOrient(dbOrientType(dbOrientType::R0));
  }
  return xfm;
}

// Adjust the transform so that when applied to an inst, the origin is in the
// ll corner of the transformed inst
void frInst::updateXform(dbTransform& xform, Point& size)
{
  Point p = xform.getOffset();
  switch (xform.getOrient()) {
    case dbOrientType::R90:
      p.addX(size.getY());
      break;
    case dbOrientType::R180:
      p.addX(size.getX());
      p.addY(size.getY());
      break;
    case dbOrientType::R270:
      p.addY(size.getX());
      break;
    case dbOrientType::MY:
      p.addX(size.getX());
      break;
    case dbOrientType::MXR90:
      break;
    case dbOrientType::MX:
      p.addY(size.getY());
      break;
    case dbOrientType::MYR90:
      p.addX(size.getY());
      p.addY(size.getX());
      break;
    // case R0: == default
    default:
      break;
  }
  xform.setOffset(p);
}

frInstTerm* frInst::getInstTerm(const std::string& name)
{
  for (auto& it : instTerms_) {
    if (it->getTerm()->getName() == name)
      return it.get();
  }
  return nullptr;
}
