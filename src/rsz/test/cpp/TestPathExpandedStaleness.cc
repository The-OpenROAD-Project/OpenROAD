// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SizeUpMove.hh"
#include "ant/AntennaChecker.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "dpl/Opendp.h"
#include "est/EstimateParasitics.h"
#include "grt/GlobalRouter.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/Graph.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/Sdc.hh"
#include "sta/Sta.hh"
#include "stt/SteinerTreeBuilder.h"
#include "tst/nangate45_fixture.h"
#include "utl/ServiceRegistry.h"

namespace rsz {

static const std::string prefix("_main/src/rsz/test/");

// in1 -> b1(BUF) -> inv1(INV) -> nd1(NAND2) -> out1
class StalePathTest : public tst::Nangate45Fixture
{
 protected:
  StalePathTest()
      : stt_(db_.get(), &logger_),
        service_registry_(&logger_),
        dp_(db_.get(), &logger_),
        ant_(db_.get(), &logger_),
        grt_(&logger_,
             &service_registry_,
             &stt_,
             db_.get(),
             sta_.get(),
             &ant_,
             &dp_),
        ep_(&logger_, &service_registry_, db_.get(), sta_.get(), &stt_, &grt_),
        resizer_(&logger_, db_.get(), sta_.get(), &stt_, &grt_, &dp_, &ep_)
  {
    readLiberty(prefix + "Nangate45/Nangate45_typ.lib");
    db_network_ = sta_->getDbNetwork();
    db_network_->setBlock(block_);
    sta_->postReadDef(block_);

    const char* layer = "metal1";
    makeBTerm(block_,
              "in1",
              {.bpins = {{.layer_name = layer, .rect = {0, 0, 10, 10}}}});
    makeBTerm(block_,
              "in2",
              {.bpins = {{.layer_name = layer, .rect = {0, 100, 10, 110}}}});
    makeBTerm(
        block_,
        "out1",
        {.io_type = odb::dbIoType::OUTPUT,
         .bpins = {{.layer_name = layer, .rect = {990, 990, 1000, 1000}}}});

    makeInst(block_,
             db_->findMaster("BUF_X1"),
             "b1",
             {.location = {100, 100},
              .status = odb::dbPlacementStatus::PLACED,
              .iterms = {{.net_name = "in1", .term_name = "A"},
                         {.net_name = "n0", .term_name = "Z"}}});
    makeInst(block_,
             db_->findMaster("INV_X1"),
             "inv1",
             {.location = {200, 100},
              .status = odb::dbPlacementStatus::PLACED,
              .iterms = {{.net_name = "n0", .term_name = "A"},
                         {.net_name = "n1", .term_name = "ZN"}}});
    makeInst(block_,
             db_->findMaster("NAND2_X1"),
             "nd1",
             {.location = {300, 150},
              .status = odb::dbPlacementStatus::PLACED,
              .iterms = {{.net_name = "n1", .term_name = "A1"},
                         {.net_name = "in2", .term_name = "A2"},
                         {.net_name = "out1", .term_name = "ZN"}}});

    makeClockOn("clk", block_->findBTerm("in1"));
    sta::Sdc* sdc = sta_->cmdSdc();
    sta_->setOutputDelay(db_network_->dbToSta(block_->findBTerm("out1")),
                         sta::RiseFallBoth::riseFall(),
                         sdc->findClock("clk"),
                         sta::RiseFall::rise(),
                         nullptr,
                         false,
                         false,
                         sta::MinMaxAll::all(),
                         false,
                         0.0f,
                         sdc);

    resizer_.initBlock();
    db_->setLogger(&logger_);
    sta_->updateTiming(true);
  }

  // sta::Sdc::makeClock takes ownership of the PinSet* and FloatSeq* and
  // frees them in Clock::setPins / ~Clock, so the raw `new` is not a leak.
  void makeClockOn(const char* clk_name, odb::dbBTerm* bt)
  {
    auto* pins = new sta::PinSet(sta_->network());
    pins->insert(db_network_->dbToSta(bt));
    auto* waveform = new sta::FloatSeq{0.0f, 0.1f};
    sta_->makeClock(clk_name, pins, false, 0.2f, waveform, "", sta_->cmdMode());
  }

  stt::SteinerTreeBuilder stt_;
  utl::ServiceRegistry service_registry_;
  dpl::Opendp dp_;
  ant::AntennaChecker ant_;
  grt::GlobalRouter grt_;
  est::EstimateParasitics ep_;
  rsz::Resizer resizer_;
  sta::dbNetwork* db_network_{nullptr};
};

// Regression for #10210 (stale Path* dereference).
//
// Flow:
//   1. Capture drvr_path at nd1/ZN (terminal logic cell, stays alive).
//   2. Delete upstream b1 + updateTiming -> free b1/inv1 Path[] slots.
//   3. Add new cone + clk2 + updateTiming -> recycle freed slots.
//   4. Assert drvr_path->prevPath()->pin() now decodes to a stale pin
//      (not nd1's real input).
//   5. Call SizeUpMove::doMove: pre-fix derefs stale prev -> SIGSEGV;
//      post-fix reads prev_arc (liberty-stable) -> safe early-return.
TEST_F(StalePathTest, SizeUpMoveSurvivesStalePathAfterUpdateTiming)
{
  sta::Network* network = sta_->network();
  sta::Graph* graph = sta_->ensureGraph();

  // 1. Capture drvr_path at nd1/ZN.
  sta::Instance* nd1 = db_network_->dbToSta(block_->findInst("nd1"));
  sta::Path* drvr_path = sta_->vertexWorstArrivalPath(
      graph->pinDrvrVertex(network->findPin(nd1, "ZN")), sta::MinMax::max());
  ASSERT_NE(drvr_path, nullptr);
  ASSERT_EQ(network->pathName(drvr_path->pin(sta_.get())), "nd1/ZN");

  // 2. Free upstream Path[] slots.
  sta_->deleteInstance(db_network_->dbToSta(block_->findInst("b1")));
  sta_->updateTiming(true);

  // 3. Recycle slots via a fresh cone + clock.
  auto* new_bt = makeBTerm(
      block_,
      "in3",
      {.bpins = {{.layer_name = "metal1", .rect = {0, 300, 10, 310}}}});
  makeBTerm(block_,
            "in4",
            {.bpins = {{.layer_name = "metal1", .rect = {0, 400, 10, 410}}}});
  odb::dbNet::create(block_, "nbuf");
  odb::dbNet::create(block_, "ninv");
  odb::dbNet::create(block_, "nfan");
  makeInst(block_,
           db_->findMaster("BUF_X1"),
           "bnew",
           {.location = {400, 300},
            .status = odb::dbPlacementStatus::PLACED,
            .iterms = {{.net_name = "in3", .term_name = "A"},
                       {.net_name = "nbuf", .term_name = "Z"}}});
  makeInst(block_,
           db_->findMaster("INV_X1"),
           "inew",
           {.location = {450, 300},
            .status = odb::dbPlacementStatus::PLACED,
            .iterms = {{.net_name = "nbuf", .term_name = "A"},
                       {.net_name = "ninv", .term_name = "ZN"}}});
  makeInst(block_,
           db_->findMaster("NAND2_X1"),
           "ndnew",
           {.location = {500, 300},
            .status = odb::dbPlacementStatus::PLACED,
            .iterms = {{.net_name = "ninv", .term_name = "A1"},
                       {.net_name = "in4", .term_name = "A2"},
                       {.net_name = "nfan", .term_name = "ZN"}}});
  makeClockOn("clk2", new_bt);
  sta_->updateTiming(true);

  // 4. Drvr still decodes to nd1/ZN (nd1 survived), but prev slot was
  // recycled: prev_pin is not one of nd1's real inputs (nd1/A1, nd1/A2).
  ASSERT_EQ(network->pathName(drvr_path->pin(sta_.get())), "nd1/ZN");
  const sta::Path* prev = drvr_path->prevPath();
  ASSERT_NE(prev, nullptr);
  const std::string prev_pin = network->pathName(prev->pin(sta_.get()));
  EXPECT_TRUE(prev_pin != "nd1/A1" && prev_pin != "nd1/A2")
      << "prev slot should be recycled; got prev_pin=" << prev_pin;

  // 5. Pre-fix SIGSEGV on stale prev; post-fix safe early-return.
  rsz::SizeUpMove size_up_move(&resizer_);
  size_up_move.init();
  (void) size_up_move.doMove(drvr_path, 0.0f);
}

}  // namespace rsz
