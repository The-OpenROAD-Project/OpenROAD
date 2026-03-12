// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "heatMapPlacementDensity.h"

#include <utility>
#include <vector>

#include "gui/heatMap.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace gui {

PlacementDensityDataSource::PlacementDensityDataSource(utl::Logger* logger)
    : HeatMapDataSource(logger,
                        "Placement Density",
                        "Placement",
                        "PlacementDensity")
{
  addBooleanSetting(
      "Taps",
      "Include taps and endcaps:",
      [this]() { return include_taps_; },
      [this](bool new_value) { include_taps_ = new_value; });
  addBooleanSetting(
      "Filler",
      "Include fillers:",
      [this]() { return include_filler_; },
      [this](bool new_value) { include_filler_ = new_value; });
  addBooleanSetting(
      "IO",
      "Include IO:",
      [this]() { return include_io_; },
      [this](bool new_value) { include_io_ = new_value; });
}

bool PlacementDensityDataSource::populateMap()
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
      if (!include_filler_ && inst->getMaster()->isFiller()) {
        continue;
      }
      if (!include_taps_
          && (inst->getMaster()->getType() == odb::dbMasterType::CORE_WELLTAP
              || inst->getMaster()->isEndCap())) {
        continue;
      }
      if (!include_io_
          && (inst->getMaster()->isPad() || inst->getMaster()->isCover())) {
        continue;
      }

      if (inst->isHierarchical()) {
        odb::dbTransform child_transform = inst->getTransform();
        child_transform.concat(transform);
        blocks.emplace_back(inst->getChild(), child_transform);
        continue;
      }
      odb::Rect inst_box = inst->getBBox()->getBox();
      transform.apply(inst_box);

      addToMap(inst_box, 100.0);
    }
  }

  return true;
}

void PlacementDensityDataSource::combineMapData(bool base_has_value,
                                                double& base,
                                                const double new_data,
                                                const double data_area,
                                                const double intersection_area,
                                                const double rect_area)
{
  base += new_data * intersection_area / rect_area;
}

void PlacementDensityDataSource::onShow()
{
  HeatMapDataSource::onShow();

  addOwner(getBlock());
}

void PlacementDensityDataSource::onHide()
{
  HeatMapDataSource::onHide();

  removeOwner();
}

void PlacementDensityDataSource::inDbInstCreate(odb::dbInst*)
{
  destroyMap();
}

void PlacementDensityDataSource::inDbInstDestroy(odb::dbInst*)
{
  destroyMap();
}

void PlacementDensityDataSource::inDbInstPlacementStatusBefore(
    odb::dbInst*,
    const odb::dbPlacementStatus&)
{
  destroyMap();
}

void PlacementDensityDataSource::inDbInstSwapMasterBefore(odb::dbInst*,
                                                          odb::dbMaster*)
{
  destroyMap();
}

void PlacementDensityDataSource::inDbInstSwapMasterAfter(odb::dbInst*)
{
  destroyMap();
}

void PlacementDensityDataSource::inDbPreMoveInst(odb::dbInst*)
{
  destroyMap();
}

void PlacementDensityDataSource::inDbPostMoveInst(odb::dbInst*)
{
  destroyMap();
}

}  // namespace gui
