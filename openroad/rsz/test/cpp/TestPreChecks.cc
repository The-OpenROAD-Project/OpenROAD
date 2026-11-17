// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>

#include "PreChecks.hh"
#include "db_sta/dbNetwork.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Scene.hh"
#include "sta/TimingArc.hh"
#include "sta/Transition.hh"
#include "tst/IntegratedFixture.h"

namespace rsz {
namespace {

class TestPreChecks : public tst::IntegratedFixture
{
 protected:
  TestPreChecks()
      : IntegratedFixture(Technology::kNangate45, "_main/src/rsz/test/")
  {
  }

  void SetUp() override
  {
    sta_->makeScenes({"fast", "slow"});
    fast_ = sta_->findScene("fast");
    slow_ = sta_->findScene("slow");
    readLiberty("_main/src/rsz/test/Nangate45/Nangate45_fast.lib", fast_);
    readLiberty("_main/src/rsz/test/Nangate45/Nangate45_slow.lib", slow_);
    auto* chip = odb::dbChip::create(db_.get(), db_->getTech());
    block_ = odb::dbBlock::create(chip, "top");
    sta_->postReadDef(block_);
    auto* inst
        = odb::dbInst::create(block_, lib_->findMaster("BUF_X1"), "load");
    pin_ = db_network_->dbToSta(inst->findITerm("A"));
    resizer_.reportBuffers(false);
  }

  float bestSlew(const sta::Scene* corner, float cap)
  {
    auto* buffer = resizer_.lowestDriveBufferCell();
    float slew = resizer_.bufferSlew(buffer, cap, corner, sta::MinMax::max());
    for (auto* cell : *resizer_.equivCells(buffer)) {
      slew = std::min(
          slew, resizer_.bufferSlew(cell, cap, corner, sta::MinMax::max()));
    }
    return slew;
  }

  sta::Scene* fast_ = nullptr;
  sta::Scene* slow_ = nullptr;
  const sta::Pin* pin_ = nullptr;
  static constexpr float kCap = 1e-15;
};

TEST_F(TestPreChecks, CacheDoesNotCrossCorners)
{
  const float fast_slew = bestSlew(fast_, kCap);
  const float slow_slew = bestSlew(slow_, kCap);
  ASSERT_GT(fast_slew, 0.0f);
  ASSERT_GT(slow_slew, fast_slew);
  const float limit = (fast_slew + slow_slew) / 2;

  PreChecks checks(&resizer_);
  EXPECT_NO_THROW(checks.checkSlewLimit(pin_, kCap, limit, fast_));
  EXPECT_THROW(checks.checkSlewLimit(pin_, kCap, limit, slow_),
               std::runtime_error);
  EXPECT_NO_THROW(checks.checkSlewLimit(pin_, kCap, limit, fast_));
}

TEST_F(TestPreChecks, SlowCornerDoesNotRejectFastCorner)
{
  const float limit = (bestSlew(fast_, kCap) + bestSlew(slow_, kCap)) / 2;
  PreChecks checks(&resizer_);
  EXPECT_NO_THROW(checks.checkSlewLimit(pin_, kCap, 1.0f, slow_));
  EXPECT_NO_THROW(checks.checkSlewLimit(pin_, kCap, limit, fast_));
}

TEST_F(TestPreChecks, RecomputesForSmallerLoad)
{
  const float large_cap = kCap * 100;
  const float small_slew = bestSlew(fast_, kCap);
  const float large_slew = bestSlew(fast_, large_cap);
  ASSERT_GT(large_slew, small_slew);
  const float limit = (small_slew + large_slew) / 2;

  PreChecks checks(&resizer_);
  EXPECT_NO_THROW(checks.checkSlewLimit(pin_, large_cap, 1.0f, fast_));
  EXPECT_NO_THROW(checks.checkSlewLimit(pin_, kCap, limit, fast_));
}

TEST_F(TestPreChecks, ImpossibleLimitNamesPinAndCorner)
{
  PreChecks checks(&resizer_);
  testing::internal::CaptureStdout();
  EXPECT_THROW(checks.checkSlewLimit(pin_, kCap, 0.0f, slow_),
               std::runtime_error);
  const std::string message = testing::internal::GetCapturedStdout();
  EXPECT_NE(message.find("RSZ-0090"), std::string::npos);
  EXPECT_NE(message.find("pin load/A at corner slow"), std::string::npos);
  EXPECT_EQ(message.find("from SDC"), std::string::npos);
}

TEST_F(TestPreChecks, MissingTargetSlewTiming)
{
  for (const auto* rf : sta::RiseFall::range()) {
    SCOPED_TRACE(rf->name());
    std::map<sta::TimingArcSet*, sta::TimingArcSeq> saved_arcs;
    std::size_t removed = 0;
    auto* library = resizer_.lowestDriveBufferCell()
                        ->sceneCell(slow_->libertyIndex(sta::MinMax::max()))
                        ->libertyLibrary();
    sta::LibertyCellIterator cell_iter(library);
    while (cell_iter.hasNext()) {
      auto* buffer = cell_iter.next();
      if (buffer->isBuffer()) {
        for (auto* arc_set : buffer->timingArcSets()) {
          auto& arcs = arc_set->arcs();
          saved_arcs.emplace(arc_set, arcs);
          removed += std::erase_if(arcs, [rf](const auto* arc) {
            return arc->toEdge()->asRiseFall() == rf;
          });
        }
      }
    }
    EXPECT_GT(removed, 0);

    PreChecks checks(&resizer_);
    testing::internal::CaptureStdout();
    EXPECT_THROW(checks.checkSlewLimit(pin_, kCap, 1.0f, slow_),
                 std::runtime_error);
    const std::string message = testing::internal::GetCapturedStdout();
    for (auto& [arc_set, arcs] : saved_arcs) {
      arc_set->arcs() = std::move(arcs);
    }
    SCOPED_TRACE(message);
    EXPECT_NE(message.find("RSZ-3247"), std::string::npos);
    EXPECT_NE(message.find("pin load/A"), std::string::npos);
    EXPECT_NE(message.find("corner slow"), std::string::npos);
    EXPECT_NE(message.find(rf->name()), std::string::npos);
    EXPECT_NO_THROW(checks.checkSlewLimit(pin_, kCap, 1.0f, slow_));
  }
}

}  // namespace
}  // namespace rsz
