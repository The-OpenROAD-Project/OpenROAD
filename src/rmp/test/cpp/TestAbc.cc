// Copyright 2023 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <unistd.h>

#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <set>

#include "abc_library_factory.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "map/scl/sclLib.h"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/Sta.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"

namespace rmp {

std::once_flag init_sta_flag;

class AbcTest : public ::testing::Test
{
 protected:
  void SetUp() override
  {
    std::call_once(init_sta_flag, []() { sta::initSta(); });
    sta_ = new sta::Sta;
    sta_->makeComponents();
    auto path = std::filesystem::canonical("./Nangate45/Nangate45_fast.lib");
    library_ = sta_->readLiberty(path.string().c_str(),
                                 sta_->findCorner("default"),
                                 /*min_max=*/nullptr,
                                 /*infer_latches=*/false);
    sta::Units* units = library_->units();
    power_unit_ = units->powerUnit();
  }

  sta::Unit* power_unit_;
  sta::Sta* sta_;
  sta::LibertyLibrary* library_;
  utl::Logger logger_;
};

TEST_F(AbcTest, CellPropertiesMatchOpenSta)
{
  AbcLibraryFactory factory(&logger_);
  factory.AddStaLibrary(library_);
  utl::deleted_unique_ptr<abc::SC_Lib> abc_library = factory.Build();

  for (size_t i = 0; i < Vec_PtrSize(&abc_library->vCells); i++) {
    abc::SC_Cell* abc_cell = static_cast<abc::SC_Cell*>(
        abc::Vec_PtrEntry(&abc_library->vCells, i));
    sta::LibertyCell* sta_cell = library_->findLibertyCell(abc_cell->pName);
    EXPECT_NE(nullptr, sta_cell);
    // Expect area matches
    EXPECT_FLOAT_EQ(abc_cell->area, sta_cell->area());

    float leakage_power = -1;
    bool exists;
    sta_cell->leakagePower(leakage_power, exists);
    if (exists) {
      EXPECT_FLOAT_EQ(abc_cell->leakage, power_unit_->staToUser(leakage_power));
    }
  }
}

TEST_F(AbcTest, DoesNotContainPhysicalCells)
{
  AbcLibraryFactory factory(&logger_);
  factory.AddStaLibrary(library_);
  utl::deleted_unique_ptr<abc::SC_Lib> abc_library = factory.Build();

  std::set<std::string> abc_cells;
  using ::testing::Contains;
  using ::testing::Not;

  for (size_t i = 0; i < Vec_PtrSize(&abc_library->vCells); i++) {
    abc::SC_Cell* abc_cell = static_cast<abc::SC_Cell*>(
        abc::Vec_PtrEntry(&abc_library->vCells, i));
    abc_cells.emplace(abc_cell->pName);
  }

  EXPECT_THAT(abc_cells, Not(Contains("ANTENNA_X1")));
  EXPECT_THAT(abc_cells, Not(Contains("FILLCELL_X1")));
}

TEST_F(AbcTest, DoesNotContainSequentialCells)
{
  AbcLibraryFactory factory(&logger_);
  factory.AddStaLibrary(library_);
  utl::deleted_unique_ptr<abc::SC_Lib> abc_library = factory.Build();

  std::set<std::string> abc_cells;
  using ::testing::Contains;
  using ::testing::Not;

  for (size_t i = 0; i < Vec_PtrSize(&abc_library->vCells); i++) {
    abc::SC_Cell* abc_cell = static_cast<abc::SC_Cell*>(
        abc::Vec_PtrEntry(&abc_library->vCells, i));
    abc_cells.emplace(abc_cell->pName);
  }

  EXPECT_THAT(abc_cells, Not(Contains("DFFRS_X2")));
}
}  // namespace rmp
