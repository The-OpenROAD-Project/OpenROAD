// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "snapper.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/dbTypes.h"
#include "utl/Logger.h"

namespace mpl {

namespace {

constexpr int kTrackSearchAttemptsLimit = 100;
constexpr int kSiteSearchAttemptsLimit = 20;

// Offsets 0, +1, -1, +2, -2, ... so that the search spreads from the start.
int alternatingOffset(const int attempt)
{
  return (attempt % 2 == 1) ? (attempt + 1) / 2 : -(attempt / 2);
}

}  // namespace

Snapper::Snapper(utl::Logger* logger) : logger_(logger), inst_(nullptr)
{
}

Snapper::Snapper(utl::Logger* logger, odb::dbInst* inst)
    : logger_(logger), inst_(inst)
{
}

void Snapper::snapMacro()
{
  loadLayerData();

  if (snap_to_site_grid_) {
    site_grid_ = findSiteGrid();
    snapToSites(odb::dbTechLayerDir::VERTICAL);
    snapToSites(odb::dbTechLayerDir::HORIZONTAL);
  } else {
    snapToTracks(odb::dbTechLayerDir::VERTICAL);
    snapToTracks(odb::dbTechLayerDir::HORIZONTAL);
  }
}

void Snapper::setSnapToSiteGrid(const bool snap_to_site_grid)
{
  snap_to_site_grid_ = snap_to_site_grid;
}

void Snapper::loadLayerData()
{
  vertical_layers_ = computeLayerDataList(odb::dbTechLayerDir::VERTICAL);
  horizontal_layers_ = computeLayerDataList(odb::dbTechLayerDir::HORIZONTAL);
}

// Snaps to the site position that aligns the largest number of pins. We first
// snap the origin to its nearest site and then jump from site to site around
// it looking for the optimal position. When no site position aligns any pin,
// the track-grid snap takes over.
void Snapper::snapToSites(const odb::dbTechLayerDir& target_direction)
{
  const LayerDataList& layers_data_list = layersOf(target_direction);
  if (layers_data_list.empty()) {
    snapToNearestSite(target_direction);
    return;
  }

  const int total_pins = totalPins(layers_data_list);
  const int pitch = sitePitch(target_direction);
  const int origin = getOrigin(target_direction);

  snapToNearestSite(target_direction);
  const int start = getOrigin(target_direction);

  int best_position = start;
  int best_aligned_pins = 0;
  for (int attempt = 0; attempt <= kSiteSearchAttemptsLimit; attempt++) {
    const int position = start + alternatingOffset(attempt) * pitch;
    setOrigin(position, target_direction);

    const int aligned_pins
        = totalAlignedPins(layers_data_list, target_direction);
    if (aligned_pins > best_aligned_pins) {
      best_aligned_pins = aligned_pins;
      best_position = position;
      if (best_aligned_pins == total_pins) {
        break;
      }
    }
  }

  if (best_aligned_pins == 0) {
    logger_->warn(
        utl::MPL,
        82,
        "Could not snap {} to the site grid in {}. No site position "
        "around it aligns any pin with the track-grid. Snapping it "
        "to the track-grid only.",
        inst_->getName(),
        target_direction == odb::dbTechLayerDir::VERTICAL ? "x" : "y");
    setOrigin(origin, target_direction);
    snapToTracks(target_direction);
    return;
  }

  setOrigin(best_position, target_direction);
  reportUnalignedPins(
      layers_data_list, target_direction, best_aligned_pins, total_pins);
}

void Snapper::snapToNearestSite(const odb::dbTechLayerDir& direction)
{
  const int grid_origin = direction == odb::dbTechLayerDir::VERTICAL
                              ? site_grid_.origin.x()
                              : site_grid_.origin.y();
  const int pitch = sitePitch(direction);
  const double sites
      = (getOrigin(direction) - grid_origin) / static_cast<double>(pitch);

  setOrigin(grid_origin + std::lround(sites) * pitch, direction);
}

// Snaps to the location that aligns the largest number of pins. We first align
// the lowest pin, which is closest to the origin, with its nearest track and
// then jump from track to track around it looking for the optimal location.
void Snapper::snapToTracks(const odb::dbTechLayerDir& target_direction)
{
  const LayerDataList& layers_data_list = layersOf(target_direction);
  if (layers_data_list.empty()) {
    snapToManufacturingGrid(target_direction);
    return;
  }

  const std::vector<int>& lowest_grid_positions
      = layers_data_list[0].available_positions;
  odb::dbITerm* lowest_grid_pin = layers_data_list[0].pins[0];

  const int lowest_pin_center_pos
      = getOrigin(target_direction)
        + getPinOffset(lowest_grid_pin, target_direction);

  auto closest_pos
      = std::ranges::lower_bound(lowest_grid_positions, lowest_pin_center_pos);

  int starting_position_index
      = std::distance(lowest_grid_positions.begin(), closest_pos);
  if (starting_position_index == lowest_grid_positions.size()) {
    starting_position_index -= 1;
  }

  snapPinToPosition(lowest_grid_pin,
                    lowest_grid_positions[starting_position_index],
                    target_direction);

  attemptSnapToExtraPatterns(
      starting_position_index, layers_data_list, target_direction);
}

void Snapper::snapToManufacturingGrid(const odb::dbTechLayerDir& direction)
{
  setOrigin(nearestManufacturingGridPosition(getOrigin(direction)), direction);
}

int Snapper::nearestManufacturingGridPosition(const int position) const
{
  const int manufacturing_grid
      = inst_->getDb()->getTech()->getManufacturingGrid();

  return std::round(position / static_cast<double>(manufacturing_grid))
         * manufacturing_grid;
}

// The grid of the smallest core site. Rows of taller sites, such as
// double-height rows, sit on it as well.
Snapper::SiteGrid Snapper::findSiteGrid()
{
  SiteGrid site_grid;
  for (odb::dbRow* row : inst_->getBlock()->getRows()) {
    odb::dbSite* site = row->getSite();
    if (row->getDirection() != odb::dbRowDir::HORIZONTAL
        || site->getClass() != odb::dbSiteClass::CORE) {
      continue;
    }

    if (!site_grid.site || site->getHeight() < site_grid.site->getHeight()) {
      site_grid = {.origin = row->getOrigin(), .site = site};
    }
  }

  if (!site_grid.site) {
    logger_->error(utl::MPL,
                   83,
                   "Could not snap {} to the site grid. The block has no core "
                   "rows.",
                   inst_->getName());
  }

  return site_grid;
}

int Snapper::sitePitch(const odb::dbTechLayerDir& direction) const
{
  return direction == odb::dbTechLayerDir::VERTICAL
             ? site_grid_.site->getWidth()
             : site_grid_.site->getHeight();
}

const Snapper::LayerDataList& Snapper::layersOf(
    const odb::dbTechLayerDir& direction) const
{
  return direction == odb::dbTechLayerDir::VERTICAL ? vertical_layers_
                                                    : horizontal_layers_;
}

int Snapper::getOrigin(const odb::dbTechLayerDir& direction)
{
  return direction == odb::dbTechLayerDir::VERTICAL ? inst_->getOrigin().x()
                                                    : inst_->getOrigin().y();
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
    if (iterm->getSigType().isSupply()) {
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

void Snapper::snapPinToPosition(odb::dbITerm* pin,
                                int position,
                                const odb::dbTechLayerDir& direction)
{
  setOrigin(
      nearestManufacturingGridPosition(position - getPinOffset(pin, direction)),
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
  const int total_pins = totalPins(layers_data_list);

  odb::dbITerm* snap_pin = layers_data_list[0].pins[0];
  const std::vector<int>& positions = layers_data_list[0].available_positions;

  int best_index = start_index;
  int best_snapped_pins = 0;

  for (int i = 0; i <= kTrackSearchAttemptsLimit; i++) {
    const int current_index = start_index + alternatingOffset(i);
    if (current_index < 0 || current_index >= positions.size()) {
      continue;
    }
    snapPinToPosition(snap_pin, positions[current_index], target_direction);

    const int snapped_pins
        = totalAlignedPins(layers_data_list, target_direction);
    if (snapped_pins > best_snapped_pins) {
      best_snapped_pins = snapped_pins;
      best_index = current_index;
      if (best_snapped_pins == total_pins) {
        break;
      }
    }
  }

  snapPinToPosition(snap_pin, positions[best_index], target_direction);
  reportUnalignedPins(
      layers_data_list, target_direction, best_snapped_pins, total_pins);
}

int Snapper::totalPins(const LayerDataList& layers_data_list)
{
  int total_pins = 0;
  for (const LayerData& data : layers_data_list) {
    total_pins += data.pins.size();
  }
  return total_pins;
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

void Snapper::reportUnalignedPins(const LayerDataList& layers_data_list,
                                  const odb::dbTechLayerDir& direction,
                                  const int aligned_pins,
                                  const int total_pins)
{
  if (aligned_pins == total_pins) {
    return;
  }

  totalAlignedPins(layers_data_list, direction, true);

  logger_->warn(utl::MPL,
                2,
                "Could not align all pins of the macro {} to the track-grid. "
                "{} out of {} pins were aligned.",
                inst_->getName(),
                aligned_pins,
                total_pins);
}

}  // namespace mpl
