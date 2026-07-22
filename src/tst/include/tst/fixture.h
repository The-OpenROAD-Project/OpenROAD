// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <string>

#include "db_sta/dbSta.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "tcl.h"
#include "tst/db_fixture.h"
#include "utl/deleter.h"

namespace tst {

// Fixture that adds an OpenSTA/dbSta instance on top of the odb-only
// DbFixture. Tests that do not need timing should derive from DbFixture
// (tst/db_fixture.h) to avoid linking OpenSTA.
class Fixture : public DbFixture
{
 protected:
  Fixture();
  ~Fixture() override;

  sta::dbSta* getSta() const { return sta_.get(); }

  // if scene == nullptr, then a scene named "default" is used
  sta::LibertyLibrary* readLiberty(const std::string& filename,
                                   sta::Scene* scene = nullptr,
                                   const sta::MinMaxAll* min_max
                                   = sta::MinMaxAll::all(),
                                   bool infer_latches = false);

  // Load a library LEF file
  odb::dbLib* loadLibaryLef(odb::dbTech* tech,
                            const char* name,
                            const std::string& lef_file);
  // Load a tech + library LEF file
  odb::dbLib* loadTechAndLib(const char* tech_name,
                             const char* lib_name,
                             const std::string& lef_file);

  // Add macros to this library
  bool updateLib(odb::dbLib* lib, const std::string& lef_file);

  utl::UniquePtrWithDeleter<Tcl_Interp> interp_;
  std::unique_ptr<sta::dbSta> sta_;
};

}  // namespace tst
