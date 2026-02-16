// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#pragma once

#include <boost/functional/hash.hpp>
#include <unordered_map>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/unfoldedModel.h"
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
  using ConnectionMap = std::unordered_map<
      std::pair<const UnfoldedChip*, const UnfoldedChip*>,
      std::vector<const UnfoldedConnection*>,
      boost::hash<std::pair<const UnfoldedChip*, const UnfoldedChip*>>>;

  void checkFloatingChips(dbMarkerCategory* top_cat,
                          const UnfoldedModel& model);
  void checkOverlappingChips(dbMarkerCategory* top_cat,
                             const UnfoldedModel& model);
  void checkConnectionRegions(dbMarkerCategory* top_cat,
                              const UnfoldedModel& model);
  void checkBumpPhysicalAlignment(dbMarkerCategory* top_cat,
                                  const UnfoldedModel& model);
  void checkNetConnectivity(dbMarkerCategory* top_cat,
                            const UnfoldedModel& model);
  bool isOverlapFullyInConnections(const UnfoldedModel& model,
                                   const UnfoldedChip* chip1,
                                   const UnfoldedChip* chip2,
                                   const ConnectionMap& connection_map) const;
  MatingSurfaces getMatingSurfaces(const UnfoldedConnection& conn) const;
  bool isValid(const UnfoldedConnection& conn) const;
  utl::Logger* logger_;
};

}  // namespace odb
