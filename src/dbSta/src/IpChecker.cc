// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include "db_sta/IpChecker.hh"

#include <set>

#include "db_sta/dbSta.hh"
#include "odb/db.h"
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

  checkManufacturingGridAlignment(master);     // LEF-CHK-001
  checkPinManufacturingGridAlignment(master);  // LEF-CHK-002
  checkPinRoutingGridAlignment(master);        // LEF-CHK-003
  checkSignalPinAccessibility(master);         // LEF-CHK-004
  checkPowerPinAccessibility(master);          // LEF-CHK-005
  checkPolygonCount(master);                   // LEF-CHK-006
  checkAntennaInfo(master);                    // LEF-CHK-007
  checkFinFetProperty(master);                 // LEF-CHK-008
  checkPinGeometryPresence(master);            // LEF-CHK-009
  checkPinMinDimensions(master);               // LEF-CHK-010
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

  // Convert to microns for display (assuming DBU = nm)
  double mfg_grid_um = mfg_grid / 1000.0;

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
  double mfg_grid_um = mfg_grid / 1000.0;
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
          return;  // One warning per pin is enough
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
    return;
  }

  std::string master_name = master->getName();

  for (odb::dbMTerm* mterm : master->getMTerms()) {
    odb::dbSigType sig_type = mterm->getSigType();
    if (sig_type == odb::dbSigType::POWER
        || sig_type == odb::dbSigType::GROUND) {
      continue;
    }

    for (odb::dbMPin* mpin : mterm->getMPins()) {
      for (odb::dbBox* box : mpin->getGeometry()) {
        odb::dbTechLayer* layer = box->getTechLayer();
        if (!layer || !layer->getRoutingLevel()) {
          continue;
        }

        odb::dbTrackGrid* track_grid = block->findTrackGrid(layer);
        if (!track_grid) {
          continue;
        }

        // Get track pitches (step) from track grid patterns
        int origin_x = 0, line_count_x = 0, x_pitch = 0;
        int origin_y = 0, line_count_y = 0, y_pitch = 0;
        if (track_grid->getNumGridPatternsX() > 0) {
          track_grid->getGridPatternX(0, origin_x, line_count_x, x_pitch);
        }
        if (track_grid->getNumGridPatternsY() > 0) {
          track_grid->getGridPatternY(0, origin_y, line_count_y, y_pitch);
        }

        if (x_pitch <= 0 || y_pitch <= 0) {
          continue;
        }

        odb::Rect rect = box->getBox();
        int pin_width = rect.dx();
        int pin_height = rect.dy();

        // Check if pin dimensions are compatible with track pitch
        bool is_horizontal
            = (layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL);

        bool compatible = false;
        if (is_horizontal) {
          // Horizontal layer: pin height should be compatible with y_pitch
          compatible
              = (pin_height % y_pitch == 0) || (y_pitch % pin_height == 0);
        } else {
          // Vertical layer: pin width should be compatible with x_pitch
          compatible = (pin_width % x_pitch == 0) || (x_pitch % pin_width == 0);
        }

        if (!compatible) {
          logger_->warn(utl::CHK,
                        30,
                        "Pin {}/{} dimensions not compatible with routing "
                        "track pitch on layer {}",
                        master_name,
                        mterm->getName(),
                        layer->getName());
          warning_count_++;
        }
      }
    }
  }
}

// Helper: Check if a pin has at least one accessible edge
bool IpChecker::hasAccessibleEdge(odb::dbMaster* master,
                                  const odb::Rect& pin_rect,
                                  odb::dbTechLayer* layer)
{
  // Get all obstructions on this layer AND layers above
  // Obstructions above can block via access to the pin
  std::vector<odb::Rect> obs_rects;
  int pin_routing_level = layer->getRoutingLevel();

  for (odb::dbBox* obs : master->getObstructions()) {
    odb::dbTechLayer* obs_layer = obs->getTechLayer();
    if (!obs_layer) {
      continue;
    }
    int obs_routing_level = obs_layer->getRoutingLevel();

    // Check obstructions on same layer or above
    if (obs_routing_level >= pin_routing_level) {
      obs_rects.push_back(obs->getBox());
    }
  }

  if (obs_rects.empty()) {
    return true;  // No obstructions, all edges accessible
  }

  // Check each edge for accessibility
  // An edge is accessible if it's not fully covered by obstructions

  // North edge (top)
  odb::Rect north_edge(
      pin_rect.xMin(), pin_rect.yMax(), pin_rect.xMax(), pin_rect.yMax() + 1);
  bool north_blocked = false;

  // South edge (bottom)
  odb::Rect south_edge(
      pin_rect.xMin(), pin_rect.yMin() - 1, pin_rect.xMax(), pin_rect.yMin());
  bool south_blocked = false;

  // East edge (right)
  odb::Rect east_edge(
      pin_rect.xMax(), pin_rect.yMin(), pin_rect.xMax() + 1, pin_rect.yMax());
  bool east_blocked = false;

  // West edge (left)
  odb::Rect west_edge(
      pin_rect.xMin() - 1, pin_rect.yMin(), pin_rect.xMin(), pin_rect.yMax());
  bool west_blocked = false;

  for (const auto& obs_rect : obs_rects) {
    if (obs_rect.intersects(north_edge)) {
      north_blocked = true;
    }
    if (obs_rect.intersects(south_edge)) {
      south_blocked = true;
    }
    if (obs_rect.intersects(east_edge)) {
      east_blocked = true;
    }
    if (obs_rect.intersects(west_edge)) {
      west_blocked = true;
    }
  }

  // Return true if at least one edge is accessible
  return !north_blocked || !south_blocked || !east_blocked || !west_blocked;
}

// LEF-CHK-004: Signal pin accessibility
void IpChecker::checkSignalPinAccessibility(odb::dbMaster* master)
{
  std::string master_name = master->getName();

  for (odb::dbMTerm* mterm : master->getMTerms()) {
    odb::dbSigType sig_type = mterm->getSigType();
    if (sig_type == odb::dbSigType::POWER
        || sig_type == odb::dbSigType::GROUND) {
      continue;
    }

    for (odb::dbMPin* mpin : mterm->getMPins()) {
      for (odb::dbBox* box : mpin->getGeometry()) {
        odb::dbTechLayer* layer = box->getTechLayer();
        if (!layer) {
          continue;
        }

        odb::Rect pin_rect = box->getBox();
        if (!hasAccessibleEdge(master, pin_rect, layer)) {
          logger_->warn(utl::CHK,
                        40,
                        "Signal pin {}/{} has no accessible edge on layer {}",
                        master_name,
                        mterm->getName(),
                        layer->getName());
          warning_count_++;
          break;  // One warning per pin
        }
      }
    }
  }
}

// LEF-CHK-005: Power pin accessibility
void IpChecker::checkPowerPinAccessibility(odb::dbMaster* master)
{
  std::string master_name = master->getName();

  for (odb::dbMTerm* mterm : master->getMTerms()) {
    odb::dbSigType sig_type = mterm->getSigType();
    if (sig_type != odb::dbSigType::POWER
        && sig_type != odb::dbSigType::GROUND) {
      continue;
    }

    for (odb::dbMPin* mpin : mterm->getMPins()) {
      for (odb::dbBox* box : mpin->getGeometry()) {
        odb::dbTechLayer* layer = box->getTechLayer();
        if (!layer) {
          continue;
        }

        odb::Rect pin_rect = box->getBox();
        if (!hasAccessibleEdge(master, pin_rect, layer)) {
          logger_->warn(utl::CHK,
                        50,
                        "Power pin {}/{} has no accessible edge on layer {}",
                        master_name,
                        mterm->getName(),
                        layer->getName());
          warning_count_++;
          break;
        }
      }
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
    std::transform(layer_name_lower.begin(),
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

// LEF-CHK-010: Pin minimum dimensions
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
        int shape_width = rect.minDXDY();

        if (static_cast<uint32_t>(shape_width) < min_width) {
          logger_->warn(utl::CHK,
                        100,
                        "Pin {}/{} geometry width ({}) less than layer "
                        "minimum width ({})",
                        master_name,
                        mterm->getName(),
                        shape_width,
                        min_width);
          warning_count_++;
        }
      }
    }
  }
}

}  // namespace sta
