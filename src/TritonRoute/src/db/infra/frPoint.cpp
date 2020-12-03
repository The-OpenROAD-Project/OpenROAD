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

#include "db/infra/frPoint.h"
#include "db/infra/frTransform.h"

using namespace std;
using namespace fr;

void frPoint::transform(const frTransform &xform) {
  frCoord tmpX = 0;
  frCoord tmpY = 0;
  //cout <<xform.orient() <<endl;
  switch(xform.orient()) {
    case frcR90 :
      tmpX = xform.xOffset() - yCoord;
      tmpY = xform.yOffset() + xCoord;
      break;
    case frcR180 :
      tmpX = xform.xOffset() - xCoord;
      tmpY = xform.yOffset() - yCoord;
      break;
    case frcR270 :
      tmpX = xform.xOffset() + yCoord;
      tmpY = xform.yOffset() - xCoord;
      break;
    case frcMY :
      tmpX = xform.xOffset() - xCoord;
      tmpY = xform.yOffset() + yCoord;
      break;
    case frcMYR90 : // MY, rotate, then shift 
      tmpX = xform.xOffset() - yCoord;
      tmpY = xform.yOffset() - xCoord;
      break;
    case frcMX :
      tmpX = xform.xOffset() + xCoord;
      tmpY = xform.yOffset() - yCoord;
      break;
    case frcMXR90 : // MX, rotate, then shift
      tmpX = xform.xOffset() + yCoord;
      tmpY = xform.yOffset() + xCoord;
      break;
    // frcR0
    default :
      tmpX = xform.xOffset() + xCoord;
      tmpY = xform.yOffset() + yCoord;
      break;
  }
  xCoord = tmpX;
  yCoord = tmpY;
}
