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
  void check(dbChip* chip);

 private:
  struct MatingSurfaces
  {
    bool valid;
    int top_z;
    int bot_z;
  };
  void checkFloatingChips(dbChip* chip, const UnfoldedModel& model);
  void checkOverlappingChips(dbChip* chip, const UnfoldedModel& model);
  void checkConnectionRegions(dbChip* chip, const UnfoldedModel& model);
  void checkBumpPhysicalAlignment(dbChip* chip, const UnfoldedModel& model);
  void checkNetConnectivity(dbChip* chip, const UnfoldedModel& model);
  bool isOverlapFullyInConnections(const UnfoldedChip* chip1,
                                   const UnfoldedChip* chip2,
                                   const Cuboid& overlap) const;
  MatingSurfaces getMatingSurfaces(const UnfoldedConnection& conn) const;
  bool isValid(const UnfoldedConnection& conn) const;
  utl::Logger* logger_;
};

}  // namespace odb
