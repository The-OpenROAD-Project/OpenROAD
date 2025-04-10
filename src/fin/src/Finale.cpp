// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "fin/Finale.h"

#include "DensityFill.h"

namespace fin {

////////////////////////////////////////////////////////////////

void Finale::init(odb::dbDatabase* db, Logger* logger)
{
  db_ = db;
  logger_ = logger;
}

void Finale::setDebug()
{
  debug_ = true;
}

void Finale::densityFill(const char* rules_filename, const odb::Rect& fill_area)
{
  DensityFill filler(db_, logger_, debug_);
  filler.fill(rules_filename, fill_area);
}

}  // namespace fin
