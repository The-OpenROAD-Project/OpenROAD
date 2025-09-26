// Copyright 2023 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <string.h>
#include <tcl.h>
#include <unistd.h>

#include <cstdlib>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "base/abc/abc.h"
#include "base/main/abcapis.h"
#include "cut/abc_library_factory.h"
#include "cut/logic_extractor.h"
#include "db_sta/MakeDbSta.hh"
#include "db_sta/dbReadVerilog.hh"
#include "db_sta/dbSta.hh"
#include "delay_optimization_strategy.h"
#include "gtest/gtest.h"
#include "map/mio/mio.h"
#include "map/scl/sclLib.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/lefin.h"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/Sta.hh"
#include "sta/Units.hh"
#include "sta/VerilogReader.hh"
#include "utl/Logger.h"
#include "utl/deleter.h"
#include "zero_slack_strategy.h"

// Headers have duplicate declarations so we include
// a forward one to get at this function without angering
// gcc.
namespace abc {
void* Abc_FrameReadLibGen();
}

namespace rmp {

using cut::AbcLibrary;
using cut::AbcLibraryFactory;
using cut::LogicCut;
using cut::LogicExtractorFactory;

std::once_flag init_sta_flag;

class AbcTest : public ::testing::Test
{
 protected:
  void SetUp() override
  {
    db_ = utl::UniquePtrWithDeleter<odb::dbDatabase>(odb::dbDatabase::create(),
                                                     &odb::dbDatabase::destroy);
    std::call_once(init_sta_flag, []() {
      sta::initSta();
      abc::Abc_Start();
    });
    db_->setLogger(&logger_);
    sta_
        = std::make_unique<sta::dbSta>(Tcl_CreateInterp(), db_.get(), &logger_);
    auto path = std::filesystem::canonical("./Nangate45/Nangate45_typ.lib");
    library_ = sta_->readLiberty(path.string().c_str(),
                                 sta_->findCorner("default"),
                                 /*min_max=*/sta::MinMaxAll::all(),
                                 /*infer_latches=*/false);

    odb::lefin lef_reader(
        db_.get(), &logger_, /*ignore_non_routing_layers=*/false);

    auto tech_lef
        = std::filesystem::canonical("./Nangate45/Nangate45_tech.lef");
    auto stdcell_lef
        = std::filesystem::canonical("./Nangate45/Nangate45_stdcell.lef");
    odb::dbTech* tech
        = lef_reader.createTech("nangate45", tech_lef.string().c_str());
    odb::dbLib* lib
        = lef_reader.createLib(tech, "nangate45", stdcell_lef.string().c_str());

    sta_->postReadLef(/*tech=*/nullptr, lib);

    sta::Units* units = library_->units();
    power_unit_ = units->powerUnit();
  }

  void LoadVerilog(const std::string& file_name, const std::string& top = "top")
  {
    // Assumes module name is "top" and clock name is "clk"
    sta::dbNetwork* network = sta_->getDbNetwork();
    ord::dbVerilogNetwork verilog_network(sta_.get());

    sta::VerilogReader verilog_reader(&verilog_network);
    verilog_reader.read(file_name.c_str());

    ord::dbLinkDesign(top.c_str(),
                      &verilog_network,
                      db_.get(),
                      &logger_,
                      /*hierarchy = */ false);

    sta_->postReadDb(db_.get());

    sta::Cell* top_cell = network->cell(network->topInstance());
    sta::Port* clk_port = network->findPort(top_cell, "clk");
    sta::Pin* clk_pin = network->findPin(network->topInstance(), clk_port);

    sta::PinSet* pinset = new sta::PinSet(network);
    pinset->insert(clk_pin);

    // 0.5ns
    double period = sta_->units()->timeUnit()->userToSta(0.5);
    sta::FloatSeq* waveform = new sta::FloatSeq;
    waveform->push_back(0);
    waveform->push_back(period / 2.0);

    sta_->makeClock("core_clock",
                    pinset,
                    /*add_to_pins=*/false,
                    /*period=*/period,
                    waveform,
                    /*comment=*/nullptr);

    sta_->ensureGraph();
    sta_->ensureLevelized();
  }
  std::map<std::string, int> AbcLogicNetworkNameToPrimaryOutputIds(
      abc::Abc_Ntk_t* network)
  {
    std::map<std::string, int> primary_output_name_to_index;
    for (int i = 0; i < abc::Abc_NtkPoNum(network); i++) {
      abc::Abc_Obj_t* po = abc::Abc_NtkPo(network, i);
      std::string po_name = abc::Abc_ObjName(po);
      primary_output_name_to_index[po_name] = i;
    }

    return primary_output_name_to_index;
  }

  utl::UniquePtrWithDeleter<odb::dbDatabase> db_;
  sta::Unit* power_unit_;
  std::unique_ptr<sta::dbSta> sta_;
  sta::LibertyLibrary* library_;
  utl::Logger logger_;
};

TEST_F(AbcTest, InsertingMappedLogicAfterOptimizationCutDoesNotThrow)
{
  AbcLibraryFactory factory(&logger_);
  factory.AddDbSta(sta_.get());
  AbcLibrary abc_library = factory.Build();

  LoadVerilog("aes_nangate45.v", /*top=*/"aes_cipher_top");

  sta::dbNetwork* network = sta_->getDbNetwork();
  sta::Vertex* flop_input_vertex = nullptr;
  for (sta::Vertex* vertex : *sta_->endpoints()) {
    if (std::string(vertex->name(network)) == "_33122_/D") {
      flop_input_vertex = vertex;
    }
  }
  EXPECT_NE(flop_input_vertex, nullptr);

  LogicExtractorFactory logic_extractor(sta_.get(), &logger_);
  logic_extractor.AppendEndpoint(flop_input_vertex);
  LogicCut cut = logic_extractor.BuildLogicCut(abc_library);

  utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> mapped_abc_network
      = cut.BuildMappedAbcNetwork(abc_library, network, &logger_);

  DelayOptimizationStrategy strat;
  utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> remapped
      = strat.Optimize(mapped_abc_network.get(), abc_library, &logger_);

  utl::UniqueName unique_name;
  EXPECT_NO_THROW(cut.InsertMappedAbcNetwork(
      remapped.get(), abc_library, network, unique_name, &logger_));
}

TEST_F(AbcTest, ResynthesisStrategyDoesNotThrow)
{
  LoadVerilog("aes_nangate45.v", /*top=*/"aes_cipher_top");

  utl::UniqueName name_generator;
  ZeroSlackStrategy zero_slack;
  EXPECT_NO_THROW(
      zero_slack.OptimizeDesign(sta_.get(), name_generator, nullptr, &logger_));
}

}  // namespace rmp
