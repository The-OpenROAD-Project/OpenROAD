// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include <fstream>
#include <string>

#include "gtest/gtest.h"
#include "odb/def2gds.h"

namespace odb::gds {
namespace {

// Test EDI layer map parsing
TEST(GDSLayerMapTest, parse_edi_layer_map)
{
  // Create a temporary layer map file
  const char* test_file = "results/test_layermap.map";
  std::filesystem::create_directory("results");
  {
    std::ofstream f(test_file);
    f << "# Test layer map\n";
    f << "Metal1\tNET\t34\t0\n";
    f << "Metal1\tPIN\t34\t0\n";
    f << "Metal1\tFILL\t34\t5\n";
    f << "Metal1\tLEFOBS\t34\t3\n";
    f << "NAME\tMetal1/PIN\t34\t10\n";
    f << "Via1\tVIA\t35\t0\n";
    f << "PR_bndry\tALL\t0\t0\n";
  }

  LayerMap map = LayerMap::parseEdiLayerMap(test_file);
  EXPECT_FALSE(map.empty());

  GdsLayerSpec spec;

  // Metal1 NET
  ASSERT_TRUE(map.lookup("Metal1", "NET", spec));
  EXPECT_EQ(spec.layer, 34);
  EXPECT_EQ(spec.datatype, 0);

  // Metal1 FILL (different datatype)
  ASSERT_TRUE(map.lookup("Metal1", "FILL", spec));
  EXPECT_EQ(spec.layer, 34);
  EXPECT_EQ(spec.datatype, 5);

  // Metal1 OBS
  ASSERT_TRUE(map.lookup("Metal1", "OBS", spec));
  EXPECT_EQ(spec.layer, 34);
  EXPECT_EQ(spec.datatype, 3);

  // Metal1 LABEL (from NAME entry)
  ASSERT_TRUE(map.lookup("Metal1", "LABEL", spec));
  EXPECT_EQ(spec.layer, 34);
  EXPECT_EQ(spec.datatype, 10);

  // Via1
  ASSERT_TRUE(map.lookup("Via1", "VIA", spec));
  EXPECT_EQ(spec.layer, 35);
  EXPECT_EQ(spec.datatype, 0);

  // PR_bndry with default purpose
  ASSERT_TRUE(map.lookup("PR_bndry", "", spec));
  EXPECT_EQ(spec.layer, 0);
  EXPECT_EQ(spec.datatype, 0);

  // Default lookup (uses NET purpose)
  ASSERT_TRUE(map.lookup("Metal1", spec));
  EXPECT_EQ(spec.layer, 34);
  EXPECT_EQ(spec.datatype, 0);

  // Nonexistent layer
  EXPECT_FALSE(map.lookup("Metal99", spec));
}

// Test KLayout .lyt layer map parsing
TEST(GDSLayerMapTest, parse_lyt_layer_map)
{
  const char* test_file = "results/test_tech.lyt";
  std::filesystem::create_directory("results");
  {
    std::ofstream f(test_file);
    f << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    f << "<technology>\n";
    f << " <name>Test</name>\n";
    f << " <reader-options>\n";
    f << "  <lefdef>\n";
    f << "   <layer-map>layer_map('metal1 : 11/0';'metal1.LABEL : "
         "11/1';'metal1.PIN : 11/2';'via1 : 12/0')</layer-map>\n";
    f << "  </lefdef>\n";
    f << " </reader-options>\n";
    f << "</technology>\n";
  }

  LayerMap map = LayerMap::parseLytLayerMap(test_file);
  EXPECT_FALSE(map.empty());

  GdsLayerSpec spec;

  // metal1 routing (NET purpose)
  ASSERT_TRUE(map.lookup("metal1", "NET", spec));
  EXPECT_EQ(spec.layer, 11);
  EXPECT_EQ(spec.datatype, 0);

  // metal1 label
  ASSERT_TRUE(map.lookup("metal1", "LABEL", spec));
  EXPECT_EQ(spec.layer, 11);
  EXPECT_EQ(spec.datatype, 1);

  // metal1 pin
  ASSERT_TRUE(map.lookup("metal1", "PIN", spec));
  EXPECT_EQ(spec.layer, 11);
  EXPECT_EQ(spec.datatype, 2);

  // via1
  ASSERT_TRUE(map.lookup("via1", "NET", spec));
  EXPECT_EQ(spec.layer, 12);
  EXPECT_EQ(spec.datatype, 0);
}

}  // namespace
}  // namespace odb::gds
