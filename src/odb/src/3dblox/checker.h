// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#pragma once

#include <unordered_map>

namespace utl {
class Logger;
}

namespace sta {
class Sta;
}

namespace odb {
class dbDatabase;
class dbMarkerCategory;
class dbChipBump;

class Checker
{
 public:
  Checker(utl::Logger* logger, dbDatabase* db);
  ~Checker() = default;
  void check();

 private:
  void computeBumpLayerMatches();
  void checkLogicalConnectivity(dbMarkerCategory* top_cat);
  void checkFloatingChips(dbMarkerCategory* top_cat);
  void checkOverlappingChips(dbMarkerCategory* top_cat);
  void checkInternalExtUsage(dbMarkerCategory* top_cat);
  void checkConnectionRegions(dbMarkerCategory* top_cat);
  void checkBumpPhysicalAlignment(dbMarkerCategory* top_cat);
  void checkBumpLayer(dbMarkerCategory* top_cat);
  void checkNetConnectivity(dbMarkerCategory* top_cat);
  void checkAlignmentMarkers(dbMarkerCategory* top_cat);
  utl::Logger* logger_;
  dbDatabase* db_;
  // Whether each bump's pin geometry lands on its region's declared contact
  // layer (dbChipRegion::getLayer()), computed once by
  // computeBumpLayerMatches(). Absent entries mean the region declares no
  // layer, so conformance is unverifiable. Session-only; reused by other
  // checks (e.g. checkLogicalConnectivity).
  std::unordered_map<dbChipBump*, bool> bump_layer_match_;
};

}  // namespace odb
