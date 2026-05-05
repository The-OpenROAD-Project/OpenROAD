// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <memory>
#include <string>
#include <vector>

#include "../../src/hier_rtlmp.h"
#include "../../src/object.h"
#include "MplTest.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace mpl {
namespace {

class TestPusher : public MplTest
{
 protected:
  void SetUp() override
  {
    MplTest::SetUp();
    db_->getChip()->getBlock()->setCoreArea(
        odb::Rect(0, 0, die_width_, die_height_));

    odb::dbLib* lib = db_->findLib("lib");
    master_ = odb::dbMaster::create(lib, "macro_master");
    master_->setType(odb::dbMasterType::BLOCK);
    master_->setWidth(macro_width_);
    master_->setHeight(macro_height_);
    master_->setFrozen();
  }

  odb::dbBlock* block() { return db_->getChip()->getBlock(); }

  // Returns a root MixedCluster with a StdCellCluster child that has a
  // non-zero area SoftMacro.  This prevents Pusher from treating the design
  // as a "single centralized macro array" and skipping the push entirely.
  std::unique_ptr<Cluster> makeRootWithStdCells()
  {
    auto root = std::make_unique<Cluster>(next_id_++, "root", &logger_);
    root->setClusterType(MixedCluster);

    auto std_cell
        = std::make_unique<Cluster>(next_id_++, "std_cells", &logger_);
    std_cell->setClusterType(StdCellCluster);
    auto std_soft = std::make_unique<SoftMacro>(std_cell.get());
    std_soft->setShapeF(macro_width_, macro_height_);
    std_cell->setSoftMacro(std::move(std_soft));
    root->addChild(std::move(std_cell));

    return root;
  }

  // Appends a HardMacroCluster child to parent placed at (x, y).
  // Returns the cluster's raw pointer
  // (owned by parent).  The matching HardMacro is owned by
  // hard_macro_storage_.
  Cluster* addMacroCluster(Cluster* parent,
                           const std::string& name,
                           int x,
                           int y,
                           int width,
                           int height)
  {
    auto cluster = std::make_unique<Cluster>(next_id_++, name, &logger_);
    cluster->setClusterType(HardMacroCluster);

    auto soft = std::make_unique<SoftMacro>(cluster.get());
    soft->setLocationF(x, y);
    soft->setShapeF(width, height);
    cluster->setSoftMacro(std::move(soft));

    auto hard_macro = std::make_unique<HardMacro>(
        odb::Point(x, y), name + "_hard", width, height, cluster.get());
    HardMacro* macro_ptr = hard_macro.get();
    hard_macro_storage_.push_back(std::move(hard_macro));

    std::vector<HardMacro*> macros = {macro_ptr};
    cluster->specifyHardMacros(macros);

    Cluster* cluster_ptr = cluster.get();
    parent->addChild(std::move(cluster));
    return cluster_ptr;
  }

  Cluster* addMacroCluster(Cluster* parent,
                           const std::string& name,
                           int x,
                           int y)
  {
    return addMacroCluster(parent, name, x, y, macro_width_, macro_height_);
  }

  const int macro_width_ = 100000;
  const int macro_height_ = 100000;
  int next_id_ = 0;
  odb::dbMaster* master_ = nullptr;
  std::vector<std::unique_ptr<HardMacro>> hard_macro_storage_;
};

// When the root cluster is a HardMacroCluster (the design is entirely made
// up of macros), pushMacrosToCoreBoundaries() returns immediately without
// touching any macro.
TEST_F(TestPusher, RootIsHardMacroCluster)
{
  auto root = std::make_unique<Cluster>(next_id_++, "root", &logger_);
  root->setClusterType(HardMacroCluster);

  auto hard_macro = std::make_unique<HardMacro>(odb::Point(10000, 10000),
                                                "root_macro",
                                                macro_width_,
                                                macro_height_,
                                                root.get());
  HardMacro* macro_ptr = hard_macro.get();
  hard_macro_storage_.push_back(std::move(hard_macro));

  std::vector<HardMacro*> macros = {macro_ptr};
  root->specifyHardMacros(macros);

  Pusher pusher(&logger_, root.get(), block(), {});
  pusher.pushMacrosToCoreBoundaries();

  EXPECT_EQ(macro_ptr->getX(), 10000);
  EXPECT_EQ(macro_ptr->getY(), 10000);
}

// When the root has exactly one HardMacroCluster child and no MixedCluster
// or non-zero StdCellCluster children, the design is treated as a single
// centralized macro array and no push is performed.
TEST_F(TestPusher, SingleCentralizedMacroArray)
{
  auto root = std::make_unique<Cluster>(next_id_++, "root", &logger_);
  root->setClusterType(MixedCluster);

  addMacroCluster(root.get(), "macro_cluster", 10000, 200000);
  HardMacro* macro_ptr = hard_macro_storage_.back().get();

  Pusher pusher(&logger_, root.get(), block(), {});
  pusher.pushMacrosToCoreBoundaries();

  EXPECT_EQ(macro_ptr->getX(), 10000);
  EXPECT_EQ(macro_ptr->getY(), 200000);
}

// A macro cluster whose distance to the left boundary is less than one
// macro width should be pushed to the left edge of the core.
TEST_F(TestPusher, MacroPushedToLeftBoundary)
{
  auto root = makeRootWithStdCells();

  // xMin = 10000; distance_to_left = 10000 < macro_width_ (100000)
  addMacroCluster(root.get(), "macro_cluster", 10000, 0);
  HardMacro* macro_ptr = hard_macro_storage_.back().get();

  Pusher pusher(&logger_, root.get(), block(), {});
  pusher.pushMacrosToCoreBoundaries();

  EXPECT_EQ(macro_ptr->getX(), 0);
}

// A macro cluster whose distance to the right boundary is less than one
// macro width should be pushed to the left edge of the core.
TEST_F(TestPusher, MacroPushedToRightBoundary)
{
  auto root = makeRootWithStdCells();

  // xMax = 490000; distance_to_right = |490000 - 500000| = 10000 < macro_width_
  const int macro_x = die_width_ - macro_width_ - 10000;
  addMacroCluster(root.get(), "macro_cluster", macro_x, 0);
  HardMacro* macro_ptr = hard_macro_storage_.back().get();

  Pusher pusher(&logger_, root.get(), block(), {});
  pusher.pushMacrosToCoreBoundaries();

  EXPECT_EQ(macro_ptr->getX(), die_width_ - macro_width_);
}

// A macro cluster whose distance to the bottom boundary is less than one
// macro height should be pushed to the left edge of the core.
TEST_F(TestPusher, MacroPushedToBottomBoundary)
{
  auto root = makeRootWithStdCells();

  // yMin = 5000; distance_to_bottom = 5000 < macro_height_ (100000)
  addMacroCluster(root.get(), "macro_cluster", 0, 5000);
  HardMacro* macro_ptr = hard_macro_storage_.back().get();

  Pusher pusher(&logger_, root.get(), block(), {});
  pusher.pushMacrosToCoreBoundaries();

  EXPECT_EQ(macro_ptr->getY(), 0);
}

// A macro cluster whose distance to the top boundary is less than one
// macro height should be pushed to the left edge of the core.
TEST_F(TestPusher, MacroPushedToTopBoundary)
{
  auto root = makeRootWithStdCells();

  // yMax = 490000; distance_to_top = |490000 - 500000| = 10000 < macro_height_
  const int macro_y = die_height_ - macro_height_ - 10000;
  addMacroCluster(root.get(), "macro_cluster", 0, macro_y);
  HardMacro* macro_ptr = hard_macro_storage_.back().get();

  Pusher pusher(&logger_, root.get(), block(), {});
  pusher.pushMacrosToCoreBoundaries();

  EXPECT_EQ(macro_ptr->getY(), die_height_ - macro_height_);
}

// A macro cluster tagged as fixed must not be moved regardless of its
// proximity to any boundary.
TEST_F(TestPusher, FixedMacroCluster)
{
  auto root = makeRootWithStdCells();

  // Build a HardMacroCluster backed by a FIRM dbInst so that
  // Cluster::setAsFixedMacro() can succeed (it requires isFixed() == true).
  auto cluster
      = std::make_unique<Cluster>(next_id_++, "fixed_cluster", &logger_);
  cluster->setClusterType(HardMacroCluster);

  odb::dbInst* inst = odb::dbInst::create(block(), master_, "fixed_inst");
  inst->setLocation(10000, 10000);
  inst->setPlacementStatus(odb::dbPlacementStatus::FIRM);

  auto hard_macro = std::make_unique<HardMacro>(inst, HardMacro::Halo{});
  hard_macro->setCluster(cluster.get());
  HardMacro* macro_ptr = hard_macro.get();
  hard_macro_storage_.push_back(std::move(hard_macro));

  cluster->setAsFixedMacro(macro_ptr);

  std::vector<HardMacro*> macros = {macro_ptr};
  cluster->specifyHardMacros(macros);

  root->addChild(std::move(cluster));

  Pusher pusher(&logger_, root.get(), block(), {});
  pusher.pushMacrosToCoreBoundaries();

  EXPECT_EQ(macro_ptr->getX(), 10000);
  EXPECT_EQ(macro_ptr->getY(), 10000);
}

// When pushing a macro cluster toward its closest horizontal boundary would
// cause it to overlap with another hard macro, the move is reverted and the
// cluster moves only vertically
TEST_F(TestPusher, PushRevertedHorizontal)
{
  auto root = makeRootWithStdCells();

  // macro1 is 10000 units from the bottom and left edges, it would normally be
  // pushed to the origin.
  addMacroCluster(root.get(), "macro1", 10000, 10000);
  HardMacro* macro1_ptr = hard_macro_storage_.back().get();

  // macro2 already occupies (0, 10000), blocking the horizontal push.
  addMacroCluster(root.get(), "macro2", 0, 10000, 5000, 5000);

  Pusher pusher(&logger_, root.get(), block(), {});
  pusher.pushMacrosToCoreBoundaries();

  // Horizontal push reverted: the destination overlaps macro2.
  // Vertical push is kept.
  EXPECT_EQ(macro1_ptr->getX(), 10000);
  EXPECT_EQ(macro1_ptr->getY(), 0);
}

// When pushing a macro cluster toward its closest vertical boundary would cause
// it to overlap with another hard macro, the move is reverted and the cluster
// moves only horizontally
TEST_F(TestPusher, PushRevertedVertical)
{
  auto root = makeRootWithStdCells();

  // macro1 is 10000 units from the bottom and left edges, it would normally be
  // pushed to the origin.
  addMacroCluster(root.get(), "macro1", 10000, 10000);
  HardMacro* macro1_ptr = hard_macro_storage_.back().get();

  // macro2 already occupies (10000, 0), blocking the horizontal push.
  addMacroCluster(root.get(), "macro2", 10000, 0, 5000, 5000);

  Pusher pusher(&logger_, root.get(), block(), {});
  pusher.pushMacrosToCoreBoundaries();

  // Vertical push reverted: the destination overlaps macro2.
  // Horizontal push is kept.
  EXPECT_EQ(macro1_ptr->getX(), 0);
  EXPECT_EQ(macro1_ptr->getY(), 10000);
}

// When pushing a macro cluster toward its closest boundaries would cause an
// overlap with a macro diagonal to the pushed macro, push the macro the bottom
// The Pusher is biased by the Boundary enum ordering of boundaries (B > L > T >
// R)
TEST_F(TestPusher, PushRevertedBiased)
{
  auto root = makeRootWithStdCells();

  // macro1 is 10000 units from the bottom and left edges, it would normally be
  // pushed to the origin.
  addMacroCluster(root.get(), "macro1", 10000, 10000);
  HardMacro* macro1_ptr = hard_macro_storage_.back().get();

  // macro2 already occupies (0, 0), blocking the last push but not the first
  // one.
  addMacroCluster(root.get(), "macro2", 0, 0, 5000, 5000);

  Pusher pusher(&logger_, root.get(), block(), {});
  pusher.pushMacrosToCoreBoundaries();

  // Last push (left) is reverted, bottom push is kept
  EXPECT_EQ(macro1_ptr->getX(), 10000);
  EXPECT_EQ(macro1_ptr->getY(), 0);
}

// When pushing a macro cluster toward its closest boundary would cause it to
// overlap with an IO blockage, the move is reverted and the cluster stays at
// its original position.
TEST_F(TestPusher, PushRevertedOnIOBlockageOverlap)
{
  auto root = makeRootWithStdCells();

  // Macro is 10000 units from the left; without the blockage it would be
  // pushed to x = 0.
  addMacroCluster(root.get(), "macro_cluster", 10000, 0);
  HardMacro* macro_ptr = hard_macro_storage_.back().get();

  // IO blockage covers the left side where the macro would land.
  const std::vector<odb::Rect> io_blockages = {odb::Rect(0, 0, 50000, 100000)};

  Pusher pusher(&logger_, root.get(), block(), io_blockages);
  pusher.pushMacrosToCoreBoundaries();

  // Push reverted: the moved cluster box overlaps the IO blockage.
  EXPECT_EQ(macro_ptr->getX(), 10000);
  EXPECT_EQ(macro_ptr->getY(), 0);
}

}  // namespace
}  // namespace mpl
