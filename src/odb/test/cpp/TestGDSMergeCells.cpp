// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include <filesystem>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/def2gds.h"
#include "odb/gdsUtil.h"
#include "odb/gdsout.h"
#include "odb/geom.h"
#include "tst/fixture.h"

namespace odb::gds {
namespace {

using tst::Fixture;

TEST_F(Fixture, merge_cells)
{
  // Create a "source" GDS file with a cell
  std::string libname = "source_lib";
  dbGDSLib* source_lib = createEmptyGDSLib(db_.get(), libname);

  dbGDSStructure* cell_a = dbGDSStructure::create(source_lib, "cell_a");
  dbGDSBoundary* bnd = dbGDSBoundary::create(cell_a);
  bnd->setLayer(10);
  bnd->setDatatype(0);
  std::vector<Point> xy = {
      Point(0, 0), Point(100, 0), Point(100, 100), Point(0, 100), Point(0, 0)};
  bnd->setXy(xy);

  // Write it to a temporary file
  std::filesystem::create_directory("results");
  const char* source_file = "results/merge_source.gds";
  GDSWriter writer(&logger_);
  writer.write_gds(source_lib, source_file);

  // Create a target library with a placeholder structure
  dbGDSLib* target_lib = createEmptyGDSLib(db_.get(), "target_lib");
  dbGDSStructure* top = dbGDSStructure::create(target_lib, "top");
  dbGDSStructure* placeholder = dbGDSStructure::create(target_lib, "cell_a");

  // The placeholder should be empty
  EXPECT_TRUE(placeholder->getGDSBoundaries().empty());

  // Create SREF from top to placeholder
  dbGDSSRef::create(top, placeholder);

  // Merge the source file
  DefToGds converter(&logger_);
  converter.mergeCells(target_lib, {source_file});

  // After merge, cell_a should have the boundary
  dbGDSStructure* merged = target_lib->findGDSStructure("cell_a");
  ASSERT_NE(merged, nullptr);
  EXPECT_EQ(merged->getGDSBoundaries().size(), 1);

  auto bnd_it = merged->getGDSBoundaries().begin();
  dbGDSBoundary* merged_bnd = *bnd_it;
  EXPECT_EQ(merged_bnd->getLayer(), 10);
  EXPECT_EQ(merged_bnd->getDatatype(), 0);
}

// Regression test: mergeCells must scale coordinates when source GDS
// has different DBU than the target (e.g., ASAP7: 4000 vs 1000 DBU/um).
TEST_F(Fixture, merge_cells_scales_dbu)
{
  // Create a source GDS at 4000 DBU/um (uu_per_dbu = 0.00025)
  dbGDSLib* source_lib = createEmptyGDSLib(db_.get(), "source_4000");
  source_lib->setUnits(0.00025, 0.00025e-6);  // 4000 DBU/um

  dbGDSStructure* cell = dbGDSStructure::create(source_lib, "test_cell");
  dbGDSBoundary* bnd = dbGDSBoundary::create(cell);
  bnd->setLayer(10);
  bnd->setDatatype(0);
  // 400x400 DBU at 4000 DBU/um = 0.1 x 0.1 um
  bnd->setXy({Point(0, 0),
              Point(400, 0),
              Point(400, 400),
              Point(0, 400),
              Point(0, 0)});

  std::filesystem::create_directory("results");
  GDSWriter writer(&logger_);
  writer.write_gds(source_lib, "results/merge_dbu_source.gds");

  // Create a target at 1000 DBU/um (uu_per_dbu = 0.001)
  dbGDSLib* target_lib = createEmptyGDSLib(db_.get(), "target_1000");
  target_lib->setUnits(0.001, 0.001e-6);  // 1000 DBU/um

  dbGDSStructure* top = dbGDSStructure::create(target_lib, "top");
  dbGDSStructure* placeholder = dbGDSStructure::create(target_lib, "test_cell");
  dbGDSSRef::create(top, placeholder);

  // Merge - should scale 4000->1000 DBU (factor 1/4)
  DefToGds converter(&logger_);
  converter.mergeCells(target_lib, {"results/merge_dbu_source.gds"});

  dbGDSStructure* merged = target_lib->findGDSStructure("test_cell");
  ASSERT_NE(merged, nullptr);
  ASSERT_EQ(merged->getGDSBoundaries().size(), 1);

  auto bnd_it = merged->getGDSBoundaries().begin();
  auto pts = (*bnd_it)->getXY();
  // 400 DBU at 4000 DBU/um = 0.1um = 100 DBU at 1000 DBU/um
  EXPECT_EQ(pts[0], Point(0, 0));
  EXPECT_EQ(pts[1], Point(100, 0));
  EXPECT_EQ(pts[2], Point(100, 100));
  EXPECT_EQ(pts[3], Point(0, 100));
  EXPECT_EQ(pts[4], Point(0, 0));
}

// Test merge when source has coarser DBU than target (scale up).
TEST_F(Fixture, merge_cells_scales_dbu_up)
{
  // Source at 1000 DBU/um
  dbGDSLib* source_lib = createEmptyGDSLib(db_.get(), "source_1000");
  source_lib->setUnits(0.001, 0.001e-6);

  dbGDSStructure* cell = dbGDSStructure::create(source_lib, "cell_up");
  dbGDSBoundary* bnd = dbGDSBoundary::create(cell);
  bnd->setLayer(10);
  bnd->setDatatype(0);
  // 100x100 DBU at 1000 DBU/um = 0.1 x 0.1 um
  bnd->setXy({Point(0, 0),
              Point(100, 0),
              Point(100, 100),
              Point(0, 100),
              Point(0, 0)});

  std::filesystem::create_directory("results");
  GDSWriter writer(&logger_);
  writer.write_gds(source_lib, "results/merge_dbu_up_source.gds");

  // Target at 4000 DBU/um
  dbGDSLib* target_lib = createEmptyGDSLib(db_.get(), "target_4000");
  target_lib->setUnits(0.00025, 0.00025e-6);

  dbGDSStructure* top = dbGDSStructure::create(target_lib, "top");
  dbGDSStructure* placeholder = dbGDSStructure::create(target_lib, "cell_up");
  dbGDSSRef::create(top, placeholder);

  DefToGds converter(&logger_);
  converter.mergeCells(target_lib, {"results/merge_dbu_up_source.gds"});

  dbGDSStructure* merged = target_lib->findGDSStructure("cell_up");
  ASSERT_NE(merged, nullptr);
  ASSERT_EQ(merged->getGDSBoundaries().size(), 1);

  auto pts = (*merged->getGDSBoundaries().begin())->getXY();
  // 100 DBU at 1000 DBU/um = 0.1um = 400 DBU at 4000 DBU/um
  EXPECT_EQ(pts[1], Point(400, 0));
  EXPECT_EQ(pts[2], Point(400, 400));
}

}  // namespace
}  // namespace odb::gds
