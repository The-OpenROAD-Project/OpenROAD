// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#pragma once

namespace utl {
class Logger;
}

namespace sta {
class Sta;
}

namespace odb {
class dbDatabase;
class UnfoldedModel;
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
  Checker(utl::Logger* logger, dbDatabase* db);
  ~Checker() = default;
  void check();

 private:
  void checkLogicalConnectivity(dbMarkerCategory* top_cat,
                                const UnfoldedModel* model);
  void checkFloatingChips(dbMarkerCategory* top_cat,
                          const UnfoldedModel* model);
  void checkOverlappingChips(dbMarkerCategory* top_cat,
                             const UnfoldedModel* model);
  void checkInternalExtUsage(dbMarkerCategory* top_cat,
                             const UnfoldedModel* model);
  void checkConnectionRegions(dbMarkerCategory* top_cat,
                              const UnfoldedModel* model);
  void checkBumpPhysicalAlignment(dbMarkerCategory* top_cat,
                                  const UnfoldedModel* model);
  void checkNetConnectivity(dbMarkerCategory* top_cat,
                            const UnfoldedModel* model);
  utl::Logger* logger_;
  dbDatabase* db_;
};

}  // namespace odb
