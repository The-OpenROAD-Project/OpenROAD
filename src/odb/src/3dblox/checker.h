// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#pragma once

#include "odb/db.h"
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
  struct ContactSurfaces
  {
    bool valid;
    int top_z;
    int bot_z;
  };
  void checkFloatingChips(const UnfoldedModel& model,
                          dbMarkerCategory* category);
  ContactSurfaces getContactSurfaces(const UnfoldedConnection& conn) const;
  bool isValid(const UnfoldedConnection& conn) const;
  utl::Logger* logger_;
};

}  // namespace odb
