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

#include "FlexRP.h"
#include "frProfileTask.h"

namespace drt {

void FlexRP::init()
{
  ProfileTask profile("RP:init");

  const auto bottomLayerNum = tech_->getBottomLayerNum();
  const auto topLayerNum = tech_->getTopLayerNum();

  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (tech_->getLayer(lNum)->getType() != dbTechLayerType::ROUTING) {
      continue;
    }
    tech_->via2ViaForbiddenLen_.push_back({});
    tech_->via2ViaPrlLen_.push_back({});
    tech_->viaForbiddenTurnLen_.push_back({});
    tech_->viaForbiddenPlanarLen_.push_back({});
    tech_->line2LineForbiddenLen_.push_back({});
    tech_->viaForbiddenThrough_.push_back({});
    for (auto& ndr : tech_->nonDefaultRules_) {
      ndr->via2ViaForbiddenLen_.push_back({});
      ndr->viaForbiddenTurnLen_.push_back({});
    }
  }
}

}  // namespace drt
