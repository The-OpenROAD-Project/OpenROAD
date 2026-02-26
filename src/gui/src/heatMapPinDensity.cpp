// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "heatMapPinDensity.h"

#include <utility>
#include <vector>

#include "gui/heatMap.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

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
