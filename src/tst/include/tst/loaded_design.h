// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
#pragma once

#include <memory>
#include <string>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "tcl.h"
#include "utl/Logger.h"
#include "utl/deleter.h"

namespace tst {

// The technologies a test fixture or LoadedDesign can be built against.
// Formerly IntegratedFixture::Technology, lifted to namespace scope so
// LoadedDesign can share it; IntegratedFixture::Technology is now an alias.
enum class Technology
{
  kNangate45,
  kSky130hd
};

// Loads the liberty and LEF for `tech` into `db`/`sta` and returns the
// resulting dbLib. Paths are resolved through the runfiles machinery.
odb::dbLib* loadTechnology(Technology tech,
                           odb::dbDatabase* db,
                           sta::dbSta* sta,
                           utl::Logger* logger);

// Reads `verilog_path` (an already-resolved filesystem path), links it as
// `top`, and brings the db and sta up to a usable state. Returns the top
// block.
//
// `hierarchy` selects the link mode: true builds the
// dbModule/dbModInst/dbModNet overlay on top of the flat netlist and marks the
// db hierarchical, false links flat only. This is the single place that
// decision is made, so LoadedDesign and IntegratedFixture cannot drift.
//
// Reports failure by throwing from utl::Logger::error(), so a netlist the
// requested link mode refuses surfaces as an exception rather than a return
// code.
odb::dbBlock* readVerilogAndLink(const std::string& verilog_path,
                                 const char* top,
                                 bool hierarchy,
                                 odb::dbDatabase* db,
                                 sta::dbSta* sta,
                                 utl::Logger* logger,
                                 odb::dbLib* lib);

// One netlist loaded through one link mode, owning everything it needs:
// its own Tcl_Interp, dbDatabase and dbSta.
//
// This exists because the tst fixture chain (DbFixture -> Fixture ->
// IntegratedFixture) derives from ::testing::Test and owns exactly one
// db_/sta_ pair that is always linked hierarchically. Tests that need to load
// the same netlist both ways -- or to load it flat at all -- cannot use those
// fixtures. LoadedDesign is deliberately not a gtest fixture so a single test
// body can build several in sequence.
//
// Only one design need be live at a time for the conformance checks this was
// written for: the gold side is a file on disk, not a second load.
class LoadedDesign
{
 public:
  // `verilog_path` must be an already-resolved, existing path; callers that
  // need runfiles resolution should do it first (tech files are resolved
  // internally, since their locations are fixed).
  LoadedDesign(Technology tech,
               const std::string& verilog_path,
               const char* top,
               bool hierarchy);
  ~LoadedDesign();

  LoadedDesign(const LoadedDesign&) = delete;
  LoadedDesign& operator=(const LoadedDesign&) = delete;

  // Emits the netlist. In hierarchical mode write_verilog walks the dbModNet
  // overlay and emits nested modules; in flat mode it emits a single module
  // with escaped hierarchical instance names.
  void writeVerilog(const std::string& path, bool include_pwr_gnd = false);

  odb::dbDatabase* getDb() const { return db_.get(); }
  sta::dbSta* getSta() const { return sta_.get(); }
  odb::dbBlock* getBlock() const { return block_; }
  sta::dbNetwork* getNetwork() const { return db_network_; }
  utl::Logger* getLogger() { return &logger_; }
  bool hasHierarchy() const { return db_network_->hasHierarchy(); }

 private:
  // Declaration order is destruction order reversed: sta_ and interp_ must go
  // before db_, matching the Fixture chain's base-class ordering.
  utl::Logger logger_;
  utl::UniquePtrWithDeleter<odb::dbDatabase> db_;
  utl::UniquePtrWithDeleter<Tcl_Interp> interp_;
  std::unique_ptr<sta::dbSta> sta_;

  odb::dbLib* lib_{nullptr};
  odb::dbBlock* block_{nullptr};
  sta::dbNetwork* db_network_{nullptr};
};

}  // namespace tst
