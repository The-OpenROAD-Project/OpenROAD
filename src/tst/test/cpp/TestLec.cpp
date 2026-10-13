// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Covers tst::runLec's classification, including the two ways kepler-formal
// reports a problem without a nonzero exit code.

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "TestNetlists.h"
#include "gtest/gtest.h"
#include "tst/db_fixture.h"
#include "tst/lec.h"
#include "tst/loaded_design.h"

namespace tst {
namespace {

std::filesystem::path workDir()
{
  const char* tmp = std::getenv("TEST_TMPDIR");
  return tmp != nullptr ? tmp : ".";
}

std::vector<std::filesystem::path> nangate45Liberty()
{
  return {getRunfilePath("_main/test/Nangate45/Nangate45_typ.lib")};
}

// The gold netlist for every case below.
std::filesystem::path netlistPath()
{
  return writeNetlist(workDir(), "lec_gold.v", kHierNetlist);
}

// Writes `text` into the work dir and returns the path. The defective variants
// are derived from the gold rather than from emitted Verilog, so the tests do
// not depend on write_verilog's formatting.
std::filesystem::path writeTemp(const std::string& name,
                                const std::string& text)
{
  return writeNetlist(workDir(), name, text);
}

// Emits the netlist through one link mode and returns the output path.
std::filesystem::path emit(bool hierarchy, const std::string& name)
{
  LoadedDesign d(Technology::kNangate45, netlistPath(), "top", hierarchy);
  const std::filesystem::path out = workDir() / name;
  d.writeVerilog(out);
  return out;
}

// Resolution is by $KEPLER_FORMAL then $PATH, and must never report a
// not-found binary as anything but not-found.
TEST(TestLec, ResolutionReportsMissingBinary)
{
  const char* saved = std::getenv("KEPLER_FORMAL");
  setenv("KEPLER_FORMAL", "/nonexistent/kepler-formal", /*overwrite=*/1);

  EXPECT_FALSE(isLecAvailable());
  const LecOutcome outcome
      = runLec("gold.v", "gate.v", {}, LecMode::kSequential, workDir());
  EXPECT_EQ(outcome.result, LecResult::kNotInstalled);
  // The message has to say how to fix it, not just that it broke.
  EXPECT_NE(outcome.detail.find("KEPLER_FORMAL"), std::string::npos);

  if (saved != nullptr) {
    setenv("KEPLER_FORMAL", saved, 1);
  } else {
    unsetenv("KEPLER_FORMAL");
  }
}

TEST(TestLec, ProvesHierOutputAgainstInput)
{
  TST_REQUIRE_LEC();
  const std::string gate = emit(/*hierarchy=*/true, "lec_hier.v");

  const LecOutcome outcome = runLec(
      netlistPath(), gate, nangate45Liberty(), LecMode::kSequential, workDir());
  EXPECT_EQ(outcome.result, LecResult::kProved)
      << toString(outcome.result) << "\n"
      << outcome.detail;
}

TEST(TestLec, ProvesFlatOutputAgainstInput)
{
  TST_REQUIRE_LEC();
  const std::string gate = emit(/*hierarchy=*/false, "lec_flat.v");

  // The flat writer renames every instance, which kCombinational would reject
  // as a boundary mismatch; kSequential compares transition systems and
  // tolerates it.
  const LecOutcome outcome = runLec(
      netlistPath(), gate, nangate45Liberty(), LecMode::kSequential, workDir());
  EXPECT_EQ(outcome.result, LecResult::kProved)
      << toString(outcome.result) << "\n"
      << outcome.detail;
}

// A real logic difference. kepler-formal still exits 0 here, so this is the
// case that proves the classifier does not read the exit code.
TEST(TestLec, DetectsCounterexample)
{
  TST_REQUIRE_LEC();
  const LecOutcome outcome
      = runLec(netlistPath(),
               writeTemp("lec_counterexample.v", counterexampleNetlist()),
               nangate45Liberty(),
               LecMode::kSequential,
               workDir());
  EXPECT_EQ(outcome.result, LecResult::kCounterexample)
      << toString(outcome.result) << "\n"
      << outcome.detail;
}

// A dropped top-level port. Reported with a zero exit code and, on older
// builds, even with a "no difference" verdict -- so neither signal can be
// trusted on its own.
TEST(TestLec, DetectsDroppedPort)
{
  TST_REQUIRE_LEC();
  const LecOutcome outcome
      = runLec(netlistPath(),
               writeTemp("lec_dropped_port.v", droppedPortNetlist()),
               nangate45Liberty(),
               LecMode::kSequential,
               workDir());
  // The assertion that matters: a netlist missing a top-level output must not
  // be reported as equivalent, whichever failure category it lands in.
  EXPECT_NE(outcome.result, LecResult::kProved)
      << "a netlist missing a top-level output port must not pass";
  EXPECT_EQ(outcome.result, LecResult::kBoundaryMismatch)
      << toString(outcome.result) << "\n"
      << outcome.detail;
}

// A config the tool rejects must land in kToolError, not be mistaken for a
// pass or for a finding about the design.
TEST(TestLec, ReportsToolErrorForUnparseableNetlist)
{
  TST_REQUIRE_LEC();
  const std::string gate = writeTemp("lec_garbage.v", "this is not verilog\n");

  const LecOutcome outcome = runLec(
      netlistPath(), gate, nangate45Liberty(), LecMode::kSequential, workDir());
  EXPECT_EQ(outcome.result, LecResult::kToolError)
      << toString(outcome.result) << "\n"
      << outcome.detail;
}

}  // namespace
}  // namespace tst
