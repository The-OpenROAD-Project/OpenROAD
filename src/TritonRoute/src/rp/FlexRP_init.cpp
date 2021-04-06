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

#include <iostream>
#include <sstream>

#include "FlexRP.h"
#include "db/infra/frTime.h"
#include "frProfileTask.h"
#include "gc/FlexGC.h"

using namespace std;
using namespace fr;

void FlexRP::init()
{
  ProfileTask profile("RP:init");

  vector<pair<frCoord, frCoord>> forbiddenRanges;
  vector<vector<pair<frCoord, frCoord>>> eightForbiddenRanges(8,
                                                              forbiddenRanges);
  vector<vector<pair<frCoord, frCoord>>> fourForbiddenRanges(4,
                                                             forbiddenRanges);
  vector<bool> fourForbidden(4, false);

  auto bottomLayerNum = getDesign()->getTech()->getBottomLayerNum();
  auto topLayerNum = getDesign()->getTech()->getTopLayerNum();

  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (tech_->getLayer(lNum)->getType() != frLayerTypeEnum::ROUTING) {
      continue;
    }
    tech_->via2ViaForbiddenLen.push_back(eightForbiddenRanges);
    tech_->via2ViaForbiddenOverlapLen.push_back(eightForbiddenRanges);
    tech_->viaForbiddenTurnLen.push_back(fourForbiddenRanges);
    tech_->viaForbiddenPlanarLen.push_back(fourForbiddenRanges);
    tech_->line2LineForbiddenLen.push_back(fourForbiddenRanges);
    tech_->viaForbiddenThrough.push_back(fourForbidden);
    for (auto& ndr : tech_->nonDefaultRules) {
      ndr->via2ViaForbiddenLen.push_back(eightForbiddenRanges);
      ndr->viaForbiddenTurnLen.push_back(fourForbiddenRanges);
    }
  }

}
