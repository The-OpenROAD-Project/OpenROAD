// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Covers the one thing no fixture could do before: load the same netlist both
// hierarchically and flat, in one process, and emit each.

#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>

#include "TestNetlists.h"
#include "gtest/gtest.h"
#include "tst/db_fixture.h"
#include "tst/loaded_design.h"

namespace tst {
namespace {

std::string workDir()
{
  const char* tmp = std::getenv("TEST_TMPDIR");
  return tmp != nullptr ? tmp : ".";
}

std::string readAll(const std::string& path)
{
  std::ifstream in(path);
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

// top instantiates block1/block2, each with a DFF. Written out once per call
// so a test that emits into the same work dir cannot read a stale copy.
std::string netlistPath()
{
  return writeNetlist(workDir(), "loaded_design_hier_in.v", kHierNetlist);
}

TEST(TestLoadedDesign, HierarchicalLinkEmitsNestedModules)
{
  LoadedDesign d(Technology::kNangate45, netlistPath(), "top", true);
  EXPECT_TRUE(d.hasHierarchy());

  const std::string out = workDir() + "/loaded_design_hier.v";
  d.writeVerilog(out);
  const std::string v = readAll(out);

  // The submodules survive as modules, and their instances are not flattened.
  EXPECT_NE(v.find("module block1"), std::string::npos);
  EXPECT_NE(v.find("block1 b1"), std::string::npos);
  EXPECT_EQ(v.find("\\b1/r1 "), std::string::npos);
}

TEST(TestLoadedDesign, FlatLinkEmitsOneModuleWithEscapedNames)
{
  LoadedDesign d(Technology::kNangate45, netlistPath(), "top", false);
  EXPECT_FALSE(d.hasHierarchy());

  const std::string out = workDir() + "/loaded_design_flat.v";
  d.writeVerilog(out);
  const std::string v = readAll(out);

  // One module, with the hierarchy folded into escaped instance names.
  EXPECT_EQ(v.find("module block1"), std::string::npos);
  EXPECT_NE(v.find("\\b1/r1 "), std::string::npos);
}

// The reason LoadedDesign is not a gtest fixture: both loads in one test body.
TEST(TestLoadedDesign, BothLinkModesInOneProcess)
{
  LoadedDesign hier(Technology::kNangate45, netlistPath(), "top", true);
  const std::string hier_out = workDir() + "/both_hier.v";
  hier.writeVerilog(hier_out);

  LoadedDesign flat(Technology::kNangate45, netlistPath(), "top", false);
  const std::string flat_out = workDir() + "/both_flat.v";
  flat.writeVerilog(flat_out);

  // Same input, two link modes, structurally different Verilog -- which is
  // exactly the surface the conformance checks look at.
  EXPECT_NE(readAll(hier_out), readAll(flat_out));
}

}  // namespace
}  // namespace tst
