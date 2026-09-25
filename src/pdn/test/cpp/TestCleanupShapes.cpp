// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Which core-grid shapes pdngen removes because a fixed instance covers
// them. A shape that lies wholly inside the outline of a fixed instance, on
// a layer the instance's master draws, is removed; every other shape is
// kept. The answer must not depend on how many fixed instances the block
// has, which of them are standard cells, or the order they were created in.

#include <algorithm>
#include <cstddef>
#include <string>
#include <tuple>
#include <vector>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/lefin.h"
#include "pdn/PdnGen.hh"
#include "tst/db_fixture.h"

namespace pdn {
namespace {

// net, layer, and the box: one special-wire rectangle, vias excluded
using Box = std::tuple<std::string, std::string, int, int, int, int>;

odb::Rect rectOf(const Box& b)
{
  return {std::get<2>(b), std::get<3>(b), std::get<4>(b), std::get<5>(b)};
}

class CleanupShapesTest : public tst::DbFixture
{
 protected:
  // One library per technology: its sites come from the first LEF, and every
  // further LEF adds its macros to the same library, so they find the sites.
  odb::dbLib* loadTechAndLib(const char* name, const std::string& lef)
  {
    odb::lefin reader(db_.get(), &logger_, false);
    return reader.createTechAndLib(name, name, getFilePath(lef).c_str());
  }

  void addLef(odb::dbLib* lib, const std::string& lef)
  {
    odb::lefin reader(db_.get(), &logger_, false);
    ASSERT_TRUE(reader.updateLib(lib, getFilePath(lef).c_str())) << lef;
  }

  // Rows of `site` across the core, alternating N and FS.
  static void makeRows(odb::dbBlock* block,
                       odb::dbSite* site,
                       const odb::Rect& core)
  {
    const int sites = core.dx() / site->getWidth();
    const int rows = core.dy() / site->getHeight();
    for (int r = 0; r < rows; r++) {
      odb::dbRow::create(
          block,
          ("ROW_" + std::to_string(r)).c_str(),
          site,
          core.xMin(),
          core.yMin() + r * site->getHeight(),
          r % 2 == 0 ? odb::dbOrientType::R0 : odb::dbOrientType::MX,
          odb::dbRowDir::HORIZONTAL,
          sites,
          site->getWidth());
    }
  }

  static std::vector<Box> boxes(odb::dbBlock* block)
  {
    std::vector<Box> out;
    for (odb::dbNet* net : block->getNets()) {
      for (odb::dbSWire* swire : net->getSWires()) {
        for (odb::dbSBox* box : swire->getWires()) {
          if (box->isVia()) {
            continue;
          }
          out.emplace_back(net->getName(),
                           box->getTechLayer()->getName(),
                           box->xMin(),
                           box->yMin(),
                           box->xMax(),
                           box->yMax());
        }
      }
    }
    std::sort(out.begin(), out.end());
    return out;
  }

  static std::vector<Box> onLayer(const std::vector<Box>& all,
                                  const std::string& layer)
  {
    std::vector<Box> out;
    std::copy_if(all.begin(), all.end(), std::back_inserter(out), [&](auto& b) {
      return std::get<1>(b) == layer;
    });
    return out;
  }
};

// sky130hd with the digital PLL whose obstructions leave its middle open:
// the met4 and met5 straps of the core grid are cut where they meet the
// PLL's obstructions, which leaves segments standing in the open middle.
class PllTest : public CleanupShapesTest
{
 protected:
  void SetUp() override
  {
    cells_ = loadTechAndLib("sky130hd",
                            "_main/src/pdn/test/sky130hd/sky130hd.tlef");
    ASSERT_NE(cells_, nullptr);
    tech_ = cells_->getTech();
    addLef(cells_, "_main/src/pdn/test/sky130hd/sky130_fd_sc_hd_merged.lef");
    addLef(cells_, "_main/src/pdn/test/sky130_pll/pll_openmiddle.lef");
    pll_lib_ = cells_;
    odb::dbChip* chip = odb::dbChip::create(db_.get(), tech_);
    block_ = odb::dbBlock::create(chip, "top");
    block_->setDefUnits(tech_->getLefUnits());
    block_->setDieArea(odb::Rect(0, 0, 279960, 280130));
    core_ = odb::Rect(10120, 10880, 269560, 269280);
    makeRows(block_, cells_->findSite("unithd"), core_);
    block_->setCoreArea(core_);
    vdd_ = odb::dbNet::create(block_, "VDD");
    vdd_->setSpecial();
    vdd_->setSigType(odb::dbSigType::POWER);
    vss_ = odb::dbNet::create(block_, "VSS");
    vss_->setSpecial();
    vss_->setSigType(odb::dbSigType::GROUND);
  }

  odb::dbInst* placePll(odb::dbPlacementStatus status)
  {
    odb::dbInst* pll = odb::dbInst::create(
        block_, pll_lib_->findMaster("digital_pll"), "PLL");
    pll->setLocation(15000, 15000);
    pll->setPlacementStatus(status);
    return pll;
  }

  // FIXED fill cells in a lattice over the core, `count` of them, created in
  // the order given; none of them is tall enough to hold a strap.
  void placeFill(int count, bool reverse)
  {
    odb::dbMaster* fill = cells_->findMaster("sky130_fd_sc_hd__fill_1");
    odb::dbSite* site = cells_->findSite("unithd");
    const int per_row = core_.dx() / fill->getWidth();
    std::vector<int> order(count);
    for (int i = 0; i < count; i++) {
      order[i] = reverse ? count - 1 - i : i;
    }
    for (int i : order) {
      const int row = (i / per_row) % (core_.dy() / site->getHeight());
      const int col = i % per_row;
      odb::dbInst* inst = odb::dbInst::create(
          block_, fill, ("fill_" + std::to_string(i)).c_str());
      inst->setLocation(core_.xMin() + col * fill->getWidth(),
                        core_.yMin() + row * site->getHeight());
      inst->setOrient(row % 2 == 0 ? odb::dbOrientType::R0
                                   : odb::dbOrientType::MX);
      inst->setPlacementStatus(odb::dbPlacementStatus::FIRM);
    }
    block_->addGlobalConnect(nullptr, "^fill_", "^VPWR$|^VPB$", vdd_, true);
    block_->addGlobalConnect(nullptr, "^fill_", "^VGND$|^VNB$", vss_, true);
  }

  // The grid of macros_grid_through_without_middle.tcl: 2 um straps at a
  // 40 um pitch on met4 and met5, connected, and a macro grid over the PLL.
  std::vector<Box> buildGrid()
  {
    block_->addGlobalConnect(nullptr, ".*", "^VPWR$", vdd_, true);
    block_->addGlobalConnect(nullptr, ".*", "^VGND$", vss_, true);
    PdnGen pdngen(db_.get(), &logger_);
    pdngen.setCoreDomain(vdd_, nullptr, vss_, {});
    VoltageDomain* core = pdngen.findDomain("Core");
    pdngen.makeCoreGrid(
        core, "Core", kGround, {}, {}, nullptr, nullptr, "STAR", {});
    odb::dbTechLayer* met4 = tech_->findLayer("met4");
    odb::dbTechLayer* met5 = tech_->findLayer("met5");
    for (Grid* grid : pdngen.findGrid("Core")) {
      pdngen.makeStrap(
          grid, met4, 2000, 0, 40000, 0, 0, false, kGrid, kCore, {}, false);
      pdngen.makeStrap(
          grid, met5, 2000, 0, 40000, 0, 0, false, kGrid, kCore, {}, false);
      pdngen.makeConnect(grid, met4, met5, 0, 0, {}, {}, 0, 0, {}, {}, {}, "");
    }
    for (odb::dbInst* inst : block_->getInsts()) {
      if (inst->getMaster()->isBlock() && inst->isFixed()) {
        pdngen.makeInstanceGrid(
            core, "Macro", kGround, inst, {0, 0, 0, 0}, true, true, {}, false);
      }
    }
    for (Grid* grid : pdngen.findGrid("Macro")) {
      pdngen.makeConnect(grid, met4, met5, 0, 0, {}, {}, 0, 0, {}, {}, {}, "");
    }
    pdngen.checkSetup();
    pdngen.buildGrids(true);
    pdngen.writeToDb(true);
    return boxes(block_);
  }

  odb::dbTech* tech_ = nullptr;
  odb::dbLib* cells_ = nullptr;
  odb::dbLib* pll_lib_ = nullptr;
  odb::dbBlock* block_ = nullptr;
  odb::Rect core_;
  odb::dbNet* vdd_ = nullptr;
  odb::dbNet* vss_ = nullptr;
};

// A met4 strap that crosses the PLL is cut at its obstructions, which leaves
// a segment standing in the PLL's open middle. That segment lies wholly
// inside a fixed instance on a layer the instance draws, so it is removed;
// the strap above the PLL stays. (Below it there is only the few microns of
// core between the PLL and the core edge.)
TEST_F(PllTest, RemovesTheCoreStrapLeftInsideAFixedMacro)
{
  odb::dbInst* pll = placePll(odb::dbPlacementStatus::FIRM);
  const std::vector<Box> met4 = onLayer(buildGrid(), "met4");
  const odb::Rect outline = pll->getBBox()->getBox();

  // the strap columns over the PLL, found where they run above it
  std::vector<std::pair<int, int>> columns;
  for (const Box& b : met4) {
    const odb::Rect r = rectOf(b);
    if (r.yMin() >= outline.yMax() && r.xMin() > outline.xMin()
        && r.xMax() < outline.xMax()) {
      columns.emplace_back(r.xMin(), r.xMax());
    }
  }
  std::sort(columns.begin(), columns.end());
  columns.erase(std::unique(columns.begin(), columns.end()), columns.end());
  ASSERT_FALSE(columns.empty()) << "no met4 strap runs above the PLL";

  for (const auto& [x0, x1] : columns) {
    for (const Box& b : met4) {
      const odb::Rect r = rectOf(b);
      if (r.xMin() == x0 && r.xMax() == x1) {
        EXPECT_FALSE(outline.contains(r)) << "segment of the strap at x " << x0
                                          << " left inside the PLL: " << r;
      }
    }
  }
}

// Fixed standard cells are fixed instances too, and a placed netlist brings
// hundreds of thousands of them. None of these fills can hold a strap, so
// the grid must come out exactly as it does without them.
TEST_F(PllTest, ManyFixedCellsThatHoldNoShapeChangeNothing)
{
  placePll(odb::dbPlacementStatus::FIRM);
  const std::vector<Box> without = buildGrid();

  // the same design again, now with the fills
  for (odb::dbNet* net : {vdd_, vss_}) {
    for (odb::dbSWire* swire : net->getSWires()) {
      odb::dbSWire::destroy(swire);
    }
  }
  placeFill(50000, false);
  const std::vector<Box> with = buildGrid();
  EXPECT_EQ(without, with);
}

// The order the fixed instances were created in decides nothing.
TEST_F(PllTest, FixedInstanceOrderDecidesNothing)
{
  placeFill(20000, true);
  placePll(odb::dbPlacementStatus::FIRM);
  const std::vector<Box> reversed = buildGrid();

  for (odb::dbInst* inst : block_->getInsts()) {
    if (!inst->getMaster()->isBlock()) {
      odb::dbInst::destroy(inst);
    }
  }
  for (odb::dbNet* net : {vdd_, vss_}) {
    for (odb::dbSWire* swire : net->getSWires()) {
      odb::dbSWire::destroy(swire);
    }
  }
  placeFill(20000, false);
  const std::vector<Box> forward = buildGrid();
  EXPECT_EQ(reversed, forward);
}

// Nangate45 with the L-shaped macro of polygon_macro_grid.tcl in the upper
// right corner of the core, so the notch -- the upper right of the L's box --
// is closed by the core edge: the followpins of the notch rows run from the
// L's inner edge to the core edge and so lie wholly inside the macro's box,
// but outside its outline, which is the OVERLAP polygon.
class LShapedMacroTest : public CleanupShapesTest
{
 protected:
  void SetUp() override
  {
    cells_
        = loadTechAndLib("ng45", "_main/src/pdn/test/Nangate45/Nangate45.lef");
    ASSERT_NE(cells_, nullptr);
    tech_ = cells_->getTech();
    addLef(cells_, "_main/src/pdn/test/nangate_polygon/polygon_macro.lef");
    macro_lib_ = cells_;
    odb::dbChip* chip = odb::dbChip::create(db_.get(), tech_);
    block_ = odb::dbBlock::create(chip, "top");
    block_->setDefUnits(tech_->getLefUnits());
    const int um = tech_->getDbUnitsPerMicron();
    block_->setDieArea(odb::Rect(0, 0, 200 * um, 200 * um));
    odb::dbSite* site = cells_->findSite("FreePDK45_38x28_10R_NP_162NW_34O");
    core_ = odb::Rect(19 * site->getWidth(),
                      10 * site->getHeight(),
                      19 * site->getWidth() + 900 * site->getWidth(),
                      10 * site->getHeight() + 120 * site->getHeight());
    makeRows(block_, site, core_);
    block_->setCoreArea(core_);
    vdd_ = odb::dbNet::create(block_, "VDD");
    vdd_->setSpecial();
    vdd_->setSigType(odb::dbSigType::POWER);
    vss_ = odb::dbNet::create(block_, "VSS");
    vss_->setSpecial();
    vss_->setSigType(odb::dbSigType::GROUND);
    macro_ = odb::dbInst::create(
        block_, macro_lib_->findMaster("polygon_macro_L"), "macro_L");
    odb::dbMaster* master = macro_->getMaster();
    macro_->setLocation(core_.xMax() - master->getWidth(),
                        core_.yMax() - master->getHeight());
    macro_->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  }

  // Followpins on metal1 and straps on metal4, as in the Tcl test.
  std::vector<Box> buildGrid()
  {
    block_->addGlobalConnect(nullptr, ".*", "^VDD$", vdd_, true);
    block_->addGlobalConnect(nullptr, ".*", "^VSS$", vss_, true);
    PdnGen pdngen(db_.get(), &logger_);
    pdngen.setCoreDomain(vdd_, nullptr, vss_, {});
    VoltageDomain* core = pdngen.findDomain("Core");
    pdngen.makeCoreGrid(
        core, "Core", kGround, {}, {}, nullptr, nullptr, "STAR", {});
    odb::dbTechLayer* metal1 = tech_->findLayer("metal1");
    odb::dbTechLayer* metal4 = tech_->findLayer("metal4");
    const int um = tech_->getDbUnitsPerMicron();
    for (Grid* grid : pdngen.findGrid("Core")) {
      // width 0: the followpins take the width of the cells' rails
      pdngen.makeFollowpin(grid, metal1, 0, kCore);
      pdngen.makeStrap(grid,
                       metal4,
                       480 * um / 1000,
                       0,
                       20 * um,
                       2 * um,
                       0,
                       false,
                       kGrid,
                       kCore,
                       {},
                       false);
      pdngen.makeConnect(
          grid, metal1, metal4, 0, 0, {}, {}, 0, 0, {}, {}, {}, "");
    }
    pdngen.checkSetup();
    pdngen.buildGrids(true);
    pdngen.writeToDb(true);
    return boxes(block_);
  }

  odb::dbTech* tech_ = nullptr;
  odb::dbLib* cells_ = nullptr;
  odb::dbLib* macro_lib_ = nullptr;
  odb::dbBlock* block_ = nullptr;
  odb::dbInst* macro_ = nullptr;
  odb::Rect core_;
  odb::dbNet* vdd_ = nullptr;
  odb::dbNet* vss_ = nullptr;
};

// The notch of the L is inside the macro's bounding box but outside its
// outline: ordinary core area over rows, so its followpins are kept, even
// though each lies wholly inside the box.
TEST_F(LShapedMacroTest, KeepsFollowpinsInTheNotch)
{
  const std::vector<Box> all = buildGrid();
  const odb::Rect bbox = macro_->getBBox()->getBox();
  // polygon_macro_L: 57 x 42 um, the upper right quarter-by-half cut away
  const odb::Rect notch(bbox.xMin() + bbox.dx() / 2,
                        bbox.yMin() + bbox.dy() / 2,
                        bbox.xMax(),
                        bbox.yMax());
  int in_notch = 0;
  for (const Box& b : onLayer(all, "metal1")) {
    const odb::Rect r = rectOf(b);
    if (notch.contains(r)) {
      in_notch++;
    }
  }
  EXPECT_GT(in_notch, 0) << "no followpin left in the notch at " << notch;
}

}  // namespace
}  // namespace pdn
