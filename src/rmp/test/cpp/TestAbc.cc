// Copyright 2023 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <tcl.h>
#include <unistd.h>

#include <array>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <set>

#include "abc_library_factory.h"
#include "base/abc/abc.h"
#include "base/io/ioAbc.h"
#include "base/main/abcapis.h"
#include "db_sta/MakeDbSta.hh"
#include "db_sta/dbSta.hh"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "map/mio/mio.h"
#include "map/scl/sclLib.h"
#include "odb/lefin.h"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/Sta.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"
#include "utl/deleter.h"


namespace abc {
  void* Abc_FrameReadLibGen();
}

namespace rmp {

std::once_flag init_sta_flag;

class AbcTest : public ::testing::Test
{
 protected:
  void SetUp() override
  {
    db_ = utl::deleted_unique_ptr<odb::dbDatabase>(odb::dbDatabase::create(),
                                                   &odb::dbDatabase::destroy);
    std::call_once(init_sta_flag, []() {
      sta::initSta();
      abc::Abc_Start();
    });
    sta_ = std::unique_ptr<sta::dbSta>(ord::makeDbSta());
    sta_->initVars(Tcl_CreateInterp(), db_.get(), &logger_);
    auto path = std::filesystem::canonical("./Nangate45/Nangate45_fast.lib");
    library_ = sta_->readLiberty(path.string().c_str(),
                                 sta_->findCorner("default"),
                                 /*min_max=*/nullptr,
                                 /*infer_latches=*/false);

    odb::lefin lef_reader(
        db_.get(), &logger_, /*ignore_non_routing_layers=*/false);

    auto tech_lef
        = std::filesystem::canonical("./Nangate45/Nangate45_tech.lef");
    auto stdcell_lef
        = std::filesystem::canonical("./Nangate45/Nangate45_stdcell.lef");
    odb::dbLib* lib = lef_reader.createTechAndLib(
        "nangate45", stdcell_lef.string().c_str(), tech_lef.string().c_str());

    sta_->postReadLef(/*tech=*/nullptr, lib);

    sta::Units* units = library_->units();
    power_unit_ = units->powerUnit();
  }

  utl::deleted_unique_ptr<odb::dbDatabase> db_;
  sta::Unit* power_unit_;
  std::unique_ptr<sta::dbSta> sta_;
  sta::LibertyLibrary* library_;
  utl::Logger logger_;
};

TEST_F(AbcTest, CellPropertiesMatchOpenSta)
{
  AbcLibraryFactory factory(&logger_);
  factory.AddDbSta(sta_.get());
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
  factory.AddDbSta(sta_.get());
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
  factory.AddDbSta(sta_.get());
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

TEST_F(AbcTest, ContainsLogicCells)
{
  AbcLibraryFactory factory(&logger_);
  factory.AddDbSta(sta_.get());
  utl::deleted_unique_ptr<abc::SC_Lib> abc_library = factory.Build();

  std::set<std::string> abc_cells;
  using ::testing::Contains;
  using ::testing::Not;

  for (size_t i = 0; i < Vec_PtrSize(&abc_library->vCells); i++) {
    abc::SC_Cell* abc_cell = static_cast<abc::SC_Cell*>(
        abc::Vec_PtrEntry(&abc_library->vCells, i));
    abc_cells.emplace(abc_cell->pName);
  }

  EXPECT_THAT(abc_cells, Contains("AND2_X1"));
  EXPECT_THAT(abc_cells, Contains("AND2_X2"));
  EXPECT_THAT(abc_cells, Contains("AND2_X4"));
  EXPECT_THAT(abc_cells, Contains("AOI21_X1"));
}

// Create standard cell library from dbsta. Then create an
// abc network with a single and gate, and make sure that it
// simulates correctly.
TEST_F(AbcTest, TestLibraryInstallation)
{
  AbcLibraryFactory factory(&logger_);
  factory.AddDbSta(sta_.get());
  utl::deleted_unique_ptr<abc::SC_Lib> abc_library = factory.Build();

  // When you set these params to zero they essentially turned off.
  abc::Abc_SclInstallGenlib(
      abc_library.get(), /*Slew=*/0, /*Gain=*/0, /*nGatesMin=*/0);
  abc::Mio_LibraryTransferCellIds();
  abc::Mio_Library_t* lib
      = static_cast<abc::Mio_Library_t*>(abc::Abc_FrameReadLibGen());

  std::map<std::string, abc::Mio_Gate_t*> gates;
  abc::Mio_Gate_t* gate = abc::Mio_LibraryReadGates(lib);
  while (gate) {
    gates[abc::Mio_GateReadName(gate)] = gate;
    gate = abc::Mio_GateReadNext(gate);
  }

  utl::deleted_unique_ptr<abc::Abc_Ntk_t> network(
      abc::Abc_NtkAlloc(abc::Abc_NtkType_t::ABC_NTK_NETLIST,
                        abc::Abc_NtkFunc_t::ABC_FUNC_MAP,
                        /*fUseMemMan=*/1),
      &abc::Abc_NtkDelete);
  abc::Abc_NtkSetName(network.get(), strdup("test_module"));

  abc::Abc_Obj_t* input_1 = abc::Abc_NtkCreatePi(network.get());
  abc::Abc_Obj_t* input_1_net = abc::Abc_NtkCreateNet(network.get());
  abc::Abc_Obj_t* input_2 = abc::Abc_NtkCreatePi(network.get());
  abc::Abc_Obj_t* input_2_net = abc::Abc_NtkCreateNet(network.get());
  abc::Abc_Obj_t* output = abc::Abc_NtkCreatePo(network.get());
  abc::Abc_Obj_t* output_net = abc::Abc_NtkCreateNet(network.get());
  abc::Abc_Obj_t* and_gate = abc::Abc_NtkCreateNode(network.get());

  abc::Abc_ObjSetData(and_gate, gates["AND2_X1"]);

  abc::Abc_ObjAddFanin(input_1_net, input_1);
  abc::Abc_ObjAddFanin(input_2_net, input_2);

  // Gate order is technically dependent on the order in which the port
  // appears in the Mio_Gate_t struct. In practice you should go a build
  // a port_name -> index map type thing to make sure the right ports
  // are connected.
  abc::Abc_ObjAddFanin(and_gate, input_1_net);  // A
  abc::Abc_ObjAddFanin(and_gate, input_2_net);  // B

  std::string output_name = "out";
  abc::Abc_ObjAssignName(output_net, output_name.data(), /*pSuffix=*/nullptr);
  abc::Abc_ObjAddFanin(output_net, and_gate);

  abc::Abc_ObjAddFanin(output, output_net);

  utl::deleted_unique_ptr<abc::Abc_Ntk_t> logic_network(
      abc::Abc_NtkToLogic(network.get()), &abc::Abc_NtkDelete);

  std::array<int, 2> input_vector = {1, 1};
  utl::deleted_unique_ptr<int> output_vector(
      abc::Abc_NtkVerifySimulatePattern(logic_network.get(),
                                        input_vector.data()),
      &free);

  EXPECT_EQ(output_vector.get()[0], 1);  // Expect that 1 & 1 == 1
}
}  // namespace rmp
