// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

// Regression for MoveCommitter's nested ECO journal accounting.
//
// Background: odb's beginEco/commitEco/undoEco support nesting natively
// (commitEco appends an inner journal to its parent; undoEco only unwinds
// the innermost level). MoveCommitter mirrors that for its in-memory
// pending/committed move accounting via a stack of per-level result vectors.
//
// Each test would fail on a flat (non-nested) MoveCommitter:
//   - NestedCommitMergesIntoParent: the old code finalized on every commit,
//     so pending counts would jump to zero halfway through.
//   - NestedRestoreUnwindsInnerOnly: the old code's rejectPendingMoves
//     would clear the parent's pending counts as well.
//   - MultisetMembershipSurvivesPartialUndo: with std::unordered_set the
//     parent's touch on the same instance would be erased when the inner
//     restore removed the inner touch.
//   - CommitOutsideAnyJournalIsSafe: an unguarded push onto an empty
//     pending_move_results_ stack would be UB.

#include <memory>
#include <string>

#include "MoveCandidate.hh"
#include "MoveCommitter.hh"
#include "OptimizerTypes.hh"
#include "db_sta/dbSta.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "tst/IntegratedFixture.h"

namespace rsz {

namespace {

// MoveCandidate stub that returns a fixed accepted result. apply() does not
// touch the database - the open ECO journal stays empty, which odb handles
// correctly on commit/undo. All we need from the candidate is to drive
// recordAcceptedResult / unrecordAcceptedResult through commit().
class FakeCandidate : public MoveCandidate
{
 public:
  FakeCandidate(Resizer& resizer,
                const Target& target,
                const MoveType type,
                sta::Instance* touched_inst)
      : MoveCandidate(resizer, target), type_(type), touched_inst_(touched_inst)
  {
  }

  MoveResult apply() override
  {
    MoveResult result;
    result.accepted = true;
    result.type = type_;
    result.move_count = 1;
    if (touched_inst_ != nullptr) {
      result.touched_instances.push_back(touched_inst_);
    }
    return result;
  }

  MoveType type() const override { return type_; }

 private:
  MoveType type_;
  sta::Instance* touched_inst_;
};

}  // namespace

class NestedJournalTest : public tst::IntegratedFixture
{
 protected:
  NestedJournalTest()
      : tst::IntegratedFixture(tst::IntegratedFixture::Technology::kNangate45,
                               "_main/src/rsz/test/")
  {
  }

  void SetUp() override
  {
    readVerilogAndSetup("TestBufferRemoval3_0.v");
    committer_ = std::make_unique<MoveCommitter>(resizer_);
    committer_->init();

    // Pick any two instances from the loaded design as targets for the fake
    // moves. We need real sta::Instance pointers because MoveCommitter inserts
    // them into its touched-instance multisets.
    auto inst_iter = block_->getInsts().begin();
    ASSERT_NE(inst_iter, block_->getInsts().end());
    inst_a_ = db_network_->dbToSta(*inst_iter);
    ASSERT_NE(inst_a_, nullptr);
    ++inst_iter;
    if (inst_iter != block_->getInsts().end()) {
      inst_b_ = db_network_->dbToSta(*inst_iter);
    } else {
      inst_b_ = inst_a_;
    }
  }

  std::unique_ptr<FakeCandidate> makeCandidate(const MoveType type,
                                               sta::Instance* inst)
  {
    return std::make_unique<FakeCandidate>(resizer_, target_, type, inst);
  }

  std::unique_ptr<MoveCommitter> committer_;
  Target target_;  // Default-constructed; FakeCandidate::apply ignores it
  sta::Instance* inst_a_{nullptr};
  sta::Instance* inst_b_{nullptr};
};

// === Outermost journal - should commit/revert all pending moves ============

TEST_F(NestedJournalTest, OuterCommitFinalizesPendingMoves)
{
  ASSERT_EQ(committer_->totalMoves(MoveType::kSizeUp), 0);

  committer_->beginJournal();
  auto cand = makeCandidate(MoveType::kSizeUp, inst_a_);
  ASSERT_TRUE(committer_->commit(*cand).accepted);
  EXPECT_EQ(committer_->pendingMoves(MoveType::kSizeUp), 1);
  EXPECT_EQ(committer_->committedMoves(MoveType::kSizeUp), 0);
  EXPECT_TRUE(committer_->hasMoves(MoveType::kSizeUp, inst_a_));

  committer_->commitJournal();
  EXPECT_EQ(committer_->pendingMoves(MoveType::kSizeUp), 0);
  EXPECT_EQ(committer_->committedMoves(MoveType::kSizeUp), 1);
  EXPECT_TRUE(committer_->hasMoves(MoveType::kSizeUp, inst_a_));
}

TEST_F(NestedJournalTest, OuterRestoreUnwindsPendingMoves)
{
  committer_->beginJournal();
  auto cand = makeCandidate(MoveType::kSizeUp, inst_a_);
  ASSERT_TRUE(committer_->commit(*cand).accepted);
  EXPECT_TRUE(committer_->hasMoves(MoveType::kSizeUp, inst_a_));

  committer_->restoreJournal();
  EXPECT_EQ(committer_->pendingMoves(MoveType::kSizeUp), 0);
  EXPECT_EQ(committer_->committedMoves(MoveType::kSizeUp), 0);
  EXPECT_FALSE(committer_->hasPendingMoves(MoveType::kSizeUp, inst_a_));
  // hasMoves() still returns true: recordAcceptedResult inserts into
  // committed_instances_by_type_ on the spot to preserve the legacy
  // "once touched, always touched" guard semantics until policies start
  // using nested journals. Tracked by the FIXME in
  // MoveCommitter::recordAcceptedResult; flip this expectation when that
  // FIXME is removed.
  EXPECT_TRUE(committer_->hasMoves(MoveType::kSizeUp, inst_a_));
}

// === Nested journal - should handle the nested journals ====================

TEST_F(NestedJournalTest, NestedCommitMergesIntoParent)
{
  committer_->beginJournal();
  auto cand_a = makeCandidate(MoveType::kSizeUp, inst_a_);
  ASSERT_TRUE(committer_->commit(*cand_a).accepted);

  committer_->beginJournal();  // nested
  auto cand_b = makeCandidate(MoveType::kSizeUp, inst_b_);
  ASSERT_TRUE(committer_->commit(*cand_b).accepted);
  EXPECT_EQ(committer_->pendingMoves(MoveType::kSizeUp), 2);

  // Inner commit must not finalize - both moves stay pending against
  // the parent journal
  committer_->commitJournal();
  EXPECT_EQ(committer_->pendingMoves(MoveType::kSizeUp), 2);
  EXPECT_EQ(committer_->committedMoves(MoveType::kSizeUp), 0);

  // Outer commit folds the accumulated pending into committed totals
  committer_->commitJournal();
  EXPECT_EQ(committer_->pendingMoves(MoveType::kSizeUp), 0);
  EXPECT_EQ(committer_->committedMoves(MoveType::kSizeUp), 2);
  EXPECT_TRUE(committer_->hasMoves(MoveType::kSizeUp, inst_a_));
  EXPECT_TRUE(committer_->hasMoves(MoveType::kSizeUp, inst_b_));
}

TEST_F(NestedJournalTest, NestedRestoreUnwindsInnerOnly)
{
  committer_->beginJournal();
  auto cand_a = makeCandidate(MoveType::kSizeUp, inst_a_);
  ASSERT_TRUE(committer_->commit(*cand_a).accepted);

  committer_->beginJournal();  // nested
  auto cand_b = makeCandidate(MoveType::kClone, inst_b_);
  ASSERT_TRUE(committer_->commit(*cand_b).accepted);
  EXPECT_EQ(committer_->pendingMoves(MoveType::kSizeUp), 1);
  EXPECT_EQ(committer_->pendingMoves(MoveType::kClone), 1);

  // Restoring the inner journal must drop B's pending touch but leave A
  // intact. hasMoves() on inst_b_/Clone still returns true because of the
  // legacy "once touched, always touched" insert in recordAcceptedResult
  // (see FIXME in MoveCommitter); use hasPendingMoves to assert the actual
  // restore behaviour.
  committer_->restoreJournal();
  EXPECT_EQ(committer_->pendingMoves(MoveType::kSizeUp), 1);
  EXPECT_EQ(committer_->pendingMoves(MoveType::kClone), 0);
  EXPECT_TRUE(committer_->hasPendingMoves(MoveType::kSizeUp, inst_a_));
  EXPECT_FALSE(committer_->hasPendingMoves(MoveType::kClone, inst_b_));
  EXPECT_TRUE(committer_->hasMoves(MoveType::kClone, inst_b_));  // FIXME

  // Outer commit then folds A into committed totals
  committer_->commitJournal();
  EXPECT_EQ(committer_->committedMoves(MoveType::kSizeUp), 1);
  EXPECT_EQ(committer_->committedMoves(MoveType::kClone), 0);
  EXPECT_TRUE(committer_->hasMoves(MoveType::kSizeUp, inst_a_));
  EXPECT_TRUE(committer_->hasMoves(MoveType::kClone, inst_b_));  // FIXME
}

TEST_F(NestedJournalTest, MultisetMembershipSurvivesPartialUndo)
{
  // Same instance touched at two levels. Restoring the inner level must remove
  // only its touch; the parent's still counts.
  committer_->beginJournal();
  auto cand_outer = makeCandidate(MoveType::kSizeUp, inst_a_);
  ASSERT_TRUE(committer_->commit(*cand_outer).accepted);

  committer_->beginJournal();
  auto cand_inner = makeCandidate(MoveType::kSizeUp, inst_a_);
  ASSERT_TRUE(committer_->commit(*cand_inner).accepted);
  EXPECT_EQ(committer_->pendingMoves(MoveType::kSizeUp), 2);
  EXPECT_TRUE(committer_->hasMoves(MoveType::kSizeUp, inst_a_));

  committer_->restoreJournal();
  EXPECT_EQ(committer_->pendingMoves(MoveType::kSizeUp), 1);
  EXPECT_TRUE(committer_->hasMoves(MoveType::kSizeUp, inst_a_));

  committer_->commitJournal();
  EXPECT_EQ(committer_->committedMoves(MoveType::kSizeUp), 1);
  EXPECT_TRUE(committer_->hasMoves(MoveType::kSizeUp, inst_a_));
}

TEST_F(NestedJournalTest, NestedRestoreLeavesParentJournalRecoverable)
{
  // After the inner journal is restored, the outer journal must still be
  // operable: another nested begin/commit, then outer commit, should fold
  // both surviving moves into the committed totals.
  committer_->beginJournal();
  auto cand_a = makeCandidate(MoveType::kSizeUp, inst_a_);
  ASSERT_TRUE(committer_->commit(*cand_a).accepted);

  committer_->beginJournal();
  auto cand_bad = makeCandidate(MoveType::kBuffer, inst_b_);
  ASSERT_TRUE(committer_->commit(*cand_bad).accepted);
  committer_->restoreJournal();
  EXPECT_EQ(committer_->pendingMoves(MoveType::kBuffer), 0);
  EXPECT_EQ(committer_->pendingMoves(MoveType::kSizeUp), 1);

  committer_->beginJournal();
  auto cand_c = makeCandidate(MoveType::kSwapPins, inst_b_);
  ASSERT_TRUE(committer_->commit(*cand_c).accepted);
  committer_->commitJournal();  // nested commit: merges into outer

  committer_->commitJournal();  // outer commit: finalizes
  EXPECT_EQ(committer_->committedMoves(MoveType::kSizeUp), 1);
  EXPECT_EQ(committer_->committedMoves(MoveType::kSwapPins), 1);
  EXPECT_EQ(committer_->committedMoves(MoveType::kBuffer), 0);
}

// === Defensive guard for non-journaled callers =============================

TEST_F(NestedJournalTest, CommitOutsideAnyJournalIsSafe)
{
  // Several policies (MeasuredVtSwapPolicy, SetupCritVtSwapPolicy, ...)
  // call commit() without a surrounding beginJournal() and follow up with
  // acceptPendingMoves() directly. The committer must not crash on the empty
  // pending_move_results_ stack and must still account for the move.
  ASSERT_EQ(committer_->pendingMoves(MoveType::kSizeUp), 0);
  auto cand = makeCandidate(MoveType::kSizeUp, inst_a_);
  ASSERT_TRUE(committer_->commit(*cand).accepted);
  EXPECT_EQ(committer_->pendingMoves(MoveType::kSizeUp), 1);

  committer_->acceptPendingMoves();
  EXPECT_EQ(committer_->pendingMoves(MoveType::kSizeUp), 0);
  EXPECT_EQ(committer_->committedMoves(MoveType::kSizeUp), 1);
}

}  // namespace rsz
