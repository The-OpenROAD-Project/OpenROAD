// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "snapper.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <numeric>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/dbTypes.h"
#include "utl/Logger.h"

namespace mpl {

Snapper::Snapper(utl::Logger* logger) : logger_(logger), inst_(nullptr)
{
}

Snapper::Snapper(utl::Logger* logger, odb::dbInst* inst)
    : logger_(logger), inst_(inst)
{
}

void Snapper::snapMacro()
{
  snap(odb::dbTechLayerDir::VERTICAL);
  snap(odb::dbTechLayerDir::HORIZONTAL);
}

void Snapper::snap(const odb::dbTechLayerDir& target_direction)
{
  LayerDataList layers_data_list = computeLayerDataList(target_direction);

  int origin = target_direction == odb::dbTechLayerDir::VERTICAL
                   ? inst_->getOrigin().x()
                   : inst_->getOrigin().y();

  if (layers_data_list.empty()) {
    const int initial_origin = origin;
    alignWithManufacturingGrid(origin);

    if (!isOriginInsideCore(origin, target_direction)) {
      // Grid rounding can move an initially contained macro past the core.
      const odb::Rect& macro_bbox = inst_->getBBox()->getBox();
      const odb::Rect& core = inst_->getBlock()->getCoreArea();
      const int manufacturing_grid
          = inst_->getDb()->getTech()->getManufacturingGrid();
      const int lower_bound
          = initial_origin
            + (target_direction == odb::dbTechLayerDir::VERTICAL
                   ? core.xMin() - macro_bbox.xMin()
                   : core.yMin() - macro_bbox.yMin());
      const int upper_bound
          = initial_origin
            + (target_direction == odb::dbTechLayerDir::VERTICAL
                   ? core.xMax() - macro_bbox.xMax()
                   : core.yMax() - macro_bbox.yMax());
      const int minimum_origin
          = static_cast<int>(std::ceil(
                lower_bound / static_cast<double>(manufacturing_grid)))
            * manufacturing_grid;
      const int maximum_origin
          = static_cast<int>(std::floor(
                upper_bound / static_cast<double>(manufacturing_grid)))
            * manufacturing_grid;

      if (minimum_origin > maximum_origin) {
        setOrigin(initial_origin, target_direction);
        return;
      }
      origin = std::clamp(origin, minimum_origin, maximum_origin);
    }

    setOrigin(origin, target_direction);
    return;
  }

  const std::vector<int>& lowest_grid_positions
      = layers_data_list[0].available_positions;
  odb::dbITerm* lowest_grid_pin = layers_data_list[0].pins[0];

  const int lowest_pin_center_pos
      = origin + getPinOffset(lowest_grid_pin, target_direction);

  auto closest_pos
      = std::ranges::lower_bound(lowest_grid_positions, lowest_pin_center_pos);

  int starting_position_index
      = std::distance(lowest_grid_positions.begin(), closest_pos);
  if (starting_position_index == lowest_grid_positions.size()) {
    starting_position_index -= 1;
  }

  attemptSnapToExtraPatterns(
      starting_position_index, layers_data_list, target_direction);
}

bool Snapper::isOriginInsideCore(
    const int origin,
    const odb::dbTechLayerDir& target_direction) const
{
  const odb::Rect& macro_bbox = inst_->getBBox()->getBox();
  const odb::Rect& core = inst_->getBlock()->getCoreArea();
  if (core.dx() <= 0 || core.dy() <= 0) {
    return true;
  }

  const int current_origin = target_direction == odb::dbTechLayerDir::VERTICAL
                                 ? inst_->getOrigin().x()
                                 : inst_->getOrigin().y();
  const int displacement = origin - current_origin;

  if (target_direction == odb::dbTechLayerDir::VERTICAL) {
    return macro_bbox.xMin() + displacement >= core.xMin()
           && macro_bbox.xMax() + displacement <= core.xMax();
  }
  return macro_bbox.yMin() + displacement >= core.yMin()
         && macro_bbox.yMax() + displacement <= core.yMax();
}

void Snapper::setOrigin(const int origin,
                        const odb::dbTechLayerDir& target_direction)
{
  if (target_direction == odb::dbTechLayerDir::VERTICAL) {
    inst_->setOrigin(origin, inst_->getOrigin().y());
  } else {
    inst_->setOrigin(inst_->getOrigin().x(), origin);
  }
}

Snapper::LayerDataList Snapper::computeLayerDataList(
    const odb::dbTechLayerDir& target_direction)
{
  TrackGridToPinListMap track_grid_to_pin_list;

  odb::dbBlock* block = inst_->getBlock();

  for (odb::dbITerm* iterm : inst_->getITerms()) {
    if (iterm->getSigType() != odb::dbSigType::SIGNAL) {
      continue;
    }

    for (odb::dbMPin* mpin : iterm->getMTerm()->getMPins()) {
      odb::dbTechLayer* layer = getPinLayer(mpin);

      if (layer->getDirection() != target_direction) {
        continue;
      }

      odb::dbTrackGrid* track_grid = block->findTrackGrid(layer);
      if (track_grid == nullptr) {
        logger_->error(
            utl::MPL, 39, "No track-grid found for layer {}", layer->getName());
      }

      track_grid_to_pin_list[track_grid].push_back(iterm);
    }
  }

  auto compare_pin_center = [&](odb::dbITerm* pin1, odb::dbITerm* pin2) {
    return (target_direction == odb::dbTechLayerDir::VERTICAL
                ? pin1->getBBox().xCenter() < pin2->getBBox().xCenter()
                : pin1->getBBox().yCenter() < pin2->getBBox().yCenter());
  };

  LayerDataList layers_data;
  for (auto& [track_grid, pins] : track_grid_to_pin_list) {
    std::vector<int> positions;
    if (target_direction == odb::dbTechLayerDir::VERTICAL) {
      track_grid->getGridX(positions);
    } else {
      track_grid->getGridY(positions);
    }
    std::ranges::sort(pins, compare_pin_center);
    layers_data.push_back(LayerData{.track_grid = track_grid,
                                    .available_positions = std::move(positions),
                                    .pins = pins});
  }

  auto compare_layer_number = [](LayerData data1, LayerData data2) {
    return (data1.track_grid->getTechLayer()->getNumber()
            < data2.track_grid->getTechLayer()->getNumber());
  };
  std::ranges::sort(layers_data, compare_layer_number);

  return layers_data;
}

odb::dbTechLayer* Snapper::getPinLayer(odb::dbMPin* pin)
{
  return (*pin->getGeometry().begin())->getTechLayer();
}

int Snapper::getPinOffset(odb::dbITerm* pin,
                          const odb::dbTechLayerDir& direction)
{
  int pin_width = 0;
  if (direction == odb::dbTechLayerDir::VERTICAL) {
    pin_width = pin->getBBox().dx();
  } else {
    pin_width = pin->getBBox().dy();
  }

  int pin_to_origin = 0;
  odb::dbMTerm* mterm = pin->getMTerm();
  if (direction == odb::dbTechLayerDir::VERTICAL) {
    pin_to_origin = mterm->getBBox().xMin();
  } else {
    pin_to_origin = mterm->getBBox().yMin();
  }

  int pin_offset = pin_to_origin + (pin_width / 2);

  const odb::dbOrientType& orientation = inst_->getOrient();
  if (direction == odb::dbTechLayerDir::VERTICAL) {
    if (orientation == odb::dbOrientType::MY
        || orientation == odb::dbOrientType::R180) {
      pin_offset = -pin_offset;
    }
  } else {
    if (orientation == odb::dbOrientType::MX
        || orientation == odb::dbOrientType::R180) {
      pin_offset = -pin_offset;
    }
  }

  return pin_offset;
}

int Snapper::getSnappedOrigin(const int position, const int pin_offset)
{
  int origin = position - pin_offset;
  alignWithManufacturingGrid(origin);
  return origin;
}

void Snapper::snapPinToPosition(odb::dbITerm* pin,
                                const int position,
                                const odb::dbTechLayerDir& direction)
{
  setOrigin(getSnappedOrigin(position, getPinOffset(pin, direction)),
            direction);
}

void Snapper::getTrackGridPattern(odb::dbTrackGrid* track_grid,
                                  int pattern_idx,
                                  int& origin,
                                  int& step,
                                  const odb::dbTechLayerDir& target_direction)
{
  int count;
  if (target_direction == odb::dbTechLayerDir::VERTICAL) {
    track_grid->getGridPatternX(pattern_idx, origin, count, step);
  } else {
    track_grid->getGridPatternY(pattern_idx, origin, count, step);
  }
}

void Snapper::attemptSnapToExtraPatterns(
    const int start_index,
    const LayerDataList& layers_data_list,
    const odb::dbTechLayerDir& target_direction)
{
  const int total_attempts = 100;
  const int total_pins = std::accumulate(layers_data_list.begin(),
                                         layers_data_list.end(),
                                         0,
                                         [](int total, const LayerData& data) {
                                           return total + data.pins.size();
                                         });

  odb::dbITerm* snap_pin = layers_data_list[0].pins[0];
  const std::vector<int>& positions = layers_data_list[0].available_positions;
  std::vector<int> legal_positions;
  legal_positions.reserve(total_attempts + 1);
  const int pin_offset = getPinOffset(snap_pin, target_direction);
  // Keep the existing nearest-first order, but cull before moving the macro.
  for (int i = 0; i <= total_attempts; i++) {
    int steps = (i % 2 == 1) ? (i + 1) / 2 : -(i / 2);
    int current_index = start_index + steps;
    if (current_index < 0
        || current_index >= layers_data_list[0].available_positions.size()) {
      continue;
    }
    const int position = positions[current_index];
    if (isOriginInsideCore(getSnappedOrigin(position, pin_offset),
                           target_direction)) {
      legal_positions.push_back(position);
    }
  }
  if (legal_positions.empty()) {
    // Keeping the original location must not bypass mandatory pin-grid rules.
    totalAlignedPins(layers_data_list, target_direction, true);
    return;
  }

  int best_position = legal_positions.front();
  int best_snapped_pins = -1;
  for (const int position : legal_positions) {
    snapPinToPosition(snap_pin, position, target_direction);
    int snapped_pins = totalAlignedPins(layers_data_list, target_direction);

    if (snapped_pins > best_snapped_pins) {
      best_snapped_pins = snapped_pins;
      best_position = position;
      if (best_snapped_pins == total_pins) {
        break;
      }
    }
  }

  snapPinToPosition(snap_pin, best_position, target_direction);

  if (best_snapped_pins != total_pins) {
    totalAlignedPins(layers_data_list, target_direction, true);

    logger_->warn(utl::MPL,
                  2,
                  "Could not align all pins of the macro {} to the track-grid. "
                  "{} out of {} pins were aligned.",
                  inst_->getName(),
                  best_snapped_pins,
                  total_pins);
  }
}

int Snapper::totalAlignedPins(const LayerDataList& layers_data_list,
                              const odb::dbTechLayerDir& direction,
                              bool error_unaligned_right_way_on_grid)
{
  int pins_aligned = 0;

  for (auto& data : layers_data_list) {
    std::vector<int> pin_centers;
    pin_centers.reserve(data.pins.size());

    for (auto& pin : data.pins) {
      pin_centers.push_back(direction == odb::dbTechLayerDir::VERTICAL
                                ? pin->getBBox().xCenter()
                                : pin->getBBox().yCenter());
    }

    int i = 0, j = 0;
    while (i < pin_centers.size() && j < data.available_positions.size()) {
      if (pin_centers[i] == data.available_positions[j]) {
        pins_aligned++;
        i++;
      } else if (pin_centers[i] < data.available_positions[j]) {
        if (error_unaligned_right_way_on_grid
            && data.track_grid->getTechLayer()->isRightWayOnGridOnly()) {
          logger_->error(utl::MPL,
                         5,
                         "Couldn't align pin {} from the RightWayOnGridOnly "
                         "layer {} with the track-grid.",
                         data.pins[i]->getName(),
                         data.track_grid->getTechLayer()->getName());
        }

        i++;
      } else {
        j++;
      }
    }
  }
  return pins_aligned;
}

void Snapper::alignWithManufacturingGrid(int& origin)
{
  const int manufacturing_grid
      = inst_->getDb()->getTech()->getManufacturingGrid();

  origin = std::round(origin / static_cast<double>(manufacturing_grid))
           * manufacturing_grid;
}

}  // namespace mpl
