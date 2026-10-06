// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "heatMapPlacementDensity.h"

#include <set>
#include <utility>
#include <vector>

#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "web/heatMap.h"

namespace web {

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

  placement_grid_x_ = getGridXSize();
  placement_grid_y_ = getGridYSize();
}

std::optional<int> PlacementDensityDataSource::getPlacementBinSize(
    const char* name) const
{
  auto* block = getBlock();
  if (block == nullptr) {
    return {};
  }
  auto* prop = odb::dbIntProperty::find(block, name);
  if (prop == nullptr || prop->getValue() <= 0) {
    return {};
  }
  return prop->getValue();
}

void PlacementDensityDataSource::restoreSettings(
    const Renderer::Settings& settings)
{
  HeatMapDataSource::restoreSettings(settings);
  // A grid saved by an earlier session, likely for another design, is not a
  // choice made for this one: let global placement still set the default.
  placement_grid_x_ = getGridXSize();
  placement_grid_y_ = getGridYSize();
}

void PlacementDensityDataSource::populateXYGrid()
{
  // Default the grid to the global placement bins, but leave it adjustable
  // (e.g. coarser to save memory on huge designs): once the user picks another
  // size, keep it.
  const auto bin_x = getPlacementBinSize("gpl_bin_size_x");
  const auto bin_y = getPlacementBinSize("gpl_bin_size_y");
  if (bin_x && bin_y && getGridXSize() == placement_grid_x_
      && getGridYSize() == placement_grid_y_) {
    odb::dbBlock* block = getBlock();
    updateGridSizes(block->dbuToMicrons(*bin_x), block->dbuToMicrons(*bin_y));
    placement_grid_x_ = getGridXSize();
    placement_grid_y_ = getGridYSize();
  }

  HeatMapDataSource::populateXYGrid();
}

bool PlacementDensityDataSource::populateMap()
{
  if (getBlock() == nullptr) {
    return false;
  }

  // Collect selected instances if filter is enabled
  const odb::PtrSet<odb::dbInst> selected_insts = getSelectedInsts();
  const bool filter = !selected_insts.empty();

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
      if (filter && selected_insts.find(inst) == selected_insts.end()) {
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

}  // namespace web
