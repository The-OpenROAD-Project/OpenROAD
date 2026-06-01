// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// These STL headers are intentionally pre-included before exposing protected
// STA fields below. Otherwise the access-specifier macro can leak into
// transitive standard-library includes on some toolchains.
// NOLINTBEGIN(misc-include-cleaner)
#include <array>
#include <functional>
#include <map>
#include <optional>
#include <ostream>
#include <ranges>
#include <set>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>
// NOLINTEND(misc-include-cleaner)

#include <cstddef>
#include <memory>

// Expose STA invalidation sets so the tests can verify the side effect of
// EstimateParasitics::updateParasitics() without adding production accessors.
#define private public
#define protected public
#include "sta/GraphDelayCalc.hh"
#include "sta/Search.hh"
#undef protected
#undef private

#include "db_sta/dbNetwork.hh"
#include "est/EstimateParasitics.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/Mode.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/SdcClass.hh"
#include "sta/Units.hh"
#include "tst/IntegratedFixture.h"

namespace est {

class TestEstimateParasitics : public tst::IntegratedFixture
{
 protected:
  TestEstimateParasitics()
      : tst::IntegratedFixture(tst::IntegratedFixture::Technology::kNangate45,
                               "_main/src/est/test/")
  {
  }

  sta::Pin* findTopPin(const char* port_name) const
  {
    sta::Instance* top_inst = db_network_->topInstance();
    sta::Cell* top_cell = db_network_->cell(top_inst);
    if (top_cell == nullptr) {
      ADD_FAILURE() << "missing top cell";
      return nullptr;
    }

    sta::Port* port = db_network_->findPort(top_cell, port_name);
    if (port == nullptr) {
      ADD_FAILURE() << "missing top port " << port_name;
      return nullptr;
    }

    sta::Pin* pin = db_network_->findPin(top_inst, port);
    if (pin == nullptr) {
      ADD_FAILURE() << "missing top pin " << port_name;
      return nullptr;
    }
    return pin;
  }

  sta::Net* flatNet(sta::Pin* pin) const
  {
    odb::dbNet* db_net = nullptr;
    if (db_network_->isTopLevelPort(pin)) {
      db_net = db_network_->flatNet(db_network_->term(pin));
    } else {
      db_net = db_network_->flatNet(pin);
    }
    if (db_net == nullptr) {
      ADD_FAILURE() << "missing flat net for " << db_network_->pathName(pin);
      return nullptr;
    }

    sta::Net* net = db_network_->dbToSta(db_net);
    if (net == nullptr) {
      ADD_FAILURE() << "missing sta net for " << db_net->getName();
      return nullptr;
    }
    return net;
  }

  void makeClock(const char* clock_name, sta::Pin* pin) const
  {
    sta::PinSet pins(db_network_);
    pins.insert(pin);

    const double period = sta_->units()->timeUnit()->userToSta(1.0);
    sta::FloatSeq waveform;
    waveform.push_back(0.0);
    waveform.push_back(period / 2.0);

    sta_->makeClock(
        clock_name, pins, false, period, waveform, "", sta_->cmdMode());
  }

  void resizeDff(const char* inst_name)
  {
    odb::dbInst* db_inst = block_->findInst(inst_name);
    ASSERT_NE(db_inst, nullptr) << "missing instance " << inst_name;

    sta::Instance* inst = db_network_->dbToSta(db_inst);
    ASSERT_NE(inst, nullptr) << "missing sta instance " << inst_name;

    sta::LibertyCell* dff_x2 = sta_->network()->findLibertyCell("DFF_X2");
    ASSERT_NE(dff_x2, nullptr);
    ASSERT_TRUE(resizer_.replaceCell(inst, dff_x2));
  }

  void clearTimingInvalidations() const
  {
    sta_->graphDelayCalc()->invalid_delays_.clear();
    sta_->graphDelayCalc()->invalid_check_edges_.clear();
    sta_->graphDelayCalc()->invalid_latch_edges_.clear();
    sta_->search()->invalid_arrivals_.clear();
    sta_->search()->invalid_requireds_.clear();
    sta_->search()->invalid_tns_.clear();
  }

  size_t timingInvalidationCount() const
  {
    return sta_->graphDelayCalc()->invalid_delays_.size()
           + sta_->graphDelayCalc()->invalid_check_edges_.size()
           + sta_->graphDelayCalc()->invalid_latch_edges_.size()
           + sta_->search()->invalid_arrivals_.size()
           + sta_->search()->invalid_requireds_.size()
           + sta_->search()->invalid_tns_.size();
  }

  void expectNoConnectedPinInvalidated(const sta::Net* net) const
  {
    std::unique_ptr<sta::NetConnectedPinIterator> pin_iter{
        sta_->network()->connectedPinIterator(net)};
    while (pin_iter->hasNext()) {
      const sta::Pin* pin = pin_iter->next();
      sta::Vertex* vertex = nullptr;
      sta::Vertex* bidirect_drvr_vertex = nullptr;
      sta_->graph()->pinVertices(pin, vertex, bidirect_drvr_vertex);
      expectNoVertexInvalidated(vertex, pin);
      expectNoVertexInvalidated(bidirect_drvr_vertex, pin);
    }
  }

 private:
  void expectNoVertexInvalidated(sta::Vertex* vertex, const sta::Pin* pin) const
  {
    if (vertex == nullptr) {
      return;
    }

    EXPECT_EQ(sta_->graphDelayCalc()->invalid_delays_.find(vertex),
              sta_->graphDelayCalc()->invalid_delays_.end())
        << db_network_->pathName(pin);
    EXPECT_EQ(sta_->search()->invalid_arrivals_.find(vertex),
              sta_->search()->invalid_arrivals_.end())
        << db_network_->pathName(pin);
    EXPECT_EQ(sta_->search()->invalid_requireds_.find(vertex),
              sta_->search()->invalid_requireds_.end())
        << db_network_->pathName(pin);
  }
};

// Verifies that an ideal clock net can be present in the incremental
// parasitic invalidation set without forcing STA delay invalidation.
//
// DFF resizing may mark the clock net parasitics invalid. For an ideal clock,
// those parasitics do not contribute to clock arrival/slew, so updateParasitics
// should skip both RC re-estimation and delaysInvalidFromFanin() for that net.
TEST_F(TestEstimateParasitics, IdealClockNetSkipsStaInvalidation)
{
  // Build a small clocked design and seed valid timing/parasitic state.
  readVerilogAndSetup("TestEstimateParasitics.v");
  sta_->updateTiming(true);

  // Use the default ideal clock from IntegratedFixture::initStaDefaultSdc().
  sta::Pin* clk_pin = findTopPin("clk");
  ASSERT_NE(clk_pin, nullptr);
  sta::Net* clk_net = flatNet(clk_pin);
  ASSERT_NE(clk_net, nullptr);

  // Model the ECO source: resizing a DFF is the class of netlist edit that can
  // make the clock net appear in the parasitic invalidation set.
  resizeDff("reg0");

  // Clear any ordinary resize-related STA invalidation so the assertions below
  // measure only updateParasitics() side effects.
  sta_->updateTiming(true);
  clearTimingInvalidations();
  ASSERT_EQ(timingInvalidationCount(), 0U);

  // Seed the exact condition under test: an ideal clock net is pending in
  // EstimateParasitics' incremental invalidation set.
  ep_.setParasiticsSrc(ParasiticsSrc::kPlacement);
  ep_.setIncrementalParasiticsEnabled(true);
  ep_.parasiticsInvalid(clk_net);
  ASSERT_TRUE(ep_.hasParasiticsInvalid());

  ep_.updateParasitics();

  // A regression calls sta_->delaysInvalidFromFanin(clk_net), which invalidates
  // the top clock port and every ideal CK load vertex.
  EXPECT_EQ(timingInvalidationCount(), 0U);
  expectNoConnectedPinInvalidated(clk_net);
  ep_.setIncrementalParasiticsEnabled(false);
}

// Verifies multi-mode ideal-clock classification for scan clocks.
//
// The original implementation rejected a pin if isIdealClock(pin, mode) was
// false in any mode. That is wrong for scan clocks that are only created in a
// test mode: the scan pin is not a clock in function mode, so that mode must be
// ignored. The fixed logic first checks isClock(pin, mode), then requires ideal
// status only in modes where the pin is actually a clock.
TEST_F(TestEstimateParasitics, ScanClockIdealOnlyInTestMode)
{
  // Do not create the default SDC. This test constructs function/test modes
  // explicitly so scan_clk is intentionally absent from function mode.
  readVerilogAndSetup("TestEstimateParasitics.v", false);

  // Create two modes, but create scan_clk only in test mode.
  sta_->setCmdMode("function");
  sta::Mode* function_mode = sta_->cmdMode();
  sta_->setCmdMode("test");
  sta::Mode* test_mode = sta_->cmdMode();

  sta::Pin* scan_clk_pin = findTopPin("scan_clk");
  ASSERT_NE(scan_clk_pin, nullptr);
  makeClock("scan_clk", scan_clk_pin);

  // This is the exact multi-mode condition being guarded:
  // scan_clk is not a function-mode clock, but it is an ideal test-mode clock.
  ASSERT_FALSE(sta_->isClock(scan_clk_pin, function_mode));
  ASSERT_TRUE(sta_->isClock(scan_clk_pin, test_mode));
  ASSERT_TRUE(sta_->isIdealClock(scan_clk_pin, test_mode));

  // Build timing/parasitics after the mode-specific clock setup is complete.
  sta_->ensureGraph();
  sta_->ensureLevelized();
  resizer_.initBlock();
  ep_.estimateWireParasitics();
  sta_->updateTiming(true);

  sta::Net* scan_clk_net = flatNet(scan_clk_pin);
  ASSERT_NE(scan_clk_net, nullptr);

  // Isolate the invalidation caused by updateParasitics().
  clearTimingInvalidations();
  ASSERT_EQ(timingInvalidationCount(), 0U);

  // Seed the scan clock net as invalid. A buggy all-modes ideal-clock check
  // treats this net as non-ideal because function mode has no scan clock.
  ep_.setParasiticsSrc(ParasiticsSrc::kPlacement);
  ep_.setIncrementalParasiticsEnabled(true);
  ep_.parasiticsInvalid(scan_clk_net);
  ASSERT_TRUE(ep_.hasParasiticsInvalid());

  ep_.updateParasitics();

  // If non-clock modes are not ignored, updateParasitics() invalidates the
  // scan clock port and scan_reg/CK through delaysInvalidFromFanin().
  EXPECT_EQ(timingInvalidationCount(), 0U);
  expectNoConnectedPinInvalidated(scan_clk_net);
  ep_.setIncrementalParasiticsEnabled(false);
}

}  // namespace est
