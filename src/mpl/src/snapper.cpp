// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "snapper.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <numeric>
#include <optional>
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

  int origin = getOrigin(target_direction);

  const std::optional<SiteGrid> site_grid = computeSiteGrid(target_direction);
  if (site_grid) {
    if (snapOnSiteGrid(*site_grid, layers_data_list, target_direction)) {
      return;
    }
    // Either the macro's pins are not on a grid consistent with the rows, so
    // it was not built on them, or no such position is legal: align its pins
    // only.
    debugPrint(logger_,
               utl::MPL,
               "snapper",
               1,
               "No legal position on the site grid aligns all pins of {} with "
               "the track-grid; snapping {} to the track-grid only.",
               inst_->getName(),
               target_direction == odb::dbTechLayerDir::VERTICAL ? "x" : "y");
    setOrigin(origin, target_direction);
  }

  if (layers_data_list.empty()) {
    alignWithManufacturingGrid(origin);
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

  snapPinToPosition(lowest_grid_pin,
                    lowest_grid_positions[starting_position_index],
                    target_direction);

  attemptSnapToExtraPatterns(
      starting_position_index, layers_data_list, target_direction);
}

// A macro built on the same rows as the block it is placed in, such as a
// hardened sub-block, holds its cells on that site grid measured from its own
// origin.  Keeping the macro's origin on the grid keeps those cells on the
// device grids (e.g. fin and poly pitches) of the cells around it.
std::optional<Snapper::SiteGrid> Snapper::computeSiteGrid(
    const odb::dbTechLayerDir& target_direction)
{
  const bool along_x = target_direction == odb::dbTechLayerDir::VERTICAL;

  // With rows of several sites (e.g. single and double height), the smallest
  // site is the base unit the others are multiples of.
  odb::dbRow* base_row = nullptr;
  for (odb::dbRow* row : inst_->getBlock()->getRows()) {
    odb::dbSite* site = row->getSite();
    if (site->getClass() != odb::dbSiteClass::CORE
        || row->getDirection() != odb::dbRowDir::HORIZONTAL) {
      continue;
    }
    if (base_row == nullptr) {
      base_row = row;
      continue;
    }
    odb::dbSite* base_site = base_row->getSite();
    if (std::pair(site->getHeight(), site->getWidth())
        < std::pair(base_site->getHeight(), base_site->getWidth())) {
      base_row = row;
    }
  }
  if (base_row == nullptr) {
    return std::nullopt;
  }

  odb::dbSite* site = base_row->getSite();
  const int pitch = along_x ? site->getWidth() : site->getHeight();
  if (pitch <= 0) {
    return std::nullopt;
  }
  const odb::Point row_origin = base_row->getOrigin();
  const int row_pos = along_x ? row_origin.x() : row_origin.y();
  const int row_residue = ((row_pos % pitch) + pitch) % pitch;

  // The macro's grid sits at the same residue from its origin as the rows'
  // does from the block's: origin + residue = residue unmirrored, and
  // origin - residue = residue mirrored (mod pitch).
  const odb::dbOrientType orient = inst_->getOrient();
  const bool mirrored = along_x ? (orient == odb::dbOrientType::MY
                                   || orient == odb::dbOrientType::R180)
                                : (orient == odb::dbOrientType::MX
                                   || orient == odb::dbOrientType::R180);

  return SiteGrid{.pitch = pitch,
                  .residue = mirrored ? (2 * row_residue) % pitch : 0};
}

// Moves the origin to the closest site position that aligns all pins with the
// track-grid without moving the macro out of the core or onto another macro
// (unless it already was).  Returns false, leaving the origin anywhere, if
// there is none.
bool Snapper::snapOnSiteGrid(const SiteGrid& site_grid,
                             const LayerDataList& layers_data_list,
                             const odb::dbTechLayerDir& target_direction)
{
  const bool along_x = target_direction == odb::dbTechLayerDir::VERTICAL;
  const int pitch = site_grid.pitch;
  const int origin = getOrigin(target_direction);

  const int offset = (((origin - site_grid.residue) % pitch) + pitch) % pitch;
  int nearest = origin - offset;
  if (2 * offset > pitch) {
    nearest += pitch;
  }

  if (!siteGridCanAlignPins(
          pitch, nearest - origin, layers_data_list, target_direction)) {
    return false;
  }

  // Bound the search by the core, or, if the macro is already outside the
  // core, by the die and wherever the macro already is.
  odb::dbBlock* block = inst_->getBlock();
  const odb::Rect bbox = inst_->getBBox()->getBox();
  const odb::Rect core = block->getCoreArea();
  odb::Rect bounds = core;
  if (!core.contains(bbox)) {
    bounds = block->getDieArea();
    bounds.merge(bbox);
  }
  const int lo_offset = (along_x ? bbox.xMin() : bbox.yMin()) - origin;
  const int hi_offset = (along_x ? bbox.xMax() : bbox.yMax()) - origin;
  const int64_t min_origin
      = static_cast<int64_t>(along_x ? bounds.xMin() : bounds.yMin())
        - lo_offset;
  const int64_t max_origin
      = static_cast<int64_t>(along_x ? bounds.xMax() : bounds.yMax())
        - hi_offset;
  if (max_origin < min_origin) {
    return false;
  }

  // Moving along one axis, the macro can only run into macros that share its
  // span on the other axis.  Ones it already overlaps are ignored.
  std::vector<odb::Rect> obstacles;
  for (odb::dbInst* other : block->getInsts()) {
    if (other == inst_ || !other->getMaster()->isBlock()
        || !other->getPlacementStatus().isPlaced()) {
      continue;
    }
    const odb::Rect other_box = other->getBBox()->getBox();
    const bool shares_span
        = along_x ? (other_box.yMax() > bbox.yMin()
                     && other_box.yMin() < bbox.yMax())
                  : (other_box.xMax() > bbox.xMin()
                     && other_box.xMin() < bbox.xMax());
    if (shares_span && !other_box.overlaps(bbox)) {
      obstacles.push_back(other_box);
    }
  }

  const int total_pins = std::accumulate(layers_data_list.begin(),
                                         layers_data_list.end(),
                                         0,
                                         [](int total, const LayerData& data) {
                                           return total + data.pins.size();
                                         });

  // Visit the site positions within the bounds in order of distance: nearest,
  // then alternating sides starting with the one closer to the original
  // origin.
  auto floor_div = [](const int64_t a, const int64_t b) {
    return a / b - (a % b != 0 && a < 0 ? 1 : 0);
  };
  const int64_t min_steps = -floor_div(nearest - min_origin, pitch);
  const int64_t max_steps = floor_div(max_origin - nearest, pitch);
  if (max_steps < min_steps) {
    return false;
  }
  const int64_t reach = std::max(std::abs(min_steps), std::abs(max_steps));
  const int first_side = nearest > origin ? -1 : 1;
  for (int64_t i = 0; i <= 2 * reach; i++) {
    const int64_t steps = (i % 2 == 1) ? first_side * ((i + 1) / 2)
                                       : -first_side * (i / 2);
    if (steps < min_steps || steps > max_steps) {
      continue;
    }
    setOrigin(static_cast<int>(nearest + steps * pitch), target_direction);

    const odb::Rect candidate_box = inst_->getBBox()->getBox();
    if (std::ranges::any_of(obstacles, [&](const odb::Rect& obstacle) {
          return obstacle.overlaps(candidate_box);
        })) {
      continue;
    }
    if (totalAlignedPins(layers_data_list, target_direction) == total_pins) {
      return true;
    }
  }

  return false;
}

// Whether some site position could align all pins, taking each track pattern
// as unbounded: if none can, none can with the actual track extents either.
// Shifting the origin by the lcm of the site pitch and the track steps repeats
// the alignments, so one such period of site positions is enough to check.
bool Snapper::siteGridCanAlignPins(const int pitch,
                                   const int nearest_shift,
                                   const LayerDataList& layers_data_list,
                                   const odb::dbTechLayerDir& target_direction)
{
  const bool along_x = target_direction == odb::dbTechLayerDir::VERTICAL;
  const int64_t max_site_steps = 1000;

  struct Pattern
  {
    int origin;
    int step;
  };
  std::vector<std::vector<Pattern>> layer_patterns;
  std::vector<std::vector<int>> layer_pin_centers;
  int64_t period = pitch;
  for (const LayerData& data : layers_data_list) {
    std::vector<Pattern>& patterns = layer_patterns.emplace_back();
    const int count = along_x ? data.track_grid->getNumGridPatternsX()
                              : data.track_grid->getNumGridPatternsY();
    for (int i = 0; i < count; i++) {
      Pattern pattern;
      getTrackGridPattern(data.track_grid,
                          i,
                          pattern.origin,
                          pattern.step,
                          target_direction);
      if (pattern.step <= 0) {
        // A single track doesn't repeat; leave it to the actual search.
        return true;
      }
      period = std::lcm(period, static_cast<int64_t>(pattern.step));
      if (period / pitch > max_site_steps) {
        return true;
      }
      patterns.push_back(pattern);
    }

    std::vector<int>& centers = layer_pin_centers.emplace_back();
    for (odb::dbITerm* pin : data.pins) {
      centers.push_back(along_x ? pin->getBBox().xCenter()
                                : pin->getBBox().yCenter());
    }
  }

  for (int64_t k = 0; k < period / pitch; k++) {
    const int64_t shift = nearest_shift + k * pitch;
    bool all_aligned = true;
    for (size_t layer = 0; layer < layer_patterns.size() && all_aligned;
         layer++) {
      for (const int center : layer_pin_centers[layer]) {
        const int64_t pos = center + shift;
        const bool aligned = std::ranges::any_of(
            layer_patterns[layer], [&](const Pattern& pattern) {
              return (pos - pattern.origin) % pattern.step == 0;
            });
        if (!aligned) {
          all_aligned = false;
          break;
        }
      }
    }
    if (all_aligned) {
      return true;
    }
  }

  return false;
}

int Snapper::getOrigin(const odb::dbTechLayerDir& target_direction)
{
  return target_direction == odb::dbTechLayerDir::VERTICAL
             ? inst_->getOrigin().x()
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

void Snapper::snapPinToPosition(odb::dbITerm* pin,
                                int position,
                                const odb::dbTechLayerDir& direction)
{
  int origin = position - getPinOffset(pin, direction);
  alignWithManufacturingGrid(origin);
  setOrigin(origin, direction);
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

  int best_index = start_index;
  int best_snapped_pins = 0;

  for (int i = 0; i <= total_attempts; i++) {
    int steps = (i % 2 == 1) ? (i + 1) / 2 : -(i / 2);

    int current_index = start_index + steps;

    if (current_index < 0
        || current_index >= layers_data_list[0].available_positions.size()) {
      continue;
    }
    snapPinToPosition(snap_pin, positions[current_index], target_direction);

    int snapped_pins = totalAlignedPins(layers_data_list, target_direction);

    if (snapped_pins > best_snapped_pins) {
      best_snapped_pins = snapped_pins;
      best_index = current_index;
      if (best_snapped_pins == total_pins) {
        break;
      }
    }
  }

  snapPinToPosition(snap_pin, positions[best_index], target_direction);

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
