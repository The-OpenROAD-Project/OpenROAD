// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors
//
// Test duplicate cell name handling across merged GDS files (PR review:
// stefanottili raised concern about duplicate cell names across files).
//
// Current behavior: when multiple GDS files contain structures with the
// same name, the elements from all files are accumulated into the same
// structure. This test documents and verifies that behavior.

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

TEST_F(Fixture, merge_duplicate_cell_names)
{
  std::filesystem::create_directory("results");
  std::vector<Point> xy = {
      Point(0, 0), Point(100, 0), Point(100, 100), Point(0, 100), Point(0, 0)};

  // Create first GDS file with cell "shared_cell" containing 1 boundary on
  // layer 10
  {
    dbGDSLib* lib1 = createEmptyGDSLib(db_.get(), "lib1");
    dbGDSStructure* cell = dbGDSStructure::create(lib1, "shared_cell");
    dbGDSBoundary* bnd = dbGDSBoundary::create(cell);
    bnd->setLayer(10);
    bnd->setDatatype(0);
    bnd->setXy(xy);

    GDSWriter writer(&logger_);
    writer.write_gds(lib1, "results/dup_file1.gds");
  }

  // Create second GDS file with cell "shared_cell" containing 1 boundary on
  // layer 20
  {
    dbGDSLib* lib2 = createEmptyGDSLib(db_.get(), "lib2");
    dbGDSStructure* cell = dbGDSStructure::create(lib2, "shared_cell");
    dbGDSBoundary* bnd = dbGDSBoundary::create(cell);
    bnd->setLayer(20);
    bnd->setDatatype(0);
    bnd->setXy(xy);

    GDSWriter writer(&logger_);
    writer.write_gds(lib2, "results/dup_file2.gds");
  }

  // Create target library and merge both files
  dbGDSLib* target = createEmptyGDSLib(db_.get(), "target");
  dbGDSStructure* top = dbGDSStructure::create(target, "top");
  dbGDSStructure* placeholder = dbGDSStructure::create(target, "shared_cell");
  dbGDSSRef::create(top, placeholder);

  DefToGds converter(&logger_);
  converter.mergeCells(target,
                       {"results/dup_file1.gds", "results/dup_file2.gds"});

  // Current behavior: both boundaries are accumulated
  dbGDSStructure* merged = target->findGDSStructure("shared_cell");
  ASSERT_NE(merged, nullptr);
  EXPECT_EQ(merged->getGDSBoundaries().size(), 2);

  // Verify both layers are present
  bool has_layer_10 = false;
  bool has_layer_20 = false;
  for (dbGDSBoundary* bnd : merged->getGDSBoundaries()) {
    if (bnd->getLayer() == 10) {
      has_layer_10 = true;
    }
    if (bnd->getLayer() == 20) {
      has_layer_20 = true;
    }
  }
  EXPECT_TRUE(has_layer_10);
  EXPECT_TRUE(has_layer_20);
}

TEST_F(Fixture, merge_same_cell_from_same_file)
{
  // Merging the same file twice should double the elements
  std::filesystem::create_directory("results");
  std::vector<Point> xy = {
      Point(0, 0), Point(100, 0), Point(100, 100), Point(0, 100), Point(0, 0)};

  {
    dbGDSLib* lib = createEmptyGDSLib(db_.get(), "source");
    dbGDSStructure* cell = dbGDSStructure::create(lib, "my_cell");
    dbGDSBoundary* bnd = dbGDSBoundary::create(cell);
    bnd->setLayer(5);
    bnd->setDatatype(0);
    bnd->setXy(xy);

    GDSWriter writer(&logger_);
    writer.write_gds(lib, "results/dup_same.gds");
  }

  dbGDSLib* target = createEmptyGDSLib(db_.get(), "target");
  dbGDSStructure* top = dbGDSStructure::create(target, "top");
  dbGDSStructure* placeholder = dbGDSStructure::create(target, "my_cell");
  dbGDSSRef::create(top, placeholder);

  DefToGds converter(&logger_);
  // Merge the same file twice
  converter.mergeCells(target,
                       {"results/dup_same.gds", "results/dup_same.gds"});

  dbGDSStructure* merged = target->findGDSStructure("my_cell");
  ASSERT_NE(merged, nullptr);
  // Elements are accumulated, so 2 boundaries
  EXPECT_EQ(merged->getGDSBoundaries().size(), 2);
}

}  // namespace
}  // namespace odb::gds
