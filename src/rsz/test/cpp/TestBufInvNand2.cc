// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include <string>

#include "ant/AntennaChecker.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "dpl/Opendp.h"
#include "est/EstimateParasitics.h"
#include "grt/GlobalRouter.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "rsz/Resizer.hh"
#include "sta/Graph.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/Sdc.hh"
#include "sta/SdcClass.hh"
#include "sta/Sta.hh"
#include "stt/SteinerTreeBuilder.h"
#include "tst/nangate45_fixture.h"
#include "utl/ServiceRegistry.h"

namespace rsz {

static const std::string prefix("_main/src/rsz/test/");

// in1 -> b1(BUF) -> inv1(INV) -> nd1(NAND2) -> out1
class BufInvNand2Test : public tst::Nangate45Fixture
{
 protected:
  BufInvNand2Test()
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
//   1. Capture drvr_path at nd1/ZN and snapshot prevPath() pointer + pin.
//   2. Delete upstream b1 + updateTiming -> free b1/inv1 Path[] slots.
//   3. Add a fresh BUF + clock + updateTiming -> recycle freed slots.
//   4. Assert the captured Path's prev slot has been recycled: pin()
//      decodes to data that belongs to a different instance than nd1's
//      real input.  When the drvr slot itself is preserved, also assert
//      the strict stale-pointer signature (same raw address, different
//      content).
TEST_F(BufInvNand2Test, StalePrevPathAfterUpdateTiming)
{
  sta::Network* network = sta_->network();
  sta::Graph* graph = sta_->ensureGraph();

  // 1. Capture drvr_path at nd1/ZN and snapshot prev identity.
  sta::Instance* nd1 = db_network_->dbToSta(block_->findInst("nd1"));
  sta::Path* drvr_path = sta_->vertexWorstArrivalPath(
      graph->pinDrvrVertex(network->findPin(nd1, "ZN")), sta::MinMax::max());
  ASSERT_NE(drvr_path, nullptr);
  ASSERT_EQ(network->pathName(drvr_path->pin(sta_.get())), "nd1/ZN");
  const sta::Path* prev_before = drvr_path->prevPath();
  ASSERT_NE(prev_before, nullptr);
  const std::string prev_pin_before
      = network->pathName(prev_before->pin(sta_.get()));

  // 2. Free upstream Path[] slots.
  sta_->deleteInstance(db_network_->dbToSta(block_->findInst("b1")));
  sta_->updateTiming(true);

  // 3. Recycle freed slots via a single fresh BUF driven by a new clock.
  auto* new_bt = makeBTerm(
      block_,
      "in3",
      {.bpins = {{.layer_name = "metal1", .rect = {0, 300, 10, 310}}}});
  odb::dbNet::create(block_, "nfan");
  makeInst(block_,
           db_->findMaster("BUF_X1"),
           "bnew",
           {.location = {400, 300},
            .status = odb::dbPlacementStatus::PLACED,
            .iterms = {{.net_name = "in3", .term_name = "A"},
                       {.net_name = "nfan", .term_name = "Z"}}});
  makeClockOn("clk2", new_bt);
  sta_->updateTiming(true);

  // 4. Staleness evidence.  Allocator behaviour decides which slot lands
  // on the recycled memory:
  //   (a) drvr slot preserved + prev slot recycled (glibc in our CI) --
  //       strict stale-pointer signature: same raw prev address, but
  //       pin() decodes to content from an unrelated instance.
  //   (b) drvr slot itself recycled (other allocators) -- drvr pin decodes
  //       to something other than nd1/ZN.
  // Either case proves the captured raw Path* outlived the slot.
  const std::string drvr_pin_after
      = network->pathName(drvr_path->pin(sta_.get()));
  const sta::Path* prev_after = drvr_path->prevPath();
  const std::string prev_pin_after
      = prev_after ? network->pathName(prev_after->pin(sta_.get()))
                   : std::string("<null>");
  const bool drvr_recycled = drvr_pin_after != "nd1/ZN";
  const bool prev_recycled = prev_after != nullptr && prev_pin_after != "nd1/A1"
                             && prev_pin_after != "nd1/A2";
  EXPECT_TRUE(drvr_recycled || prev_recycled)
      << "expected slot reuse to be demonstrable; drvr=" << drvr_pin_after
      << " prev=" << prev_pin_after;
  if (!drvr_recycled && prev_after != nullptr) {
    EXPECT_EQ(prev_after, prev_before)
        << "stale-pointer signature: prev_path_ address unchanged";
    EXPECT_NE(prev_pin_after, prev_pin_before)
        << "but slot content should differ after free+reuse. before="
        << prev_pin_before << " after=" << prev_pin_after;
  }
}

}  // namespace rsz
