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

void frInst::getBBox(Rect& boxIn) const
{
  getMaster()->getBBox(boxIn);
  dbTransform xform;
  getTransform(xform);
  Point s(boxIn.xMax(), boxIn.yMax());
  updateXform(xform, s);
  xform.apply(boxIn);
}

void frInst::getBoundaryBBox(Rect& boxIn) const
{
  getMaster()->getDieBox(boxIn);
  dbTransform xform;
  getTransform(xform);
  Point s(boxIn.xMax(), boxIn.yMax());
  updateXform(xform, s);
  xform.apply(boxIn);
}

void frInst::getUpdatedXform(dbTransform& in, bool noOrient) const
{
  getTransform(in);
  Rect mbox;
  getMaster()->getDieBox(mbox);
  Point origin(mbox.xMin(), mbox.yMin());
  dbTransform(in.getOrient(), Point(0, 0)).apply(origin);
  Point offset(in.getOffset());
  offset.x() += origin.getX();
  offset.y() += origin.getY();
  in.setOffset(offset);
  if (!noOrient) {
    Point s(mbox.xMax(), mbox.yMax());
    updateXform(in, s);
  } else {
    in.setOrient(dbOrientType(dbOrientType::R0));
  }
}

// Adjust the transform so that when applied to an inst, the origin is in the
// ll corner of the transformed inst
void frInst::updateXform(dbTransform& xform, Point& size)
{
  Point p = xform.getOffset();
  int& x = p.x();
  int& y = p.y();
  switch (xform.getOrient()) {
    case dbOrientType::R90:
      x += size.getY();
      break;
    case dbOrientType::R180:
      x += size.getX();
      y += size.getY();
      break;
    case dbOrientType::R270:
      y += size.getX();
      break;
    case dbOrientType::MY:
      x += size.getX();
      break;
    case dbOrientType::MXR90:
      break;
    case dbOrientType::MX:
      y += size.getY();
      break;
    case dbOrientType::MYR90:
      x += size.getY();
      y += size.getX();
      break;
    // case R0: == default
    default:
      break;
  }
  xform.setOffset(p);
}


  frInstTerm* frInst::getInstTerm(const std::string& name) {
      for (auto& it : instTerms_) {
          if (it->getTerm()->getName() == name)
              return it.get();
      }
      return nullptr;
  }
