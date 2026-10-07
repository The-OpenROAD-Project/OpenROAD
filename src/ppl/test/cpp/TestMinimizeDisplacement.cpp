// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
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
    addLayers();
    pin_placer_.runHungarianMatching(minimize_displacement);
  }

  void addLayers()
  {
    odb::dbTech* tech = db_->getTech();
    pin_placer_.addHorLayer(tech->findLayer("metal3"));
    pin_placer_.addVerLayer(tech->findLayer("metal2"));
  }

  std::vector<SlotPosition> slotGrid()
  {
    addLayers();
    std::vector<SlotPosition> grid = pin_placer_.buildSlotGrid();
    pin_placer_.clear();
    return grid;
  }

  std::vector<odb::dbBTerm*> bterms(int first, int size) const
  {
    std::vector<odb::dbBTerm*> all;
    for (odb::dbBTerm* bterm : block_->getBTerms()) {
      all.push_back(bterm);
    }
    return {all.begin() + first, all.begin() + first + size};
  }

  // Groups larger than a section are placed during fallback mode.
  std::vector<odb::dbBTerm*> addFallbackGroup(int first, int size)
  {
    std::vector<odb::dbBTerm*> group = bterms(first, size);
    block_->addBTermGroup(group, true);
    return group;
  }

  void putPinAt(odb::dbBTerm* bterm, const SlotPosition& slot)
  {
    std::vector<odb::dbBPin*> bpins;
    for (odb::dbBPin* bpin : bterm->getBPins()) {
      bpins.push_back(bpin);
    }
    for (odb::dbBPin* bpin : bpins) {
      odb::dbBPin::destroy(bpin);
    }
    odb::dbBPin* bpin = odb::dbBPin::create(bterm);
    odb::dbTechLayer* layer = db_->getTech()->findRoutingLayer(slot.layer);
    const int half = layer->getWidth() / 2;
    odb::dbBox::create(bpin,
                       layer,
                       slot.pos.x() - half,
                       slot.pos.y() - half,
                       slot.pos.x() + half,
                       slot.pos.y() + half);
    bpin->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  }

  // The first of the size free bottom slots in a row at the middle of the
  // longest such run.
  static int middleOfFreeBottomRun(const std::vector<SlotPosition>& grid,
                                   int size,
                                   int* run_length = nullptr)
  {
    int best_begin = -1;
    int best_length = 0;
    for (int s = 0; s < grid.size();) {
      if (grid[s].edge != Edge::bottom || grid[s].blocked) {
        ++s;
        continue;
      }
      const int begin = s;
      while (s < grid.size() && grid[s].edge == Edge::bottom
             && !grid[s].blocked) {
        ++s;
      }
      if (s - begin > best_length) {
        best_begin = begin;
        best_length = s - begin;
      }
    }
    if (run_length != nullptr) {
      *run_length = best_length;
    }
    if (best_length < size) {
      return -1;
    }
    return best_begin + (best_length - size) / 2;
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

// First-fit would move a legal group to the first free slots of the die.
TEST_F(MinimizeDisplacement, KeepsALegalGroupOfFallbackMode)
{
  const std::vector<SlotPosition> grid = slotGrid();
  const std::vector<odb::dbBTerm*> group = addFallbackGroup(0, 20);
  const int start = middleOfFreeBottomRun(grid, group.size());
  ASSERT_GT(start, 0);
  for (int i = 0; i < group.size(); ++i) {
    putPinAt(group[i], grid[start + i]);
  }

  pin_placer_.getParameters()->setSlotsPerSection(10);
  placePins(true);
  for (int i = 0; i < group.size(); ++i) {
    EXPECT_EQ(group[i]->getBBox().xCenter(), grid[start + i].pos.x());
  }
}

// Group a kept at the middle of the only free run leaves no room for group b,
// so first-fit places both.
TEST_F(MinimizeDisplacement, PlacesGroupsThatOnlyFitPacked)
{
  const odb::Rect die = block_->getDieArea();
  block_->addBlockedRegionForPins(
      {die.xMin(), die.yMax(), die.xMax(), die.yMax()});
  block_->addBlockedRegionForPins(
      {die.xMin(), die.yMin(), die.xMin(), die.yMax()});
  block_->addBlockedRegionForPins(
      {die.xMax(), die.yMin(), die.xMax(), die.yMax()});
  const int size = block_->getBTerms().size() / 2;
  std::vector<SlotPosition> grid = slotGrid();
  const int free_begin = middleOfFreeBottomRun(grid, 2 * size + 6);
  ASSERT_GE(free_begin, 1);
  const int free_end = free_begin + 2 * size + 6;
  block_->addBlockedRegionForPins(
      {die.xMin(),
       die.yMin(),
       (grid[free_begin - 1].pos.x() + grid[free_begin].pos.x()) / 2,
       die.yMin()});
  block_->addBlockedRegionForPins(
      {(grid[free_end - 1].pos.x() + grid[free_end].pos.x()) / 2,
       die.yMin(),
       die.xMax(),
       die.yMin()});

  grid = slotGrid();
  int free_run = 0;
  const std::vector<odb::dbBTerm*> a = addFallbackGroup(0, size);
  const int start = middleOfFreeBottomRun(grid, size, &free_run);
  ASSERT_GE(free_run, 2 * size);
  ASSERT_LT(free_run, 3 * size);
  for (int i = 0; i < size; ++i) {
    putPinAt(a[i], grid[start + i]);
  }
  addFallbackGroup(size, size);

  pin_placer_.getParameters()->setSlotsPerSection(10);
  EXPECT_NO_THROW(placePins(true));
}

// The group pins move as much at any bottom slot, half of them coming from
// the left edge and half from the right, so only the mirrored pins, which the
// group drags to the top edge, pick its slots.
TEST_F(MinimizeDisplacement, CountsTheMirroredPinsOfAGroup)
{
  const std::vector<SlotPosition> grid = slotGrid();
  const int size = 20;
  const std::vector<odb::dbBTerm*> group = addFallbackGroup(0, size);
  const std::vector<odb::dbBTerm*> mirrored = bterms(size, size);

  auto lowest = [&](const Edge edge) {
    std::vector<SlotPosition> slots;
    for (const SlotPosition& slot : grid) {
      if (slot.edge == edge && !slot.blocked) {
        slots.push_back(slot);
      }
    }
    std::ranges::sort(slots, [](const SlotPosition& a, const SlotPosition& b) {
      return a.pos.y() < b.pos.y();
    });
    return slots;
  };
  const std::vector<SlotPosition> left = lowest(Edge::left);
  const std::vector<SlotPosition> right = lowest(Edge::right);
  ASSERT_GE(left.size(), size / 2);
  ASSERT_GE(right.size(), size / 2);
  for (int i = 0; i < size / 2; ++i) {
    putPinAt(group[i], left[i]);
    putPinAt(group[size / 2 + i], right[i]);
  }

  const int start = middleOfFreeBottomRun(grid, size);
  ASSERT_GT(start, 0);
  std::vector<SlotPosition> top;
  for (int i = 0; i < size; ++i) {
    const SlotPosition& bottom = grid[start + i];
    const auto it = std::ranges::find_if(grid, [&](const SlotPosition& slot) {
      return slot.edge == Edge::top && slot.layer == bottom.layer
             && slot.pos.x() == bottom.pos.x();
    });
    ASSERT_NE(it, grid.end());
    top.push_back(*it);
    putPinAt(mirrored[i], *it);
    group[i]->setMirroredBTerm(mirrored[i]);
  }

  pin_placer_.getParameters()->setSlotsPerSection(10);
  placePins(true);
  const int mid_y = block_->getDieArea().yCenter();
  for (int i = 0; i < size; ++i) {
    EXPECT_EQ(group[i]->getBBox().xCenter(), grid[start + i].pos.x());
    EXPECT_LT(group[i]->getBBox().yCenter(), mid_y);
    EXPECT_EQ(mirrored[i]->getBBox().xCenter(), top[i].pos.x());
    EXPECT_GT(mirrored[i]->getBBox().yCenter(), mid_y);
  }
}

}  // namespace
}  // namespace ppl
