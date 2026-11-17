// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <string>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "sta/PowerClass.hh"
#include "tst/nangate45_fixture.h"
#include "utl/Logger.h"
#include "web/heatMap.h"

namespace web {
namespace {

// Gives every bin of the map the same value, so any bin left without one was
// cleared by the data source itself.
class UniformHeatMap : public HeatMapDataSource
{
 public:
  explicit UniformHeatMap(utl::Logger* logger)
      : HeatMapDataSource(logger, "Uniform", "Uniform")
  {
  }

 protected:
  bool populateMap() override
  {
    addToMap(getBounds(), 1.0);
    return true;
  }

  void combineMapData(bool /* base_has_value */,
                      double& base,
                      double new_data,
                      double /* data_area */,
                      double /* intersection_area */,
                      double /* rect_area */) override
  {
    base = new_data;
  }
};

class HeatMapTest : public tst::Nangate45Fixture
{
 protected:
  void SetUp() override { dbu_ = block_->getDbUnitsPerMicron(); }

  // Bins that hold a value, and how many of those lie inside `region`.
  struct BinCounts
  {
    int with_value = 0;
    int with_value_in_region = 0;
  };

  BinCounts countBins(const odb::Rect& region)
  {
    UniformHeatMap heat_map(getLogger());
    heat_map.setChip(chip_);
    heat_map.ensureMap();
    EXPECT_TRUE(heat_map.isPopulated());

    BinCounts counts;
    for (const auto& map_col : heat_map.getMap()) {
      for (const auto& map_pt : map_col) {
        if (map_pt == nullptr || !map_pt->has_value) {
          continue;
        }
        counts.with_value++;
        if (region.contains(map_pt->rect)) {
          counts.with_value_in_region++;
        }
      }
    }
    return counts;
  }

  int dbu_ = 0;
};

// 100um x 100um with the default 10um grid: a 10 x 10 map.
TEST_F(HeatMapTest, RectangularDieKeepsEveryBin)
{
  const int side = 100 * dbu_;
  block_->setDieArea(odb::Rect(0, 0, side, side));

  const BinCounts counts = countBins(odb::Rect(0, 0, side, side));
  EXPECT_EQ(counts.with_value, 100);
}

// An L-shaped die: the same 100um square without its upper right 50um
// quadrant.  The 25 bins in that quadrant are outside the die.
TEST_F(HeatMapTest, PolygonDieDropsBinsOutsideTheDie)
{
  const int side = 100 * dbu_;
  const int half = side / 2;
  block_->setDieArea(odb::Polygon(std::vector<odb::Point>{
      {0, 0}, {0, side}, {half, side}, {half, half}, {side, half}, {side, 0}}));

  const odb::Rect missing_quadrant(half, half, side, side);
  const BinCounts counts = countBins(missing_quadrant);
  EXPECT_EQ(counts.with_value, 75);
  EXPECT_EQ(counts.with_value_in_region, 0);
}

// One option of each kind, both feeding the value it maps, and a count of how
// often the map data is built.
class OptionHeatMap : public HeatMapDataSource
{
 public:
  explicit OptionHeatMap(utl::Logger* logger)
      : HeatMapDataSource(logger, "Options", "Options")
  {
    addBooleanSetting(
        "Doubled",
        "Doubled:",
        [this]() { return doubled_; },
        [this](bool value) { doubled_ = value; });
    addMultipleChoiceSetting(
        "Value",
        "Value:",
        []() { return std::vector<std::string>{"1", "3"}; },
        [this]() { return value_; },
        [this](const std::string& value) { value_ = value; });
  }

  int populate_calls = 0;

 protected:
  bool populateMap() override
  {
    ++populate_calls;
    const double value = value_ == "3" ? 3.0 : 1.0;
    addToMap(getBounds(), doubled_ ? 2 * value : value);
    return true;
  }

  void combineMapData(bool /* base_has_value */,
                      double& base,
                      double new_data,
                      double /* data_area */,
                      double /* intersection_area */,
                      double /* rect_area */) override
  {
    base = new_data;
  }

 private:
  bool doubled_ = false;
  std::string value_ = "1";
};

// An OptionHeatMap already built over a 100um x 100um die: a 10 x 10 map.
class HeatMapSettingsTest : public HeatMapTest
{
 protected:
  void SetUp() override
  {
    HeatMapTest::SetUp();
    block_->setDieArea(odb::Rect(0, 0, 100 * dbu_, 100 * dbu_));
    heat_map_.setChip(chip_);
    heat_map_.ensureMap();
    ASSERT_EQ(heat_map_.populate_calls, 1);
  }

  // Changes one setting the way the Web GUI and gui::set_heatmap do: all of
  // the current settings go back, with only this one changed.
  void change(const std::string& name, const Renderer::Setting& value)
  {
    Renderer::Settings settings = heat_map_.getSettings();
    settings[name] = value;
    heat_map_.setSettings(settings);
    heat_map_.ensureMap();
  }

  double binValue() const { return heat_map_.getMap()[0][0]->value; }

  OptionHeatMap heat_map_{getLogger()};
};

TEST_F(HeatMapSettingsTest, ChoiceOptionRebuildsMap)
{
  change("Value", std::string("3"));
  EXPECT_EQ(heat_map_.populate_calls, 2);
  EXPECT_DOUBLE_EQ(binValue(), 3.0);
}

TEST_F(HeatMapSettingsTest, BooleanOptionRebuildsMap)
{
  change("Doubled", true);
  EXPECT_EQ(heat_map_.populate_calls, 2);
  EXPECT_DOUBLE_EQ(binValue(), 2.0);
}

// Every change sends all the settings back, so the unchanged options and the
// settings that only recolor the map must not rebuild it.
TEST_F(HeatMapSettingsTest, UnchangedOptionsKeepMap)
{
  heat_map_.setSettings(heat_map_.getSettings());
  heat_map_.ensureMap();
  change("Alpha", 200);
  EXPECT_EQ(heat_map_.populate_calls, 1);
}

TEST_F(HeatMapSettingsTest, GridChangeRebuildsMap)
{
  change("GridX", 20.0);
  EXPECT_EQ(heat_map_.populate_calls, 2);
  EXPECT_EQ(heat_map_.getMap().shape()[0], 5U);
  EXPECT_EQ(heat_map_.getMap().shape()[1], 10U);
}

TEST_F(HeatMapSettingsTest, GridSizeIsBounded)
{
  const double max = heat_map_.getGridSizeMaximumValue();
  change("GridY", 2 * max);
  EXPECT_DOUBLE_EQ(heat_map_.getGridYSize(), max);
}

TEST_F(HeatMapSettingsTest, UseSelectedOnlyRebuildsMap)
{
  change("UseSelectedOnly", true);
  EXPECT_TRUE(heat_map_.useSelectedOnly());
  EXPECT_EQ(heat_map_.populate_calls, 2);
}

// Records the power the data source spreads over the map, before the map
// is rescaled for display.
class RecordingPowerDensity : public PowerDensityDataSource
{
 public:
  using PowerDensityDataSource::PowerDensityDataSource;

  double total = 0.0;

 protected:
  void combineMapData(bool base_has_value,
                      double& base,
                      double new_data,
                      double data_area,
                      double intersection_area,
                      double rect_area) override
  {
    PowerDensityDataSource::combineMapData(base_has_value,
                                           base,
                                           new_data,
                                           data_area,
                                           intersection_area,
                                           rect_area);
    total += new_data / data_area * intersection_area;
  }
};

class PowerDensityTest : public tst::Nangate45Fixture
{
 protected:
  void SetUp() override
  {
    readLiberty("_main/test/Nangate45/Nangate45_typ.lib");
    sta_->postReadDb(db_.get());
    const int dbu = block_->getDbUnitsPerMicron();
    block_->setDieArea(odb::Rect(0, 0, 20 * dbu, 20 * dbu));
    block_->setCoreArea(odb::Rect(0, 0, 20 * dbu, 20 * dbu));
    inst_ = odb::dbInst::create(block_, lib_->findMaster("BUF_X1"), "buf");
    inst_->setOrigin(5 * dbu, 5 * dbu);
    inst_->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    power_
        = sta_->power(sta_->getDbNetwork()->dbToSta(inst_), sta_->scenes()[0]);
  }

  // The power the map holds with only the named component enabled.
  double mapPower(bool internal, bool leakage, bool switching)
  {
    RecordingPowerDensity heat_map(sta_.get(), getLogger());
    heat_map.setChip(chip_);
    heat_map.setSettings({{"Internal", internal},
                          {"Leakage", leakage},
                          {"Switching", switching}});
    heat_map.ensureMap();
    return heat_map.total;
  }

  odb::dbInst* inst_ = nullptr;
  sta::PowerResult power_;
};

TEST_F(PowerDensityTest, LeakageOptionShowsLeakagePower)
{
  ASSERT_GT(power_.leakage(), 0.0);
  ASSERT_NE(power_.leakage(), power_.switching());
  EXPECT_FLOAT_EQ(mapPower(false, true, false), power_.leakage());
}

TEST_F(PowerDensityTest, SwitchingOptionShowsSwitchingPower)
{
  EXPECT_FLOAT_EQ(mapPower(false, false, true), power_.switching());
}

TEST_F(PowerDensityTest, InternalOptionShowsInternalPower)
{
  EXPECT_FLOAT_EQ(mapPower(true, false, false), power_.internal());
}

}  // namespace
}  // namespace web
