// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <map>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/defin.h"
#include "odb/geom.h"
#include "ppl/IOPlacer.h"
#include "tst/fixture.h"

namespace ppl {
namespace {

// Ranking slots by wirelength discards a pin placement the caller has already
// optimized, which is what global_placement -place_ios leaves behind.
// Minimizing displacement ranks them by distance instead, so the assignment
// legalizes that placement rather than replacing it.
class MinimizeDisplacement : public tst::Fixture
{
 protected:
  void SetUp() override
  {
    odb::dbLib* lib = loadTechAndLib(
        "nangate45", "nangate45", "_main/test/Nangate45/Nangate45.lef");
    ASSERT_NE(lib, nullptr);
    odb::dbChip* chip = odb::dbChip::create(db_.get(), lib->getTech());
    std::vector<odb::dbLib*> libs{lib};
    odb::defin reader(db_.get(), &logger_);
    const std::string def = getFilePath("_main/src/ppl/test/gcd.def");
    ASSERT_TRUE(reader.readChip(libs, def.c_str(), chip));
    block_ = chip->getBlock();
  }

  // place_pins -hor_layers metal3 -ver_layers metal2
  void placePins(bool minimize_displacement)
  {
    odb::dbTech* tech = db_->getTech();
    pin_placer_.addHorLayer(tech->findLayer("metal3"));
    pin_placer_.addVerLayer(tech->findLayer("metal2"));
    pin_placer_.runHungarianMatching(minimize_displacement);
  }

  std::map<std::string, odb::Point> pinCenters() const
  {
    std::map<std::string, odb::Point> centers;
    for (odb::dbBTerm* bterm : block_->getBTerms()) {
      const odb::Rect box = bterm->getBBox();
      centers[bterm->getName()] = {box.xCenter(), box.yCenter()};
    }
    return centers;
  }

  // Every pin's wirelength-optimal edge becomes the opposite one, while the
  // pins themselves stay put.
  void mirrorCellsAcrossTheCore()
  {
    const odb::Rect core = block_->getCoreArea();
    for (odb::dbInst* inst : block_->getInsts()) {
      const odb::Point at = inst->getLocation();
      inst->setLocation(
          core.xMin() + core.xMax() - at.x() - inst->getMaster()->getWidth(),
          at.y());
    }
  }

  int movedSince(const std::map<std::string, odb::Point>& from) const
  {
    int moved = 0;
    for (const auto& [name, at] : pinCenters()) {
      if (at != from.at(name)) {
        ++moved;
      }
    }
    return moved;
  }

  odb::dbBlock* block_ = nullptr;
  IOPlacer pin_placer_{db_.get(), &logger_};
};

TEST_F(MinimizeDisplacement, KeepsAPlacementTheCellsNoLongerFavor)
{
  placePins(false);
  const auto placed = pinCenters();
  mirrorCellsAcrossTheCore();

  placePins(true);
  EXPECT_EQ(movedSince(placed), 0);
}

TEST_F(MinimizeDisplacement, IsOnlyForTheRunItIsGivenTo)
{
  placePins(false);
  const auto placed = pinCenters();
  mirrorCellsAcrossTheCore();

  placePins(true);
  placePins(false);
  EXPECT_EQ(movedSince(placed), placed.size());
}

}  // namespace
}  // namespace ppl
