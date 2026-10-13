// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

// Regression for the hold-buffer counter on a rolled-back insertion.
//
// repairEndHold() opens an ECO journal around every hold-buffer insertion so
// a fix that makes setup worse can be undone with journalRestore(). The old
// code reset inserted_buffer_count_ to 0 on that rollback instead of leaving
// the already-committed buffers counted. journalRestore() only undoes the one
// insertion just attempted, so wiping the whole count under-reports every
// buffer kept earlier in the run. That corrupts the "Inserted N hold buffers"
// line, the per-iteration table, the design__instance__count__hold_buffer
// metric, and defeats the max_buffer_percent safety valve (which compares
// against the count that was just zeroed).
//
// The gcd design at a tight 0.5ns clock is setup-critical: several hold fixes
// commit, then a later fix is rolled back because it pushes the worst setup
// slack negative. With the bug, holdBufferCount() collapses to the handful of
// buffers committed after the last rollback while the netlist still holds all
// of them; with the fix the count matches the buffers actually in the block.
// The invariant asserted here - reported count == buffers physically added -
// holds for any rollback pattern on correct code and only breaks when a
// rollback wipes the count.

#include <string>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/defin.h"
#include "rsz/Resizer.hh"
#include "sta/Clock.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/NetworkClass.hh"
#include "sta/Sdc.hh"
#include "sta/Transition.hh"
#include "sta/Units.hh"
#include "tst/IntegratedFixture.h"

namespace rsz {

class RepairHoldRollbackTest : public tst::IntegratedFixture
{
 protected:
  RepairHoldRollbackTest()
      : tst::IntegratedFixture(tst::IntegratedFixture::Technology::kNangate45,
                               "_main/src/rsz/test/")
  {
  }

  void SetUp() override
  {
    // Read the placed gcd design directly from DEF (no high-level placement
    // commands). It is already used by the repair_hold9 Tcl test.
    odb::dbChip* chip = odb::dbChip::create(db_.get(), db_->getTech());
    odb::defin def_reader(db_.get(), &logger_, odb::defin::DEFAULT);
    std::vector<odb::dbLib*> search_libs;
    for (odb::dbLib* lib : db_->getLibs()) {
      search_libs.push_back(lib);
    }
    def_reader.readChip(
        search_libs,
        getFilePath("_main/src/rsz/test/gcd_nangate45_placed.def").c_str(),
        chip);
    block_ = db_->getChip()->getBlock();
    sta_->postReadDef(block_);

    makeTightClock();

    sta_->ensureGraph();
    sta_->ensureLevelized();
    resizer_.initBlock();
    ep_.estimateWireParasitics();
  }

  // A single "clk" clock with a deliberately tight 0.5ns period so the design
  // is setup-critical and hold fixes contend with setup slack.
  void makeTightClock()
  {
    sta::Cell* top_cell = db_network_->cell(db_network_->topInstance());
    ASSERT_NE(top_cell, nullptr);
    sta::Port* clk_port = db_network_->findPort(top_cell, "clk");
    ASSERT_NE(clk_port, nullptr);
    sta::Pin* clk_pin
        = db_network_->findPin(db_network_->topInstance(), clk_port);
    sta::PinSet clk_pins(db_network_);
    clk_pins.insert(clk_pin);

    const double period = sta_->units()->timeUnit()->userToSta(0.5);
    sta::FloatSeq waveform;
    waveform.push_back(0);
    waveform.push_back(period / 2.0);
    sta_->makeClock("clk",
                    clk_pins,
                    /*add_to_pins=*/false,
                    period,
                    waveform,
                    /*comment=*/"",
                    sta_->cmdMode());
  }

  // Number of buffer instances currently in the block.
  int bufferInstCount() const
  {
    int count = 0;
    for (odb::dbInst* inst : block_->getInsts()) {
      sta::LibertyCell* cell
          = db_network_->libertyCell(db_network_->dbToSta(inst));
      if (cell != nullptr && cell->isBuffer()) {
        ++count;
      }
    }
    return count;
  }
};

TEST_F(RepairHoldRollbackTest, HoldBufferCountSurvivesRollback)
{
  const int buffers_before = bufferInstCount();

  // Large hold margin on a setup-critical design forces some hold fixes to be
  // rolled back after others have committed. max_buffer_percent is set high so
  // the (correctly counted) buffers never trip the RSZ-0060 safety valve.
  const double hold_margin = sta_->units()->timeUnit()->userToSta(0.25);
  resizer_.repairHold(/*setup_margin=*/0.0,
                      hold_margin,
                      /*allow_setup_violations=*/false,
                      /*max_buffer_percent=*/1.0,
                      /*max_passes=*/10000,
                      /*max_iterations=*/-1,
                      /*match_cell_footprint=*/false,
                      /*verbose=*/false);

  const int buffers_added = bufferInstCount() - buffers_before;

  // The run must actually insert hold buffers, otherwise it exercises nothing.
  EXPECT_GT(buffers_added, 0);

  // The reported count must equal the buffers actually left in the netlist.
  // The old code zeroed inserted_buffer_count_ on a rolled-back insertion, so
  // this reported far fewer buffers than are physically present.
  EXPECT_EQ(resizer_.holdBufferCount(), buffers_added);
}

}  // namespace rsz
