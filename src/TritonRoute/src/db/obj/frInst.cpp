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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "db/obj/frInst.h"
#include "frBlock.h"
using namespace std;
using namespace fr;

void frInst::getBBox (frBox &boxIn) const {
  getRefBlock()->getBBox(boxIn);
  frTransform xform;
  getTransform(xform);
  frPoint origin(boxIn.left(), boxIn.bottom());
  origin.transform(frTransform(0, 0, xform.orient()));
  xform.set(xform.xOffset() + origin.x(), xform.yOffset() + origin.y());
  frPoint s(boxIn.right(), boxIn.top());
  xform.updateXform(s);
  boxIn.transform(xform);
}

void frInst::getBoundaryBBox (frBox &boxIn) const {
  getRefBlock()->getBoundaryBBox(boxIn);
  frTransform xform;
  getTransform(xform);
  frPoint origin(boxIn.left(), boxIn.bottom());
  origin.transform(frTransform(0, 0, xform.orient()));
  xform.set(xform.xOffset() + origin.x(), xform.yOffset() + origin.y());
  frPoint s(boxIn.right(), boxIn.top());
  xform.updateXform(s);
  boxIn.transform(xform);
}

void frInst::getUpdatedXform(frTransform &in, bool noOrient) const {
  getTransform(in);
  frBox mbox;
  getRefBlock()->getBoundaryBBox(mbox);
  frPoint origin(mbox.left(), mbox.bottom());
  origin.transform(frTransform(0, 0, in.orient()));
  in.set(in.xOffset() + origin.x(), in.yOffset() + origin.y());
  if (!noOrient) {
    frPoint s(mbox.right(), mbox.top());
    in.updateXform(s);
  } else {
    in.set(frOrient(frcR0));
  }
}
