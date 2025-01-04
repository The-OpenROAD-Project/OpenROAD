//////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, Precision Innovations Inc.
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

#include "heatMapPinDensity.h"

#include <utility>

#include "db_sta/dbNetwork.hh"
#include "odb/db.h"
#include "odb/dbTransform.h"

namespace gui {

PinDensityDataSource::PinDensityDataSource(utl::Logger* logger)
    : RealValueHeatMapDataSource(logger,
                                 "pins",
                                 "Pin Density",
                                 "Pin",
                                 "PinDensity")
{
}

bool PinDensityDataSource::populateMap()
{
  if (getBlock() == nullptr) {
    return false;
  }

  // Iterate through blocks hierarchically to gather the flattened data
  // for this view.
  std::vector<std::pair<odb::dbBlock*, odb::dbTransform>> blocks
      = {{getBlock(), odb::dbTransform()}};

  while (!blocks.empty()) {
    auto [block, transform] = blocks.back();
    blocks.pop_back();

    for (auto* inst : block->getInsts()) {
      if (!inst->getPlacementStatus().isPlaced()) {
        continue;
      }

      if (inst->isHierarchical()) {
        odb::dbTransform child_transform = inst->getTransform();
        child_transform.concat(transform);
        blocks.emplace_back(inst->getChild(), child_transform);
        continue;
      }

      for (odb::dbITerm* iterm : inst->getITerms()) {
        if (iterm->getSigType().isSupply()) {
          continue;
        }

        // Get iterm bbox
        odb::Rect bbox;
        bbox.mergeInit();
        for (auto& [layer, geom_bbox] : iterm->getGeometries()) {
          bbox.merge(geom_bbox);
        }
        if (bbox.isInverted()) {
          continue;
        }

        transform.apply(bbox);
        addToMap(bbox, 1);
      }
    }
  }

  return true;
}

void PinDensityDataSource::combineMapData(bool base_has_value,
                                          double& base,
                                          const double new_data,
                                          const double data_area,
                                          const double intersection_area,
                                          const double rect_area)
{
  base += (new_data / data_area) * intersection_area;
}

void PinDensityDataSource::onShow()
{
  HeatMapDataSource::onShow();

  addOwner(getBlock());
}

void PinDensityDataSource::onHide()
{
  HeatMapDataSource::onHide();

  removeOwner();
}

void PinDensityDataSource::inDbInstCreate(odb::dbInst*)
{
  destroyMap();
}

void PinDensityDataSource::inDbInstCreate(odb::dbInst*, odb::dbRegion*)
{
  destroyMap();
}

void PinDensityDataSource::inDbInstDestroy(odb::dbInst*)
{
  destroyMap();
}

void PinDensityDataSource::inDbInstPlacementStatusBefore(
    odb::dbInst*,
    const odb::dbPlacementStatus&)
{
  destroyMap();
}

void PinDensityDataSource::inDbInstSwapMasterBefore(odb::dbInst*,
                                                    odb::dbMaster*)
{
  destroyMap();
}

void PinDensityDataSource::inDbInstSwapMasterAfter(odb::dbInst*)
{
  destroyMap();
}

void PinDensityDataSource::inDbPreMoveInst(odb::dbInst*)
{
  destroyMap();
}

void PinDensityDataSource::inDbPostMoveInst(odb::dbInst*)
{
  destroyMap();
}

}  // namespace gui
