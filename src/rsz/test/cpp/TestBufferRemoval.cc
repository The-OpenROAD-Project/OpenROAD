// Copyright 2023 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <unistd.h>

#include <filesystem>
#include <memory>
#include <mutex>

#include "ant/AntennaChecker.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "gmock/gmock.h"
#include "grt/GlobalRouter.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/lefin.h"
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
#include "tcl.h"
#include "tst/fixture.h"
#include "utl/CallBackHandler.h"
#include "utl/Logger.h"
#include "utl/deleter.h"

namespace rsz {

static const std::string prefix("_main/src/rsz/test/");

class BufRemTest : public tst::Fixture
{
 protected:
  BufRemTest()
      :  // initializer resizer
        stt_(db_.get(), &logger_),
        callback_handler_(&logger_),
        dp_(db_.get(), &logger_),
        ant_(db_.get(), &logger_),
        grt_(&logger_,
             &callback_handler_,
             &stt_,
             db_.get(),
             sta_.get(),
             &ant_,
             &dp_),
        ep_(&logger_, &callback_handler_, db_.get(), sta_.get(), &stt_, &grt_),
        resizer_(&logger_, db_.get(), sta_.get(), &stt_, &grt_, &dp_, &ep_)
  {
  }

  void SetUp() override
  {
    library_ = readLiberty(prefix + "Nangate45/Nangate45_typ.lib");
    loadTechAndLib("tech", "Nangate45", prefix + "Nangate45/Nangate45.lef");

    db_network_ = sta_->getDbNetwork();

    // create a chain consisting of 4 buffers
    odb::dbChip* chip = odb::dbChip::create(db_.get(), db_->getTech());
    odb::dbBlock* block = odb::dbBlock::create(chip, "top");
    db_network_->setBlock(block);
    block->setDieArea(odb::Rect(0, 0, 1000, 1000));
    // register proper callbacks for timer like read_def
    sta_->postReadDef(block);

    const char* layer = "metal1";

    makeBTerm(block,
              "in1",
              {.bpins = {{.layer_name = layer, .rect = {0, 0, 10, 10}}}});

    odb::dbBTerm* outPort = makeBTerm(
        block,
        "out1",
        {.io_type = odb::dbIoType::OUTPUT,
         .bpins = {{.layer_name = layer, .rect = {990, 990, 1000, 1000}}}});

    makeBTerm(
        block,
        "out2",
        {.io_type = odb::dbIoType::OUTPUT,
         .bpins = {{.layer_name = layer, .rect = {980, 980, 1000, 990}}}});

    auto makeInst = [&](const char* master_name,
                        const char* inst_name,
                        const odb::Point& location,
                        const char* in_net,
                        const char* out_net) {
      odb::dbMaster* master = db_->findMaster(master_name);
      return tst::Fixture::makeInst(
          block,
          master,
          inst_name,
          {.location = location,
           .status = odb::dbPlacementStatus::PLACED,
           .iterms = {{.net_name = in_net, .term_name = "A"},
                      {.net_name = out_net, .term_name = "Z"}}});
    };

    makeInst("BUF_X1", "b1", {100, 100}, "in1", "n1");
    makeInst("BUF_X2", "b2", {200, 200}, "n1", "n2");
    makeInst("BUF_X4", "b3", {300, 300}, "n2", "n3");
    makeInst("BUF_X8", "b4", {400, 400}, "n3", "out1");
    makeInst("BUF_X8", "b5", {500, 500}, "n1", "out2");

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

  stt::SteinerTreeBuilder stt_;
  utl::CallBackHandler callback_handler_;
  dpl::Opendp dp_;
  ant::AntennaChecker ant_;
  grt::GlobalRouter grt_;
  est::EstimateParasitics ep_;
  rsz::Resizer resizer_;

  sta::LibertyLibrary* library_{nullptr};
  sta::dbNetwork* db_network_{nullptr};
  sta::PathAnalysisPt* pathAnalysisPt_{nullptr};
  sta::Vertex* outVertex_{nullptr};
};

TEST_F(BufRemTest, SlackImproves)
{
  const float origArrival
      = sta_->vertexArrival(outVertex_, sta::RiseFall::rise(), pathAnalysisPt_);

  // Remove buffers 'b2' and 'b3' from the buffer chain
  odb::dbChip* chip = db_->getChip();
  odb::dbBlock* block = chip->getBlock();

  resizer_.initBlock();
  db_->setLogger(&logger_);

  {
    est::IncrementalParasiticsGuard guard(&ep_);

    resizer_.journalBeginTest();
    resizer_.logger()->setDebugLevel(utl::RSZ, "journal", 1);

    auto insts = std::make_unique<sta::InstanceSeq>();
    odb::dbInst* inst1 = block->findInst("b2");
    sta::Instance* sta_inst1 = db_network_->dbToSta(inst1);
    insts->emplace_back(sta_inst1);

    odb::dbInst* inst2 = block->findInst("b3");
    sta::Instance* sta_inst2 = db_network_->dbToSta(inst2);
    insts->emplace_back(sta_inst2);

    resizer_.removeBuffers(*insts);
    resizer_.journalRestoreTest();
  }

  float newArrival
      = sta_->vertexArrival(outVertex_, sta::RiseFall::rise(), pathAnalysisPt_);

  EXPECT_LE(newArrival, origArrival);
}

}  // namespace rsz
