// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <memory>
#include <string>
#include <vector>

#include "MplTest.h"
#include "gtest/gtest.h"
#include "mpl/rtl_mp.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace mpl {
namespace {

class TestBlockMacroChannels : public MplTest
{
 protected:
  void SetUp() override
  {
    MplTest::SetUp();

    macro_placer_ = std::make_unique<MacroPlacer>(db_.get(),
                                                  /*sta=*/nullptr,
                                                  &logger_,
                                                  /*tritonpart=*/nullptr);

    odb::dbLib* library = db_->findLib("lib");

    macro_master_ = odb::dbMaster::create(library, "block_master");
    macro_master_->setType(odb::dbMasterType::BLOCK);
    macro_master_->setWidth(master_width_);
    macro_master_->setHeight(master_height_);
    macro_master_->setFrozen();
  }

  odb::dbInst* createFixedMacro(const std::string& name,
                                const int origin_x,
                                const int origin_y)
  {
    odb::dbInst* instance = odb::dbInst::create(
        db_->getChip()->getBlock(), macro_master_, name.c_str());
    instance->setLocation(origin_x, origin_y);
    instance->setPlacementStatus(odb::dbPlacementStatus::FIRM);
    return instance;
  }

  std::vector<odb::Rect> getBlockages() const
  {
    std::vector<odb::Rect> rectangles;
    for (odb::dbBlockage* blockage :
         db_->getChip()->getBlock()->getBlockages()) {
      rectangles.push_back(blockage->getBBox()->getBox());
    }
    return rectangles;
  }

  const int master_width_ = 100000;
  const int master_height_ = 100000;
  const int macro_x_ = 50000;
  const int macro_y_ = 50000;
  std::unique_ptr<MacroPlacer> macro_placer_;
  odb::dbMaster* macro_master_ = nullptr;
};

// DEF halo only: blockage equals master bbox expanded by the DEF halo.
TEST_F(TestBlockMacroChannels, DefHaloOnly)
{
  odb::dbInst* instance = createFixedMacro("macro", macro_x_, macro_y_);
  instance->setHalo(5000, 5000, 5000, 5000, /*is_soft=*/false);

  macro_placer_->blockMacroChannels();

  const std::vector<odb::Rect> blockages = getBlockages();
  ASSERT_EQ(blockages.size(), 1);
  EXPECT_EQ(blockages[0], odb::Rect(45000, 45000, 155000, 155000));
}

// Base halo is floored by the DEF halo per side: the larger of the two wins.
TEST_F(TestBlockMacroChannels, BaseHaloFloorsDefHalo)
{
  odb::dbInst* instance = createFixedMacro("macro", macro_x_, macro_y_);
  instance->setHalo(5000, 5000, 5000, 5000, /*is_soft=*/false);
  macro_placer_->setBaseHalo(10000, 1000, 10000, 1000);

  macro_placer_->blockMacroChannels();

  const std::vector<odb::Rect> blockages = getBlockages();
  ASSERT_EQ(blockages.size(), 1);
  // Horizontal: base 10000 > DEF 5000. Vertical: DEF 5000 > base 1000.
  EXPECT_EQ(blockages[0], odb::Rect(40000, 45000, 160000, 155000));
}

// Per-macro halo has top priority over both the base halo and the DEF halo.
TEST_F(TestBlockMacroChannels, PerMacroHaloOverrides)
{
  odb::dbInst* instance = createFixedMacro("macro", macro_x_, macro_y_);
  instance->setHalo(5000, 5000, 5000, 5000, /*is_soft=*/false);
  macro_placer_->setBaseHalo(10000, 10000, 10000, 10000);
  macro_placer_->setMacroHalo(instance, 20000, 20000, 20000, 20000);

  macro_placer_->blockMacroChannels();

  const std::vector<odb::Rect> blockages = getBlockages();
  ASSERT_EQ(blockages.size(), 1);
  EXPECT_EQ(blockages[0], odb::Rect(30000, 30000, 170000, 170000));
}

// Macros with a soft DEF halo are skipped even when a base halo is set.
TEST_F(TestBlockMacroChannels, SoftDefHaloSkipped)
{
  odb::dbInst* instance = createFixedMacro("macro", macro_x_, macro_y_);
  instance->setHalo(5000, 5000, 5000, 5000, /*is_soft=*/true);
  macro_placer_->setBaseHalo(10000, 10000, 10000, 10000);

  macro_placer_->blockMacroChannels();

  EXPECT_TRUE(getBlockages().empty());
}

// When the macro has no DEF halo, the base halo alone defines the blockage.
TEST_F(TestBlockMacroChannels, BaseHaloOnly)
{
  createFixedMacro("macro", macro_x_, macro_y_);
  macro_placer_->setBaseHalo(8000, 4000, 6000, 2000);

  macro_placer_->blockMacroChannels();

  const std::vector<odb::Rect> blockages = getBlockages();
  ASSERT_EQ(blockages.size(), 1);
  EXPECT_EQ(blockages[0], odb::Rect(42000, 46000, 156000, 152000));
}

// Unplaced macros are ignored.
TEST_F(TestBlockMacroChannels, UnplacedMacroSkipped)
{
  odb::dbInst::create(db_->getChip()->getBlock(), macro_master_, "macro");

  macro_placer_->blockMacroChannels();

  EXPECT_TRUE(getBlockages().empty());
}

// Placed macros are ignored: MPL can still move them as they're not fixed.
TEST_F(TestBlockMacroChannels, PlacedMacroSkipped)
{
  odb::dbInst* instance
      = odb::dbInst::create(db_->getChip()->getBlock(), macro_master_, "macro");
  instance->setLocation(macro_x_, macro_y_);
  instance->setPlacementStatus(odb::dbPlacementStatus::PLACED);

  macro_placer_->blockMacroChannels();

  EXPECT_TRUE(getBlockages().empty());
}

}  // namespace
}  // namespace mpl
