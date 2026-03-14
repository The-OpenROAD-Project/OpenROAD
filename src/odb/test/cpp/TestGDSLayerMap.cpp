// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include <filesystem>
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

// Test comma-separated EDI type fields (e.g., "NET,SPNET,PIN,LEFPIN,VIA")
TEST(GDSLayerMapTest, parse_edi_comma_separated_types)
{
  const char* test_file = "results/test_comma_types.map";
  std::filesystem::create_directory("results");
  {
    std::ofstream f(test_file);
    f << "# ihp-sg13g2 style comma-separated types\n";
    f << "Metal1\tNET,SPNET,PIN,LEFPIN,VIA\t8\t0\n";
    f << "Metal1\tPIN,LEFPIN\t8\t2\n";
    f << "Metal1\tFILL\t8\t22\n";
    f << "Metal1\tLEFOBS\t8\t4\n";
    f << "NAME\tMetal1/PIN\t8\t25\n";
    f << "Via1\tPIN\t19\t0\n";
    f << "Via1\tLEFPIN\t19\t0\n";
    f << "Via1\tVIA\t19\t0\n";
    f << "Metal2\tNET,SPNET,PIN,LEFPIN,VIA\t10\t0\n";
  }

  LayerMap map = LayerMap::parseEdiLayerMap(test_file);
  GdsLayerSpec spec;

  // NET from comma list → 8/0
  ASSERT_TRUE(map.lookup("Metal1", "NET", spec));
  EXPECT_EQ(spec.layer, 8);
  EXPECT_EQ(spec.datatype, 0);

  // VIA from comma list → 8/0
  ASSERT_TRUE(map.lookup("Metal1", "VIA", spec));
  EXPECT_EQ(spec.layer, 8);
  EXPECT_EQ(spec.datatype, 0);

  // PIN should be 8/2 (from the more specific "PIN,LEFPIN" line)
  // Later entries override earlier ones, matching EDI convention where
  // general lines come first and specific overrides come after.
  ASSERT_TRUE(map.lookup("Metal1", "PIN", spec));
  EXPECT_EQ(spec.layer, 8);
  EXPECT_EQ(spec.datatype, 2);

  // FILL → 8/22
  ASSERT_TRUE(map.lookup("Metal1", "FILL", spec));
  EXPECT_EQ(spec.layer, 8);
  EXPECT_EQ(spec.datatype, 22);

  // OBS → 8/4
  ASSERT_TRUE(map.lookup("Metal1", "OBS", spec));
  EXPECT_EQ(spec.layer, 8);
  EXPECT_EQ(spec.datatype, 4);

  // LABEL → 8/25
  ASSERT_TRUE(map.lookup("Metal1", "LABEL", spec));
  EXPECT_EQ(spec.layer, 8);
  EXPECT_EQ(spec.datatype, 25);

  // Metal2 NET from comma list
  ASSERT_TRUE(map.lookup("Metal2", "NET", spec));
  EXPECT_EQ(spec.layer, 10);
  EXPECT_EQ(spec.datatype, 0);

  // Via1 VIA
  ASSERT_TRUE(map.lookup("Via1", "VIA", spec));
  EXPECT_EQ(spec.layer, 19);
  EXPECT_EQ(spec.datatype, 0);
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

// Test that lookup falls back to "drawing" purpose and lowercase (sky130 style)
TEST(GDSLayerMapTest, lookup_fallback_drawing_and_lowercase)
{
  const char* test_file = "results/test_sky130.lyt";
  std::filesystem::create_directory("results");
  {
    std::ofstream f(test_file);
    f << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    f << "<technology>\n";
    f << " <reader-options>\n";
    f << "  <lefdef>\n";
    // sky130-style: all purposes are lowercase with dots
    f << "   <layer-map>layer_map("
         "'met1.drawing : 68/20';"
         "'met1.pin : 68/16';"
         "'met1.label : 68/5';"
         "'via.drawing : 68/44';"
         "'met2.drawing : 69/20';"
         "'met2.pin : 69/16';"
         "'via2.drawing : 69/44'"
         ")</layer-map>\n";
    f << "  </lefdef>\n";
    f << " </reader-options>\n";
    f << "</technology>\n";
  }

  LayerMap map = LayerMap::parseLytLayerMap(test_file);
  GdsLayerSpec spec;

  // Routing: purpose "NET" should fall back to "drawing"
  ASSERT_TRUE(map.lookup("met1", "NET", spec));
  EXPECT_EQ(spec.layer, 68);
  EXPECT_EQ(spec.datatype, 20);

  ASSERT_TRUE(map.lookup("met2", "NET", spec));
  EXPECT_EQ(spec.layer, 69);
  EXPECT_EQ(spec.datatype, 20);

  // Vias: purpose "VIA" should fall back to "drawing"
  ASSERT_TRUE(map.lookup("via", "VIA", spec));
  EXPECT_EQ(spec.layer, 68);
  EXPECT_EQ(spec.datatype, 44);

  ASSERT_TRUE(map.lookup("via2", "VIA", spec));
  EXPECT_EQ(spec.layer, 69);
  EXPECT_EQ(spec.datatype, 44);

  // Pins: purpose "PIN" should fall back to lowercase "pin"
  ASSERT_TRUE(map.lookup("met1", "PIN", spec));
  EXPECT_EQ(spec.layer, 68);
  EXPECT_EQ(spec.datatype, 16);

  ASSERT_TRUE(map.lookup("met2", "PIN", spec));
  EXPECT_EQ(spec.layer, 69);
  EXPECT_EQ(spec.datatype, 16);

  // Labels: purpose "LABEL" should fall back to lowercase "label"
  ASSERT_TRUE(map.lookup("met1", "LABEL", spec));
  EXPECT_EQ(spec.layer, 68);
  EXPECT_EQ(spec.datatype, 5);

  // Default lookup (single-arg) uses "NET" purpose
  ASSERT_TRUE(map.lookup("met1", spec));
  EXPECT_EQ(spec.layer, 68);
  EXPECT_EQ(spec.datatype, 20);
}

// Test that .lyt files with empty layer_map() fall back to <map-file>
TEST(GDSLayerMapTest, lyt_mapfile_fallback)
{
  std::filesystem::create_directory("results");

  // Create an EDI map file
  const char* map_file = "results/test_fallback.map";
  {
    std::ofstream f(map_file);
    f << "# EDI layer map for fallback test\n";
    f << "Metal1\tNET\t8\t0\n";
    f << "Metal1\tPIN\t8\t2\n";
    f << "Via1\tVIA\t19\t0\n";
    f << "Metal2\tNET\t10\t0\n";
  }

  // Create a .lyt file with empty layer_map() but a <map-file> reference
  const char* lyt_file = "results/test_mapfile_fallback.lyt";
  {
    std::ofstream f(lyt_file);
    f << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    f << "<technology>\n";
    f << " <reader-options>\n";
    f << "  <lefdef>\n";
    f << "   <layer-map>layer_map()</layer-map>\n";
    f << "  </lefdef>\n";
    f << " </reader-options>\n";
    f << " <map-file>test_fallback.map</map-file>\n";
    f << "</technology>\n";
  }

  LayerMap map = LayerMap::parseLytLayerMap(lyt_file);
  EXPECT_FALSE(map.empty());

  GdsLayerSpec spec;

  ASSERT_TRUE(map.lookup("Metal1", "NET", spec));
  EXPECT_EQ(spec.layer, 8);
  EXPECT_EQ(spec.datatype, 0);

  ASSERT_TRUE(map.lookup("Metal1", "PIN", spec));
  EXPECT_EQ(spec.layer, 8);
  EXPECT_EQ(spec.datatype, 2);

  ASSERT_TRUE(map.lookup("Via1", "VIA", spec));
  EXPECT_EQ(spec.layer, 19);
  EXPECT_EQ(spec.datatype, 0);

  ASSERT_TRUE(map.lookup("Metal2", "NET", spec));
  EXPECT_EQ(spec.layer, 10);
  EXPECT_EQ(spec.datatype, 0);
}

}  // namespace
}  // namespace odb::gds
