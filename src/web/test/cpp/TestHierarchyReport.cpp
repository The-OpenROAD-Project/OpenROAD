// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// A flat design (DEF-only: defin creates no dbModules) reaches the hierarchy
// browser with one top module holding every instance, so there is nothing to
// walk.  These cover the fallback that rebuilds a tree from the instance
// names themselves, and -- just as important -- that a design with real
// modules or with no recoverable names is left exactly as it was.

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "boost/json/object.hpp"
#include "boost/json/serialize.hpp"
#include "color.h"
#include "db_sta/dbSta.hh"
#include "gtest/gtest.h"
#include "hierarchy_report.h"
#include "odb/db.h"
#include "tst/nangate45_fixture.h"

namespace web {
namespace {

class HierarchyReportTest : public tst::Nangate45Fixture
{
 protected:
  void SetUp() override
  {
    // getInstanceType() classifies cells from liberty, and the counts below
    // depend on that classification.
    readLiberty("_main/test/Nangate45/Nangate45_typ.lib");
    block_->setDieArea(odb::Rect(0, 0, 100000, 100000));
  }

  odb::dbInst* placeInst(const char* name, const char* master_name = "BUF_X1")
  {
    odb::dbMaster* master = lib_->findMaster(master_name);
    EXPECT_NE(master, nullptr) << "Master not found: " << master_name;
    odb::dbInst* inst = odb::dbInst::create(block_, master, name);
    EXPECT_NE(inst, nullptr) << "Instance not created: " << name;
    inst->setLocation(x_, 0);
    inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    x_ += 1000;
    return inst;
  }

  HierarchyResult report(int depth = kDefaultNameGroupDepth)
  {
    return HierarchyReport(block_, getSta(), depth).getReport();
  }

  // The groups, by display name, in the order getReport() emitted them.
  static std::vector<std::string> groupNames(const HierarchyResult& result)
  {
    std::vector<std::string> names;
    for (const auto& node : result.nodes) {
      if (node.node_kind == HierarchyNodeKind::kNameGroup) {
        names.push_back(node.inst_name);
      }
    }
    return names;
  }

  static const HierarchyNode* findGroup(const HierarchyResult& result,
                                        const std::string& name)
  {
    for (const auto& node : result.nodes) {
      if (node.node_kind == HierarchyNodeKind::kNameGroup
          && node.inst_name == name) {
        return &node;
      }
    }
    return nullptr;
  }

  int x_ = 0;
};

TEST_F(HierarchyReportTest, RebuildsTreeFromInstanceNamePaths)
{
  placeInst("riscv/dp/_1_");
  placeInst("riscv/dp/_2_");
  placeInst("riscv/ctrl/_3_");
  placeInst("_top_level_");

  const HierarchyResult result = report();

  ASSERT_TRUE(result.name_grouped);
  EXPECT_FALSE(result.name_groups_capped);
  // Alphabetical within a level, parent before child, top first.
  EXPECT_EQ(groupNames(result),
            (std::vector<std::string>{"top", "riscv", "ctrl", "dp"}));

  // Hierarchical counts roll up; local counts do not.
  const HierarchyNode* top = findGroup(result, "top");
  ASSERT_NE(top, nullptr);
  EXPECT_EQ(top->insts, 4);
  EXPECT_EQ(top->local_insts, 1) << "only _top_level_ sits at the top";
  EXPECT_EQ(top->modules, 3) << "riscv, riscv/ctrl, riscv/dp";
  EXPECT_EQ(top->local_modules, 1) << "riscv alone is a direct child";

  const HierarchyNode* riscv = findGroup(result, "riscv");
  ASSERT_NE(riscv, nullptr);
  EXPECT_EQ(riscv->insts, 3);
  EXPECT_EQ(riscv->local_insts, 0) << "nothing is named riscv/<leaf>";
  EXPECT_EQ(riscv->local_modules, 2);

  const HierarchyNode* dp = findGroup(result, "dp");
  ASSERT_NE(dp, nullptr);
  EXPECT_EQ(dp->insts, 2);
  EXPECT_EQ(dp->local_insts, 2);
  EXPECT_EQ(dp->local_modules, 0);
}

TEST_F(HierarchyReportTest, LeavesAFlatDesignWithNoPathsAlone)
{
  // yosys-flattened names carry nothing to recover.  The synthesis must not
  // fire, and the tree must be the one this design has always produced --
  // which is what lets the fallback run unconditionally, with no heuristic
  // deciding whether the design "looks" hierarchical.
  placeInst("_16763_");
  placeInst("_16772_");

  const HierarchyResult result = report();

  EXPECT_FALSE(result.name_grouped);
  EXPECT_TRUE(result.inst_group.empty());
  EXPECT_EQ(groupNames(result), std::vector<std::string>{});
  ASSERT_FALSE(result.nodes.empty());
  EXPECT_EQ(result.nodes[0].node_kind, HierarchyNodeKind::kModule);
  EXPECT_EQ(result.nodes[0].insts, 2);
}

TEST_F(HierarchyReportTest, DoesNotSplitOnEscapedDelimiters)
{
  // The whole reason this goes through dbBlock rather than a split on '/':
  // the delimiter inside \/ belongs to the leaf name.  Splitting naively
  // would invent a group called "b" and it would look perfectly plausible.
  placeInst(R"(a/b\/c)");
  placeInst("a/plain");

  const HierarchyResult result = report();

  ASSERT_TRUE(result.name_grouped);
  EXPECT_EQ(groupNames(result), (std::vector<std::string>{"top", "a"}));

  const HierarchyNode* a = findGroup(result, "a");
  ASSERT_NE(a, nullptr);
  EXPECT_EQ(a->local_insts, 2) << R"(b\/c is a leaf of a, not a group)";
}

TEST_F(HierarchyReportTest, DepthCapStopsTheTreeGrowing)
{
  placeInst("a/b/c/d/_1_");

  EXPECT_EQ(groupNames(report(/*depth=*/1)),
            (std::vector<std::string>{"top", "a"}));
  EXPECT_EQ(groupNames(report(/*depth=*/2)),
            (std::vector<std::string>{"top", "a", "b"}));
  EXPECT_EQ(groupNames(report(/*depth=*/4)),
            (std::vector<std::string>{"top", "a", "b", "c", "d"}));

  // Capping folds the instance into the deepest group that survived, so no
  // instance is lost from the counts at any depth.
  for (const int depth : {1, 2, 4}) {
    const HierarchyResult result = report(depth);
    ASSERT_FALSE(result.nodes.empty());
    EXPECT_EQ(result.nodes[0].insts, 1) << "at depth " << depth;
  }
}

TEST_F(HierarchyReportTest, EveryInstanceResolvesToAGroup)
{
  odb::dbInst* deep = placeInst("riscv/dp/_1_");
  odb::dbInst* shallow = placeInst("riscv/_2_");
  odb::dbInst* top_level = placeInst("_3_");

  const HierarchyResult result = report();
  ASSERT_TRUE(result.name_grouped);

  // The tile renderer reads this per instance per tile, so it is indexed by
  // dbInst id rather than mapped.
  ASSERT_GT(result.inst_group.size(), deep->getId());
  ASSERT_GT(result.inst_group.size(), shallow->getId());
  ASSERT_GT(result.inst_group.size(), top_level->getId());

  const HierarchyNode* dp = findGroup(result, "dp");
  const HierarchyNode* riscv = findGroup(result, "riscv");
  const HierarchyNode* top = findGroup(result, "top");
  ASSERT_NE(dp, nullptr);
  ASSERT_NE(riscv, nullptr);
  ASSERT_NE(top, nullptr);

  EXPECT_EQ(result.inst_group[deep->getId()], dp->odb_id);
  EXPECT_EQ(result.inst_group[shallow->getId()], riscv->odb_id);
  EXPECT_EQ(result.inst_group[top_level->getId()], top->odb_id);
  EXPECT_EQ(top->odb_id, 0u) << "the top level is group 0, the default";
}

TEST_F(HierarchyReportTest, NameGroupsAreColoredAndSerialized)
{
  placeInst("riscv/dp/_1_");
  placeInst("_2_");

  const HierarchyResult result = report();
  ASSERT_TRUE(result.name_grouped);

  // Groups carry a color key, exactly as modules do -- the client protocol
  // and the tile lookup do not change between the two modes.
  const boost::json::object json = serializeHierarchyResult(result);
  ASSERT_TRUE(json.contains("name_grouped"));
  EXPECT_TRUE(json.at("name_grouped").as_bool());

  int colored = 0;
  for (const auto& value : json.at("nodes").as_array()) {
    const boost::json::object& node = value.as_object();
    if (node.contains("node_kind")
        && node.at("node_kind").as_int64()
               == static_cast<int>(HierarchyNodeKind::kNameGroup)) {
      EXPECT_TRUE(node.contains("odb_id"));
      EXPECT_EQ(node.at("color").as_array().size(), 3u);
      colored++;
    }
  }
  EXPECT_EQ(colored, 3) << "top, riscv, dp";

  // And the pre-rendered static report colors them too.
  const std::map<uint32_t, Color> colors = computeDefaultModuleColors(result);
  EXPECT_EQ(colors.size(), 3u);
}

TEST_F(HierarchyReportTest, RealModulesAreNeverReplacedByNameGroups)
{
  // A design that has a module tree keeps it, even when the instance names
  // also carry paths.  Mixing the two would mix the color key spaces the
  // tile renderer looks up.
  odb::dbModule* child = odb::dbModule::create(block_, "child_mod");
  ASSERT_NE(child, nullptr);
  ASSERT_NE(odb::dbModInst::create(block_->getTopModule(), child, "u1"),
            nullptr);
  placeInst("riscv/dp/_1_");

  const HierarchyResult result = report();

  EXPECT_FALSE(result.name_grouped);
  EXPECT_TRUE(groupNames(result).empty());
  ASSERT_FALSE(result.nodes.empty());
  EXPECT_EQ(result.nodes[0].node_kind, HierarchyNodeKind::kModule);
}

}  // namespace
}  // namespace web
