// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The JSON the request handler answers with, built from the report structs.

#include <string>
#include <utility>
#include <vector>

#include "boost/json/serialize.hpp"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "tile_generator.h"
#include "timing_report.h"
#include "tst/nangate45_fixture.h"

namespace web {
namespace {

class SerializeTest : public tst::Nangate45Fixture
{
 protected:
  static bool contains(const std::string& haystack, const std::string& needle)
  {
    return haystack.find(needle) != std::string::npos;
  }
};

TEST_F(SerializeTest, SerializeTimingPathsRoundTrip)
{
  TimingPathSummary p;
  p.start_clk = "clk1";
  p.end_clk = "clk2";
  p.slack = -0.5f;
  p.arrival = 1.0f;
  p.required = 1.5f;
  p.skew = 0.1f;
  p.path_delay = 0.9f;
  p.logic_depth = 3;
  p.fanout = 5;
  p.start_pin = "a/Z";
  p.end_pin = "b/D";

  TimingNode node;
  node.pin_name = "a/Z";
  node.fanout = 2;
  node.is_rising = true;
  node.is_clock = false;
  node.time = 0.5f;
  node.delay = 0.1f;
  node.slew = 0.02f;
  node.load = 0.01f;
  p.data_nodes.push_back(std::move(node));

  std::vector<TimingPathSummary> paths = {std::move(p)};
  const std::string json = boost::json::serialize(serializeTimingPaths(paths));

  EXPECT_TRUE(contains(json, "\"clk1\""));
  EXPECT_TRUE(contains(json, "\"clk2\""));
  EXPECT_TRUE(contains(json, "\"a/Z\""));
  EXPECT_TRUE(contains(json, "\"paths\":"));
  EXPECT_TRUE(contains(json, "\"data_nodes\":"));
  EXPECT_TRUE(contains(json, "\"capture_nodes\":"));
}

TEST_F(SerializeTest, SerializeSlackHistogramRoundTrip)
{
  SlackHistogramResult h;
  h.bins.push_back({-1.0f, 0.0f, 10, true});
  h.bins.push_back({0.0f, 1.0f, 20, false});
  h.unconstrained_count = 5;
  h.total_endpoints = 35;
  h.time_unit = "ns";

  const std::string json = boost::json::serialize(serializeSlackHistogram(h));

  EXPECT_TRUE(contains(json, "\"bins\":"));
  EXPECT_TRUE(contains(json, "\"unconstrained_count\":5"));
  EXPECT_TRUE(contains(json, "\"total_endpoints\":35"));
  EXPECT_TRUE(contains(json, "\"time_unit\":\"ns\""));
}

TEST_F(SerializeTest, SerializeChartFiltersRoundTrip)
{
  ChartFilters f;
  f.path_groups.emplace_back("default");
  f.clocks.emplace_back("clk");

  const std::string json = boost::json::serialize(serializeChartFilters(f));

  EXPECT_TRUE(contains(json, "\"path_groups\":"));
  EXPECT_TRUE(contains(json, "\"default\""));
  EXPECT_TRUE(contains(json, "\"clocks\":"));
  EXPECT_TRUE(contains(json, "\"clk\""));
}

TEST_F(SerializeTest, SerializeTechResponse)
{
  TileGenerator gen(getDb(), /*sta=*/nullptr, getLogger());
  const std::string json = boost::json::serialize(serializeTechResponse(gen));

  EXPECT_TRUE(contains(json, "\"layers\":"));
  EXPECT_TRUE(contains(json, "\"sites\":"));
  EXPECT_TRUE(contains(json, "\"has_liberty\":"));
  EXPECT_TRUE(contains(json, "\"dbu_per_micron\":"));
}

// Non-routing tech layers (Nangate45's MASTERSLICE poly/active and OVERLAP)
// are grouped into an "Other" category node in the layer hierarchy, mirroring
// the Qt GUI's displayControls grouping.
TEST_F(SerializeTest, SerializeTechResponseGroupsOtherLayers)
{
  TileGenerator gen(getDb(), /*sta=*/nullptr, getLogger());
  const std::string json = boost::json::serialize(serializeTechResponse(gen));

  EXPECT_TRUE(contains(json, "\"layer_hierarchy\":"));
  EXPECT_TRUE(contains(json, "\"type\":\"category\""));
  EXPECT_TRUE(contains(json, "\"name\":\"Other\""));
}

TEST_F(SerializeTest, SerializeBoundsShapesReady)
{
  TileGenerator gen(getDb(), /*sta=*/nullptr, getLogger());

  const std::string json_true
      = boost::json::serialize(serializeBoundsResponse(gen, true));
  EXPECT_TRUE(contains(json_true, "\"shapes_ready\":true"));

  const std::string json_false
      = boost::json::serialize(serializeBoundsResponse(gen, false));
  EXPECT_TRUE(contains(json_false, "\"shapes_ready\":false"));
}

// A 3DBlox top chip owns no dbBlock, so the fanout histogram has to pool the
// chiplets' nets rather than report on a single block.
TEST_F(SerializeTest, FanoutHistogramPoolsEveryBlock)
{
  odb::dbNet::create(block_, "n1");
  odb::dbNet::create(block_, "n2");

  odb::dbChip* other
      = odb::dbChip::create(getDb(), getDb()->getTech(), "other");
  odb::dbBlock* other_block = odb::dbBlock::create(other, "other_top");
  odb::dbNet::create(other_block, "m1");

  const FanoutHistogramResult first = computeFanoutHistogram({block_});
  const FanoutHistogramResult second = computeFanoutHistogram({other_block});
  ASSERT_GT(first.total_nets, 0);
  ASSERT_GT(second.total_nets, 0);

  const FanoutHistogramResult pooled
      = computeFanoutHistogram({block_, other_block});
  EXPECT_EQ(pooled.total_nets, first.total_nets + second.total_nets);

  // Null entries are skipped, not counted as an empty block.
  const FanoutHistogramResult with_null
      = computeFanoutHistogram({block_, nullptr});
  EXPECT_EQ(with_null.total_nets, first.total_nets);
}

}  // namespace
}  // namespace web
