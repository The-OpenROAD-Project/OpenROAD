// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

// Setup repair profiler, modes 1 and 2.

#include "TestResizerMtFixture.hh"

namespace rsz {

TEST_F(TestResizerMt, RepairSetupProfilerOffProducesNoOutput)
{
  ProfileTestDirectory directory;

  {
    testing::internal::CaptureStdout();
    EXPECT_NO_THROW({
      RepairSetupProfiler profiler(
          resizer_, RepairSetupProfiler::Mode::kOff, directory.root().string());
      EXPECT_FALSE(RepairSetupProfiler::enabled());
      EXPECT_FALSE(RepairSetupProfiler::analysis());
      const ProfileScope scope("prepare");
      const ProfileIteration iteration("test_pass");
      RepairSetupProfiler::event("off_marker");
      RepairSetupProfiler::event("off_event");
      RepairSetupProfiler::workerTime("estimation", 1.0);
      RepairSetupProfiler::qor("off_qor", -1.0, -2.0);
      profiler.beginPhase("LEGACY", 1);
      profiler.complete(true);
    });
    EXPECT_TRUE(testing::internal::GetCapturedStdout().empty());
    EXPECT_EQ(RepairSetupProfiler::current(), nullptr);
    EXPECT_FALSE(std::filesystem::exists(directory.root()));
  }
}

TEST_F(TestResizerMt, RepairSetupProfilerConsecutiveCallsKeepFreshFiles)
{
  ProfileTestDirectory directory;
  const std::vector<std::string> filenames{
      "stats.csv", "events.csv", "timing.csv", "metadata.csv"};
  std::vector<std::string> first_outputs;
  for (int run = 1; run <= 2; ++run) {
    {
      RepairSetupProfiler profiler(resizer_,
                                   RepairSetupProfiler::Mode::kTiming,
                                   directory.root().string());
      EXPECT_TRUE(RepairSetupProfiler::enabled());
      EXPECT_FALSE(RepairSetupProfiler::analysis());
      profiler.beginPhase("LEGACY", 1);
      const ProfileIteration iteration("test_pass");
      RepairSetupProfiler::event(run == 1 ? "first_only" : "second_only");
      RepairSetupProfiler::qor("test_qor", -1.0, -2.0);
      profiler.complete(true);
    }

    EXPECT_EQ(RepairSetupProfiler::current(), nullptr);
    const std::filesystem::path run_dir
        = directory.root() / ("run_" + std::to_string(run));
    EXPECT_FALSE(std::filesystem::exists(run_dir / "analysis"));
    for (size_t index = 0; index < filenames.size(); ++index) {
      const std::string contents = readProfileFile(run_dir / filenames[index]);
      EXPECT_FALSE(contents.empty());
      if (run == 1) {
        first_outputs.push_back(contents);
      } else {
        EXPECT_EQ(
            readProfileFile(directory.root() / "run_1" / filenames[index]),
            first_outputs[index]);
      }
    }
  }

  const std::string second_stats
      = readProfileFile(directory.root() / "run_2/stats.csv");

  EXPECT_NE(first_outputs[0].find(",\"count\",\"first_only\",\"\",1,1,1\n"),
            std::string::npos);
  EXPECT_NE(second_stats.find(",\"count\",\"second_only\",\"\",1,1,1\n"),
            std::string::npos);
  EXPECT_EQ(second_stats.find("first_only"), std::string::npos);
  EXPECT_EQ(readProfileFile(directory.root() / "run_2/events.csv"),
            first_outputs[1]);
  EXPECT_NE(readProfileFile(directory.root() / "run_2/metadata.csv")
                .find("\"status\",\"completed\"\n"),
            std::string::npos);
}

TEST_F(TestResizerMt, RepairSetupProfilerEventSummaryAndTrace)
{
  ProfileTestDirectory directory;

  for (const RepairSetupProfiler::Mode mode :
       {RepairSetupProfiler::Mode::kTiming,
        RepairSetupProfiler::Mode::kEvents}) {
    {
      RepairSetupProfiler profiler(resizer_, mode, directory.root().string());
      profiler.beginPhase("LEGACY", 1);
      const ProfileIteration iteration("test_pass");
      RepairSetupProfiler::event(
          "retry",
          {.metric = "retries", .detail = "small_batch", .trace = true});
      RepairSetupProfiler::event("targets_collected", 3);
      RepairSetupProfiler::event("journal_undo_begin",
                                 {.count = std::nullopt, .trace = true});
      profiler.complete(true);
    }

    const std::filesystem::path run_dir
        = directory.root() / ("run_" + std::to_string(static_cast<int>(mode)));
    const std::string stats = readProfileFile(run_dir / "stats.csv");
    const std::string events = readProfileFile(run_dir / "events.csv");
    EXPECT_NE(stats.find(",\"count\",\"iterations\",\"\",1,1,1\n"),
              std::string::npos);
    EXPECT_NE(stats.find(",\"count\",\"retries\",\"\",1,1,1\n"),
              std::string::npos);
    EXPECT_NE(stats.find(",\"count\",\"targets_collected\",\"\",3,3,3\n"),
              std::string::npos);
    EXPECT_EQ(stats.find("journal_undo_begin"), std::string::npos);
    EXPECT_EQ(events.find("retries"), std::string::npos);
    if (mode == RepairSetupProfiler::Mode::kTiming) {
      EXPECT_EQ(events.find("\"retry\""), std::string::npos);
      EXPECT_EQ(events.find("\"journal_undo_begin\""), std::string::npos);
    } else {
      EXPECT_NE(events.find("\"retry\""), std::string::npos);
      EXPECT_NE(events.find("\"journal_undo_begin\""), std::string::npos);
    }
  }
}

TEST_F(TestResizerMt, RepairSetupProfilerNestedCommitThenOuterUndo)
{
  ProfileTestDirectory directory;
  resizer_.runRepairSetupPreamble();
  MoveCommitter committer(resizer_);
  committer.init();
  const Target target = makeTarget("path_out", "path_target", "ZN");
  ASSERT_NE(target.driver_pin, nullptr);
  StubBufferMoveCandidate outer(resizer_, target, 2);
  StubBufferMoveCandidate inner(resizer_, target, 3);
  StubBufferMoveCandidate rejected(resizer_, target, 1, false);

  {
    RepairSetupProfiler profiler(resizer_,
                                 RepairSetupProfiler::Mode::kEvents,
                                 directory.root().string());
    profiler.beginPhase("LEGACY", 1);
    const ProfileIteration iteration("legacy_pass");
    const auto commit = [&](MoveCandidate& candidate) {
      RepairSetupProfiler::moveGenerated(candidate);
      RepairSetupProfiler::moveEstimated(candidate, candidate.estimate());
      RepairSetupProfiler::moveSelected(candidate);
      return committer.commit(candidate);
    };
    committer.beginJournal();
    EXPECT_TRUE(commit(outer).accepted);
    EXPECT_EQ(committer.pendingMoves(MoveType::kBuffer), 2);
    committer.beginJournal();
    EXPECT_TRUE(commit(inner).accepted);
    EXPECT_FALSE(commit(rejected).accepted);
    committer.commitJournal(false);
    EXPECT_EQ(committer.pendingMoves(MoveType::kBuffer), 5);
    EXPECT_EQ(committer.committedMoves(MoveType::kBuffer), 0);
    committer.restoreJournal();
    EXPECT_EQ(committer.pendingMoves(MoveType::kBuffer), 0);
    EXPECT_EQ(committer.committedMoves(MoveType::kBuffer), 0);
    profiler.complete(false);
  }

  EXPECT_EQ(RepairSetupProfiler::current(), nullptr);
  const std::filesystem::path run_dir = directory.root() / "run_1";
  const std::string move_field
      = std::string("\"") + moveName(MoveType::kBuffer) + "\"";

  const std::string stats = readProfileFile(run_dir / "stats.csv");
  for (const char* metric : {"generated",
                             "estimated",
                             "estimate_legal",
                             "selected",
                             "apply_attempted"}) {
    EXPECT_NE(stats.find(std::string(",\"count\",\"") + metric + "\","
                         + move_field + ",3,3,3\n"),
              std::string::npos)
        << metric;
  }

  // Reverts count successful apply calls, not the five reported buffer edits.
  for (const char* metric : {"apply_succeeded", "reverted"}) {
    EXPECT_NE(stats.find(std::string(",\"count\",\"") + metric + "\","
                         + move_field + ",2,2,2\n"),
              std::string::npos)
        << metric;
  }

  EXPECT_NE(
      stats.find(",\"count\",\"apply_failed\"," + move_field + ",1,1,1\n"),
      std::string::npos);
  EXPECT_NE(stats.find(",\"count\",\"reported_move_count\"," + move_field
                       + ",5,5,5\n"),
            std::string::npos);
  EXPECT_NE(stats.find(",\"count\",\"journal_begin\",\"\",2,2,2\n"),
            std::string::npos);
  for (const char* metric : {"journal_commit", "journal_undo"}) {
    EXPECT_NE(
        stats.find(std::string(",\"count\",\"") + metric + "\",\"\",1,1,1\n"),
        std::string::npos)
        << metric;
  }

  EXPECT_EQ(stats.find("checkpoint_accepted"), std::string::npos);
  const std::string events = readProfileFile(run_dir / "events.csv");
  EXPECT_NE(
      events.find(",\"LEGACY\",1,1,1,1,\"apply_begin\"," + move_field + ","),
      std::string::npos);
  EXPECT_NE(
      events.find(",\"LEGACY\",1,2,2,2,\"apply_begin\"," + move_field + ","),
      std::string::npos);
  for (const char* move :
       {",1,0,1,\"move_reverted\",", ",2,0,1,\"move_reverted\","}) {
    EXPECT_NE(events.find(move + move_field + ","), std::string::npos);
  }

  EXPECT_EQ(events.find(",3,0,1,\"move_reverted\","), std::string::npos);
  EXPECT_EQ(events.find("move_checkpoint_accepted"), std::string::npos);
  EXPECT_NE(readProfileFile(run_dir / "metadata.csv")
                .find("\"open_journals\",\"0\"\n"),
            std::string::npos);
}

}  // namespace rsz
