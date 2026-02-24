// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#pragma once

#include <unordered_map>
#include <vector>

#include "odb/db.h"
#include "odb/unfoldedModel.h"
#include "utl/Logger.h"
#include "utl/unionFind.h"

namespace odb {
class dbChip;
class dbMarkerCategory;

struct MatingSurfaces
{
  bool valid;
  int top_z;
  int bot_z;
};

class Checker
{
 public:
  Checker(utl::Logger* logger);
  ~Checker() = default;
  void check(dbChip* chip, int bump_pitch_tolerance);

 private:
  void checkFloatingChips(dbMarkerCategory* top_cat,
                          const UnfoldedModel& model);
  void checkOverlappingChips(dbMarkerCategory* top_cat,
                             const UnfoldedModel& model);
  void checkInternalExtUsage(dbMarkerCategory* top_cat,
                             const UnfoldedModel& model);
  void checkConnectionRegions(dbMarkerCategory* top_cat,
                              const UnfoldedModel& model);
  void checkBumpPhysicalAlignment(dbMarkerCategory* top_cat,
                                  const UnfoldedModel& model);
  void checkNetConnectivity(dbMarkerCategory* top_cat,
                            const UnfoldedModel& model,
                            int bump_pitch_tolerance);

  void checkIntraChipConnectivity(
      const UnfoldedNet& net,
      utl::UnionFind& uf,
      std::unordered_map<const UnfoldedRegion*, std::vector<int>>&
          region_bumps);

  void checkInterChipConnectivity(
      const UnfoldedNet& net,
      utl::UnionFind& uf,
      const std::unordered_map<const UnfoldedRegion*, std::vector<int>>&
          region_bumps,
      const std::unordered_map<const UnfoldedRegion*,
                               std::vector<const UnfoldedConnection*>>&
          region_connections,
      int bump_pitch_tolerance);

  void connectBumpsBetweenRegions(const std::vector<int>& set_a,
                                  const std::vector<int>& set_b,
                                  int tolerance,
                                  const UnfoldedNet& net,
                                  utl::UnionFind& uf);

  void verifyChipConnectivity(const std::vector<int>& indices,
                              const UnfoldedNet& net,
                              utl::UnionFind& uf);

  utl::Logger* logger_;
  static constexpr int kBumpMarkerHalfSize = 100;
};

}  // namespace odb
