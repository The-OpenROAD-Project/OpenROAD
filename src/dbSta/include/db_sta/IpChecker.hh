// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#pragma once

#include <string>

#include "odb/db.h"
#include "odb/geom.h"

namespace sta {
class dbSta;
}  // namespace sta

namespace utl {
class Logger;
}  // namespace utl

namespace sta {

// IP Checker: Currently validates LEF macro definitions
// Checks performed:
// LEF-CHK-001: Macro dimensions aligned to manufacturing grid
// LEF-CHK-002: Pin coordinates aligned to manufacturing grid
// LEF-CHK-003: Pin routing grid alignment
// LEF-CHK-004-005: Pin accessibility (signal and power)
// LEF-CHK-006: Polygon count
// LEF-CHK-007: Antenna information present
// LEF-CHK-008: FinFET technology detection (info only)
// LEF-CHK-009: Pin geometry presence
// LEF-CHK-010a: Pin minimum width (perpendicular to routing direction)
// LEF-CHK-010b: Pin minimum area

class IpChecker
{
 public:
  IpChecker(odb::dbDatabase* db, dbSta* sta, utl::Logger* logger);

  // Configuration
  void setMaxPolygons(int max) { max_polygons_ = max; }
  void setVerbose(bool verbose) { verbose_ = verbose; }

  // Check a specific macro by name
  // Returns true if no warnings found
  bool checkMaster(const std::string& master_name);

  // Check all macros in loaded libraries
  // Returns true if no warnings found
  bool checkAll();

  // Get warning count from last check
  int getWarningCount() const { return warning_count_; }

 private:
  // Reset warning counter before a new check
  void reset();

  // LEF checks - run on a single dbMaster
  void checkLefMaster(odb::dbMaster* master);

  // LEF-CHK-001: Macro dimensions aligned to manufacturing grid
  void checkManufacturingGridAlignment(odb::dbMaster* master);

  // LEF-CHK-002: Pin coordinates aligned to manufacturing grid
  void checkPinManufacturingGridAlignment(odb::dbMaster* master);

  // LEF-CHK-003: Pin routing grid alignment
  void checkPinRoutingGridAlignment(odb::dbMaster* master);

  // LEF-CHK-004-005: Pin accessibility (signal and power)
  void checkPinAccessibility(odb::dbMaster* master);

  // LEF-CHK-006: Polygon count
  void checkPolygonCount(odb::dbMaster* master);

  // LEF-CHK-007: Antenna information present
  void checkAntennaInfo(odb::dbMaster* master);

  // LEF-CHK-008: FinFET property (info only)
  void checkFinFetProperty(odb::dbMaster* master);

  // LEF-CHK-009: Pin geometry presence
  void checkPinGeometryPresence(odb::dbMaster* master);

  // LEF-CHK-010a: Pin minimum width (perpendicular to routing direction)
  void checkPinMinDimensions(odb::dbMaster* master);

  // LEF-CHK-010b: Pin minimum area
  void checkPinMinArea(odb::dbMaster* master);

  // Helper: Check if a pin shape has at least one accessible edge
  bool hasAccessibleEdge(odb::dbMaster* master,
                         const odb::Rect& pin_rect,
                         odb::dbTechLayer* layer);

  // Member variables
  odb::dbDatabase* db_;
  dbSta* sta_;
  utl::Logger* logger_;

  // Configuration
  int max_polygons_ = 10000;
  bool verbose_ = false;

  // Warning counter
  int warning_count_ = 0;
};

}  // namespace sta
