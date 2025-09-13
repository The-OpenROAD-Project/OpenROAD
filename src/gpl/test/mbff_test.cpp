// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-, The OpenROAD Authors

#include "src/gpl/src/mbff.h"

#include <cstdint>
#include <string>

#include "absl/log/check.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "db_sta/dbSta.hh"
#include "gtest/gtest.h"
#include "ord/Design.h"
#include "ord/OpenRoad.hh"
#include "ord/Tech.h"
#include "tcl.h"
#include "tclDecls.h"

namespace gpl {

class MBFFTestPeer
{
 public:
  static bool IsValidTray(MBFF& uut, odb::dbInst* tray)
  {
    return uut.IsValidTray(tray);
  }
};

namespace {

// Create a cell insidde the main block of database db.
odb::dbInst* CreateTmpCell(const char* name,
                           const char* lib_name,
                           const char* master_name,
                           odb::dbDatabase* db)
{
  odb::dbBlock* block = db->getChip()->getBlock();

  // Find a unique name.
  int64_t idx = 0;
  std::string cell_name = name;

  while (block->findInst(cell_name.c_str()) != nullptr) {
    cell_name = absl::StrCat(name, ++idx);
  }

  odb::dbLib* lib = db->findLib(lib_name);
  odb::dbMaster* master = lib->findMaster(master_name);
  return odb::dbInst::create(block, master, cell_name.c_str());
}

// Wrapper class for Tcl_Interp.
class TclInterp
{
 public:
  TclInterp()
  {
    interp_ = Tcl_CreateInterp();
    CHECK(Tcl_Init(interp_) != TCL_ERROR)
        << absl::StrFormat("Tcl_Init failed %s", Tcl_GetStringResult(interp_));
  }

  ~TclInterp()
  {
    if (interp_ != nullptr) {
      Tcl_DeleteInterp(interp_);
    }
  }

  operator Tcl_Interp*() { return interp_; }

 private:
  Tcl_Interp* interp_ = nullptr;
};

TEST(MBFFTest, FlopsCanBeIdentifiedAsATrayAndNot)
{
  TclInterp tcl_interp;

  ord::Tech tech;
  ord::Design design(&tech);

  ord::OpenRoad* ord = design.getOpenRoad();
  ord->init(/*tcl_interp=*/tcl_interp,
            /*log_filename=*/nullptr,
            /*metrics_filename=*/nullptr,
            /*batch_mode=*/true);

  odb::dbDatabase* db = ord->getDb();
  sta::dbSta* sta = ord->getSta();
  utl::Logger* log = ord->getLogger();
  rsz::Resizer* rsz = ord->getResizer();

  // Read tech.
  tech.readLiberty("src/gpl/test/library/test/test0.lib");
  tech.readLef("src/gpl/test/library/test/test0.lef");
  sta->postReadLef(db->getTech(), db->findLib("test0"));

  // Create empty chip and design.
  odb::dbChip* chip = odb::dbChip::create(db, tech.getTech());
  ord->readDef("src/gpl/test/library/test/test0.def",
               chip,
               /*continue_on_errors=*/false,
               /*floorplan_init=*/false,
               /*incremental=*/false);

  MBFF mbff(db,
            sta,
            log,
            rsz,
            /*threads=*/1,
            /*multistart=*/20,
            /*num_paths=*/0,
            /*debug_graphics=*/false);

  // Retreive masters, create a test cell, and assert that they are correctly
  // identified as either a tray or not a tray.
  EXPECT_EQ(db->findLib("test0")->getMasters().size(), 6);

  EXPECT_FALSE(MBFFTestPeer::IsValidTray(
      mbff, CreateTmpCell("test_tray", "test0", "INV", db)));
  EXPECT_FALSE(MBFFTestPeer::IsValidTray(
      mbff, CreateTmpCell("test_tray", "test0", "DFF", db)));
  EXPECT_TRUE(MBFFTestPeer::IsValidTray(
      mbff, CreateTmpCell("test_tray", "test0", "MBFF2", db)));
  EXPECT_TRUE(MBFFTestPeer::IsValidTray(
      mbff, CreateTmpCell("test_tray", "test0", "MBFF2SE", db)));
  EXPECT_TRUE(MBFFTestPeer::IsValidTray(
      mbff, CreateTmpCell("test_tray", "test0", "MBFF2CLPS", db)));
  EXPECT_TRUE(MBFFTestPeer::IsValidTray(
      mbff, CreateTmpCell("test_tray", "test0", "MBFF2SECLPS", db)));
}

}  // namespace
}  // namespace gpl
