// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <vector>

#include "odb/PtrSetMap.h"
#include "odb/geom.h"

namespace odb {
class dbInst;
class dbITerm;
class dbMPin;
class dbSite;
class dbTechLayer;
class dbTechLayerDir;
class dbTrackGrid;
}  // namespace odb

namespace utl {
class Logger;
}

namespace mpl {

class Snapper
{
 public:
  Snapper(utl::Logger* logger);
  Snapper(utl::Logger* logger, odb::dbInst* inst);

  void snapMacro();

  void setMacro(odb::dbInst* inst) { inst_ = inst; }
  void setSnapToSiteGrid(bool snap_to_site_grid);

 private:
  struct LayerData
  {
    odb::dbTrackGrid* track_grid;
    std::vector<int> available_positions;
    // ordered by pin centers
    std::vector<odb::dbITerm*> pins;
  };

  struct SiteGrid
  {
    odb::Point origin;
    odb::dbSite* site{nullptr};
  };

  // ordered by TrackGrid layer number
  using LayerDataList = std::vector<LayerData>;
  using TrackGridToPinListMap
      = odb::PtrMap<odb::dbTrackGrid, std::vector<odb::dbITerm*>>;

  void snapToSites();
  void snapToSites(const odb::dbTechLayerDir& target_direction);
  int nearestSitePosition(const odb::dbTechLayerDir& direction);

  void snapToTracks();
  void snapToTracks(const odb::dbTechLayerDir& target_direction);
  void snapToManufacturingGrid(const odb::dbTechLayerDir& direction);
  int nearestManufacturingGridPosition(int position) const;
  bool hasMultiHeightRows() const;
  SiteGrid findSiteGrid();
  int sitePitch(const odb::dbTechLayerDir& direction) const;
  std::vector<odb::Rect> findObstacles() const;
  bool isLegalPosition() const;
  const LayerDataList& layerDataList(
      const odb::dbTechLayerDir& direction) const;
  int getOrigin(const odb::dbTechLayerDir& direction);
  void setOrigin(int origin, const odb::dbTechLayerDir& direction);
  static int totalPins(const LayerDataList& layers_data_list);
  int totalAlignedPins(const LayerDataList& layers_data_list,
                       const odb::dbTechLayerDir& direction,
                       bool error_unaligned_right_way_on_grid = false);
  void reportUnalignedPins(const LayerDataList& layers_data_list,
                           const odb::dbTechLayerDir& direction,
                           int aligned_pins,
                           int total_pins);

  LayerDataList computeLayerDataList(
      const odb::dbTechLayerDir& target_direction);
  odb::dbTechLayer* getPinLayer(odb::dbMPin* pin);
  void getTrackGridPattern(odb::dbTrackGrid* track_grid,
                           int pattern_idx,
                           int& origin,
                           int& step,
                           const odb::dbTechLayerDir& target_direction);
  int getPinOffset(odb::dbITerm* pin, const odb::dbTechLayerDir& direction);
  void snapPinToPosition(odb::dbITerm* pin,
                         int position,
                         const odb::dbTechLayerDir& direction);
  void attemptSnapToExtraPatterns(int start_index,
                                  const LayerDataList& layers_data_list,
                                  const odb::dbTechLayerDir& target_direction);

  utl::Logger* logger_;
  odb::dbInst* inst_;

  LayerDataList vertical_layers_;
  LayerDataList horizontal_layers_;

  bool snap_to_site_grid_{false};
  SiteGrid site_grid_;
  std::vector<odb::Rect> obstacles_;
};

}  // namespace mpl
