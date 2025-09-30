// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <tcl.h>

#include "db_sta/dbSta.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "utl/Logger.h"
#include "utl/deleter.h"

namespace tst {

class Fixture : public ::testing::Test
{
 protected:
  Fixture();
  ~Fixture() override;

  // if corner == nullptr, then a corner named "default" is used
  sta::LibertyLibrary* readLiberty(const char* filename,
                                   sta::Corner* corner = nullptr,
                                   const sta::MinMaxAll* min_max
                                   = sta::MinMaxAll::all(),
                                   bool infer_latches = false);

  // Load a tech LEF file
  odb::dbTech* loadTechLef(const char* name, const char* lef_file);

  // Load a library LEF file
  odb::dbLib* loadLibaryLef(odb::dbTech* tech,
                            const char* name,
                            const char* lef_file);
  // Load a tech + library LEF file
  odb::dbLib* loadTechAndLib(const char* tech_name,
                             const char* lib_name,
                             const char* lef_file);

  utl::Logger logger_;
  utl::UniquePtrWithDeleter<odb::dbDatabase> db_;
  utl::UniquePtrWithDeleter<Tcl_Interp> interp_;
  std::unique_ptr<sta::dbSta> sta_;
};

}  // namespace tst
