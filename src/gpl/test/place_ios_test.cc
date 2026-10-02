// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <functional>
#include <initializer_list>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "gpl/Replace.h"
#include "gtest/gtest.h"
#include "ifp/InitFloorplan.hh"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/defin.h"
#include "odb/geom.h"
#include "odb/isotropy.h"
#include "ppl/IOPlacer.h"
#include "src/gpl/src/nesterovBase.h"
#include "tst/fixture.h"
#include "utl/Logger.h"

namespace gpl {
namespace {

using Pins = std::vector<std::pair<float, size_t>>;

Pins pinsAt(const std::vector<float>& slots)
{
  Pins pins;
  for (size_t i = 0; i < slots.size(); ++i) {
    pins.emplace_back(slots[i], i);
  }
  return pins;
}

TEST(IoSlotFit, SlotIndexAtInterpolatesAndClamps)
{
  const std::vector<float> slots = {0, 10, 20, 40};
  EXPECT_FLOAT_EQ(slotIndexAt(slots, -5), 0);
  EXPECT_FLOAT_EQ(slotIndexAt(slots, 50), 3);
  EXPECT_FLOAT_EQ(slotIndexAt(slots, 10), 1);
  EXPECT_FLOAT_EQ(slotIndexAt(slots, 30), 2.5);
}

TEST(IoSlotFit, SlotIndexAtTakesTheLastOfEqualSlots)
{
  // Two layers can put a slot at the same position.
  const std::vector<float> slots = {0, 10, 10, 20};
  EXPECT_FLOAT_EQ(slotIndexAt(slots, 10), 2);
}

TEST(IoSlotFit, SlotCoordinateAtInvertsSlotIndexAt)
{
  const std::vector<float> slots = {0, 10, 20, 40};
  for (const float q : {0.0f, 5.0f, 10.0f, 30.0f, 40.0f}) {
    EXPECT_FLOAT_EQ(slotCoordinateAt(slots, slotIndexAt(slots, q)), q);
  }
  EXPECT_FLOAT_EQ(slotCoordinateAt(slots, -1), 0);
  EXPECT_FLOAT_EQ(slotCoordinateAt(slots, 7), 40);
}

TEST(IoSlotFit, PoolAdjacentViolatorsAveragesAndClamps)
{
  std::vector<float> z = {3, 1, 2};
  poolAdjacentViolators(z, 0, 10);
  EXPECT_EQ(z, (std::vector<float>{2, 2, 2}));

  z = {5};
  poolAdjacentViolators(z, 0, 3);
  EXPECT_EQ(z, std::vector<float>{3});
}

TEST(IoSlotFit, FitSpacingSpreadsAStackAroundItsCenter)
{
  EXPECT_EQ(fitSpacing(pinsAt({5, 5, 5, 9}), 0, 20),
            (std::vector<float>{4, 5, 6, 9}));
}

TEST(IoSlotFit, FitSpacingKeepsSpacedPins)
{
  EXPECT_EQ(fitSpacing(pinsAt({1, 3, 7}), 0, 20),
            (std::vector<float>{1, 3, 7}));
}

TEST(IoSlotFit, FitSpacingStaysInsideTheSlots)
{
  EXPECT_EQ(fitSpacing(pinsAt({9, 9, 9}), 0, 10),
            (std::vector<float>{7, 8, 9}));
}

TEST(IoSlotFit, FitSpacingSharesTooFewSlotsEvenly)
{
  const std::vector<float> fit = fitSpacing(pinsAt({0, 0, 0, 0}), 0, 3);
  ASSERT_EQ(fit.size(), 4);
  for (size_t r = 0; r < fit.size(); ++r) {
    EXPECT_NEAR(fit[r], r * 2.0f / 3, 1e-5);
  }
}

TEST(IoSlotFit, UnrollKeepsAClusterAcrossTheSeamTogether)
{
  // Pins 0 and 1 sit just after the end of a 100-slot ring, 98 and 99 just
  // before it; the widest gap is between 1 and 98.
  Pins pins = pinsAt({0, 1, 98, 99});
  const float start = unrollRingAtWidestGap(pins, 100);
  EXPECT_EQ(pins, (Pins{{98, 2}, {99, 3}, {100, 0}, {101, 1}}));
  EXPECT_FLOAT_EQ(start, 49);
}

TEST(IoSlotFit, UnrollLeavesRoomOnBothSidesOfAStack)
{
  // Five pins want one slot; the rest of the ring is empty.
  Pins pins = pinsAt({0, 0, 0, 0, 0});
  const float start = unrollRingAtWidestGap(pins, 100);
  EXPECT_FLOAT_EQ(start, -50);
  EXPECT_EQ(fitSpacing(pins, start, 100),
            (std::vector<float>{-2, -1, 0, 1, 2}));
}

// global_placement -place_ios on simple01, driven through Replace and the pin
// placer the way the Tcl commands do.
class PlaceIos : public tst::Fixture
{
 protected:
  void SetUp() override
  {
    odb::dbLib* lib
        = loadTechAndLib("ng45", "ng45", "_main/src/gpl/test/nangate45.lef");
    ASSERT_NE(lib, nullptr);
    odb::dbChip* chip = odb::dbChip::create(db_.get(), lib->getTech());
    std::vector<odb::dbLib*> libs{lib};
    odb::defin reader(db_.get(), &logger_);
    const std::string def = getFilePath("_main/src/gpl/test/simple01.def");
    ASSERT_TRUE(reader.readChip(libs, def.c_str(), chip));
    block_ = chip->getBlock();
    die_ = block_->getDieArea();
  }

  odb::dbTechLayer* layer(const char* name) const
  {
    return db_->getTech()->findLayer(name);
  }

  odb::dbBTerm* port(const std::string& name) const
  {
    odb::dbBTerm* bterm = block_->findBTerm(name.c_str());
    EXPECT_NE(bterm, nullptr) << name;
    return bterm;
  }

  std::vector<odb::dbBTerm*> ports(const char* bus, int from, int to) const
  {
    std::vector<odb::dbBTerm*> bterms;
    for (int i = from; i < to; ++i) {
      bterms.push_back(port(std::string(bus) + "[" + std::to_string(i) + "]"));
    }
    return bterms;
  }

  // Every port but keep loses its shape.
  void unplacePorts(const odb::dbBTerm* keep = nullptr)
  {
    for (odb::dbBTerm* bterm : block_->getBTerms()) {
      if (bterm == keep) {
        continue;
      }
      const auto bpins = bterm->getBPins();
      for (odb::dbBPin* bpin :
           std::vector<odb::dbBPin*>(bpins.begin(), bpins.end())) {
        odb::dbBPin::destroy(bpin);
      }
    }
  }

  // simple01 places its ports FIXED, which -place_ios keeps as anchors.
  void unfixPorts()
  {
    for (odb::dbBTerm* bterm : block_->getBTerms()) {
      for (odb::dbBPin* bpin : bterm->getBPins()) {
        bpin->setPlacementStatus(odb::dbPlacementStatus::PLACED);
      }
    }
  }

  // set_place_config -io_pin_hor_layers -io_pin_ver_layers
  void setPinLayers(std::initializer_list<const char*> hor,
                    std::initializer_list<const char*> ver)
  {
    for (const auto& [name, layers] :
         {std::pair{"place_config_io_pin_hor_layers", hor},
          std::pair{"place_config_io_pin_ver_layers", ver}}) {
      std::string names;
      for (const char* layer_name : layers) {
        names += std::string(names.empty() ? "" : " ") + layer_name;
      }
      if (!names.empty()) {
        odb::dbStringProperty::create(block_, name, names.c_str());
      }
    }
  }

  void excludeEdge(const odb::Direction2D& edge, int begin, int end)
  {
    block_->addBlockedRegionForPins(
        block_->findConstraintRegion(edge, begin, end));
  }

  void place(PlaceOptions options = {})
  {
    options.placeIosMode = true;
    options.initDensityPenaltyFactor = 0.01;
    replace_.doPlace(1, options);
    replace_.reset();
  }

  void expectError(const std::function<void()>& run, const char* id)
  {
    try {
      run();
      ADD_FAILURE() << "expected " << id;
    } catch (const std::runtime_error& error) {
      EXPECT_STREQ(error.what(), id);
    }
  }

  // Pins the final legalization moved off the solved positions.
  int placeAndCountMoved()
  {
    logger_.setDebugLevel(utl::PPL, "displacement", 1);
    logger_.redirectStringBegin();
    place();
    const std::string log = logger_.redirectStringEnd();
    const size_t at = log.find("Moved ");
    EXPECT_NE(at, std::string::npos) << log;
    int moved = -1;
    std::sscanf(log.c_str() + at, "Moved %d", &moved);
    return moved;
  }

  odb::dbBox* pinBox(odb::dbBTerm* bterm) const
  {
    const auto bpins = bterm->getBPins();
    EXPECT_EQ(bpins.size(), 1) << bterm->getName();
    odb::dbBPin* bpin = *bpins.begin();
    EXPECT_TRUE(bpin->getPlacementStatus().isPlaced()) << bterm->getName();
    return *bpin->getBoxes().begin();
  }

  // The die edge the pin box runs inward from, or 0 if off the perimeter.
  char edgeOf(const odb::Rect& box) const
  {
    if (box.xMin() <= die_.xMin()) {
      return 'L';
    }
    if (box.xMax() >= die_.xMax()) {
      return 'R';
    }
    if (box.yMin() <= die_.yMin()) {
      return 'B';
    }
    if (box.yMax() >= die_.yMax()) {
      return 'T';
    }
    return 0;
  }

  static int alongEdge(const odb::Rect& box, char edge)
  {
    return edge == 'L' || edge == 'R' ? box.yCenter() : box.xCenter();
  }

  void expectOnTrack(odb::dbBTerm* bterm) const
  {
    odb::dbBox* box = pinBox(bterm);
    odb::dbTechLayer* pin_layer = box->getTechLayer();
    odb::dbTrackGrid* grid = block_->findTrackGrid(pin_layer);
    ASSERT_NE(grid, nullptr) << pin_layer->getName();
    const bool horizontal
        = pin_layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL;
    const odb::Rect rect = box->getBox();
    const int coord = horizontal ? rect.yCenter() : rect.xCenter();
    const std::vector<int>& tracks
        = horizontal ? grid->getGridY() : grid->getGridX();
    EXPECT_TRUE(std::ranges::binary_search(tracks, coord))
        << bterm->getName() << " at " << coord << " is off the "
        << pin_layer->getName() << " tracks";
  }

  // The boxes run inward from opposite edges, so the pair is checked for
  // symmetry about the die center.
  void expectMirrored(odb::dbBTerm* master, odb::dbBTerm* follower) const
  {
    const odb::Rect m = pinBox(master)->getBox();
    const odb::Rect f = pinBox(follower)->getBox();
    const bool across_x
        = m.yCenter() == f.yCenter()
          && m.xCenter() + f.xCenter() == die_.xMin() + die_.xMax();
    const bool across_y
        = m.xCenter() == f.xCenter()
          && m.yCenter() + f.yCenter() == die_.yMin() + die_.yMax();
    EXPECT_TRUE(across_x || across_y)
        << follower->getName() << " is not the mirror of " << master->getName();
  }

  odb::dbBlock* block_ = nullptr;
  odb::Rect die_;
  ppl::IOPlacer pin_placer_{db_.get(), &logger_};
  Replace replace_{db_.get(),
                   sta_.get(),
                   nullptr,
                   nullptr,
                   &pin_placer_,
                   &logger_};
};

TEST_F(PlaceIos, UnplacedPinsLandOnTracksOffTheExcludedEdge)
{
  unplacePorts();
  const auto masters = ports("req_msg", 0, 2);
  const auto followers = ports("resp_msg", 0, 2);
  for (size_t i = 0; i < masters.size(); ++i) {
    masters[i]->setMirroredBTerm(followers[i]);
  }
  excludeEdge(odb::north, die_.xMin(), die_.xMax());
  setPinLayers({"metal5"}, {"metal6"});
  place();

  for (odb::dbBTerm* bterm : block_->getBTerms()) {
    const char edge = edgeOf(pinBox(bterm)->getBox());
    EXPECT_NE(edge, 0) << bterm->getName() << " is off the die perimeter";
    EXPECT_NE(edge, 'T') << bterm->getName() << " is on the excluded edge";
    expectOnTrack(bterm);
  }
  for (size_t i = 0; i < masters.size(); ++i) {
    expectMirrored(masters[i], followers[i]);
  }
}

TEST_F(PlaceIos, PinsLandOnTracksAroundEveryEdge)
{
  unplacePorts();
  setPinLayers({"metal5"}, {"metal6"});
  place();

  for (odb::dbBTerm* bterm : block_->getBTerms()) {
    expectOnTrack(bterm);
  }
}

TEST_F(PlaceIos, WholePerimeterExcludedIsRejected)
{
  unplacePorts();
  excludeEdge(odb::north, die_.xMin(), die_.xMax());
  excludeEdge(odb::south, die_.xMin(), die_.xMax());
  excludeEdge(odb::west, die_.yMin(), die_.yMax());
  excludeEdge(odb::east, die_.yMin(), die_.yMax());
  setPinLayers({"metal5"}, {"metal6"});
  expectError([this] { place(); }, "PPL-0024");
}

TEST_F(PlaceIos, PinLayersMustBeSet)
{
  expectError([this] { place(); }, "GPL-0174");

  // place_pins -hor_layers/-ver_layers last for their own run only.
  pin_placer_.addHorLayer(layer("metal5"));
  pin_placer_.addVerLayer(layer("metal6"));
  pin_placer_.runHungarianMatching(false);
  expectError([this] { place(); }, "GPL-0174");

  setPinLayers({"metal5"}, {});
  expectError([this] { place(); }, "GPL-0174");
}

TEST_F(PlaceIos, SolveSkipsTheSlotsThePinPlacerBlocks)
{
  unplacePorts();
  odb::dbTechLayer* metal6 = layer("metal6");
  for (int x = 5000; x < die_.xMax(); x += 4000) {
    odb::dbObstruction::create(
        block_, metal6, x, die_.yMin(), x + 1200, die_.yMin() + 800);
    odb::dbObstruction::create(
        block_, metal6, x, die_.yMax() - 800, x + 1200, die_.yMax());
  }
  setPinLayers({"metal5"}, {"metal6"});

  EXPECT_EQ(placeAndCountMoved(), 0);
}

TEST_F(PlaceIos, PinsUseTheSlotsOfEveryLayer)
{
  unplacePorts();
  const int mid_x = die_.xCenter();
  const int mid_y = die_.yCenter();
  odb::dbTechLayer* metal3 = layer("metal3");
  odb::dbTechLayer* metal4 = layer("metal4");
  odb::dbObstruction::create(
      block_, metal4, die_.xMin(), die_.yMin(), mid_x, die_.yMin() + 800);
  odb::dbObstruction::create(
      block_, metal4, die_.xMin(), die_.yMax() - 800, mid_x, die_.yMax());
  odb::dbObstruction::create(
      block_, metal3, die_.xMin(), die_.yMin(), die_.xMin() + 800, mid_y);
  odb::dbObstruction::create(
      block_, metal3, die_.xMax() - 800, die_.yMin(), die_.xMax(), mid_y);
  setPinLayers({"metal3", "metal5"}, {"metal4", "metal6"});

  // The pin placer's sections can put two pins at one position on different
  // layers into the same section, which moves one of them.
  EXPECT_LE(placeAndCountMoved(), 2);

  std::map<std::string, int> per_layer;
  for (odb::dbBTerm* bterm : block_->getBTerms()) {
    expectOnTrack(bterm);
    ++per_layer[pinBox(bterm)->getTechLayer()->getName()];
  }
  for (const char* name : {"metal3", "metal4", "metal5", "metal6"}) {
    EXPECT_GT(per_layer[name], 0) << name;
  }
}

// A follower is a reflection of its master, so it lands among pins the
// spacing never saw together with it.
TEST_F(PlaceIos, MirroredFollowersKeepTheSlotPitch)
{
  unplacePorts();
  const auto masters = ports("req_msg", 0, 8);
  const auto followers = ports("resp_msg", 0, 8);
  for (size_t i = 0; i < masters.size(); ++i) {
    masters[i]->setMirroredBTerm(followers[i]);
  }
  block_->addBTermsToConstraint(
      ports("req_msg", 10, 22),
      block_->findConstraintRegion(odb::east, die_.yMin(), die_.yMax()));
  setPinLayers({"metal5"}, {"metal6"});
  place();

  // metal5/metal6 both step 560, and the default spacing is two tracks.
  constexpr int kPitch = 1120;
  std::map<char, std::vector<int>> along;
  for (odb::dbBTerm* bterm : block_->getBTerms()) {
    const odb::Rect box = pinBox(bterm)->getBox();
    const char edge = edgeOf(box);
    ASSERT_NE(edge, 0) << bterm->getName() << " is off the die perimeter";
    along[edge].push_back(alongEdge(box, edge));
  }
  for (auto& [edge, coords] : along) {
    std::ranges::sort(coords);
    for (size_t i = 1; i < coords.size(); ++i) {
      EXPECT_GE(coords[i] - coords[i - 1], kPitch) << "on edge " << edge;
    }
  }
  for (size_t i = 0; i < masters.size(); ++i) {
    expectMirrored(masters[i], followers[i]);
  }
}

// The constraints the solve does not model itself must hold after the pin
// placer legalizes it. The pin placer cannot yet take mirrored pairs, a group
// and an excluded stretch all at once, so the parameter picks mirror or
// exclude.
class PlaceIosConstraints : public PlaceIos,
                            public testing::WithParamInterface<bool>
{
};

TEST_P(PlaceIosConstraints, HoldAfterLegalization)
{
  const bool with_exclude = GetParam();
  odb::dbBTerm* fixed = port("clk");
  const odb::Rect fixed_at = pinBox(fixed)->getBox();
  (*fixed->getBPins().begin())
      ->setPlacementStatus(odb::dbPlacementStatus::FIRM);
  unplacePorts(fixed);

  std::vector<odb::dbBTerm*> masters;
  std::vector<odb::dbBTerm*> followers;
  if (!with_exclude) {
    masters = ports("req_msg", 0, 4);
    followers = ports("resp_msg", 0, 4);
    for (size_t i = 0; i < masters.size(); ++i) {
      masters[i]->setMirroredBTerm(followers[i]);
    }
  }
  const auto group = ports("req_msg", 20, 26);
  block_->addBTermGroup(group, false);
  const int excluded_to = 15 * block_->getDbUnitsPerMicron();
  if (with_exclude) {
    excludeEdge(odb::west, 0, excluded_to);
  }
  setPinLayers({"metal5"}, {"metal6"});
  place();

  EXPECT_EQ(fixed->getBBox(), fixed_at);

  std::vector<odb::dbBox*> boxes;
  for (odb::dbBTerm* bterm : block_->getBTerms()) {
    odb::dbBox* box = pinBox(bterm);
    const odb::Rect rect = box->getBox();
    if (with_exclude && bterm != fixed && edgeOf(rect) == 'L') {
      EXPECT_GE(rect.yCenter(), excluded_to)
          << bterm->getName() << " is in the excluded stretch";
    }
    for (odb::dbBox* other : boxes) {
      const odb::Rect o = other->getBox();
      EXPECT_FALSE(other->getTechLayer() == box->getTechLayer()
                   && o.xMin() < rect.xMax() && rect.xMin() < o.xMax()
                   && o.yMin() < rect.yMax() && rect.yMin() < o.yMax())
          << bterm->getName() << " overlaps another pin";
    }
    boxes.push_back(box);
  }

  for (size_t i = 0; i < masters.size(); ++i) {
    expectMirrored(masters[i], followers[i]);
  }

  // The group shares one edge and no other pin sits inside its span.
  const char edge = edgeOf(pinBox(group.front())->getBox());
  std::vector<int> coords;
  for (odb::dbBTerm* bterm : group) {
    const odb::Rect box = pinBox(bterm)->getBox();
    EXPECT_EQ(edgeOf(box), edge) << bterm->getName() << " left the group edge";
    coords.push_back(alongEdge(box, edge));
  }
  const auto [lo, hi] = std::ranges::minmax(coords);
  for (odb::dbBTerm* bterm : block_->getBTerms()) {
    if (std::ranges::find(group, bterm) != group.end()) {
      continue;
    }
    const odb::Rect box = pinBox(bterm)->getBox();
    if (edgeOf(box) == edge) {
      const int c = alongEdge(box, edge);
      EXPECT_FALSE(c > lo && c < hi)
          << bterm->getName() << " sits inside the pin group";
    }
  }
}

INSTANTIATE_TEST_SUITE_P(MirrorOrExclude,
                         PlaceIosConstraints,
                         testing::Bool(),
                         [](const testing::TestParamInfo<bool>& info) {
                           return info.param ? "Exclude" : "Mirror";
                         });

// Pins constrained to an up: region move in 2D on the pin shape pattern
// layer; the rest stay on the die perimeter.
TEST_F(PlaceIos, TopLayerPinsStayInTheirRegion)
{
  unfixPorts();
  odb::dbTechLayer* metal10 = layer("metal10");
  // define_pin_shape_pattern -x_step 1.6 -y_step 1.6 -region {5 5 25 25}
  // -size {1.6 2.5} in a 2000 DBU/um design.
  block_->setBTermTopLayerGrid({.layer = metal10,
                                .x_step = 3200,
                                .y_step = 3200,
                                .region = odb::Rect(10000, 10000, 50000, 50000),
                                .pin_width = 3200,
                                .pin_height = 5000,
                                .keepout = metal10->getSpacing(5000)});
  const odb::Rect region(20000, 20000, 40000, 40000);
  std::vector<odb::dbBTerm*> constrained;
  for (const char* name : {"clk", "reset", "req_val", "resp_rdy"}) {
    constrained.push_back(port(name));
  }
  block_->addBTermsToConstraint(constrained, region);
  setPinLayers({"metal1"}, {"metal2"});

  PlaceOptions options;
  options.initialPlaceMaxIter = 0;
  place(options);

  for (odb::dbBTerm* bterm : block_->getBTerms()) {
    odb::dbBox* box = pinBox(bterm);
    const odb::Rect rect = box->getBox();
    if (std::ranges::find(constrained, bterm) != constrained.end()) {
      EXPECT_EQ(box->getTechLayer(), metal10) << bterm->getName();
      EXPECT_TRUE(region.intersects(odb::Point(rect.xCenter(), rect.yCenter())))
          << bterm->getName() << " left its up: region";
    } else {
      EXPECT_NE(edgeOf(rect), 0)
          << bterm->getName() << " is off the die perimeter";
    }
  }
}

// -place_ios on nearly empty designs: a handful of tie cells, each driving one
// output port, plus many unconnected ports, in cores of 4 to 14 um. Once the
// cells settle only the IO pins move, which used to round the step length to
// zero or below (GPL-0305), depending on the core size.
class PlaceIosFewCells : public PlaceIos,
                         public testing::WithParamInterface<int>
{
 protected:
  void SetUp() override
  {
    odb::dbLib* lib = loadTechAndLib(
        "nangate45", "nangate45", "_main/test/Nangate45/Nangate45.lef");
    ASSERT_NE(lib, nullptr);
    odb::dbChip* chip = odb::dbChip::create(db_.get(), lib->getTech());
    block_ = odb::dbBlock::create(chip, "few_cells");
    block_->setDefUnits(lib->getTech()->getLefUnits());
    const int dbu = block_->getDbUnitsPerMicron();

    odb::dbMaster* tie = db_->findMaster("LOGIC0_X1");
    for (int i = 0; i < 8; ++i) {
      const std::string name = "out_" + std::to_string(i);
      odb::dbInst* inst = odb::dbInst::create(block_, tie, name.c_str());
      odb::dbNet* net = odb::dbNet::create(block_, name.c_str());
      inst->findITerm("Z")->connect(net);
      odb::dbBTerm::create(net, name.c_str())->setIoType(odb::dbIoType::OUTPUT);
    }
    for (int i = 0; i < 24; ++i) {
      const std::string name = "in_" + std::to_string(i);
      odb::dbNet* net = odb::dbNet::create(block_, name.c_str());
      odb::dbBTerm::create(net, name.c_str())->setIoType(odb::dbIoType::INPUT);
    }

    // Rows over a core_um square core with a 5 um margin to the die.
    const int core_um = GetParam();
    const int margin = 5 * dbu;
    block_->setDieArea(
        odb::Rect(0, 0, (core_um + 10) * dbu, (core_um + 10) * dbu));
    odb::dbSite* site = lib->findSite("FreePDK45_38x28_10R_NP_162NW_34O");
    const int num_sites = core_um * dbu / site->getWidth();
    const int num_rows = core_um * dbu / site->getHeight();
    for (int r = 0; r < num_rows; ++r) {
      odb::dbRow::create(block_,
                         ("row_" + std::to_string(r)).c_str(),
                         site,
                         margin,
                         margin + r * site->getHeight(),
                         r % 2 ? odb::dbOrientType::MX : odb::dbOrientType::R0,
                         odb::dbRowDir::HORIZONTAL,
                         num_sites,
                         site->getWidth());
    }
    block_->setCoreArea(block_->computeCoreArea());
    ifp::InitFloorplan(block_, &logger_, sta_->getDbNetwork()).makeTracks();
    die_ = block_->getDieArea();
  }
};

TEST_P(PlaceIosFewCells, CellsAndPinsEndPlaced)
{
  setPinLayers({"metal5"}, {"metal6"});
  PlaceOptions options;
  options.placeIosMode = true;
  replace_.doPlace(1, options);

  const odb::Rect core = block_->getCoreArea();
  for (odb::dbInst* inst : block_->getInsts()) {
    EXPECT_TRUE(inst->isPlaced()) << inst->getName();
    EXPECT_TRUE(core.contains(inst->getBBox()->getBox())) << inst->getName();
  }
  for (odb::dbBTerm* bterm : block_->getBTerms()) {
    EXPECT_NE(edgeOf(pinBox(bterm)->getBox()), 0)
        << bterm->getName() << " is off the die perimeter";
  }
}

INSTANTIATE_TEST_SUITE_P(CoreUm, PlaceIosFewCells, testing::Range(4, 15));

}  // namespace
}  // namespace gpl
