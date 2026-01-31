// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include "utl/IpChecker.h"

#include <fstream>
#include <set>

#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "utl/Logger.h"

namespace utl {

IpChecker::IpChecker(odb::dbDatabase* db, sta::dbSta* sta, Logger* logger)
    : db_(db), sta_(sta), logger_(logger)
{
}

void IpChecker::clearResults()
{
  results_.clear();
  track_grid_warning_emitted_ = false;
}

void IpChecker::addWarning(const std::string& code,
                           const std::string& message,
                           const std::string& object)
{
  results_.emplace_back(code, message, object);
}

bool IpChecker::checkMaster(const std::string& master_name)
{
  clearResults();

  if (!db_) {
    addWarning("IPC-001", "No database loaded");
    reportResults();
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
    addWarning("IPC-002", "Master not found: " + master_name);
    reportResults();
    return false;
  }

  checkLefMaster(master);
  reportResults();

  return getWarningCount() == 0;
}

bool IpChecker::checkAll()
{
  clearResults();

  if (!db_) {
    addWarning("IPC-001", "No database loaded");
    reportResults();
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
    addWarning("IPC-003", "No masters found in database");
  }

  reportResults();
  return getWarningCount() == 0;
}

void IpChecker::checkLefMaster(odb::dbMaster* master)
{
  if (!master) {
    return;
  }

  // Run all LEF checks
  checkManufacturingGridAlignment(master);       // LEF-001
  checkPinManufacturingGridAlignment(master);    // LEF-002
  checkPinRoutingGridAlignment(master);          // LEF-003
  checkSignalPinAccessibility(master);           // LEF-004
  checkPowerPinAccessibility(master);            // LEF-005
  checkPolygonCount(master);                     // LEF-006
  checkAntennaInfo(master);                      // LEF-007
  checkFinFetProperty(master);                   // LEF-008
  checkPinGeometryPresence(master);              // LEF-009
  checkPinMinDimensions(master);                 // LEF-010
}

// LEF-001: Macro dimensions aligned to manufacturing grid
void IpChecker::checkManufacturingGridAlignment(odb::dbMaster* master)
{
  odb::dbTech* tech = db_->getTech();
  if (!tech || !tech->hasManufacturingGrid()) {
    return;
  }

  int mfg_grid = tech->getManufacturingGrid();
  if (mfg_grid <= 0) {
    return;
  }

  uint32_t width = master->getWidth();
  uint32_t height = master->getHeight();
  std::string master_name = master->getName();

  if (width % mfg_grid != 0) {
    addWarning("LEF-001",
               "Macro width (" + std::to_string(width)
                   + ") not aligned to manufacturing grid ("
                   + std::to_string(mfg_grid) + ")",
               master_name);
  }

  if (height % mfg_grid != 0) {
    addWarning("LEF-001",
               "Macro height (" + std::to_string(height)
                   + ") not aligned to manufacturing grid ("
                   + std::to_string(mfg_grid) + ")",
               master_name);
  }
}

// LEF-002: Pin coordinates aligned to manufacturing grid
void IpChecker::checkPinManufacturingGridAlignment(odb::dbMaster* master)
{
  odb::dbTech* tech = db_->getTech();
  if (!tech || !tech->hasManufacturingGrid()) {
    return;
  }

  int mfg_grid = tech->getManufacturingGrid();
  if (mfg_grid <= 0) {
    return;
  }

  std::string master_name = master->getName();

  for (odb::dbMTerm* mterm : master->getMTerms()) {
    for (odb::dbMPin* mpin : mterm->getMPins()) {
      for (odb::dbBox* box : mpin->getGeometry()) {
        int xMin = box->xMin();
        int yMin = box->yMin();
        int xMax = box->xMax();
        int yMax = box->yMax();

        bool aligned = true;
        std::string coords;

        if (xMin % mfg_grid != 0) {
          aligned = false;
          coords += "xMin=" + std::to_string(xMin) + " ";
        }
        if (yMin % mfg_grid != 0) {
          aligned = false;
          coords += "yMin=" + std::to_string(yMin) + " ";
        }
        if (xMax % mfg_grid != 0) {
          aligned = false;
          coords += "xMax=" + std::to_string(xMax) + " ";
        }
        if (yMax % mfg_grid != 0) {
          aligned = false;
          coords += "yMax=" + std::to_string(yMax) + " ";
        }

        if (!aligned) {
          addWarning("LEF-002",
                     "Pin geometry not aligned to manufacturing grid ("
                         + std::to_string(mfg_grid) + "): " + coords,
                     master_name + "/" + mterm->getName());
        }
      }
    }
  }
}

// LEF-003: Pin routing grid alignment
void IpChecker::checkPinRoutingGridAlignment(odb::dbMaster* master)
{
  odb::dbChip* chip = db_->getChip();
  if (!chip) {
    return;
  }

  odb::dbBlock* block = chip->getBlock();
  if (!block) {
    if (!track_grid_warning_emitted_) {
      addWarning("LEF-003",
                 "No design block loaded, skipping routing grid checks");
      track_grid_warning_emitted_ = true;
    }
    return;
  }

  // Check if any track grids are defined
  auto track_grids = block->getTrackGrids();
  if (track_grids.empty()) {
    if (!track_grid_warning_emitted_) {
      addWarning("LEF-003",
                 "No track grids defined, skipping routing grid checks");
      track_grid_warning_emitted_ = true;
    }
    return;
  }

  std::string master_name = master->getName();

  for (odb::dbMTerm* mterm : master->getMTerms()) {
    // Skip power/ground pins for routing grid check
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

        // Get pin center
        int pin_center_x = (box->xMin() + box->xMax()) / 2;
        int pin_center_y = (box->yMin() + box->yMax()) / 2;

        // Check alignment with tracks
        bool x_aligned = false;
        bool y_aligned = false;

        const std::vector<int>& grid_x = track_grid->getGridX();
        const std::vector<int>& grid_y = track_grid->getGridY();

        // Check X alignment
        for (int track_x : grid_x) {
          if (pin_center_x == track_x) {
            x_aligned = true;
            break;
          }
        }

        // Check Y alignment
        for (int track_y : grid_y) {
          if (pin_center_y == track_y) {
            y_aligned = true;
            break;
          }
        }

        // For horizontal layers, check Y alignment; for vertical, check X
        bool is_horizontal
            = (layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL);
        bool aligned = is_horizontal ? y_aligned : x_aligned;

        if (!aligned) {
          addWarning("LEF-003",
                     "Pin not aligned to routing grid on layer "
                         + layer->getName(),
                     master_name + "/" + mterm->getName());
        }
      }
    }
  }
}

// LEF-004: Signal pin accessibility
void IpChecker::checkSignalPinAccessibility(odb::dbMaster* master)
{
  std::string master_name = master->getName();

  // Get obstructions
  std::set<odb::dbBox*> obstructions;
  for (odb::dbBox* obs : master->getObstructions()) {
    obstructions.insert(obs);
  }

  if (obstructions.empty()) {
    return;  // No obstructions to check against
  }

  for (odb::dbMTerm* mterm : master->getMTerms()) {
    // Only check signal pins
    odb::dbSigType sig_type = mterm->getSigType();
    if (sig_type == odb::dbSigType::POWER
        || sig_type == odb::dbSigType::GROUND) {
      continue;
    }

    for (odb::dbMPin* mpin : mterm->getMPins()) {
      for (odb::dbBox* pin_box : mpin->getGeometry()) {
        odb::dbTechLayer* pin_layer = pin_box->getTechLayer();
        if (!pin_layer) {
          continue;
        }

        for (odb::dbBox* obs : obstructions) {
          odb::dbTechLayer* obs_layer = obs->getTechLayer();
          if (obs_layer != pin_layer) {
            continue;
          }

          // Check for overlap
          if (pin_box->xMin() < obs->xMax() && pin_box->xMax() > obs->xMin()
              && pin_box->yMin() < obs->yMax()
              && pin_box->yMax() > obs->yMin()) {
            addWarning("LEF-004",
                       "Signal pin obstructed on layer " + obs_layer->getName(),
                       master_name + "/" + mterm->getName());
            break;  // One warning per pin is enough
          }
        }
      }
    }
  }
}

// LEF-005: Power pin accessibility
void IpChecker::checkPowerPinAccessibility(odb::dbMaster* master)
{
  std::string master_name = master->getName();

  // Get obstructions
  std::set<odb::dbBox*> obstructions;
  for (odb::dbBox* obs : master->getObstructions()) {
    obstructions.insert(obs);
  }

  if (obstructions.empty()) {
    return;
  }

  for (odb::dbMTerm* mterm : master->getMTerms()) {
    odb::dbSigType sig_type = mterm->getSigType();
    if (sig_type != odb::dbSigType::POWER
        && sig_type != odb::dbSigType::GROUND) {
      continue;
    }

    for (odb::dbMPin* mpin : mterm->getMPins()) {
      for (odb::dbBox* pin_box : mpin->getGeometry()) {
        odb::dbTechLayer* pin_layer = pin_box->getTechLayer();
        if (!pin_layer) {
          continue;
        }

        for (odb::dbBox* obs : obstructions) {
          odb::dbTechLayer* obs_layer = obs->getTechLayer();
          if (obs_layer != pin_layer) {
            continue;
          }

          // Check for full obstruction (pin completely covered)
          if (obs->xMin() <= pin_box->xMin() && obs->xMax() >= pin_box->xMax()
              && obs->yMin() <= pin_box->yMin()
              && obs->yMax() >= pin_box->yMax()) {
            addWarning("LEF-005",
                       "Power pin fully obstructed on layer "
                           + obs_layer->getName(),
                       master_name + "/" + mterm->getName());
            break;
          }
        }
      }
    }
  }
}

// LEF-006: Polygon count (heuristic warning)
void IpChecker::checkPolygonCount(odb::dbMaster* master)
{
  std::string master_name = master->getName();

  int polygon_count = 0;

  // Count obstruction polygons
  for (odb::dbBox* obs : master->getObstructions()) {
    polygon_count++;
    (void) obs;  // Suppress unused warning
  }

  // Count pin geometry polygons
  for (odb::dbMTerm* mterm : master->getMTerms()) {
    for (odb::dbMPin* mpin : mterm->getMPins()) {
      for (odb::dbBox* box : mpin->getGeometry()) {
        polygon_count++;
        (void) box;
      }
    }
  }

  if (polygon_count > config_.max_polygons) {
    addWarning("LEF-006",
               "Excessive polygon count: " + std::to_string(polygon_count)
                   + " (threshold: " + std::to_string(config_.max_polygons)
                   + ")",
               master_name);
  }
}

// LEF-007: Antenna information present
void IpChecker::checkAntennaInfo(odb::dbMaster* master)
{
  std::string master_name = master->getName();

  for (odb::dbMTerm* mterm : master->getMTerms()) {
    // Skip power/ground pins
    odb::dbSigType sig_type = mterm->getSigType();
    if (sig_type == odb::dbSigType::POWER
        || sig_type == odb::dbSigType::GROUND) {
      continue;
    }

    if (!mterm->hasDefaultAntennaModel()) {
      addWarning("LEF-007",
                 "Missing antenna model for signal pin",
                 master_name + "/" + mterm->getName());
    }
  }
}

// LEF-008: FinFET fin grid property
void IpChecker::checkFinFetProperty(odb::dbMaster* master)
{
  // This check looks for LEF58 fin-related properties on tech layers
  // If the technology appears to be FinFET but lacks fin properties, warn
  odb::dbTech* tech = db_->getTech();
  if (!tech) {
    return;
  }

  // Heuristic: check if any layer has "fin" in property names
  bool has_fin_layer = false;

  for (odb::dbTechLayer* layer : tech->getLayers()) {
    // Check for fin-related layer names or properties
    std::string layer_name = layer->getName();
    if (layer_name.find("fin") != std::string::npos
        || layer_name.find("FIN") != std::string::npos) {
      has_fin_layer = true;
      break;
    }
  }

  // If we detected a FinFET technology, log it in verbose mode
  if (config_.verbose && has_fin_layer) {
    // This is informational only - no warning needed
    logger_->info(IPC, 7, "FinFET technology detected for {}", master->getName());
  }
}

// LEF-009: Pin geometry presence
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
      addWarning("LEF-009",
                 "Pin has no geometry",
                 master_name + "/" + mterm->getName());
    }
  }
}

// LEF-010: Pin minimum dimensions
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
        if (min_width <= 0) {
          continue;
        }

        uint32_t width = box->getDX();
        uint32_t height = box->getDY();

        // The smaller dimension is the "width" of the shape
        uint32_t shape_width = std::min(width, height);

        if (shape_width < min_width) {
          addWarning("LEF-010",
                     "Pin geometry width (" + std::to_string(shape_width)
                         + ") less than layer minimum width ("
                         + std::to_string(min_width) + ")",
                     master_name + "/" + mterm->getName());
        }
      }
    }
  }
}

void IpChecker::reportResults() const
{
  if (!logger_) {
    return;
  }

  int warnings = getWarningCount();

  for (const auto& result : results_) {
    std::string msg = "[" + result.code + "]";
    if (!result.object.empty()) {
      msg += " " + result.object + ":";
    }
    msg += " " + result.message;

    logger_->warn(IPC, 1, "{}", msg);
  }

  // Summary
  logger_->info(IPC, 2, "IP Check complete: {} warnings", warnings);

  // Error out if any warnings were found
  if (warnings > 0) {
    logger_->error(IPC, 3, "IP Check failed with {} warnings", warnings);
  }
}

void IpChecker::writeReport(const std::string& filename) const
{
  std::ofstream file(filename);
  if (!file.is_open()) {
    if (logger_) {
      logger_->warn(IPC, 4, "Unable to open report file: {}", filename);
    }
    return;
  }

  file << "IP Checker Report\n";
  file << "==================\n\n";

  for (const auto& result : results_) {
    file << "WARNING [" << result.code << "]";
    if (!result.object.empty()) {
      file << " " << result.object;
    }
    file << ": " << result.message << "\n";
  }

  file << "\nSummary: " << getWarningCount() << " warnings\n";

  file.close();
}

}  // namespace utl
