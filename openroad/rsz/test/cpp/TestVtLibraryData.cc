// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "gtest/gtest.h"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/Liberty.hh"
#include "tst/IntegratedFixture.h"

namespace rsz {

class VtLibraryDataTest : public tst::IntegratedFixture
{
 protected:
  VtLibraryDataTest()
      : tst::IntegratedFixture(tst::IntegratedFixture::Technology::kNangate45,
                               "_main/openroad/rsz/test/")
  {
  }

  void addImplant(const char* cell_name, const char* layer_name)
  {
    odb::dbMaster* master = db_->findMaster(cell_name);
    ASSERT_NE(master, nullptr);
    odb::dbTechLayer* layer = odb::dbTechLayer::create(
        db_->getTech(), layer_name, odb::dbTechLayerType::IMPLANT);
    ASSERT_NE(layer, nullptr);
    odb::dbBox::create(master, layer, 0, 0, 10, 10);
  }

  void runHoldWithClockBuffers(bool disable_buffer_pruning = false)
  {
    addImplant("BUF_X1", "DATA_VT");
    addImplant("CLKBUF_X1", "CLOCK_VT");
    readVerilogAndSetup("TestResizerMt_DelayEstimator.v");
    if (disable_buffer_pruning) {
      odb::dbBoolProperty::create(block_, "disable_buffer_pruning", true);
    }

    resizer_.runRepairSetupPreamble();
    EXPECT_EQ(resizer_.vtCategoryCountWithDontUse(), 1);

    resizer_.repairHold(/*setup_margin=*/0.0,
                        /*hold_margin=*/0.0,
                        /*allow_setup_violations=*/false,
                        /*max_buffer_percent=*/1.0f,
                        /*max_passes=*/0,
                        /*max_iterations=*/0,
                        /*match_cell_footprint=*/false,
                        /*verbose=*/false,
                        /*path_group=*/nullptr);
    EXPECT_EQ(resizer_.vtCategoryCountWithDontUse(), 2);
  }
};

TEST_F(VtLibraryDataTest, UnclassifiedBuffersDoNotAddVtCategory)
{
  addImplant("BUF_X1", "DATA_VT");
  sta::LibertyCell* buffer = sta_->network()->findLibertyCell("BUF_X1");
  ASSERT_NE(buffer, nullptr);
  resizer_.setDontUse(buffer, true);

  sta::LibertyCellSeq buffers;
  resizer_.getBufferList(buffers);

  // BUF_X1 has an explicit VT even while dont_use; all other buffers have
  // no implant layer and must not invent a second VT alternative.
  EXPECT_EQ(resizer_.vtCategoryCountWithDontUse(), 1);
}

TEST_F(VtLibraryDataTest, HoldBufferOverrideDoesNotLeaveStaleVtCategories)
{
  // Hold repair temporarily includes clock buffers. The next setup preamble
  // must restore the data-buffer VT view even when its old list is nonempty.
  runHoldWithClockBuffers();
  resizer_.runRepairSetupPreamble();
  EXPECT_EQ(resizer_.vtCategoryCountWithDontUse(), 1);
}

TEST_F(VtLibraryDataTest, HoldBufferOverrideRefreshesWithoutPruning)
{
  runHoldWithClockBuffers(/*disable_buffer_pruning=*/true);
  resizer_.runRepairSetupPreamble();
  EXPECT_EQ(resizer_.vtCategoryCountWithDontUse(), 1);
}

TEST_F(VtLibraryDataTest, VtEquivLookupRefreshesAfterHoldBufferOverride)
{
  runHoldWithClockBuffers();

  // A VT lookup can occur before the next setup preamble. It must not use
  // the hold-specific library data restored alongside a data-buffer list.
  sta::LibertyCell* buffer = sta_->network()->findLibertyCell("BUF_X1");
  ASSERT_NE(buffer, nullptr);
  resizer_.getVTEquivCells(buffer);
  EXPECT_EQ(resizer_.vtCategoryCountWithDontUse(), 1);
}

}  // namespace rsz
