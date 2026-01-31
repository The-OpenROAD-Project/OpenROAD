// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>

#include "odb/db.h"

namespace sta {
class dbSta;
class LibertyLibrary;
class LibertyCell;
class LibertyPort;
}  // namespace sta

namespace utl {

class Logger;

// Result of a single check 
struct CheckResult
{
  std::string code;     // e.g., "LEF-001", "LEF-003"
  std::string message;  // description
  std::string object;   // Name of the object with the issue (macro, pin, etc.)

  CheckResult(const std::string& c,
              const std::string& msg,
              const std::string& obj = "")
      : code(c), message(msg), object(obj)
  {
  }
};

// Configuration for the IP checker
struct IpCheckerConfig
{
  int max_polygons = 10000;  // Polygon count warning threshold
  bool verbose = false;      // Include informational messages in output
};

// IP Checker utility for validating LEF macros
class IpChecker
{
 public:
  IpChecker(odb::dbDatabase* db, sta::dbSta* sta, Logger* logger);

  // Set configuration options
  void setConfig(const IpCheckerConfig& config) { config_ = config; }
  const IpCheckerConfig& getConfig() const { return config_; }

  // Check a specific macro by name
  // Returns true if no warnings found
  bool checkMaster(const std::string& master_name);

  // Check all macros in loaded libraries
  // Returns true if no warnings found
  bool checkAll();

  // Get results from the last check
  const std::vector<CheckResult>& getResults() const { return results_; }

  // Get warning count
  int getWarningCount() const { return static_cast<int>(results_.size()); }

  // Write results to a file
  void writeReport(const std::string& filename) const;

  // Print results to logger
  void reportResults() const;

 private:
  // Clear results before a new check
  void clearResults();

  // Add a warning result
  void addWarning(const std::string& code,
                  const std::string& message,
                  const std::string& object = "");

  // LEF checks - run on a single dbMaster
  void checkLefMaster(odb::dbMaster* master);

  // LEF-001: Macro dimensions aligned to manufacturing grid
  void checkManufacturingGridAlignment(odb::dbMaster* master);

  // LEF-002: Pin coordinates aligned to manufacturing grid
  void checkPinManufacturingGridAlignment(odb::dbMaster* master);

  // LEF-003: Pin routing grid alignment and minimum width
  void checkPinRoutingGridAlignment(odb::dbMaster* master);

  // LEF-004: Signal pin accessibility (not obstructed)
  void checkSignalPinAccessibility(odb::dbMaster* master);

  // LEF-005: Power pin accessibility
  void checkPowerPinAccessibility(odb::dbMaster* master);

  // LEF-006: Polygon count (heuristic warning)
  void checkPolygonCount(odb::dbMaster* master);

  // LEF-007: Antenna information present
  void checkAntennaInfo(odb::dbMaster* master);

  // LEF-008: FinFET fin grid property
  void checkFinFetProperty(odb::dbMaster* master);

  // LEF-009: Pin geometry presence
  void checkPinGeometryPresence(odb::dbMaster* master);

  // LEF-010: Pin minimum dimensions (width)
  void checkPinMinDimensions(odb::dbMaster* master);

  // Member variables
  odb::dbDatabase* db_;
  sta::dbSta* sta_;
  Logger* logger_;
  IpCheckerConfig config_;
  std::vector<CheckResult> results_;

  // Track grid availability for routing grid checks
  bool track_grid_warning_emitted_ = false;
};

}  // namespace utl
