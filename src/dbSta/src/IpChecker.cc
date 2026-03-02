// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include "db_sta/IpChecker.hh"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <numeric>
#include <string>
#include <vector>

#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace sta {

IpChecker::IpChecker(odb::dbDatabase* db, dbSta* sta, utl::Logger* logger)
    : db_(db), sta_(sta), logger_(logger)
{
}

void IpChecker::reset()
{
  warning_count_ = 0;
}

bool IpChecker::checkMaster(const std::string& master_name)
{
  reset();

  if (!db_) {
    logger_->warn(utl::CHK, 1, "No database loaded");
    warning_count_++;
    return false;
  }

  odb::dbMaster* master = nullptr;
  for (odb::dbLib* lib : db_->getLibs()) {
    master = lib->findMaster(master_name.c_str());
    if (master) {
      break;
    }
  }

  if (!master) {
    logger_->warn(utl::CHK, 2, "Master not found: {}", master_name);
    warning_count_++;
    return false;
  }

  checkLefMaster(master);

  if (warning_count_ > 0) {
    logger_->error(
        utl::CHK, 3, "IP Check failed with {} warnings", warning_count_);
  }

  return warning_count_ == 0;
}

bool IpChecker::checkAll()
{
  reset();

  if (!db_) {
    logger_->warn(utl::CHK, 5, "No database loaded");
    warning_count_++;
    return false;
  }

  int master_count = 0;
  for (odb::dbLib* lib : db_->getLibs()) {
    for (odb::dbMaster* master : lib->getMasters()) {
      checkLefMaster(master);
      master_count++;
    }
  }

  if (master_count == 0) {
    logger_->warn(utl::CHK, 4, "No masters found in database");
    warning_count_++;
  }

  if (warning_count_ > 0) {
    logger_->error(
        utl::CHK, 6, "IP Check failed with {} warnings", warning_count_);
  }

  return warning_count_ == 0;
}

void IpChecker::checkLefMaster(odb::dbMaster* master)
{
  if (!master) {
    return;
  }
  // Only check BLOCK macros, not standard cells.
  if (!master->isBlock()) {
    logger_->info(utl::CHK,
                  8,
                  "Skipping master {} (class is not BLOCK)",
                  master->getName());
    return;
  }

  checkManufacturingGridAlignment(master);     // LEF-CHK-001
  checkPinManufacturingGridAlignment(master);  // LEF-CHK-002
  checkPinRoutingGridAlignment(master);        // LEF-CHK-003
  checkPinAccessibility(master);               // LEF-CHK-004-005
  checkPolygonCount(master);                   // LEF-CHK-006
  checkAntennaInfo(master);                    // LEF-CHK-007
  checkFinFetProperty(master);                 // LEF-CHK-008
  checkPinGeometryPresence(master);            // LEF-CHK-009
  checkPinMinDimensions(master);               // LEF-CHK-010a
  checkPinMinArea(master);                     // LEF-CHK-010b
}

// LEF-CHK-001: Macro dimensions aligned to manufacturing grid
void IpChecker::checkManufacturingGridAlignment(odb::dbMaster* master)
{
  odb::dbTech* tech = db_->getTech();
  if (!tech || !tech->hasManufacturingGrid()) {
    return;
  }

  int mfg_grid = tech->getManufacturingGrid();
  uint32_t width = master->getWidth();
  uint32_t height = master->getHeight();
  std::string master_name = master->getName();

  // Convert to microns for display
  double dbu_per_micron = tech->getDbUnitsPerMicron();
  double mfg_grid_um = mfg_grid / dbu_per_micron;

  if (width % mfg_grid != 0) {
    logger_->warn(
        utl::CHK,
        10,
        "Master {} width not aligned to manufacturing grid ({:.3f} um)",
        master_name,
        mfg_grid_um);
    warning_count_++;
  }

  if (height % mfg_grid != 0) {
    logger_->warn(
        utl::CHK,
        11,
        "Master {} height not aligned to manufacturing grid ({:.3f} um)",
        master_name,
        mfg_grid_um);
    warning_count_++;
  }
}

// LEF-CHK-002: Pin coordinates aligned to manufacturing grid
void IpChecker::checkPinManufacturingGridAlignment(odb::dbMaster* master)
{
  odb::dbTech* tech = db_->getTech();
  if (!tech || !tech->hasManufacturingGrid()) {
    return;
  }

  int mfg_grid = tech->getManufacturingGrid();
  double dbu_per_micron = tech->getDbUnitsPerMicron();
  double mfg_grid_um = mfg_grid / dbu_per_micron;
  std::string master_name = master->getName();

  for (odb::dbMTerm* mterm : master->getMTerms()) {
    for (odb::dbMPin* mpin : mterm->getMPins()) {
      for (odb::dbBox* box : mpin->getGeometry()) {
        odb::Rect rect = box->getBox();

        // Check first misaligned edge and report
        if (rect.xMin() % mfg_grid != 0 || rect.yMin() % mfg_grid != 0
            || rect.xMax() % mfg_grid != 0 || rect.yMax() % mfg_grid != 0) {
          logger_->warn(
              utl::CHK,
              20,
              "Pin {}/{} not aligned to manufacturing grid ({:.3f} um)",
              master_name,
              mterm->getName(),
              mfg_grid_um);
          warning_count_++;
          return;  // One warning per master is enough
        }
      }
    }
  }
}

// LEF-CHK-003: Pin routing grid alignment
void IpChecker::checkPinRoutingGridAlignment(odb::dbMaster* master)
{
  odb::dbChip* chip = db_->getChip();
  if (!chip) {
    return;
  }

  odb::dbBlock* block = chip->getBlock();
  if (!block) {
    return;
  }

  auto track_grids = block->getTrackGrids();
  if (track_grids.empty()) {
    logger_->info(utl::CHK,
                  12,
                  "Skipping pin routing grid alignment check for {} "
                  "due to no tracks present in the design.",
                  master->getName());
    return;
  }

  std::string master_name = master->getName();

  // Collect minimum-width signal pin centers grouped by layer
  // key: layer, value: list of pin center positions along routing direction
  std::map<odb::dbTechLayer*, std::vector<int>> layer_pin_centers;

  for (odb::dbMTerm* mterm : master->getMTerms()) {
    if (mterm->getSigType().isSupply()) {
      continue;
    }

    for (odb::dbMPin* mpin : mterm->getMPins()) {
      for (odb::dbBox* box : mpin->getGeometry()) {
        odb::dbTechLayer* layer = box->getTechLayer();
        if (!layer || !layer->getRoutingLevel()) {
          continue;
        }

        odb::Rect rect = box->getBox();
        uint32_t min_width = layer->getWidth();
        bool is_horizontal
            = (layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL);

        // Only check minimum-width pins
        // For wider pins, routing might connect regardless of offset
        int pin_dim = is_horizontal ? rect.dy() : rect.dx();
        if (min_width > 0 && static_cast<uint32_t>(pin_dim) > min_width) {
          continue;  // Wider than minimum, skip for now
        }

        // Pin center along the routing direction
        int center = is_horizontal ? rect.yCenter() : rect.xCenter();
        layer_pin_centers[layer].push_back(center);
      }
    }
  }

  // For each layer, compute GCD of distances and check against track pitch
  for (auto& [layer, centers] : layer_pin_centers) {
    if (centers.size() < 2) {
      continue;  // Need at least 2 pins to compute distances
    }

    // Sort to compute distances between consecutive pin centers.
    // Sorting is needed because pins are collected per-mterm, not in spatial
    // order. Consecutive distances after sorting give the minimal spacings
    // whose GCD represents the pin grid.
    std::ranges::sort(centers);
    int distance_gcd = 0;
    for (size_t i = 1; i < centers.size(); i++) {
      int dist = centers[i] - centers[i - 1];
      if (dist > 0) {
        distance_gcd = std::gcd(distance_gcd, dist);
      }
    }

    if (distance_gcd == 0) {
      continue;  // All pins at same position
    }

    odb::dbTrackGrid* track_grid = block->findTrackGrid(layer);
    if (!track_grid) {
      continue;
    }

    // Compute the effective pitch across all track patterns on this layer.
    // Multiple patterns with different offsets create a finer effective pitch.
    bool is_horizontal
        = (layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL);
    int num_patterns = is_horizontal ? track_grid->getNumGridPatternsY()
                                     : track_grid->getNumGridPatternsX();

    if (num_patterns == 0) {
      continue;
    }

    // Collect all origins and pitches
    std::vector<int> origins;
    int effective_pitch = 0;

    for (int i = 0; i < num_patterns; i++) {
      int origin = 0, line_count = 0, pitch = 0;
      if (is_horizontal) {
        track_grid->getGridPatternY(i, origin, line_count, pitch);
      } else {
        track_grid->getGridPatternX(i, origin, line_count, pitch);
      }

      if (pitch > 0) {
        effective_pitch = std::gcd(effective_pitch, pitch);
        origins.push_back(origin);
      }
    }

    // The offsets between pattern origins also refine the effective pitch
    for (size_t i = 1; i < origins.size(); i++) {
      int offset_diff = std::abs(origins[i] - origins[0]);
      if (offset_diff > 0) {
        effective_pitch = std::gcd(effective_pitch, offset_diff);
      }
    }

    if (effective_pitch <= 0) {
      continue;
    }

    // Pin distance GCD must be a multiple of the effective pitch
    bool compatible = (distance_gcd % effective_pitch == 0);

    if (!compatible) {
      logger_->warn(utl::CHK,
                    30,
                    "Master {} signal pins on layer {} cannot be aligned "
                    "to any track pattern (pin distance GCD={} is not a "
                    "multiple of effective track pitch {})",
                    master_name,
                    layer->getName(),
                    distance_gcd,
                    effective_pitch);
      warning_count_++;
    }
  }
}

// Helper: Check if a pin has at least one accessible path
// Access can come from:
//   1. Same-layer routing (at least one edge not blocked by same-layer OBS)
//   2. Via from above (pin area not fully covered by OBS on layer above)
//   3. Via from below (pin area not fully covered by OBS on layer below)
bool IpChecker::hasAccessibleEdge(odb::dbMaster* master,
                                  const odb::Rect& pin_rect,
                                  odb::dbTechLayer* layer)
{
  int pin_routing_level = layer->getRoutingLevel();

  // Collect obstructions by relationship to pin layer
  std::vector<odb::Rect> same_layer_obs;
  std::vector<odb::Rect> above_layer_obs;
  std::vector<odb::Rect> below_layer_obs;

  for (odb::dbBox* obs : master->getObstructions()) {
    odb::dbTechLayer* obs_layer = obs->getTechLayer();
    if (!obs_layer) {
      continue;
    }
    int obs_level = obs_layer->getRoutingLevel();
    odb::Rect obs_rect = obs->getBox();

    if (obs_level == pin_routing_level) {
      same_layer_obs.push_back(obs_rect);
    } else if (obs_level == pin_routing_level + 1) {
      above_layer_obs.push_back(obs_rect);
    } else if (obs_level == pin_routing_level - 1) {
      below_layer_obs.push_back(obs_rect);
    }
  }

  // Check 1: Same-layer edge access
  // If any edge of the pin is not blocked, routing can reach it.
  // A pin edge on the macro boundary is always accessible from outside.
  if (same_layer_obs.empty()) {
    return true;  // No same-layer obstructions, all edges accessible
  }

  // Get macro bounding box (origin is always 0,0 in master coords)
  int macro_width = static_cast<int>(master->getWidth());
  int macro_height = static_cast<int>(master->getHeight());

  // An edge touching the macro boundary is accessible from outside
  bool north_on_boundary = (pin_rect.yMax() >= macro_height);
  bool south_on_boundary = (pin_rect.yMin() <= 0);
  bool east_on_boundary = (pin_rect.xMax() >= macro_width);
  bool west_on_boundary = (pin_rect.xMin() <= 0);

  if (north_on_boundary || south_on_boundary || east_on_boundary
      || west_on_boundary) {
    return true;  // Pin extends to macro boundary, accessible from outside
  }

  // For interior pins, check if any edge is free from obstructions.
  odb::Rect north_edge(
      pin_rect.xMin(), pin_rect.yMax(), pin_rect.xMax(), pin_rect.yMax() + 1);
  odb::Rect south_edge(
      pin_rect.xMin(), pin_rect.yMin() - 1, pin_rect.xMax(), pin_rect.yMin());
  odb::Rect east_edge(
      pin_rect.xMax(), pin_rect.yMin(), pin_rect.xMax() + 1, pin_rect.yMax());
  odb::Rect west_edge(
      pin_rect.xMin() - 1, pin_rect.yMin(), pin_rect.xMin(), pin_rect.yMax());

  bool north_blocked = false;
  bool south_blocked = false;
  bool east_blocked = false;
  bool west_blocked = false;

  for (const auto& obs_rect : same_layer_obs) {
    if (obs_rect.overlaps(north_edge)) {
      north_blocked = true;
    }
    if (obs_rect.overlaps(south_edge)) {
      south_blocked = true;
    }
    if (obs_rect.overlaps(east_edge)) {
      east_blocked = true;
    }
    if (obs_rect.overlaps(west_edge)) {
      west_blocked = true;
    }
  }

  if (!north_blocked || !south_blocked || !east_blocked || !west_blocked) {
    return true;  // At least one edge is accessible on the same layer
  }

  // All edges blocked on same layer. Check via access from above/below.

  // Check 2: Access from above — pin area must NOT be fully covered
  // by obstructions on the layer above
  bool above_blocked = false;
  for (const auto& obs_rect : above_layer_obs) {
    if (obs_rect.contains(pin_rect)) {
      above_blocked = true;
      break;
    }
  }
  if (!above_blocked && pin_routing_level > 0) {
    return true;  // Can drop a via from above
  }

  // Check 3: Access from below — pin area must NOT be fully covered
  // by obstructions on the layer below
  bool below_blocked = false;
  for (const auto& obs_rect : below_layer_obs) {
    if (obs_rect.contains(pin_rect)) {
      below_blocked = true;
      break;
    }
  }
  if (!below_blocked && pin_routing_level > 1) {
    return true;  // Can access via from below
  }

  // All access paths blocked
  return false;
}

// LEF-CHK-004-005: Pin accessibility (signal and power)
// A pin is considered accessible if at least one of its shapes has
// an accessible edge on its layer.
void IpChecker::checkPinAccessibility(odb::dbMaster* master)
{
  std::string master_name = master->getName();

  for (odb::dbMTerm* mterm : master->getMTerms()) {
    bool any_shape_accessible = false;
    bool has_shapes = false;

    for (odb::dbMPin* mpin : mterm->getMPins()) {
      for (odb::dbBox* box : mpin->getGeometry()) {
        odb::dbTechLayer* layer = box->getTechLayer();
        if (!layer) {
          continue;
        }

        has_shapes = true;
        odb::Rect pin_rect = box->getBox();
        if (hasAccessibleEdge(master, pin_rect, layer)) {
          any_shape_accessible = true;
          break;
        }
      }

      if (any_shape_accessible) {
        break;
      }
    }

    if (has_shapes && !any_shape_accessible) {
      bool is_power = mterm->getSigType().isSupply();
      logger_->warn(utl::CHK,
                    40,
                    "{} pin {}/{} has no accessible edge on any shape",
                    is_power ? "Power" : "Signal",
                    master_name,
                    mterm->getName());
      warning_count_++;
    }
  }
}

// LEF-CHK-006: Polygon count
void IpChecker::checkPolygonCount(odb::dbMaster* master)
{
  std::string master_name = master->getName();

  int polygon_count = 0;

  for (odb::dbBox* obs : master->getObstructions()) {
    polygon_count++;
    (void) obs;
  }

  for (odb::dbMTerm* mterm : master->getMTerms()) {
    for (odb::dbMPin* mpin : mterm->getMPins()) {
      polygon_count += mpin->getGeometry().size();
    }
  }

  if (polygon_count > max_polygons_) {
    logger_->warn(utl::CHK,
                  60,
                  "Master {} has more than {} polygons, "
                  "which will significantly slow down processes",
                  master_name,
                  max_polygons_);
    warning_count_++;
  }
}

// LEF-CHK-007: Antenna information present
void IpChecker::checkAntennaInfo(odb::dbMaster* master)
{
  std::string master_name = master->getName();

  for (odb::dbMTerm* mterm : master->getMTerms()) {
    odb::dbSigType sig_type = mterm->getSigType();
    if (sig_type == odb::dbSigType::POWER
        || sig_type == odb::dbSigType::GROUND) {
      continue;
    }

    if (!mterm->hasDefaultAntennaModel()) {
      logger_->warn(utl::CHK,
                    70,
                    "Pin {}/{} missing antenna model",
                    master_name,
                    mterm->getName());
      warning_count_++;
    }
  }
}

// LEF-CHK-008: FinFET property (info only)
void IpChecker::checkFinFetProperty(odb::dbMaster* master)
{
  if (!verbose_) {
    return;
  }

  odb::dbTech* tech = db_->getTech();
  if (!tech) {
    return;
  }

  for (odb::dbTechLayer* layer : tech->getLayers()) {
    std::string layer_name_lower = layer->getName();
    std::ranges::transform(layer_name_lower.begin(),
                           layer_name_lower.end(),
                           layer_name_lower.begin(),
                           [](unsigned char c) { return std::tolower(c); });
    if (layer_name_lower.find("fin") != std::string::npos) {
      logger_->info(
          utl::CHK, 80, "FinFET technology detected for {}", master->getName());
      return;
    }
  }
}

// LEF-CHK-009: Pin geometry presence
void IpChecker::checkPinGeometryPresence(odb::dbMaster* master)
{
  std::string master_name = master->getName();

  for (odb::dbMTerm* mterm : master->getMTerms()) {
    bool has_geometry = false;

    for (odb::dbMPin* mpin : mterm->getMPins()) {
      if (!mpin->getGeometry().empty()) {
        has_geometry = true;
        break;
      }
    }

    if (!has_geometry) {
      logger_->warn(utl::CHK,
                    90,
                    "Pin {}/{} has no geometry",
                    master_name,
                    mterm->getName());
      warning_count_++;
    }
  }
}

// LEF-CHK-010a: Pin minimum width (perpendicular to routing direction)
// Width is the dimension perpendicular to the preferred routing direction.
// For a HORIZONTAL layer, width = dx. For VERTICAL, width = dy.
void IpChecker::checkPinMinDimensions(odb::dbMaster* master)
{
  std::string master_name = master->getName();

  for (odb::dbMTerm* mterm : master->getMTerms()) {
    for (odb::dbMPin* mpin : mterm->getMPins()) {
      for (odb::dbBox* box : mpin->getGeometry()) {
        odb::dbTechLayer* layer = box->getTechLayer();
        if (!layer) {
          continue;
        }

        uint32_t min_width = layer->getWidth();
        if (min_width == 0) {
          continue;
        }

        odb::Rect rect = box->getBox();

        // Width is perpendicular to routing direction
        int shape_width;
        if (layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
          shape_width = rect.dy();  // height is the "width" for horizontal
        } else {
          shape_width = rect.dx();  // width is the "width" for vertical
        }

        if (static_cast<uint32_t>(shape_width) < min_width) {
          logger_->warn(utl::CHK,
                        100,
                        "Pin {}/{} on layer {} has width {} perpendicular "
                        "to routing direction, less than layer min width {}",
                        master_name,
                        mterm->getName(),
                        layer->getName(),
                        shape_width,
                        min_width);
          warning_count_++;
        }
      }
    }
  }
}

// LEF-CHK-010b: Pin minimum area
void IpChecker::checkPinMinArea(odb::dbMaster* master)
{
  std::string master_name = master->getName();
  int dbu_per_micron = db_->getTech()->getDbUnitsPerMicron();

  for (odb::dbMTerm* mterm : master->getMTerms()) {
    for (odb::dbMPin* mpin : mterm->getMPins()) {
      for (odb::dbBox* box : mpin->getGeometry()) {
        odb::dbTechLayer* layer = box->getTechLayer();
        if (!layer || !layer->hasArea()) {
          continue;
        }

        // getArea() returns microns^2, convert to DBU^2
        double min_area_um2 = layer->getArea();
        int64_t min_area_dbu2 = static_cast<int64_t>(
            min_area_um2 * dbu_per_micron * dbu_per_micron);

        odb::Rect rect = box->getBox();
        int64_t shape_area
            = static_cast<int64_t>(rect.dx()) * static_cast<int64_t>(rect.dy());

        if (shape_area < min_area_dbu2) {
          logger_->warn(utl::CHK,
                        110,
                        "Pin {}/{} on layer {} has area {} less than "
                        "layer minimum area {}",
                        master_name,
                        mterm->getName(),
                        layer->getName(),
                        shape_area,
                        min_area_dbu2);
          warning_count_++;
        }
      }
    }
  }
}

}  // namespace sta
