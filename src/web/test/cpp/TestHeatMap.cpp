// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

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
