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

#include "db/taObj/taTrack.h"
#include "db/taObj/taFig.h"
#include "db/taObj/taPin.h"
#include "frDesign.h"

using namespace std;
using namespace fr;

// for pathseg, return overlapped length * #overlaps per interval
// for via, return #overlaps
frUInt4 taTrack::getCost(frCoord x1, frCoord x2, int type, taPinFig* fig) const {
  frUInt4 cost = 0;
  decltype(costPlanar.equal_range(boost::icl::interval<frCoord>::closed(x1,x2))) itRes;
  switch(type) {
    case 0:
      itRes = costPlanar.equal_range(boost::icl::interval<frCoord>::closed(x1,x2));
      break;
    case 1:
      itRes = costVia1.equal_range(boost::icl::interval<frCoord>::closed(x1,x2));
      break;
    case 2:
      itRes = costVia2.equal_range(boost::icl::interval<frCoord>::closed(x1,x2));
      break;
    default:
      ;
  }
  for (auto it = itRes.first; it != itRes.second; ++it) {
    auto &intv = it->first;
    auto &objS = it->second;
    frUInt4 tmpCost = 0;
    frUInt4 len = max(intv.upper() - intv.lower(), 1);
    for (auto &obj: objS) {
      if (obj == nullptr) {
        tmpCost++;
      // only add cost for diff-net
      } else if (obj->typeId() == frcNet) {
        if (fig->getPin()->getGuide()->getNet() != obj) {
          tmpCost++;
        }
      // two taObjs
      } else if (obj->typeId() == tacPathSeg || obj->typeId() == tacVia) {
        auto taObj = static_cast<taPinFig*>(obj);
        if (fig->getPin()->getGuide()->getNet() != taObj->getPin()->getGuide()->getNet()) {
          tmpCost++;
        }
      } else {
        cout <<"Warning: taTrack::getCost unsupported type" <<endl;
      }
    }
    cost += tmpCost * len;
  }
  return cost;
}


