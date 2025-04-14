// Copyright 2025 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <unistd.h>

#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "nangate45_test_fixture.h"
#include "odb/db.h"
#include "odb/defin.h"
#include "utl/Logger.h"

namespace odb {

TEST_F(Nangate45TestFixture, PolygonalFloorplanCreatesBlockagesInNegativeSpace)
{
  // Arrange
  defin def_parser(db_.get(), &logger_);
  std::vector<dbLib*> libs = {lib_.get()};

  // Act
  dbChip* chip = def_parser.createChip(
      libs, "data/nangate45_polygon_floorplan.def", lib_->getTech());
  EXPECT_NE(chip, nullptr);

  // Assert
  dbBlock* block = chip->getBlock();

  // Assert that there is one virtual blockage for this floorplan.
  // There's essentially 1 rectangle cut out of it.
  dbSet<dbBlockage> blockages = block->getBlockages();
  std::vector<dbBlockage*> virtual_blockages;
  for (dbBlockage* blockage : blockages) {
    if (!blockage->isSystemReserved()) {
      continue;
    }
    virtual_blockages.push_back(blockage);
  }
  EXPECT_EQ(virtual_blockages.size(), 1);
  EXPECT_EQ(virtual_blockages[0]->getBBox()->getDX(), 100000);
  EXPECT_EQ(virtual_blockages[0]->getBBox()->getDY(), 150000);

  // 1 obstruction for each via and metal layer in Nangate 45
  // should be 21 including poly layers.
  dbSet<dbObstruction> obstructions = block->getObstructions();
  std::vector<dbObstruction*> virtual_obstructions;
  for (dbObstruction* obstruction : obstructions) {
    if (!obstruction->isSystemReserved()) {
      continue;
    }
    virtual_obstructions.push_back(obstruction);
    EXPECT_EQ(obstruction->getBBox()->getDX(), 100000);
    EXPECT_EQ(obstruction->getBBox()->getDY(), 150000);
  }
  EXPECT_EQ(virtual_obstructions.size(), 21);
}

TEST_F(Nangate45TestFixture, SettingTheFloorplanTwiceClearsSystemBlockages)
{
  // Arrange
  defin def_parser(db_.get(), &logger_);
  std::vector<dbLib*> libs = {lib_.get()};

  // Act
  dbChip* chip = def_parser.createChip(
      libs, "data/nangate45_polygon_floorplan.def", lib_->getTech());
  EXPECT_NE(chip, nullptr);

  odb::Polygon new_die_area({{0, 0},
                             {0, 300000},
                             {400000, 300000},
                             {400000, 160000},
                             {300000, 160000},
                             {300000, 0}});

  dbBlock* block = chip->getBlock();
  block->setDieArea(new_die_area);

  // Assert that there is one virtual blockage for this floorplan.
  // There's essentially 1 rectangle cut out of it.
  dbSet<dbBlockage> blockages = block->getBlockages();
  std::vector<dbBlockage*> virtual_blockages;
  for (dbBlockage* blockage : blockages) {
    if (!blockage->isSystemReserved()) {
      continue;
    }
    virtual_blockages.push_back(blockage);
  }
  EXPECT_EQ(virtual_blockages.size(), 1);
  EXPECT_EQ(virtual_blockages[0]->getBBox()->getDX(), 100000);
  EXPECT_EQ(virtual_blockages[0]->getBBox()->getDY(), 160000);

  // 1 obstruction for each via and metal layer in Nangate 45
  // should be 21 including poly layers.
  dbSet<dbObstruction> obstructions = block->getObstructions();
  std::vector<dbObstruction*> virtual_obstructions;
  for (dbObstruction* obstruction : obstructions) {
    if (!obstruction->isSystemReserved()) {
      continue;
    }
    virtual_obstructions.push_back(obstruction);
    EXPECT_EQ(obstruction->getBBox()->getDX(), 100000);
    EXPECT_EQ(obstruction->getBBox()->getDY(), 160000);
  }
  EXPECT_EQ(virtual_obstructions.size(), 21);
}

TEST_F(Nangate45TestFixture, DeletingSystemBlockagesThrows)
{
  // Arrange
  defin def_parser(db_.get(), &logger_);
  std::vector<dbLib*> libs = {lib_.get()};

  // Act
  dbChip* chip = def_parser.createChip(
      libs, "data/nangate45_polygon_floorplan.def", lib_->getTech());
  EXPECT_NE(chip, nullptr);
  dbBlock* block = chip->getBlock();

  // Assert

  // Assert that there is one virtual blockage for this floorplan.
  // There's essentially 1 rectangle cut out of it.
  dbSet<dbBlockage> blockages = block->getBlockages();
  std::vector<dbBlockage*> virtual_blockages;
  for (dbBlockage* blockage : blockages) {
    if (!blockage->isSystemReserved()) {
      continue;
    }
    virtual_blockages.push_back(blockage);
  }
  EXPECT_EQ(virtual_blockages.size(), 1);
  EXPECT_THROW(
      { dbBlockage::destroy(virtual_blockages[0]); }, std::runtime_error);
}

TEST_F(Nangate45TestFixture, DeletingSystemObstructionsThrows)
{
  // Arrange
  defin def_parser(db_.get(), &logger_);
  std::vector<dbLib*> libs = {lib_.get()};

  // Act
  dbChip* chip = def_parser.createChip(
      libs, "data/nangate45_polygon_floorplan.def", lib_->getTech());
  EXPECT_NE(chip, nullptr);
  dbBlock* block = chip->getBlock();
  // Assert

  // Assert that there is one virtual blockage for this floorplan.
  // There's essentially 1 rectangle cut out of it.
  dbSet<dbObstruction> obstructions = block->getObstructions();
  std::vector<dbObstruction*> virtual_obstructions;
  for (dbObstruction* obstruction : obstructions) {
    if (!obstruction->isSystemReserved()) {
      continue;
    }
    virtual_obstructions.push_back(obstruction);
  }
  EXPECT_EQ(virtual_obstructions.size(), 21);
  EXPECT_THROW(
      { dbObstruction::destroy(virtual_obstructions[0]); }, std::runtime_error);
}

}  // namespace odb
