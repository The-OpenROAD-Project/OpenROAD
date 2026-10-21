// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Diagnostics raised while elaborating SystemVerilog reach the user through
// utl::Logger, rather than writing to stderr and aborting the process. These
// tests pin both halves: the SYN id the caller sees, and the message content
// that locates the problem in the user's HDL.
//
// The messages also name the file:line inside third-party/slang-elab/ that
// gave up. That part is deliberately not asserted on: it moves whenever the
// submodule does, and it is not what these tests are about.

#include <string>
#include <vector>

#include "driver.h"
#include "gtest/gtest.h"
#include "tst/db_fixture.h"
#include "utl/Logger.h"

// No gtest_main: ABC (via syn_ir → TritModel) ships its own main() that wins
// over gtest_main, so this TU defines its own.
int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

namespace syn {
namespace {

class ElabDiagTest : public tst::DbFixture
{
 protected:
  // Elaborates `file`, which is expected to fail, and returns everything the
  // logger emitted. `expected_id` is the SYN id utl::Logger throws.
  std::string elaborateExpectingError(const std::string& file,
                                      const std::vector<std::string>& options,
                                      const std::string& expected_id)
  {
    std::vector<std::string> args = options;
    args.push_back(getFilePath("_main/src/syn/test/" + file));

    std::string thrown;

    getLogger()->redirectStringBegin();
    try {
      elaborate(getLogger(), args, /* sta = */ nullptr);
    } catch (const std::exception& e) {
      thrown = e.what();
    }
    const std::string captured = getLogger()->redirectStringEnd();

    // An abort would have taken the process down instead of unwinding here.
    EXPECT_EQ(thrown, expected_id) << "captured log:\n" << captured;
    return captured;
  }
};

// An unimplemented expression reports its kind, where it sits in the user's
// HDL, and the text it was built from.
TEST_F(ElabDiagTest, UnimplementedExpressionLocatesTheExpression)
{
  const std::string log = elaborateExpectingError(
      "elab_diag_expr.sv", {"--top", "top"}, "SYN-0074");

  EXPECT_NE(log.find("(expression kind: NamedValue)"), std::string::npos)
      << log;
  EXPECT_NE(log.find("elab_diag_expr.sv:4:24: b"), std::string::npos) << log;
}

// Same for an unimplemented statement. The source text is flattened onto one
// line, so the three-line fork block arrives as "fork y <= x; join".
TEST_F(ElabDiagTest, UnimplementedStatementLocatesTheStatement)
{
  const std::string log = elaborateExpectingError(
      "elab_diag_stmt.sv", {"--top", "top"}, "SYN-0075");

  EXPECT_NE(log.find("(statement kind: Block)"), std::string::npos) << log;
  EXPECT_NE(log.find("elab_diag_stmt.sv:6:7: fork y <= x; join"),
            std::string::npos)
      << log;
}

// The vendored frontend calls log_error() directly. The stub in log_stubs.h
// routes it to utl::Logger instead of stderr+abort, which is what lets
// third-party/slang-elab/ stay unmodified.
TEST_F(ElabDiagTest, FrontendLogErrorReachesTheLogger)
{
  const std::string log = elaborateExpectingError(
      "elab_diag_hier.sv", {"--top", "top", "--keep-hierarchy"}, "SYN-0079");

  EXPECT_NE(log.find("Hierarchical (non-dissolved) module instantiation "
                     "not supported without Yosys"),
            std::string::npos)
      << log;
}

// A diagnostic raised by the driver itself, which has a logger in hand and
// does not go through the frontend's hooks.
TEST_F(ElabDiagTest, MultipleTopLevelModulesIsAnError)
{
  const std::string log
      = elaborateExpectingError("elab_diag_tops.sv", {}, "SYN-0078");

  EXPECT_NE(log.find("Expected exactly one top-level module, got 2"),
            std::string::npos)
      << log;
}

}  // namespace
}  // namespace syn
