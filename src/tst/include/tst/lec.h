// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "gtest/gtest.h"

namespace tst {

// Logical/sequential equivalence checking via kepler-formal, invoked as a
// subprocess.
//
// Never linked: kepler-formal is GPL-3.0 and OpenROAD is BSD-3-Clause, so
// linking would pull the combined work under GPL. In-tree precedent for
// shelling out: popen in src/rcx/src/parse.cpp, std::system in
// src/web/src/web_serve.cpp.
//
// Under bazel the binary is built from the pinned @kepler-formal module and
// arrives in the test's runfiles (//src/tst:kepler_formal_bin is a data dep of
// //src/tst:lec), so no locally installed copy is needed. Outside bazel it is
// resolved at run time from $KEPLER_FORMAL or $PATH; see findKeplerFormal.
//
// Building it needs one accommodation: kepler-formal's @onetbb dependency is a
// rules_foreign_cc cmake() target, and rules_foreign_cc bakes bazel's cxxopts
// into CMAKE_CXX_FLAGS, which CMake reuses at link time, so OpenROAD's global
// `-xc++` (a NixOS accommodation, .bazelrc) would make clang parse .o files as
// C++ source. //bazel:kepler.bzl strips that one flag for that subgraph only.
//
// However it is resolved, a missing binary is a test *failure* with an
// actionable message, never a silent pass and never a skip. See
// assertLecAvailable.

enum class LecMode
{
  // Combinational equivalence, cutting sequential boundaries. Matches
  // boundary points by name, so it cannot compare a hierarchical netlist
  // against a flat one (flat write_verilog renames every instance).
  kCombinational,
  // Sequential equivalence, comparing transition systems. Tolerates the
  // instance renaming, and is strict about boundary sets. Always paired with
  // the `binary` encoding -- see runLec.
  kSequential,
};

enum class LecResult
{
  // Equivalent, with every observable output covered. The only pass.
  kProved,
  // Ran and found no difference, but did not cover every output -- e.g. a
  // dropped internal connection leaves outputs depending on a no-driver cone.
  // A failure: "inconclusive" treated as success is how an equivalence
  // harness becomes decorative.
  kPartial,
  // Ran but the output could not be read as a proof: no recognized verdict, or
  // (in kSequential) a verdict with no coverage figure attached. A failure,
  // deliberately -- an unrecognized output must never be read as success, since
  // that is how a harness keeps reporting green after the tool it depends on
  // changes underneath it.
  kInconclusive,
  // Not equivalent; a counterexample was found.
  kCounterexample,
  // The two designs' boundary sets differ, so the tool refused before
  // solving. Usually a real finding: a dropped or renamed port.
  kBoundaryMismatch,
  // Config rejected, netlist failed to parse or load, etc.
  kToolError,
  // The binary could not be resolved. Never treated as a pass.
  kNotInstalled,
};

struct LecOutcome
{
  LecResult result{LecResult::kNotInstalled};
  // Human-readable, actionable: the verdict line, the boundary sets that
  // differed, or the counterexample. A bare "not equivalent" is unactionable.
  std::string detail;
  // Path to kepler-formal's own log, which carries the per-output detail.
  std::filesystem::path log_path;
  // Path to the generated config, kept so a failure can be reproduced by hand.
  std::filesystem::path config_path;
};

// Locates the kepler-formal binary. Resolution order:
//   1. $KEPLER_FORMAL, if set (an explicit path; must be executable)
//   2. the runfile from //src/tst:kepler_formal_bin
//   3. `kepler-formal` on $PATH
// Returns an empty string when not found.
//
// The runfile is what a bazel test normally uses, and it is the only pinned
// one of the three: it is built from the version MODULE.bazel names, so every
// machine and CI run agree. That matters more than it sounds -- kepler versions
// differ in ways that change verdicts, and an unpinned binary is how a suite
// starts disagreeing with CI for reasons nobody can see.
//
// $KEPLER_FORMAL wins anyway, because someone debugging a kepler change needs
// their own build to take precedence over the pinned one. Under bazel that
// override has to be forwarded into the test sandbox explicitly:
//
//   bazel test --test_env=KEPLER_FORMAL=/path/to/kepler-formal
//   //src/tst:TestLec
//
// which is deliberately not set repo-wide: the pinned runfile is what CI and a
// normal developer run should use, and forwarding it by default would let a
// stray value in someone's shell silently replace it.
std::filesystem::path findKeplerFormal();

// True when findKeplerFormal() returns a usable path.
bool isLecAvailable();

// Fails the calling gtest with an actionable message when the binary is
// missing. Deliberately a failure rather than GTEST_SKIP: LEC is the only
// oracle these tests have, so a skipped run proves nothing while still
// reporting green -- the exact failure mode that left the older eqy-based
// harness inert for years.
//
// Prefer TST_REQUIRE_LEC() in test bodies.
void assertLecAvailable();

// Fails the current test with that message and stops it. A macro because a
// fatal gtest failure only returns from the function containing it, so calling
// assertLecAvailable() directly would let the test run on and report a second,
// redundant failure.
#define TST_REQUIRE_LEC()                     \
  do {                                        \
    ::tst::assertLecAvailable();              \
    if (::testing::Test::HasFatalFailure()) { \
      return;                                 \
    }                                         \
  } while (0)

// Compares `gate_v` against `gold_v`. Writes a config and a log into
// `work_dir`; both paths come back in the outcome.
//
// In kSequential mode this sets `sec_encoding: dual_rail_steady`. That is not a
// detail, and the right answer has already changed once with the kepler
// revision -- see the comment at the `sec_encoding` line in lec.cpp before
// touching it.
LecOutcome runLec(const std::filesystem::path& gold_v,
                  const std::filesystem::path& gate_v,
                  const std::vector<std::filesystem::path>& liberty,
                  LecMode mode,
                  const std::filesystem::path& work_dir);

// "proved" / "counterexample" / ... for test messages.
const char* toString(LecResult result);

}  // namespace tst
