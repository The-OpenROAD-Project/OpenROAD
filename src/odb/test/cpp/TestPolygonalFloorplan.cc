// Copyright 2023 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <unistd.h>

#include <memory>
#include <sstream>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "nangate45_test_fixture.h"
#include "odb/db.h"
#include "odb/defin.h"
#include "utl/Logger.h"

namespace odb {

using ::testing::HasSubstr;

TEST_F(Nangate45TestFixture, AbstractLefWriterMapsTieOffToSignal)
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
    if (!blockage->isVirtual()) {
      continue;
    }
    virtual_blockages.push_back(blockage);
  }
  EXPECT_EQ(virtual_blockages.size(), 1);

  // 1 obstruction for each via and metal layer in Nangate 45
  // should be 21 including poly layers.
  dbSet<dbObstruction> obstructions = block->getObstructions();
  std::vector<dbObstruction*> virtual_obstructions;
  for (dbObstruction* obstruction : obstructions) {
    if (!obstruction->isVirtual()) {
      continue;
    }
    virtual_obstructions.push_back(obstruction);
  }
  EXPECT_EQ(virtual_obstructions.size(), 21);
}

}  // namespace odb
