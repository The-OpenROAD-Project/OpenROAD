// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#pragma once

#include "odb/db.h"
#include "odb/geom.h"
#include "unfoldedModel.h"
#include "utl/Logger.h"

namespace odb {
class dbChip;
class dbMarkerCategory;

class Checker
{
 public:
  Checker(utl::Logger* logger);
  ~Checker() = default;
  void check(odb::dbChip* chip);

 private:
  void checkFloatingChips(const UnfoldedModel& model,
                          dbMarkerCategory* category);
  void checkOverlappingChips(const UnfoldedModel& model,
                             dbMarkerCategory* category);
  void checkConnectionRegions(const UnfoldedModel& model,
                              dbChip* chip,
                              dbMarkerCategory* category);
  void checkBumpPhysicalAlignment(const UnfoldedModel& model,
                                  dbMarkerCategory* category);
  void checkNetConnectivity(const UnfoldedModel& model,
                            dbChip* chip,
                            dbMarkerCategory* category);

  bool isOverlapFullyInConnections(const UnfoldedChip* chip1,
                                   const UnfoldedChip* chip2,
                                   const Cuboid& overlap) const;

  utl::Logger* logger_;
};

}  // namespace odb
