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

#include "AbstractSteinerRenderer.h"
#include "db_sta/MakeDbSta.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "dpl/MakeOpendp.h"
#include "gmock/gmock.h"
#include "grt/GlobalRouter.h"
#include "gtest/gtest.h"
#include "odb/lefin.h"
#include "rsz/MakeResizer.hh"
#include "rsz/Resizer.hh"
#include "sta/Corner.hh"
#include "sta/FuncExpr.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/PathAnalysisPt.hh"
#include "sta/Search.hh"
#include "sta/Sta.hh"
#include "sta/Units.hh"
#include "stt/SteinerTreeBuilder.h"
#include "utl/Logger.h"
#include "utl/deleter.h"

namespace rsz {

std::once_flag init_sta_flag;

class BufRemTest : public ::testing::Test
{
 protected:
  void SetUp() override
  {
    // this will be so much easier with read_def
    db_ = utl::deleted_unique_ptr<odb::dbDatabase>(odb::dbDatabase::create(),
                                                   &odb::dbDatabase::destroy);
    std::call_once(init_sta_flag, []() { sta::initSta(); });
    sta_ = std::unique_ptr<sta::dbSta>(ord::makeDbSta());
    sta_->initVars(Tcl_CreateInterp(), db_.get(), &logger_);
    auto path = std::filesystem::canonical("./Nangate45/Nangate45_typ.lib");
    library_ = sta_->readLiberty(path.string().c_str(),
                                 sta_->findCorner("default"),
                                 sta::MinMaxAll::all(),
                                 /*infer_latches=*/false);
    odb::lefin lefParser(
        db_.get(), &logger_, /*ignore_non_routing_layers*/ false);
    const char* libName = "Nangate45.lef";
    odb::dbLib* dbLib = lefParser.createTechAndLib(
        "tech", libName, "./Nangate45/Nangate45.lef");

    sta_->postReadLef(/*tech=*/nullptr, dbLib);

    sta::Units* units = library_->units();
    power_unit_ = units->powerUnit();
    db_network_ = sta_->getDbNetwork();

    // create a chain consisting of 4 buffers
    odb::dbChip* chip = odb::dbChip::create(db_.get());
    odb::dbBlock* block = odb::dbBlock::create(chip, "top");
    db_network_->setBlock(block);
    block->setDieArea(odb::Rect(0, 0, 1000, 1000));
    // register proper callbacks for timer like read_def
    sta_->postReadDef(block);
    odb::dbModule* module = odb::dbModule::create(block, "top");
    odb::dbMaster* bufx1 = db_->findMaster("BUF_X1");
    odb::dbMaster* bufx2 = db_->findMaster("BUF_X2");
    odb::dbMaster* bufx4 = db_->findMaster("BUF_X4");
    odb::dbMaster* bufx8 = db_->findMaster("BUF_X8");

    odb::dbInst* b1 = odb::dbInst::create(block, bufx1, "b1", module);
    b1->setSourceType(odb::dbSourceType::TIMING);
    b1->setLocation(100, 100);
    b1->setPlacementStatus(odb::dbPlacementStatus::PLACED);

    odb::dbInst* b2 = odb::dbInst::create(block, bufx2, "b2", module);
    b2->setSourceType(odb::dbSourceType::TIMING);
    b2->setLocation(200, 200);
    b2->setPlacementStatus(odb::dbPlacementStatus::PLACED);

    odb::dbInst* b3 = odb::dbInst::create(block, bufx4, "b3", module);
    b3->setSourceType(odb::dbSourceType::TIMING);
    b3->setLocation(300, 300);
    b3->setPlacementStatus(odb::dbPlacementStatus::PLACED);

    odb::dbInst* b4 = odb::dbInst::create(block, bufx8, "b4", module);
    b4->setSourceType(odb::dbSourceType::TIMING);
    b4->setLocation(400, 400);
    b4->setPlacementStatus(odb::dbPlacementStatus::PLACED);

    odb::dbInst* b5 = odb::dbInst::create(block, bufx8, "b5", module);
    b4->setSourceType(odb::dbSourceType::TIMING);
    b4->setLocation(500, 500);
    b4->setPlacementStatus(odb::dbPlacementStatus::PLACED);

    odb::dbNet* in1 = odb::dbNet::create(block, "in1");
    odb::dbNet* n1 = odb::dbNet::create(block, "n1");
    odb::dbNet* n2 = odb::dbNet::create(block, "n2");
    odb::dbNet* n3 = odb::dbNet::create(block, "n3");
    odb::dbNet* out1 = odb::dbNet::create(block, "out1");
    odb::dbNet* out2 = odb::dbNet::create(block, "out2");

    odb::dbWire::create(in1);
    odb::dbWire::create(n1);
    odb::dbWire::create(n2);
    odb::dbWire::create(n3);
    odb::dbWire::create(out1);
    odb::dbWire::create(out2);

    in1->setSigType(odb::dbSigType::SIGNAL);
    n1->setSigType(odb::dbSigType::SIGNAL);
    n2->setSigType(odb::dbSigType::SIGNAL);
    n3->setSigType(odb::dbSigType::SIGNAL);
    out1->setSigType(odb::dbSigType::SIGNAL);

    odb::dbBTerm* inPort = odb::dbBTerm::create(in1, "in1");
    inPort->setIoType(odb::dbIoType::INPUT);
    odb::dbBPin* inPin = odb::dbBPin::create(inPort);
    odb::dbTechLayer* layer = block->getTech()->findRoutingLayer(0);
    odb::dbBox::create(inPin, layer, 0, 0, 10, 10);
    inPin->setPlacementStatus(odb::dbPlacementStatus::FIRM);

    odb::dbBTerm* outPort = odb::dbBTerm::create(out1, "out1");
    outPort->setIoType(odb::dbIoType::OUTPUT);
    odb::dbBPin* outPin = odb::dbBPin::create(outPort);
    layer = block->getTech()->findRoutingLayer(0);
    odb::dbBox::create(outPin, layer, 990, 990, 1000, 1000);
    outPin->setPlacementStatus(odb::dbPlacementStatus::FIRM);

    odb::dbBTerm* outPort2 = odb::dbBTerm::create(out2, "out2");
    outPort2->setIoType(odb::dbIoType::OUTPUT);
    odb::dbBPin* outPin2 = odb::dbBPin::create(outPort2);
    layer = block->getTech()->findRoutingLayer(0);
    odb::dbBox::create(outPin2, layer, 980, 980, 1000, 990);
    outPin2->setPlacementStatus(odb::dbPlacementStatus::FIRM);

    odb::dbITerm* inTerm = getFirstInput(b1);
    inTerm->connect(in1);

    odb::dbITerm* outTerm = b1->getFirstOutput();
    inTerm = getFirstInput(b2);
    inTerm->connect(n1);
    outTerm->connect(n1);

    outTerm = b2->getFirstOutput();
    inTerm = getFirstInput(b3);
    inTerm->connect(n2);
    outTerm->connect(n2);

    outTerm = b3->getFirstOutput();
    inTerm = getFirstInput(b4);
    inTerm->connect(n3);
    outTerm->connect(n3);

    outTerm = b4->getFirstOutput();
    outTerm->connect(out1);

    outTerm = b5->getFirstOutput();
    outTerm->connect(out2);
    inTerm = getFirstInput(b5);
    inTerm->connect(n1);

    // initialize STA
    sta::Corner* corner = sta_->cmdCorner();
    sta::PathAPIndex pathAPIndex
        = corner->findPathAnalysisPt(sta::MinMax::max())->index();
    sta::Corners* corners = sta_->search()->corners();
    pathAnalysisPt_ = corners->findPathAnalysisPt(pathAPIndex);
    sta::Graph* graph = sta_->ensureGraph();
    sta::Pin* outStaPin = db_network_->dbToSta(outPort);
    outVertex_ = graph->pinLoadVertex(outStaPin);
  }

  odb::dbITerm* getFirstInput(odb::dbInst* inst) const
  {
    odb::dbSet<odb::dbITerm> iterms = inst->getITerms();
    for (odb::dbITerm* iterm : iterms) {
      if (iterm->isInputSignal()) {
        return iterm;
      }
    }
    return nullptr;
  }

  utl::deleted_unique_ptr<odb::dbDatabase> db_;
  sta::Unit* power_unit_;
  std::unique_ptr<sta::dbSta> sta_;
  sta::LibertyLibrary* library_;
  utl::Logger logger_;
  sta::dbNetwork* db_network_;
  sta::PathAnalysisPt* pathAnalysisPt_;
  rsz::Resizer* resizer_;
  sta::Vertex* outVertex_;
};

TEST_F(BufRemTest, SlackImproves)
{
  // initializer resizer
  resizer_ = new rsz::Resizer;
  stt::SteinerTreeBuilder* stt = new stt::SteinerTreeBuilder;
  grt::GlobalRouter* grt = new grt::GlobalRouter;
  dpl::Opendp* dp = new dpl::Opendp;
  resizer_->init(&logger_, db_.get(), sta_.get(), stt, grt, dp, nullptr);

  float origArrival
      = sta_->vertexArrival(outVertex_, sta::RiseFall::rise(), pathAnalysisPt_);

  // Remove buffers 'b2' and 'b3' from the buffer chain
  odb::dbChip* chip = db_->getChip();
  odb::dbBlock* block = chip->getBlock();

  resizer_->initBlock();
  db_->setLogger(&logger_);
  resizer_->journalBeginTest();
  resizer_->logger()->setDebugLevel(utl::RSZ, "journal", 1);

  auto insts = std::make_unique<sta::InstanceSeq>();
  odb::dbInst* inst1 = block->findInst("b2");
  sta::Instance* sta_inst1 = db_network_->dbToSta(inst1);
  insts->emplace_back(sta_inst1);

  odb::dbInst* inst2 = block->findInst("b3");
  sta::Instance* sta_inst2 = db_network_->dbToSta(inst2);
  insts->emplace_back(sta_inst2);

  resizer_->removeBuffers(*insts, /* recordJournal */ true);

  resizer_->journalRestoreTest();

  float newArrival
      = sta_->vertexArrival(outVertex_, sta::RiseFall::rise(), pathAnalysisPt_);

  EXPECT_LE(newArrival, origArrival);
}

}  // namespace rsz
