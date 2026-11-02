// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <cstddef>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "ppl/IOPlacer.h"
#include "tst/fixture.h"

namespace ppl {
namespace {

// The distances between neighbor slots of each layer, in tracks of the layer.
using Pitches = std::map<std::string, std::set<int>>;

// The slots of the pins on gcd with horizontal layers metal3 (0.14um tracks)
// and metal5 (0.28um), and vertical layers metal2 (0.19um) and metal4
// (0.28um).
class SlotGrid : public tst::Fixture
{
 protected:
  void SetUp() override
  {
    loadTechAndLib(
        "nangate45", "nangate45", "_main/test/Nangate45/Nangate45.lef");
    block_ = readDef("_main/src/ppl/test/gcd.def");
  }

  odb::dbTechLayer* layer(const char* name) const
  {
    return db_->getTech()->findLayer(name);
  }

  void addPinLayers()
  {
    pin_placer_.addHorLayer(layer("metal3"));
    pin_placer_.addHorLayer(layer("metal5"));
    pin_placer_.addVerLayer(layer("metal2"));
    pin_placer_.addVerLayer(layer("metal4"));
  }

  Pitches slotPitches()
  {
    addPinLayers();
    const std::vector<SlotPosition> grid = pin_placer_.buildSlotGrid();
    pin_placer_.clear();

    Pitches pitches;
    for (const char* name : {"metal2", "metal3", "metal4", "metal5"}) {
      odb::dbTechLayer* tech_layer = layer(name);
      const bool vertical
          = tech_layer->getDirection() == odb::dbTechLayerDir::VERTICAL;
      int origin, count, step;
      odb::dbTrackGrid* tracks = block_->findTrackGrid(tech_layer);
      if (vertical) {
        tracks->getGridPatternX(0, origin, count, step);
      } else {
        tracks->getGridPatternY(0, origin, count, step);
      }

      std::map<Edge, std::vector<int>> coords;
      for (const SlotPosition& slot : grid) {
        if (slot.layer == tech_layer->getRoutingLevel()) {
          coords[slot.edge].push_back(vertical ? slot.pos.x() : slot.pos.y());
        }
      }
      for (auto& [edge, edge_coords] : coords) {
        std::ranges::sort(edge_coords);
        for (size_t i = 1; i < edge_coords.size(); ++i) {
          const int distance = edge_coords[i] - edge_coords[i - 1];
          EXPECT_EQ(distance % step, 0) << name;
          pitches[name].insert(distance / step);
        }
      }
    }
    return pitches;
  }

  void setLayerMinDistance(const char* name, int min_dist, bool in_tracks)
  {
    pin_placer_.getParameters()->setLayerMinDistance(
        layer(name)->getRoutingLevel(), min_dist, in_tracks);
  }

  // set_place_config -io_pin_layer_min_distance layer_min_dists
  //   -io_pin_min_distance_in_tracks
  void saveLayerMinDistance(const char* layer_min_dists)
  {
    odb::dbStringProperty::create(
        block_, "place_config_io_pin_layer_min_distance", layer_min_dists);
    odb::dbBoolProperty::create(
        block_, "place_config_io_pin_layer_min_distance_in_tracks", true);
  }

  odb::dbBlock* block_ = nullptr;
  IOPlacer pin_placer_{db_.get(), &logger_};
};

TEST_F(SlotGrid, LayerMinDistanceOverridesTheGlobalOne)
{
  Parameters* parms = pin_placer_.getParameters();
  parms->setMinDistance(1);
  parms->setMinDistanceInTracks(true);
  setLayerMinDistance("metal2", 2, true);
  setLayerMinDistance("metal3", 2, true);

  EXPECT_EQ(
      slotPitches(),
      (Pitches{
          {"metal2", {2}}, {"metal3", {2}}, {"metal4", {1}}, {"metal5", {1}}}));
}

// 0.14um alone is one track on every layer.
TEST_F(SlotGrid, LayerMinDistanceInMicrons)
{
  Parameters* parms = pin_placer_.getParameters();
  parms->setMinDistance(block_->micronsToDbu(0.14));
  setLayerMinDistance("metal2", block_->micronsToDbu(0.38), false);
  setLayerMinDistance("metal3", block_->micronsToDbu(0.28), false);

  EXPECT_EQ(
      slotPitches(),
      (Pitches{
          {"metal2", {2}}, {"metal3", {2}}, {"metal4", {1}}, {"metal5", {1}}}));
}

// Without a global min distance, the other layers use the default of two
// tracks.
TEST_F(SlotGrid, LayersWithoutALayerMinDistanceKeepTheDefault)
{
  setLayerMinDistance("metal3", 1, true);

  EXPECT_EQ(
      slotPitches(),
      (Pitches{
          {"metal2", {2}}, {"metal3", {1}}, {"metal4", {2}}, {"metal5", {2}}}));
}

TEST_F(SlotGrid, ReadsTheLayerMinDistanceOfThePlaceConfig)
{
  saveLayerMinDistance("metal3 1 metal5 1");

  const PinPlacementSettings settings = pin_placer_.getSettings();
  EXPECT_EQ(settings.layer_min_distance,
            (std::map<int, int>{{layer("metal3")->getRoutingLevel(), 1},
                                {layer("metal5")->getRoutingLevel(), 1}}));
  EXPECT_TRUE(settings.layer_min_distance_in_tracks);
  EXPECT_EQ(
      slotPitches(),
      (Pitches{
          {"metal2", {2}}, {"metal3", {1}}, {"metal4", {2}}, {"metal5", {1}}}));
}

// Like every place_pins switch, -layer_min_distance replaces the whole saved
// list for its run, and the saved list applies when it is not given.
TEST_F(SlotGrid, RunLayerMinDistanceReplacesThePlaceConfigOne)
{
  saveLayerMinDistance("metal3 1 metal5 1");
  setLayerMinDistance("metal3", 3, true);

  EXPECT_EQ(
      slotPitches(),
      (Pitches{
          {"metal2", {2}}, {"metal3", {3}}, {"metal4", {2}}, {"metal5", {2}}}));
}

TEST_F(SlotGrid, RunMinDistanceKeepsThePlaceConfigLayerMinDistance)
{
  saveLayerMinDistance("metal3 1");
  Parameters* parms = pin_placer_.getParameters();
  parms->setMinDistance(3);
  parms->setMinDistanceInTracks(true);

  EXPECT_EQ(
      slotPitches(),
      (Pitches{
          {"metal2", {3}}, {"metal3", {1}}, {"metal4", {3}}, {"metal5", {3}}}));
}

// Like a finished run, a failed one leaves none of its settings or slots.
TEST_F(SlotGrid, AFailedRunLeavesNothingForTheNextRun)
{
  addPinLayers();
  for (const char* name : {"metal2", "metal3", "metal4", "metal5"}) {
    setLayerMinDistance(name, 1000, true);
  }
  EXPECT_THROW(pin_placer_.runHungarianMatching(), std::runtime_error);
  EXPECT_TRUE(pin_placer_.getParameters()->getLayerMinDistances().empty());

  EXPECT_EQ(
      slotPitches(),
      (Pitches{
          {"metal2", {2}}, {"metal3", {2}}, {"metal4", {2}}, {"metal5", {2}}}));
  addPinLayers();
  EXPECT_NO_THROW(pin_placer_.runHungarianMatching());
}

// Building the grid merges in the saved settings, which must not stay behind
// for the caller's run when it fails.
TEST_F(SlotGrid, AFailedSlotGridLeavesTheCallerState)
{
  saveLayerMinDistance("metal2 1000 metal3 1000 metal4 1000 metal5 1000");
  Parameters* parms = pin_placer_.getParameters();
  parms->setMinDistance(5);
  parms->setMinDistanceInTracks(true);
  addPinLayers();
  EXPECT_THROW(pin_placer_.buildSlotGrid(), std::runtime_error);
  EXPECT_EQ(parms->getMinDistance(), 5);
  EXPECT_TRUE(parms->getLayerMinDistances().empty());
}

}  // namespace
}  // namespace ppl
