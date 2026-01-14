// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "frProfileTask.h"
#include "odb/dbTypes.h"
#include "rp/FlexRP.h"

using odb::dbTechLayerType;

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
