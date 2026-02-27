// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include <unistd.h>

#include <filesystem>
#include <memory>
#include <string>

#include "ant/AntennaChecker.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "gmock/gmock.h"
#include "grt/GlobalRouter.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/lefin.h"
#include "rsz/Resizer.hh"
#include "sta/FuncExpr.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/Search.hh"
#include "sta/SearchClass.hh"
#include "sta/Sta.hh"
#include "stt/SteinerTreeBuilder.h"
#include "tst/fixture.h"
#include "tst/nangate45_fixture.h"
#include "utl/CallBackHandler.h"
#include "utl/Logger.h"
#include "utl/deleter.h"

namespace rsz {

static const std::string prefix("_main/src/rsz/test/");

class BufRemTest : public tst::Nangate45Fixture
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
    library_ = readLiberty(prefix + "Nangate45/Nangate45_typ.lib");
    db_network_ = sta_->getDbNetwork();
    db_network_->setBlock(block_);
    block_->setDieArea(odb::Rect(0, 0, 1000, 1000));
    // register proper callbacks for timer like read_def
    sta_->postReadDef(block_);

    // create a chain consisting of 4 buffers
    const char* layer = "metal1";

    makeBTerm(block_,
              "in1",
              {.bpins = {{.layer_name = layer, .rect = {0, 0, 10, 10}}}});

    odb::dbBTerm* outPort = makeBTerm(
        block_,
        "out1",
        {.io_type = odb::dbIoType::OUTPUT,
         .bpins = {{.layer_name = layer, .rect = {990, 990, 1000, 1000}}}});

    makeBTerm(
        block_,
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
          block_,
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
  sta::Vertex* outVertex_{nullptr};
};

TEST_F(BufRemTest, SlackImproves)
{
  const float origArrival = sta_->arrival(outVertex_,
                                          sta::RiseFallBoth::riseFall(),
                                          sta_->scenes(),
                                          sta::MinMax::max());

  // Remove buffers 'b2' and 'b3' from the buffer chain
  resizer_.initBlock();
  db_->setLogger(&logger_);

  {
    est::IncrementalParasiticsGuard guard(&ep_);

    resizer_.journalBeginTest();
    resizer_.logger()->setDebugLevel(utl::RSZ, "journal", 1);

    auto insts = std::make_unique<sta::InstanceSeq>();
    odb::dbInst* inst1 = block_->findInst("b2");
    sta::Instance* sta_inst1 = db_network_->dbToSta(inst1);
    insts->emplace_back(sta_inst1);

    odb::dbInst* inst2 = block_->findInst("b3");
    sta::Instance* sta_inst2 = db_network_->dbToSta(inst2);
    insts->emplace_back(sta_inst2);

    resizer_.removeBuffers(*insts);
    const float newArrival = sta_->arrival(outVertex_,
                                           sta::RiseFallBoth::riseFall(),
                                           sta_->scenes(),
                                           sta::MinMax::max());

    EXPECT_LT(newArrival, origArrival);
    resizer_.journalRestoreTest();
  }

  const float restoredArrival = sta_->arrival(outVertex_,
                                              sta::RiseFallBoth::riseFall(),
                                              sta_->scenes(),
                                              sta::MinMax::max());

  EXPECT_EQ(restoredArrival, origArrival);
}

}  // namespace rsz
