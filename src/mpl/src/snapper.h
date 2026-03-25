// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <vector>

namespace odb {
class dbInst;
class dbITerm;
class dbMPin;
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

  void setMacro(odb::dbInst* inst) { inst_ = inst; }
  void snapMacro();

 private:
  struct LayerData
  {
    odb::dbTrackGrid* track_grid;
    std::vector<int> available_positions;
    // ordered by pin centers
    std::vector<odb::dbITerm*> pins;
  };
  // ordered by TrackGrid layer number
  using LayerDataList = std::vector<LayerData>;
  using TrackGridToPinListMap
      = std::map<odb::dbTrackGrid*, std::vector<odb::dbITerm*>>;

  void snap(const odb::dbTechLayerDir& target_direction);
  void alignWithManufacturingGrid(int& origin);
  void setOrigin(int origin, const odb::dbTechLayerDir& target_direction);
  int totalAlignedPins(const LayerDataList& layers_data_list,
                       const odb::dbTechLayerDir& direction,
                       bool error_unaligned_right_way_on_grid = false);

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
};

}  // namespace mpl
